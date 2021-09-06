import paskisoc.Paski_SOC
import spinal.core._
import spinal.core.sim._

import scala.sys.process._

object Paski_CPU_Simulate {
  def test_paski_cpu() : Unit = {
    SimConfig
      .withWave
      .allOptimisation
      .compile(new Paski_SOC(ClockDomain.external("sys", frequency = FixedFrequency(50 MHz), config = ClockDomainConfig(resetKind = SYNC)), isSimulate = true))
      .doSim { dut =>
        dut.cpu_clk_inst.forkStimulus(2)

        sleep(3000000)
        simSuccess()
      }
  }

  def main(args: Array[String]) : Unit = {
    try {
      test_paski_cpu()
    } catch {
      case e:Exception => e.printStackTrace()
    }
    ("gtkwave " + "simWorkspace/Paski_SOC/test.vcd").!!
  }
}
