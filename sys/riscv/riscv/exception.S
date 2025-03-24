/*-
 * Copyright (c) 2015-2018 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Portions of this software were developed by SRI International and the
 * University of Cambridge Computer Laboratory under DARPA/AFRL contract
 * FA8750-10-C-0237 ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Portions of this software were developed by the University of Cambridge
 * Computer Laboratory as part of the CTSRD Project, with support from the
 * UK Higher Education Innovation Fund (HEIF).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include "assym.inc"

#include <machine/trap.h>
#include <machine/riscvreg.h>

.macro save_registers mode
	ADDI_PTR	PTR(sp), PTR(sp), -(TF_SIZE)

	S_PTR	PTR(ra), (TF_RA)(PTR(sp))
	S_PTR	PTR(tp), (TF_TP)(PTR(sp))
	S_PTR	PTR(gp), (TF_GP)(PTR(sp))

.if \mode == 0	/* We came from userspace. */
	/* Load our pcpu */
	L_PTR	PTR(tp), (TF_SIZE + KF_TP)(PTR(sp))
.endif

	S_PTR	PTR(t0), (TF_T + 0 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(t1), (TF_T + 1 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(t2), (TF_T + 2 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(t3), (TF_T + 3 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(t4), (TF_T + 4 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(t5), (TF_T + 5 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(t6), (TF_T + 6 * PTR_WIDTH)(PTR(sp))

	S_PTR	PTR(s0), (TF_S + 0 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s1), (TF_S + 1 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s2), (TF_S + 2 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s3), (TF_S + 3 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s4), (TF_S + 4 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s5), (TF_S + 5 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s6), (TF_S + 6 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s7), (TF_S + 7 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s8), (TF_S + 8 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s9), (TF_S + 9 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s10), (TF_S + 10 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(s11), (TF_S + 11 * PTR_WIDTH)(PTR(sp))

	S_PTR	PTR(a0), (TF_A + 0 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(a1), (TF_A + 1 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(a2), (TF_A + 2 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(a3), (TF_A + 3 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(a4), (TF_A + 4 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(a5), (TF_A + 5 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(a6), (TF_A + 6 * PTR_WIDTH)(PTR(sp))
	S_PTR	PTR(a7), (TF_A + 7 * PTR_WIDTH)(PTR(sp))

.if \mode == 1
	/* Store kernel sp */
	li	t1, TF_SIZE
	ADD_PTR	PTR(t0), PTR(sp), t1
	S_PTR	PTR(t0), (TF_SP)(PTR(sp))
.else
	/* Store user sp */
	csrr	PTR(t0), PTR_CSR(sscratch)
	S_PTR	PTR(t0), (TF_SP)(PTR(sp))
.endif
	MV_PTR	PTR(t0), PTR_NULL
	csrw	PTR_CSR(sscratch), PTR(t0)
	csrr	PTR(t0), PTR_CSR(sepc)
	S_PTR	PTR(t0), (TF_SEPC)(PTR(sp))
#ifdef __CHERI__
	csrr	PTR(t0), ddc
	csc	PTR(t0), (TF_DDC)(PTR(sp))
#endif

#ifdef __CHERI__
.if \mode == 0
	/* Load our DDC */
	MV_PTR	PTR(t0), PTR_NULL
	csrw	ddc, PTR(t0)
.endif
#endif

	csrr	t0, sstatus
	sd	t0, (TF_SSTATUS)(PTR(sp))

#ifdef __CHERI__
	csrr	t0, stval2
	sd	t0, (TF_STVAL2)(PTR(sp))
#endif

.if \mode == 1
	/* Disable user address access for supervisor mode exceptions. */
	li	t0, SSTATUS_SUM
	csrc	sstatus, t0
.endif
	csrr	t0, stval
	sd	t0, (TF_STVAL)(PTR(sp))
#ifdef __CHERI__
	csrr	t0, stval2
	sd	t0, (TF_STVAL2)(PTR(sp))
#endif
	csrr	t0, scause
	sd	t0, (TF_SCAUSE)(PTR(sp))
.endm

.macro load_registers mode
	ld	t0, (TF_SSTATUS)(PTR(sp))
.if \mode == 0
	/* Ensure user interrupts will be enabled on eret */
	li	t1, SSTATUS_SPIE
	or	t0, t0, t1
.else
	/*
	 * Disable interrupts for supervisor mode exceptions.
	 * For user mode exceptions we have already done this
	 * in do_ast.
	 */
	li	t1, ~SSTATUS_SIE
	and	t0, t0, t1
.endif
	csrw	sstatus, t0

.option push
#ifdef __CHERI__
	/*
	 * Switch to user DDC.  After this point, all stack accesses
	 * must use 'csp' instead of 'sp'.
	 */
	L_PTR	PTR(t0), (TF_DDC)(PTR(sp))
	csrw	ddc, PTR(t0)
#endif

	L_PTR	PTR(t0), (TF_SEPC)(PTR(sp))
	csrw	PTR_CSR(sepc), PTR(t0)

.if \mode == 0
	/* We go to userspace. Load user csp */
	L_PTR	PTR(t0), (TF_SP)(PTR(sp))
	csrw	PTR_CSR(sscratch), PTR(t0)

	/* Store our pcpu */
	S_PTR	PTR(tp), (TF_SIZE + KF_TP)(PTR(sp))
	L_PTR	PTR(tp), (TF_TP)(PTR(sp))

	/* And restore the user's global pointer */
	L_PTR	PTR(gp), (TF_GP)(PTR(sp))
.endif

	L_PTR	PTR(ra), (TF_RA)(PTR(sp))

	L_PTR	PTR(t0), (TF_T + 0 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(t1), (TF_T + 1 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(t2), (TF_T + 2 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(t3), (TF_T + 3 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(t4), (TF_T + 4 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(t5), (TF_T + 5 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(t6), (TF_T + 6 * PTR_WIDTH)(PTR(sp))

	L_PTR	PTR(s0), (TF_S + 0 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s1), (TF_S + 1 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s2), (TF_S + 2 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s3), (TF_S + 3 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s4), (TF_S + 4 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s5), (TF_S + 5 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s6), (TF_S + 6 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s7), (TF_S + 7 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s8), (TF_S + 8 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s9), (TF_S + 9 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s10), (TF_S + 10 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(s11), (TF_S + 11 * PTR_WIDTH)(PTR(sp))

	L_PTR	PTR(a0), (TF_A + 0 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(a1), (TF_A + 1 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(a2), (TF_A + 2 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(a3), (TF_A + 3 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(a4), (TF_A + 4 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(a5), (TF_A + 5 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(a6), (TF_A + 6 * PTR_WIDTH)(PTR(sp))
	L_PTR	PTR(a7), (TF_A + 7 * PTR_WIDTH)(PTR(sp))

	ADDI_PTR	PTR(sp), PTR(sp), (TF_SIZE)
.endm

.macro	do_ast
	/* Disable interrupts */
	csrr	a4, sstatus
1:
	csrci	sstatus, (SSTATUS_SIE)

	L_PTR	PTR(a1), PC_CURTHREAD(PTR(tp))
	lw	a2, TD_AST(PTR(a1))

	beqz	a2, 2f

	/* Restore interrupts */
	andi	a4, a4, (SSTATUS_SIE)
	csrs	sstatus, a4

	/* Handle the ast */
	MV_PTR	PTR(a0), PTR(sp)
	call	_C_LABEL(ast)

	/* Re-check for new ast scheduled */
	j	1b
2:
.endm

ENTRY(cpu_exception_handler)
	csrrw	PTR(sp), PTR_CSR(sscratch), PTR(sp)
	beqz	sp, 1f
	/* User mode detected */
	j	cpu_exception_handler_user
1:
	/* Supervisor mode detected */
	csrrw	PTR(sp), PTR_CSR(sscratch), PTR(sp)
	j	cpu_exception_handler_supervisor
END(cpu_exception_handler)

ENTRY(cpu_exception_handler_supervisor)
	save_registers 1
	MV_PTR	PTR(a0), PTR(sp)
	call	_C_LABEL(do_trap_supervisor)
	load_registers 1
	sret
END(cpu_exception_handler_supervisor)

ENTRY(cpu_exception_handler_user)
	save_registers 0
	MV_PTR	PTR(a0), PTR(sp)
	call	_C_LABEL(do_trap_user)
	do_ast
	load_registers 0
	csrrw	PTR(sp), PTR_CSR(sscratch), PTR(sp)
	sret
END(cpu_exception_handler_user)
