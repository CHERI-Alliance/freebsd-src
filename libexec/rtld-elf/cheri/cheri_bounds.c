/*-
 * Copyright (c) 2024 John Baldwin
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * This software was developed by SRI International, the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology), and Capabilities Limited under Defense Advanced Research
 * Projects Agency / Air Force Research Laboratory (DARPA/AFRL) Contract
 * No. FA8750-24-C-B047 ("DEC").
 */

#include <sys/types.h>

#include "rtld.h"
#include "rtld_malloc.h"

bool
create_pcc_caps(Obj_Entry *obj, const char *name)
{
	const Elf_Phdr *ph;
	const char *pcc_cap;
	unsigned long i, j;

	for (ph = obj->phdr; ph < obj->phdr + obj->phnum; ph++) {
		switch (ph->p_type) {
		case PT_CHERI_PCC:
			obj->npcc_caps++;
			break;
		}
	}

	if (obj->npcc_caps == 0)
		return (true);

	i = 0;
	obj->pcc_caps = xcalloc(obj->npcc_caps, sizeof(*obj->pcc_caps));
	for (ph = obj->phdr; ph < obj->phdr + obj->phnum; ph++) {
		switch (ph->p_type) {
		case PT_CHERI_PCC:
			pcc_cap = obj->text_rodata_cap + ph->p_vaddr;
			pcc_cap = cheri_bounds_set_exact(pcc_cap, ph->p_memsz);
			if (!cheri_tag_get(pcc_cap)) {
				_rtld_error("pcc_cap %#p is not exact for %s",
				    pcc_cap, name);
				return (false);
			}
			obj->pcc_caps[i] = pcc_cap;
			i++;
			break;
		}
	}

	/*
	 * Require each PCC capability to be non-overlapping with
	 * other PCC capabilities.
	 */
	for (i = 1; i < obj->npcc_caps; i++) {
		pcc_cap = obj->pcc_caps[i];
		for (j = 0; j < i; j++) {
			if (cheri_is_address_inbounds(pcc_cap,
				cheri_base_get(obj->pcc_caps[j])) ||
			    cheri_is_address_inbounds(obj->pcc_caps[j],
				cheri_base_get(pcc_cap))) {
				_rtld_error(
				    "Overlapping PCC capabilities for %s",
				    name);
				return (false);
			}
		}
	}
	return (true);
}

/*
 * Returns a code pointer to the instruction at the relative offset
 * into the mapped object.  If the object includes PCC bounds via
 * PT_CHERI_PCC headers, the pointer uses the bounds from the relevant
 * header.  Otherwise the pointer uses bounds for the entire object.
 */
const char *
pcc_cap(const Obj_Entry *obj, Elf_Off offset)
{
	Elf_Addr addr;
	const char *pcc_cap;

	if (obj->npcc_caps == 0) {
		pcc_cap = obj->text_rodata_cap + offset;
		return (cheri_perms_clear(pcc_cap, CAP_RELOC_REMOVE_PERMS));
	}

	addr = (Elf_Addr)(uintptr_t)obj->relocbase + offset;
	for (unsigned long i = 0; i < obj->npcc_caps; i++) {
		pcc_cap = obj->pcc_caps[i];
		if (addr >= (ptraddr_t)pcc_cap &&
		    addr < cheri_top_get(pcc_cap)) {
			pcc_cap = cheri_address_set(pcc_cap, addr);
			return (cheri_perms_clear(pcc_cap,
			    CAP_RELOC_REMOVE_PERMS));
		}
	}
	return (NULL);
}
