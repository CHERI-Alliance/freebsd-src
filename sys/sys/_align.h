/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 SRI International
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
 */
#ifndef _SYS__ALIGN_H_
#define	_SYS__ALIGN_H_

/*
 * Round p up to the alignment of the largest, non-floating point, type
 * that fits in a register.  In practice this has always been the size
 * of a pointer (but spelled in a wide varity of ways) and with CHERI
 * that needs to remain true to support file descriptor passing within
 * the kernel.
 *
 * Unlike historic implementations, _ALIGN macro preserves both the type
 * and provenance of p.
 *
 * These interfaces are obsolete and should be be used in new code.
 */
#define	_ALIGNBYTES	(sizeof(void *) - 1)
#define	_ALIGN(p)	__align_up((p), _ALIGNBYTES + 1)

#endif /* !_SYS__ALIGN_H_ */
