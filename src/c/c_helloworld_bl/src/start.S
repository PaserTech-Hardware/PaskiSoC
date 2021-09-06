.align 2

.section .stack_sec
SPV_Stack_Pool:
	.rept 0x100
	.word 0x00000000
	.endr

.section .text._start

.extern _copy_rwdata
.type _copy_rwdata, %function

.extern _test_mode
.type _test_mode, %function

.globl _start
.type _start, %function
_start:
   /* Initialize stack pointer */
   la	t2, BOOT_Stack_End
   lw	t2, 0(t2)
   mv	sp, t2

   /* initialize stack content as 0xFF */
   li	t0, 0xFFFFFFFF
   la   t1, BOOT_Stack_Start
   lw	t1, 0(t1)
   li	a0, 0x4
stack_fill_ff:
   sw   t0, 0(t2)
   sub	t2, t2, a0
   bne	t1, t2, stack_fill_ff /* loop whilst r0 < r1 */
   sw   t0, 0(t2)
   
   jal	x1, _copy_rwdata
   jal	x1, cpu0_entry_start

_deadloop:
   j	_deadloop /* Should Never Be Here */

_copy_rwdata:
	la	a0, __rw_region_offset
	la	a2, __rw_region_length
	la	a4, __readwrite_start
	li	a3, 0x0
reloc_rwdata:
	beq	a2, a3, end_reloc_rwdata
	add a5, a3, a0
	lw	a1, 0(a5)
	add a5, a3, a4
	sw a1, 0(a5)
	addi	a3, a3, 0x4
	j	reloc_rwdata
end_reloc_rwdata:
	ret

BOOT_Stack_End:
	.word SPV_Stack_Pool + 0x400 - 4 

BOOT_Stack_Start:
	.word SPV_Stack_Pool