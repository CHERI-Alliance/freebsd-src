/*-
 * Copyright (c) 2014 Andrew Turner
 * Copyright (c) 2014 The FreeBSD Foundation
 *
 * Portions of this software were developed by Andrew Turner
 * under sponsorship from the FreeBSD Foundation
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
 *
 */

#include <machine/asm.h>
#include <machine/setjmp.h>
#include <sys/elf_common.h>

ENTRY(_setjmp)
	/* Store the magic value and stack pointer */
	ldr	x8, .Lmagic
	mov	PTR(9), PTRN(sp)
	stp	PTR(8), PTR(9), [PTR(0)], #(PTR_WIDTH * 2)

	/* Store the general purpose registers and lr */
	stp	PTR(19), PTR(20), [PTR(0)], #(PTR_WIDTH * 2)
	stp	PTR(21), PTR(22), [PTR(0)], #(PTR_WIDTH * 2)
	stp	PTR(23), PTR(24), [PTR(0)], #(PTR_WIDTH * 2)
	stp	PTR(25), PTR(26), [PTR(0)], #(PTR_WIDTH * 2)
	stp	PTR(27), PTR(28), [PTR(0)], #(PTR_WIDTH * 2)
	stp	PTR(29), PTRN(lr), [PTR(0)], #(PTR_WIDTH * 2)

#ifndef _STANDALONE
	/* Store the vfp registers */
	stp	d8, d9, [PTR(0)], #16
	stp	d10, d11, [PTR(0)], #16
	stp	d12, d13, [PTR(0)], #16
	stp	d14, d15, [PTR(0)], #16
#endif

	/* Return value */
	mov	x0, #0
	ret
	.align	3
.Lmagic:
	.quad	_JB_MAGIC__SETJMP
END(_setjmp)

ENTRY(_longjmp)
	/* Check the magic value */
	ldr	PTR(8), [PTR(0)], #(PTR_WIDTH)
	ldr	x9, .Lmagic
	cmp	x8, x9
	b.ne	botch

	/* Restore the stack pointer */
	ldr	PTR(8), [PTR(0)], #(PTR_WIDTH)
	mov	PTRN(sp), PTR(8)

	/* Restore the general purpose registers and lr */
	ldp	PTR(19), PTR(20), [PTR(0)], #(PTR_WIDTH * 2)
	ldp	PTR(21), PTR(22), [PTR(0)], #(PTR_WIDTH * 2)
	ldp	PTR(23), PTR(24), [PTR(0)], #(PTR_WIDTH * 2)
	ldp	PTR(25), PTR(26), [PTR(0)], #(PTR_WIDTH * 2)
	ldp	PTR(27), PTR(28), [PTR(0)], #(PTR_WIDTH * 2)
	ldp	PTR(29), PTRN(lr), [PTR(0)], #(PTR_WIDTH * 2)

#ifndef _STANDALONE
	/* Restore the vfp registers */
	ldp	d8, d9, [PTR(0)], #16
	ldp	d10, d11, [PTR(0)], #16
	ldp	d12, d13, [PTR(0)], #16
	ldp	d14, d15, [PTR(0)], #16
#endif

	/* Load the return value */
	cmp	x1, #0
	csinc	x0, x1, xzr, ne
	ret

botch:
#ifdef _STANDALONE
	b	botch
#else
	bl	_C_LABEL(longjmperror)
	bl	_C_LABEL(abort)
#endif
END(_longjmp)

GNU_PROPERTY_AARCH64_FEATURE_1_NOTE(GNU_PROPERTY_AARCH64_FEATURE_1_VAL)
