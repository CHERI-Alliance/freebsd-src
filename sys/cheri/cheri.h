/*-
 * Copyright (c) 2011-2017 Robert N. M. Watson
 * Copyright (c) 2015 SRI International
 * All rights reserved.
 * 
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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

#ifndef _CHERI_CHERI_H_
#define _CHERI_CHERI_H_

#ifdef _KERNEL
struct thread;
/*
 * Functions to construct userspace capabilities.
 */
void *_cheri_capability_build_user_code(struct thread *td, uint32_t perms,
    ptraddr_t basep, size_t length, off_t off, const char* func, int line);
void *_cheri_capability_build_user_data(uint32_t perms, ptraddr_t basep,
     size_t length, off_t off, const char* func, int line, bool exact);
void *_cheri_capability_build_user_rwx(uint32_t perms, ptraddr_t basep,
    size_t length, off_t off, const char* func, int line, bool exact);
void *_cheri_capability_build_user_rwx_unchecked(uint32_t perms,
    ptraddr_t basep, size_t length, off_t off, const char* func, int line,
    bool exact);

#define cheri_capability_build_user_code(td, perms, basep, length, off)	\
	_cheri_capability_build_user_code(td, perms, basep, length, off, \
	    __func__, __LINE__)
#define cheri_capability_build_user_data(perms, basep, length, off)	\
	_cheri_capability_build_user_data(perms, basep, length, off,	\
	    __func__, __LINE__, true)
#define cheri_capability_build_inexact_user_data(perms, basep, length, off) \
	_cheri_capability_build_user_data(perms, basep, length, off,	\
	    __func__, __LINE__, false)
#define cheri_capability_build_user_rwx(perms, basep, length, off)	\
	_cheri_capability_build_user_rwx(perms, basep, length, off,	\
	    __func__, __LINE__, true)
#define cheri_capability_build_user_rwx_unchecked(perms, basep, length, off) \
	_cheri_capability_build_user_rwx_unchecked(perms, basep, length, \
	     off, __func__, __LINE__, true)

/* Root kernel capability */
extern void *kernel_root_cap;

/*
 * Initialize root caps.
 */
void userspace_root_cap_init(void *);

/*
 * Global sysctl definitions.
 */
SYSCTL_DECL(_security_cheri);
SYSCTL_DECL(_security_cheri_stats);
extern bool	security_cheri_bound_legacy_capabilities;

#endif /* _KERNEL */

#include <machine/cheri.h>

#endif /* _CHERI_CHERI_H_ */
