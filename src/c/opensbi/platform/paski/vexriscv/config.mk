#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2021 Angelic47 <admin@angelic47.com>
#

# Compiler flags
platform-cppflags-y =
platform-cflags-y =
platform-asflags-y =
platform-ldflags-y =

# Command for platform specific "make run"
platform-runcmd = echo Angelic47/Paski

PLATFORM_RISCV_XLEN = 32
PLATFORM_RISCV_ABI = ilp32
PLATFORM_RISCV_ISA = rv32ima
PLATFORM_RISCV_CODE_MODEL = medany

# Blobs to build
FW_TEXT_START=0x41F00000
FW_DYNAMIC=n
FW_JUMP=y
FW_JUMP_ADDR=0x40000020
FW_JUMP_FDT_ADDR=0x41EF0000
FW_PAYLOAD=n
FW_PAYLOAD_OFFSET=0x40000000
FW_PAYLOAD_FDT_ADDR=0x40EF0000
