package paskisoc

import spinal.core._
import vexriscv.ip.{DataCacheConfig, InstructionCacheConfig}
import vexriscv.plugin._
import vexriscv.{Riscv, VexRiscv, VexRiscvConfig, plugin}

object Paski_CPU_Linux extends Component {
  def cpu(isSimulate: Boolean = false) = new VexRiscv(
    config = VexRiscvConfig(
      plugins = List(
        new IBusCachedPlugin(
          resetVector = if(isSimulate) 0x40000020l else 0x80000000l,
          prediction = DYNAMIC_TARGET,
          config = InstructionCacheConfig(
            cacheSize = 4096 * 1,
            bytePerLine = 32,
            wayCount = 1,
            addressWidth = 32,
            cpuDataWidth = 32,
            memDataWidth = 32,
            catchIllegalAccess = true,
            catchAccessFault = true,
            asyncTagMemory = false,
            twoCycleRam = false,
            twoCycleCache = true
          ),
          memoryTranslatorPortConfig = MmuPortConfig(
            portTlbSize = 4
          )
        ),
        new DBusCachedPlugin(
          dBusCmdMasterPipe = true,
          dBusCmdSlavePipe = true,
          dBusRspSlavePipe = true,
          config = new DataCacheConfig(
            cacheSize         = 4096*1,
            bytePerLine       = 32,
            wayCount          = 1,
            addressWidth      = 32,
            cpuDataWidth      = 32,
            memDataWidth      = 32,
            catchAccessError  = true,
            catchIllegal      = true,
            catchUnaligned    = true,
            withExclusive = false,
            withInvalidate = false,
            withLrSc = true,
            withAmo = true
          ),
          memoryTranslatorPortConfig = MmuPortConfig(
            portTlbSize = 4
          )
        ),
        //new CsrPlugin(CsrPluginConfig.smallest),
        new DecoderSimplePlugin(
          catchIllegalInstruction = false
        ),
        new RegFilePlugin(
          regFileReadyKind = plugin.SYNC,
          zeroBoot = false
        ),
        new IntAluPlugin,
        new SrcPlugin(
          separatedAddSub = false,
          executeInsertion = true
        ),
        new LightShifterPlugin,
        new HazardSimplePlugin(
          bypassExecute           = true,
          bypassMemory            = true,
          bypassWriteBack         = true,
          bypassWriteBackBuffer   = true,
          pessimisticUseSrc       = false,
          pessimisticWriteRegFile = false,
          pessimisticAddressMatch = false
        ),
        new MulPlugin,
        new DivPlugin,
        new BranchPlugin(
          earlyBranch = false,
          catchAddressMisaligned = false
        ),
        new YamlPlugin("cpu.yaml"),
        new CsrPlugin(CsrPluginConfig.linuxMinimal(0x40000020l).copy(
          ebreakGen = true,
          misaExtensionsInit = Riscv.misaToInt("imas"),
          misaAccess = CsrAccess.READ_ONLY,
          mtvecAccess         = CsrAccess.READ_WRITE,
          medelegAccess       = CsrAccess.READ_WRITE,
          midelegAccess       = CsrAccess.READ_WRITE
        )),
        /*
        new StaticMemoryTranslatorPlugin(
          ioRange      = _(31 downto 28) === 0xF
        ),
         */
        new MmuPlugin(
          ioRange = (x => x(31 downto 28) === 0xF)
        ),
        new DebugPlugin(ClockDomain.current.clone(reset = Bool().setName("debugReset")))
      )
    )
  )

  def apply(isSimulate: Boolean = false) = cpu(isSimulate)
}