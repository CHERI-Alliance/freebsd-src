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

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/sysproto.h>

int
sys_cheri_cidcap_alloc(struct thread *td, struct cheri_cidcap_alloc_args *uap)
{
	return (ENOSYS);
}

int
sys_cheri_revoke_get_shadow(struct thread *td,
    struct cheri_revoke_get_shadow_args *uap)
{
	return (ENOSYS);
}

int
sys_cheri_revoke(struct thread *td, struct cheri_revoke_args *uap)
{
	return (ENOSYS);
}

int
sys_msetname(struct thread *td, struct msetname_args *uap)
{
	return (ENOSYS);
}
