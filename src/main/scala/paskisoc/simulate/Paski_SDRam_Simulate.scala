package paskisoc.simulate

import paskisoc.{Paski_SDRam, Paski_SOC}
import spinal.core._
import spinal.core.sim._

import scala.sys.process._

object Paski_SDRam_Simulate {
  def test_paski_sdram() : Unit = {
    SimConfig
      .withWave
      .allOptimisation
      .compile(new Paski_SDRam(ClockDomain.external("sdram", frequency = FixedFrequency(100 MHz), config = ClockDomainConfig(resetKind = SYNC))))
      .doSim { dut =>
        dut.sdram_clk_inst.forkStimulus(2)

        sleep(10000)
        simSuccess()
      }
  }

  def main(args: Array[String]) : Unit = {
    try {
      test_paski_sdram()
    } catch {
      case e:Exception => e.printStackTrace()
    }
    ("gtkwave " + "simWorkspace/Paski_SDRam/test.vcd").!!
  }
}
