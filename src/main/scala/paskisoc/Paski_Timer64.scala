package spinal.lib.misc
import spinal.core._
import spinal.lib._
import spinal.lib.bus.misc.BusSlaveFactory


case class Paski_Timer64() extends Component{
  val io = new Bundle {
    val tick  = in Bool()
    val clear = in Bool()
    val limit = in UInt (64 bits)

    val full  = out Bool()
    val value = out UInt (64 bits)
  }
  val counter = Reg(UInt(64 bits))
  val limitHit = counter === io.limit
  val inhibitFull = RegInit(False)
  when(io.tick){
    inhibitFull := limitHit
    counter := counter + (!limitHit).asUInt
  }
  when(io.clear){
    counter := 0
    inhibitFull := False
  }
  io.full  := limitHit && io.tick && !inhibitFull
  io.value := counter


  def driveFrom(busCtrl : BusSlaveFactory,baseAddress : BigInt)
               (ticks : Seq[Bool],clears : Seq[Bool]) = new Area {
    //Address 0x0 => clear/tick masks + bus
    val ticksEnable  = busCtrl.createReadAndWrite(Bits(ticks.length bits) ,baseAddress + 0x0,0) init(0)
    val clearsEnable = busCtrl.createReadAndWrite(Bits(clears.length bits),baseAddress + 0x0,16) init(0)
    val busClearing  = False
    val counterCache = Reg(UInt(64 bits))
    val counterLimit = Reg(UInt(64 bits))

    //Address 0x4 => read/write limit (+ auto clear) High 32 bits
    //Address 0x8 => read/write limit (+ auto clear) Low 32 bits
    busCtrl.driveAndRead(counterLimit(63 downto 32),baseAddress + 0x4)
    busClearing.setWhen(busCtrl.isWriting(baseAddress + 0x4))
    busCtrl.driveAndRead(counterLimit(31 downto 0),baseAddress + 0x8)
    busClearing.setWhen(busCtrl.isWriting(baseAddress + 0x8))
    io.limit := counterLimit

    //Address 0xC => send timer value to timer value cache
    when(busCtrl.isWriting(baseAddress + 0xC)) {
      counterCache := io.value
    }

    //Address 0x10 => read timer cache value Hign 32 bits / write => clear timer value
    //Address 0x14 => read timer cache value Low 32 bits / write => clear timer value
    busCtrl.read(counterCache(63 downto 32),baseAddress + 0x10)
    busClearing.setWhen(busCtrl.isWriting(baseAddress + 0x10))
    busCtrl.read(counterCache(31 downto 0),baseAddress + 0x14)
    busClearing.setWhen(busCtrl.isWriting(baseAddress + 0x14))

    io.clear := (clearsEnable & clears.asBits).orR | busClearing
    io.tick  := (ticksEnable  & ticks.asBits ).orR
  }
}