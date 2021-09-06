package paskisoc

import spinal.core._
import spinal.lib._
import spinal.lib.bus.amba3.apb.{Apb3, Apb3Config, Apb3SlaveFactory}
import spinal.lib.misc.{InterruptCtrl, Paski_Timer64, Prescaler, Timer}

/**
 * Created by PIC32F_USER on 25/08/2016.
 */
object PaskiTimerCtrl{
  def getApb3Config() = new Apb3Config(
    addressWidth = 8,
    dataWidth = 32
  )
}

case class PaskiTimerCtrlExternal() extends Bundle{
  val clear = Bool()
  val tick = Bool()
}

case class PaskiTimerCtrl() extends Component {
  val io = new Bundle{
    val apb = slave(Apb3(PaskiTimerCtrl.getApb3Config()))
    val external = in(PaskiTimerCtrlExternal())
    val interrupt = out Bool()
  }
  val external = BufferCC(io.external)

  val prescaler = Prescaler(16)
  val timerA = Paski_Timer64()
  val timerB = Paski_Timer64()

  val busCtrl = Apb3SlaveFactory(io.apb)
  val prescalerBridge = prescaler.driveFrom(busCtrl,0x00)

  val timerABridge = timerA.driveFrom(busCtrl,0x40)(
    ticks  = List(True, prescaler.io.overflow),
    clears = List(timerA.io.full)
  )

  val timerBBridge = timerB.driveFrom(busCtrl,0x60)(
    ticks  = List(True, prescaler.io.overflow, external.tick),
    clears = List(timerB.io.full, external.clear)
  )

  val interruptCtrl = InterruptCtrl(2)
  val interruptCtrlBridge = interruptCtrl.driveFrom(busCtrl,0x10)
  interruptCtrl.io.inputs(0) := timerA.io.full
  interruptCtrl.io.inputs(1) := timerB.io.full
  io.interrupt := interruptCtrl.io.pendings.orR
}
