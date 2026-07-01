/*-
 * Copyright (c) 2026 Capabilities Limited
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * This software was developed by Capabilities Limited with funding from
 * Innovate UK and the Department for Science, Innovation and Technology
 * for the adoption and diffusion of CHERI technology under project
 * 10168042 (“CheriBSD feature extraction, maturity, and testing”).
 */

#include <sys/systm.h>

#include <cheri/cheric.h>
#include <cheri/cherireg.h>

static const int perm_load_cap =
#if defined(HAS_CHERI_PERM_LOAD_STORE_CAP)
    CHERI_PERM_LOAD_CAP
#elif defined(HAS_CHERI_PERM_CAP)
    CHERI_PERM_CAP
#endif
    ;

/*
 * We clear the capability load permission so that memcpy doesn't load
 * any tags from src.
 */
void *
memcpy_data(void *dst, const void *src, size_t length)
{
	return (memcpy(dst, cheri_perms_clear(src, perm_load_cap), length));
}

void *
memmove_data(void *dst, const void *src, size_t length)
{
	return (memmove(dst, cheri_perms_clear(src, perm_load_cap), length));
}
