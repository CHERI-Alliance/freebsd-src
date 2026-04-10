/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 John Baldwin
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
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

#include <sys/stddef.h>
#include <sys/types.h>
#include <machine/cherireg.h>
#include <cheri_init_globals.h>

/* Referenced from locore. */
void kernel_cap_relocs_cb(void *arg, bool function, bool constant,
    ptraddr_t object, void **src);

void
kernel_cap_relocs_cb(void *arg, bool function, bool constant,
    ptraddr_t object, void **src)
{
	void *cap;

	cap = __builtin_cheri_address_set(arg, object);
	if (function) {
		cap = __builtin_cheri_perms_and(cap, CHERI_PERMS_KERNEL_CODE);
#ifdef CHERI_FLAGS_CAP_MODE
		cap = __builtin_cheri_flags_set(cap, CHERI_FLAGS_CAP_MODE);
#endif
	} else if (constant) {
		cap = __builtin_cheri_perms_and(cap, CHERI_PERMS_KERNEL_RODATA);
	} else {
		cap = __builtin_cheri_perms_and(cap, CHERI_PERMS_KERNEL_DATA);
	}
	*src = cap;
}

/* Can't include <sys/cheri.h>. */
typedef void (cap_relocs_cb)(void *arg, bool function, bool constant,
    ptraddr_t object, void **src);

int	init_linker_file_cap_relocs(const void *start_relocs,
	    const void *stop_relocs, void *data_cap, ptraddr_t base_addr,
	    bool can_set_code_bounds, cap_relocs_cb *cb, void *cb_arg);

int	init_linker_file_cap_irelocs(const void *start_relocs,
	    const void *stop_relocs, void *root_cap, ptraddr_t base_addr);

/* Can't include <sys/systm.h>. */
int	printf(const char *, ...) __printflike(1, 2);

int
init_linker_file_cap_relocs(const void *start_relocs, const void *stop_relocs,
    void *data_cap, ptraddr_t base_addr, bool can_set_code_bounds,
    cap_relocs_cb *cb, void *cb_arg)
{
	/*
	 * This cannot use cheri_init_globals_impl directly as symbols
	 * for kernel modules in the vnet and dpcpu sets need to use
	 * alternate base addresses and capabilities.  Instead, we
	 * invoke a caller-supplied callback on each capability to
	 * request the base address and source capability.
	 */
	for (const struct capreloc *reloc = start_relocs;
	     reloc < (const struct capreloc *)stop_relocs; reloc++) {
		const void **dest = __builtin_cheri_address_set(data_cap,
		    reloc->capability_location + base_addr);
		void *src;
		bool function, constant;

		if (reloc->object == 0) {
			*dest = 0;
			continue;
		}
		if (reloc->permissions & indirect_reloc_flag) {
			// Skip IFUNC relocations until link_elf_ireloc()
			continue;
		}
		function = reloc->permissions == function_reloc_flag ||
		    reloc->permissions == (function_reloc_flag |
		    code_reloc_flag);
		constant = reloc->permissions == constant_reloc_flag;
		if (reloc->permissions != 0 && !function && !constant) {
			printf("kldload: unexpected capreloc type %#zx\n",
			    reloc->permissions);
			return (-1);
		}
		cb(cb_arg, function, constant, reloc->object, &src);
		if ((!function || can_set_code_bounds) && reloc->size != 0)
			src = __builtin_cheri_bounds_set(src, reloc->size);
		src = (char *)src + reloc->offset;
		if (function) {
			/* Convert function pointers to sentries: */
			src = __builtin_cheri_seal_entry(src);
		}
		*dest = src;
	}

	return (0);
}

int
init_linker_file_cap_irelocs(const void *start_relocs, const void *stop_relocs,
    void *root_cap, ptraddr_t base_addr)
{
	for (const struct capreloc *reloc = start_relocs;
	     reloc < (const struct capreloc *)stop_relocs; reloc++) {
		const void **dest = __builtin_cheri_address_set(root_cap,
		    reloc->capability_location + base_addr);
		void *(*resolver)(void);
		if ((reloc->permissions & indirect_reloc_flag) == 0) {
			continue;
		}
		if (reloc->permissions != (function_reloc_flag |
		    indirect_reloc_flag)) {
			printf("kldload: unexpected ifunc capreloc type %#zx\n",
			    reloc->permissions);
			return (-1);
		}

		if (reloc->offset != 0) {
			printf("kldload: unexpected ifunc capreloc offset\n");
			return (-1);
		}

		resolver = __builtin_cheri_address_set(root_cap, reloc->object);
		resolver = __builtin_cheri_perms_and(resolver,
		    CHERI_PERMS_KERNEL_CODE);
#ifdef CHERI_FLAGS_CAP_MODE
		resolver = __builtin_cheri_flags_set(resolver,
		    CHERI_FLAGS_CAP_MODE);
#endif
		resolver = __builtin_cheri_seal_entry(resolver);
		*dest = resolver();
	}

	return (0);
}
