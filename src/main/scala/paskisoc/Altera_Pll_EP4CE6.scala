package paskisoc

import spinal.core._

class Altera_Pll_EP4CE6(inclk : ClockDomain) extends BlackBox {

  // Define IO of the VHDL entity / Verilog module
  val io = new Bundle {
    val areset = in Bool()
    val inclk0 = in Bool()
    val c0 = out Bool()
  }

  mapClockDomain(
    inclk,
    clock = io.inclk0,
    reset = io.areset,
    resetActiveLevel = LOW
  )
  val out_clk_100mhz = ClockDomain(
    clock = io.c0,
    reset = io.areset,
    frequency = FixedFrequency(100 MHz),
    config = ClockDomainConfig(
      resetKind = ASYNC,
      resetActiveLevel = LOW
    )
  )

  noIoPrefix()

}
