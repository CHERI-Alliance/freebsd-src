/*-
 * Copyright (c) 2015-2020 Ruslan Bukin <br@bsdpad.com>
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
#include <machine/setjmp.h>
#include <machine/riscvreg.h>

#include "assym.inc"
    
/*
 * One of the fu* or su* functions failed, return -1.
 */
ENTRY(fsu_fault)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a1))	/* Reset the handler function */
	EXIT_USER_ACCESS(a1)
fsu_fault_nopcb:
	li	a0, -1
	ret
END(fsu_fault)

/*
 * int casueword32(volatile uint32_t *, uint32_t, uint32_t *, uint32_t)
 */
ENTRY(casueword32)
	li	a4, (VM_MAXUSER_ADDRESS-3)
	bgeu	a0, a4, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a4))	/* And set it */
	ENTER_USER_ACCESS(a4)

	lr.w	a4, 0(PTR(a0))		/* Load-exclusive the data */
	bne	a4, a1, 1f		/* If not equal then exit */
	sc.w	a5, a3, 0(PTR(a0))	/* Store the new data */
	beqz	a5, 2f			/* Success */

1:	li	a5, 1			/* Normalize failure result */
2:
	EXIT_USER_ACCESS(a6)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a6))	/* Reset the fault handler */
	sw	a4, 0(PTR(a2))		/* Store the read data */
	mv	a0, a5			/* Success indicator */
	ret				/* Return */
END(casueword32)

/*
 * int casueword(volatile u_long *, u_long, u_long *, u_long)
 */
ENTRY(casueword)
	li	a4, (VM_MAXUSER_ADDRESS-7)
	bgeu	a0, a4, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a4))	/* And set it */
	ENTER_USER_ACCESS(a4)

	lr.d	a4, 0(PTR(a0))		/* Load-exclusive the data */
	bne	a4, a1, 1f		/* If not equal then exit */
	sc.d	a5, a3, 0(PTR(a0))	/* Store the new data */
	beqz	a5, 2f			/* Success */
1:	li	a5, 1			/* Normalize failure result */
2:
	EXIT_USER_ACCESS(a6)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a6))	/* Reset the fault handler */
	sd	a4, 0(PTR(a2))		/* Store the read data */
	mv	a0, a5			/* Success indicator */
	ret				/* Return */
END(casueword)

/*
 * int fubyte(volatile const void *)
 */
ENTRY(fubyte)
	li	a1, VM_MAXUSER_ADDRESS
	bgeu	a0, a1, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a1))	/* And set it */
	ENTER_USER_ACCESS(a1)
	lbu	a0, 0(PTR(a0))		/* Try loading the data */
	EXIT_USER_ACCESS(a1)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a1))	/* Reset the fault handler */
	ret				/* Return */
END(fubyte)

/*
 * int fuword(volatile const void *)
 */
ENTRY(fuword16)
	li	a1, (VM_MAXUSER_ADDRESS-1)
	bgeu	a0, a1, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a1))	/* And set it */
	ENTER_USER_ACCESS(a1)
	lhu	a0, 0(PTR(a0))		/* Try loading the data */
	EXIT_USER_ACCESS(a1)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a1))	/* Reset the fault handler */
	ret				/* Return */
END(fuword16)

/*
 * int32_t fueword32(volatile const void *, int32_t *)
 */
ENTRY(fueword32)
	li	a2, (VM_MAXUSER_ADDRESS-3)
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	lw	a0, 0(PTR(a0))		/* Try loading the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	sw	a0, 0(PTR(a1))		/* Save the data in kernel space */
	li	a0, 0			/* Success */
	ret				/* Return */
END(fueword32)

/*
 * long fueword(volatile const void *, int64_t *)
 * int64_t fueword64(volatile const void *, int64_t *)
 */
ENTRY(fueword)
EENTRY(fueword64)
	li	a2, (VM_MAXUSER_ADDRESS-7)
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	ld	a0, 0(PTR(a0))		/* Try loading the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	sd	a0, 0(PTR(a1))		/* Save the data in kernel space */
	li	a0, 0			/* Success */
	ret				/* Return */
EEND(fueword64)
END(fueword)

#ifdef __CHERI__
/*
 * int fueptr(volatile const void __capability *, intcap_t *)
 */
ENTRY(fueptr)
	li	a2, (VM_MAXUSER_ADDRESS-7)
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	lc	PTR(a0), 0(PTR(a0))	/* Try loading the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	sc	PTR(a0), 0(PTR(a1))		/* Save the data in kernel space */
	li	a0, 0			/* Success */
	ret				/* Return */
END(fueptr)
#endif

/*
 * int subyte(volatile void * __capability, int)
 */
ENTRY(subyte)
	li	a2, VM_MAXUSER_ADDRESS
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	sb	a1, 0(PTR(a0))		/* Try storing the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	li	a0, 0			/* Success */
	ret				/* Return */
END(subyte)

/*
 * int suword16(volatile void *, int)
 */
ENTRY(suword16)
	li	a2, (VM_MAXUSER_ADDRESS-1)
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	sh	a1, 0(PTR(a0))		/* Try storing the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	li	a0, 0			/* Success */
	ret				/* Return */
END(suword16)

/*
 * int suword32(volatile void *, int)
 */
ENTRY(suword32)
	li	a2, (VM_MAXUSER_ADDRESS-3)
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	sw	a1, 0(PTR(a0))		/* Try storing the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	li	a0, 0			/* Success */
	ret				/* Return */
END(suword32)

/*
 * int suword(volatile void *, long)
 */
ENTRY(suword)
EENTRY(suword64)
	li	a2, (VM_MAXUSER_ADDRESS-7)
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	sd	a1, 0(PTR(a0))		/* Try storing the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	li	a0, 0			/* Success */
	ret				/* Return */
EEND(suword64)
END(suword)

#ifdef __CHERI__
/*
 * int suptr(volatile void * __capability, intcap_t)
 */
ENTRY(suptr)
	li	a2, (VM_MAXUSER_ADDRESS-3)
	bgeu	a0, a2, fsu_fault_nopcb
	LA_PTR	PTR(a6), fsu_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(PTR(a6), PTR(a2))	/* And set it */
	ENTER_USER_ACCESS(a2)
	sc	PTR(a1), 0(PTR(a0))	/* Try storing the data */
	EXIT_USER_ACCESS(a2)
	SET_FAULT_HANDLER(PTR_NULL, PTR(a2))	/* Reset the fault handler */
	li	a0, 0			/* Success */
	ret				/* Return */
END(suptr)
#endif

ENTRY(setjmp)
	/* Store the stack pointer */
	S_PTR	PTR(sp), 0(PTR(a0))
	ADDI_PTR	PTR(a0), PTR(a0), PTR_WIDTH

	/* Store the general purpose registers and ra */
	S_PTR	PTR(s0), (0 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s1), (1 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s2), (2 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s3), (3 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s4), (4 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s5), (5 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s6), (6 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s7), (7 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s8), (8 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s9), (9 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s10), (10 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(s11), (11 * PTR_WIDTH)(PTR(a0))
	S_PTR	PTR(ra), (12 * PTR_WIDTH)(PTR(a0))

	/* Return value */
	li	a0, 0
	ret
END(setjmp)

ENTRY(longjmp)
	/* Restore the stack pointer */
	L_PTR	PTR(sp), 0(PTR(a0))
	ADDI_PTR	PTR(a0), PTR(a0), PTR_WIDTH

	/* Restore the general purpose registers and ra */
	L_PTR	PTR(s0), (0 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s1), (1 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s2), (2 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s3), (3 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s4), (4 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s5), (5 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s6), (6 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s7), (7 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s8), (8 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s9), (9 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s10), (10 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(s11), (11 * PTR_WIDTH)(PTR(a0))
	L_PTR	PTR(ra), (12 * PTR_WIDTH)(PTR(a0))

	/* Load the return value */
	mv	a0, a1
	ret
END(longjmp)
