package paskisoc

import spinal.core._
import spinal.lib.com.jtag.Jtag
import spinal.lib.com.spi.SpiMaster
import spinal.lib.com.uart.Uart
import spinal.lib.io.TriStateArray
import spinal.lib.{master, slave}
import spinal.lib.memory.sdram.sdr.SdramInterface

class TopModule extends Component {

  val io = new Bundle {
    val clk = in Bool()
    val reset = in Bool()
    val gpio = master(TriStateArray(Global_Config_Data.gpioBits() bits))
    val uart = master(Uart())
    var sdram = master(SdramInterface(Global_Config_Data.sdram_layout()))
    val sdram_clk = out Bool()
    val spi = master(SpiMaster(ssWidth = Global_Config_Data.spi_sd_config().ctrlGenerics.ssWidth))
    val jtag = slave(Jtag())
    val debugReset = in Bool()
  }

  val sys_clk = ClockDomain(
    clock = io.clk,
    reset = io.reset,
    frequency = FixedFrequency(50 MHz),
    config = ClockDomainConfig(
      resetActiveLevel = LOW,
      resetKind = ASYNC
  ))

  /*
  val altera_pll = new Altera_Pll_EP4CE6(sys_clk)
  altera_pll.setBlackBoxName("pll")

  val cpu_clk = altera_pll.out_clk_100mhz
   */
  val soc = new Paski_SOC(sys_clk)

  io.gpio <> soc.io.gpio
  io.uart <> soc.io.uart
  io.sdram <> soc.io.sdram
  io.sdram_clk := io.clk
  io.spi <> soc.io.spi
  io.jtag <> soc.io.jtag
  soc.io.debugReset := io.debugReset
}
