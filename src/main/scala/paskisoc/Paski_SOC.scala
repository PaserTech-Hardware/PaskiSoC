package paskisoc

import spinal.core._
import spinal.lib.bus.amba3.apb.{Apb3Decoder, Apb3Gpio}
import spinal.lib.bus.amba4.axi.{Axi4CrossbarFactory, Axi4ReadOnly, Axi4Shared, Axi4SharedOnChipRam, Axi4SharedToApb3Bridge}
import spinal.lib.com.jtag.Jtag
import spinal.lib.com.spi.{Apb3SpiMasterCtrl, SpiMaster}
import spinal.lib.com.uart.{Apb3UartCtrl, Uart, UartCtrlGenerics, UartCtrlInitConfig, UartCtrlMemoryMappedConfig, UartParityType, UartStopType}
import spinal.lib.eda.altera.ResetEmitterTag
import spinal.lib.io.TriStateArray
import spinal.lib.{BufferCC, master, slave}
import spinal.lib.memory.sdram.sdr.SdramInterface
import spinal.lib.misc.BinTools
import spinal.lib.soc.pinsec.PinsecTimerCtrl
import vexriscv.plugin.{CsrPlugin, DBusCachedPlugin, DBusSimplePlugin, DebugPlugin, IBusCachedPlugin, IBusSimplePlugin}

class Paski_SOC(cpu_clk: ClockDomain, isSimulate: Boolean = false) extends Component {

  val onChipRamSize  = Global_Config_Data.onChipRamSize()
  val gpioBits = Global_Config_Data.gpioBits()

  val cpu_clk_inst = cpu_clk

  val cpu_area = new ClockingArea(cpu_clk) {

    val ram : Axi4SharedOnChipRam = if(isSimulate) {
      val ramimpl = Axi4SharedOnChipRam(
        dataWidth = 32,
        byteCount = 32 MiB,
        idWidth = 4
      )
      BinTools.initRam(ramimpl.ram, "src/c/u-boot.bin")
      ramimpl
    } else {
      val ramimpl = Axi4SharedOnChipRam(
        dataWidth = 32,
        byteCount = onChipRamSize,
        idWidth = 4
      )
      BinTools.initRam(ramimpl.ram, "src/c/c_helloworld_bl/output/hello_demo.bin")
      ramimpl
    }

    val sdram = if(!isSimulate) {
      new Paski_SDRam(cpu_clk) // todo
    } else {
      null
    }

    val apbBridge = Axi4SharedToApb3Bridge(
      addressWidth = 20,
      dataWidth    = 32,
      idWidth      = 4
    )

    val gpioCtrl = Apb3Gpio(
      gpioWidth = gpioBits,
      withReadSync = true
    )

    val uartCtrl = Apb3UartCtrl(UartCtrlMemoryMappedConfig(
      uartCtrlConfig = UartCtrlGenerics(
        dataWidthMax      = 8,
        clockDividerWidth = 8,
        preSamplingSize   = 1,
        samplingSize      = 5,
        postSamplingSize  = 2
      ),
      initConfig = UartCtrlInitConfig(
        baudrate = 115200,
        dataLength = 7,
        parity = UartParityType.NONE,
        stop = UartStopType.ONE
      ),
      busCanWriteClockDividerConfig = true,
      busCanWriteFrameConfig = true,
      txFifoDepth = 8,
      rxFifoDepth = 8
    ))

    val timerCtrl = PaskiTimerCtrl()
    timerCtrl.io.external.clear := False
    timerCtrl.io.external.tick := False

    val spiSDCtrl = Apb3SpiMasterCtrl(Global_Config_Data.spi_sd_config())

    val cpu = Paski_CPU_Linux(isSimulate)

    var iBus : Axi4ReadOnly = null
    var dBus : Axi4Shared = null
    var jtag : Jtag = null
    var jtag_debugRst : Bool = null
    for(plugin <- cpu.config.plugins) plugin match {
      case plugin : IBusSimplePlugin => iBus = plugin.iBus.toAxi4ReadOnly()
      case plugin : IBusCachedPlugin => iBus = plugin.iBus.toAxi4ReadOnly()
      case plugin : DBusSimplePlugin => dBus = plugin.dBus.toAxi4Shared()
      case plugin : DBusCachedPlugin => dBus = plugin.dBus.toAxi4Shared()
      case plugin : CsrPlugin        => {
        plugin.externalInterrupt := False
        plugin.externalInterruptS := False
        plugin.timerInterrupt := timerCtrl.io.interrupt
      }
      case plugin : DebugPlugin      => plugin.debugClockDomain {
        jtag = plugin.io.bus.fromJtag()
        plugin.io.resetOut
          .addTag(ResetEmitterTag(plugin.debugClockDomain))
          .parent = null //Avoid the io bundle to be interpreted as a QSys conduit
        jtag_debugRst = plugin.debugClockDomain.reset
      }
      case _ =>
    }

    val axiCrossbar = Axi4CrossbarFactory()
    // crossbar添加从设备
    if(isSimulate) {
      axiCrossbar.addSlaves(
        ram.io.axi       -> (0x40000000L,   32 MiB),
        apbBridge.io.axi -> (0xF0000000L,   1 MiB)
      )
    } else {
      axiCrossbar.addSlaves(
        ram.io.axi       -> (0x80000000L,   onChipRamSize),
        sdram.io.axi     -> (0x40000000L,   sdram.sdramLayout.capacity),
        apbBridge.io.axi -> (0xF0000000L,   1 MiB)
      )
    }


    // crossbar设定主从
    if(isSimulate) {
      axiCrossbar.addConnections(
        iBus       -> List(ram.io.axi),
        dBus       -> List(ram.io.axi, apbBridge.io.axi)
      )
    } else {
      axiCrossbar.addConnections(
        iBus       -> List(ram.io.axi, sdram.io.axi),
        dBus       -> List(ram.io.axi, sdram.io.axi, apbBridge.io.axi)
      )
    }

    // 添加流水线来增加fmax
    axiCrossbar.addPipelining(apbBridge.io.axi)((crossbar,bridge) => {
      crossbar.sharedCmd.halfPipe() >> bridge.sharedCmd
      crossbar.writeData.halfPipe() >> bridge.writeData
      crossbar.writeRsp             << bridge.writeRsp
      crossbar.readRsp              << bridge.readRsp
    })

    axiCrossbar.addPipelining(ram.io.axi)((crossbar,ctrl) => {
      crossbar.sharedCmd.halfPipe()  >>  ctrl.sharedCmd
      crossbar.writeData            >/-> ctrl.writeData
      crossbar.writeRsp              <<  ctrl.writeRsp
      crossbar.readRsp               <<  ctrl.readRsp
    })

    if(!isSimulate) {
      axiCrossbar.addPipelining(sdram.io.axi)((crossbar, ctrl) => {
        crossbar.sharedCmd.halfPipe() >> ctrl.sharedCmd
        crossbar.writeData >/-> ctrl.writeData
        crossbar.writeRsp << ctrl.writeRsp
        crossbar.readRsp << ctrl.readRsp
      })
    }

    axiCrossbar.addPipelining(iBus)((cpu,crossbar) => {
      cpu.readCmd               >>  crossbar.readCmd
      cpu.readRsp               <-< crossbar.readRsp //Data cache directly use read responses without buffering, so pipeline it for FMax
    })

    axiCrossbar.addPipelining(dBus)((cpu,crossbar) => {
      cpu.sharedCmd             >>  crossbar.sharedCmd
      cpu.writeData             >>  crossbar.writeData
      cpu.writeRsp              <<  crossbar.writeRsp
      cpu.readRsp               <-< crossbar.readRsp //Data cache directly use read responses without buffering, so pipeline it for FMax
    })

    // 构建axi总线核
    axiCrossbar.build()

    val apbDecoder = Apb3Decoder(
      master = apbBridge.io.apb,
      slaves = List(
        gpioCtrl.io.apb -> (0x00000, 4 KiB),
        uartCtrl.io.apb -> (0x01000, 4 KiB),
        spiSDCtrl.io.apb -> (0x02000, 4 KiB),
        timerCtrl.io.apb -> (0x03000, 4 KiB)
      )
    )
  }

  val io = new Bundle {
    val gpio = master(TriStateArray(gpioBits bits))
    val uart = master(Uart())
    val jtag = slave(Jtag())
    val debugReset = in Bool()
    val sdram = if(!isSimulate) {
      master(SdramInterface(cpu_area.sdram.sdramLayout))
    } else {
      null
    }
    val spi = master(SpiMaster(ssWidth = Global_Config_Data.spi_sd_config().ctrlGenerics.ssWidth))
  }
  io.gpio <> cpu_area.gpioCtrl.io.gpio
  io.uart <> cpu_area.uartCtrl.io.uart
  if(!isSimulate) {
    io.sdram <> cpu_area.sdram.io.sdram
  }
  io.spi <> cpu_area.spiSDCtrl.io.spi
  io.jtag <> cpu_area.jtag
  cpu_area.jtag_debugRst := io.debugReset

}