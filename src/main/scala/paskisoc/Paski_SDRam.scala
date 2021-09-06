package paskisoc

import spinal.core._
import spinal.lib.bus.amba4.axi.Axi4Shared
import spinal.lib.{master, slave}
import spinal.lib.memory.sdram.SdramGeneration.SDR
import spinal.lib.memory.sdram.SdramLayout
import spinal.lib.memory.sdram.sdr.{Axi4SharedSdramCtrl, SdramInterface, SdramTimings}

class Paski_SDRam(sdram_clk: ClockDomain) extends Component {

  val sdram_clk_inst = sdram_clk

  val sdramLayout = Global_Config_Data.sdram_layout()
  val sdramTimings = Global_Config_Data.sdram_timings()

  val sdramCAS = Global_Config_Data.sdram_cas()

  val sdramArea = new ClockingArea(sdram_clk) {
    val sdramCtrl = Axi4SharedSdramCtrl(
      axiDataWidth = 32,
      axiIdWidth = 4,
      layout = sdramLayout,
      timing = sdramTimings,
      CAS = sdramCAS
    )
  }

  val io = new Bundle {
    val axi = slave(Axi4Shared(sdramArea.sdramCtrl.axiConfig))
    val sdram = master(SdramInterface(sdramLayout))
  }
  io.sdram <> sdramArea.sdramCtrl.io.sdram
  io.axi >> sdramArea.sdramCtrl.io.axi
}
