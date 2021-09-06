package paskisoc

import spinal.core._
import spinal.lib.com.spi.{SpiMasterCtrlGenerics, SpiMasterCtrlMemoryMappedConfig}
import spinal.lib.memory.sdram.SdramGeneration.SDR
import spinal.lib.memory.sdram.SdramLayout
import spinal.lib.memory.sdram.sdr.SdramTimings

object Global_Config_Data extends Component {
  def onChipRamSize() = 8 KiB

  def gpioBits() = 4

  def sdram_layout() = SdramLayout(
    generation = SDR,
    bankWidth   = 2, // 2bit bank选择信号 (4个bank)
    columnWidth = 9, // CA0 - CA8
    rowWidth    = 13, // RA0 - RA12
    dataWidth   = 16  // 16bit 数据位宽
  )

  def sdram_timings() = SdramTimings(
    bootRefreshCount =   8,
    tPOW             = 200 us,
    tREF             =  64 ms,
    tRC              =  80 ns,
    tRFC             =  80 ns,
    tRAS             =  60 ns,
    tRP              =  40 ns,
    tRCD             =  40 ns,
    cMRD             =  3,
    tWR              =  60 ns,
    cWR              =  3
  )

  def sdram_cas() = 3

  def spi_sd_config() = SpiMasterCtrlMemoryMappedConfig(
    ctrlGenerics = SpiMasterCtrlGenerics(
      ssWidth = 1,
      timerWidth = 8,
      dataWidth = 8
    ),
    cmdFifoDepth = 32,
    rspFifoDepth = 32
  )
}