/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2011-2018 Robert N. M. Watson
 * All rights reserved.
 * Copyright (c) 2020 John Baldwin
 *
 * Portions of this software were developed by SRI International and
 * the University of Cambridge Computer Laboratory under DARPA/AFRL
 * contract (FA8750-10-C-0237) ("CTSRD"), as part of the DARPA CRASH
 * research programme.
 *
 * Portions of this software were developed by SRI International and
 * the University of Cambridge Computer Laboratory (Department of
 * Computer Science and Technology) under DARPA contract
 * HR0011-18-C-0016 ("ECATS"), as part of the DARPA SSITH research
 * programme.
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

#ifndef _MACHINE_CHERIREG_H_
#define	_MACHINE_CHERIREG_H_

#define	CHERICAP_SIZE		__SIZEOF_CHERI_CAPABILITY__

#ifdef _KERNEL
/*
 * CHERI ISA-defined constants for capabilities -- suitable for inclusion from
 * assembly source code.
 */
#define	CHERI_PERM_WRITE		(1 << 0)	/* 0x00000001 */
#define	CHERI_PERM_LOAD_MUTABLE		(1 << 1)	/* 0x00000002 */
#define	CHERI_PERM_ELEVATE_LEVEL	(1 << 2)	/* 0x00000004 */
#define	CHERI_PERM_STORE_LEVEL		(1 << 3)	/* 0x00000008 */
#define	CHERI_PERM_CAPABILITY_LEVEL	(1 << 4)	/* 0x00000010 */
#define	CHERI_PERM_CAP			(1 << 5)	/* 0x00000020 */
#define	CHERI_PERM_SYSTEM_REGS		(1 << 16)	/* 0x00010000 */
#define	CHERI_PERM_EXECUTE		(1 << 17)	/* 0x00020000 */
#define	CHERI_PERM_READ			(1 << 18)	/* 0x00040000 */
#endif /* _KERNEL */

/*
 * User-defined permission bits.
 * These should be defined in cheriintrin.h, but aren't yet.
 */
#define	CHERI_PERM_SW0			(1 << 6)	/* 0x00000040 */
#define	CHERI_PERM_SW1			(1 << 7)	/* 0x00000080 */
#define	CHERI_PERM_SW2			(1 << 8)	/* 0x00000100 */
#define	CHERI_PERM_SW3			(1 << 9)	/* 0x00000200 */

/*
 * Re-define these because RVY cheriintrin.h uses different names.
 * XXX-AM: Ideally we unify on a single naming convention.
 */
#define	CHERI_PERM_STORE		CHERI_PERM_WRITE
#define	CHERI_PERM_LOAD			CHERI_PERM_READ
#define	CHERI_PERM_GLOBAL		CHERI_PERM_CAPABILITY_LEVEL
#define	CHERI_PERM_STORE_LOCAL_CAP	CHERI_PERM_STORE_LEVEL

/* Supported architecture permission bits feature flags */
#define	HAS_CHERI_PERM_CAP
#define	HAS_CHERI_PERM_LOAD_MUTABLE

/*
 * CHERI_PERMS_SWALL: Mask of all available software-defined permissions
 * CHERI_PERMS_HWALL: Mask of all available hardware-defined permissions
 */
#define	CHERI_PERMS_SWALL						\
	(CHERI_PERM_SW0 | CHERI_PERM_SW1 | CHERI_PERM_SW2 |		\
	CHERI_PERM_SW3)

#define	_CHERI_PERMS_HWALL_COMMON					\
	(CHERI_PERM_GLOBAL | CHERI_PERM_EXECUTE |			\
	CHERI_PERM_LOAD | CHERI_PERM_STORE |				\
	CHERI_PERM_STORE_LOCAL_CAP | CHERI_PERM_SYSTEM_REGS)
#define	CHERI_PERMS_HWALL						\
	(CHERI_PERM_CAP | CHERI_PERM_ELEVATE_LEVEL |			\
	CHERI_PERM_LOAD_MUTABLE | _CHERI_PERMS_HWALL_COMMON)

/*
 * vm_prot_t to capability permission bits
 */
#define	CHERI_PERMS_PROT2PERM_READ					\
	CHERI_PERM_LOAD
#define	CHERI_PERMS_PROT2PERM_WRITE					\
	CHERI_PERM_STORE
#define	CHERI_PERMS_PROT2PERM_READ_CAP					\
	(CHERI_PERM_CAP | CHERI_PERM_LOAD_MUTABLE |			\
	CHERI_PERM_ELEVATE_LEVEL)
#define	CHERI_PERMS_PROT2PERM_WRITE_CAP					\
	(CHERI_PERM_CAP | CHERI_PERM_STORE_LOCAL_CAP)
#define	CHERI_PERMS_PROT2PERM_EXEC					\
	(CHERI_PERM_EXECUTE | CHERI_PERMS_PROT2PERM_READ |		\
	    CHERI_PERMS_PROT2PERM_READ_CAP)

/*
 * Hardware defines a kind of tripartite taxonomy: memory, type, and CID.
 * They're all squished together in the permission bits, so define masks
 * that give us a kind of "kind" for capabilities.  A capability may belong
 * to zero, one, or more than one of these.
 */
#define _CHERI_PERMS_HWALL_MEMORY_COMMON				\
	(CHERI_PERM_EXECUTE | CHERI_PERM_LOAD | CHERI_PERM_STORE |	\
	CHERI_PERM_STORE_LOCAL_CAP)
#define CHERI_PERMS_HWALL_MEMORY					\
	(CHERI_PERM_CAP | CHERI_PERM_LOAD_MUTABLE |			\
	CHERI_PERM_ELEVATE_LEVEL | _CHERI_PERMS_HWALL_MEMORY_COMMON)

#define CHERI_PERMS_HWALL_OTYPE

/*
 * Basic userspace permission mask; CHERI_PERM_EXECUTE will be added for
 * executable capabilities ($pcc); CHERI_PERM_STORE, CHERI_PERM_STORE_CAP,
 * and CHERI_PERM_STORE_LOCAL_CAP will be added for data permissions ($dcc).
 *
 * All user software permissions are included along with
 * CHERI_PERM_SYSCALL.  CHERI_PERM_SW_VMEM will be added for
 * permissions returned from mmap().
 */
#define	_CHERI_PERMS_USERSPACE_COMMON					\
	(CHERI_PERM_GLOBAL | CHERI_PERM_LOAD  |				\
	(CHERI_PERMS_SWALL & ~(CHERI_PERM_SW_VMEM | CHERI_PERM_SYSCALL)))
#define	CHERI_PERMS_USERSPACE						\
	(CHERI_PERM_CAP | CHERI_PERM_LOAD_MUTABLE |			\
	CHERI_PERM_ELEVATE_LEVEL | _CHERI_PERMS_USERSPACE_COMMON)

#define	CHERI_PERMS_USERSPACE_RODATA					\
	(CHERI_PERMS_USERSPACE)

#define	CHERI_PERMS_USERSPACE_RODATA_NOCAP				\
	(CHERI_PERM_GLOBAL | CHERI_PERM_LOAD)

#define	CHERI_PERMS_USERSPACE_DATA					\
	(CHERI_PERMS_USERSPACE_RODATA | CHERI_PERM_STORE |		\
	CHERI_PERM_STORE_LOCAL_CAP)

#define	CHERI_PERMS_USERSPACE_CODE					\
	(CHERI_PERMS_USERSPACE_RODATA | CHERI_PERM_EXECUTE |		\
	CHERI_PERM_SYSCALL)

/*
 * Corresponding permission masks for kernel code and data; these are
 * currently a bit broad, and should be narrowed over time as the kernel
 * becomes more capability-aware.
 */
#define	CHERI_PERMS_KERNEL						\
	(CHERI_PERM_GLOBAL | CHERI_PERM_LOAD | CHERI_PERM_CAP |		\
	CHERI_PERM_LOAD_MUTABLE | CHERI_PERM_ELEVATE_LEVEL)

#define	CHERI_PERMS_KERNEL_RODATA					\
	(CHERI_PERMS_KERNEL)

#define	CHERI_PERMS_KERNEL_DATA						\
	(CHERI_PERMS_KERNEL_RODATA | CHERI_PERM_STORE |			\
	CHERI_PERM_STORE_LOCAL_CAP)

#define	CHERI_PERMS_KERNEL_CODE						\
	(CHERI_PERMS_KERNEL_RODATA | CHERI_PERM_EXECUTE |		\
	CHERI_PERM_SYSTEM_REGS)

/*
 * Permission mask that encodes the permission bits associated to
 * the RWX memory access control.
 * These are separate from the permission bits that encode other
 * properties of capabilities (e.g. sealing or ASR).
 */
#define	CHERI_PERMS_RWX_MASK						\
	(CHERI_PERM_LOAD | CHERI_PERM_STORE | CHERI_PERM_CAP |		\
	CHERI_PERM_STORE_LOCAL_CAP | CHERI_PERM_LOAD_MUTABLE |		\
	CHERI_PERM_EXECUTE | CHERI_PERM_SYSCALL)

#define	CHERI_FLAGS_CAP_MODE	0x0
#define	CHERI_FLAGS_CAP_MODE_MASK	0x1
#define	CHERI_FLAGS_LEGACY_MODE				\
	(~CHERI_FLAGS_CAP_MODE & CHERI_FLAGS_CAP_MODE_MASK)

/*
 * RV64Y defines a single otype bit for sealed capabilities.
 */
#define	CHERI_OTYPE_BITS	(1)

#ifdef _KERNEL
#define	CHERI_OTYPE_UNSEALED	(0l)
#define	CHERI_OTYPE_SENTRY	(1l)
#endif

/*
 * Derive an unbounded pointer before initial relocation.  For
 * purecap, derive the pointer from PCC.
 */
#ifdef __CHERI__
#define	CHERI_RODATA_PTR(x) ({						\
	__typeof__((0, x)) _p;						\
									\
	__asm__ (							\
	    "lly %0, %c1\n\t"						\
	    : "=C" (_p) : "i" (x));					\
	_p; })
#else
#define	CHERI_RODATA_PTR(x)	(&(*x))
#endif

#endif /* !_MACHINE_CHERIREG_H_ */
