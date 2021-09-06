package paskisoc

import spinal.core._
import spinal.lib.io.InOutWrapper

object main {
  def main(args: Array[String]) {
    SpinalVerilog(InOutWrapper(new TopModule())).printPruned()
  }
}