/*-
 * Copyright (c) 2011-2017 Robert N. M. Watson
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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sysent.h>
#include <sys/systm.h>

#ifdef INVARIANTS
#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#endif

#include <cheri/cheri.h>
#include <cheri/cheric.h>

/* Set to -1 to prevent it from being zeroed with the rest of BSS */
static void *userspace_root_cap = (void *)(intptr_t)-1;

void
userspace_root_cap_init(void *cap)
{
	userspace_root_cap = cap;
}

/*
 * Build a new userspace capability derived from userspace_root_cap.
 * The resulting capability may include both read and execute permissions,
 * but not write, and will be a sentry capability. For architectures that use
 * flags, the flags for the resulting capability will be set based on what is
 * expected by userspace for the specified thread.
 */
void *
_cheri_capability_build_user_code(struct thread *td, uint32_t perms,
    ptraddr_t basep, size_t length, off_t off, const char* func, int line)
{
	void *tmpcap;

	KASSERT((perms & ~CHERI_CAP_USER_CODE_PERMS) == 0,
	    ("%s:%d: perms %x has permission not in CHERI_CAP_USER_CODE_PERMS %x",
	    func, line, perms, CHERI_CAP_USER_CODE_PERMS));

	tmpcap = _cheri_capability_build_user_rwx(
	    perms & CHERI_CAP_USER_CODE_PERMS, basep, length, off, func, line,
	    true);

	if (SV_PROC_FLAG(td->td_proc, SV_CHERI))
		tmpcap = cheri_capmode(tmpcap);

	return (cheri_sentry_create(tmpcap));
}

/*
 * Build a new userspace capability derived from userspace_root_cap.
 * The resulting capability may include read and write permissions, but
 * not execute.
 */
void *
_cheri_capability_build_user_data(uint32_t perms, ptraddr_t basep,
    size_t length, off_t off, const char* func, int line, bool exact)
{

	KASSERT((perms & ~CHERI_CAP_USER_DATA_PERMS) == 0,
	    ("%s:%d: perms %x has permission not in CHERI_CAP_USER_DATA_PERMS %x",
	    func, line, perms, CHERI_CAP_USER_DATA_PERMS));

	return (_cheri_capability_build_user_rwx(
	    perms & CHERI_CAP_USER_DATA_PERMS, basep, length, off, func, line,
	    exact));
}

/*
 * Build a new userspace capability derived from userspace_root_cap.
 * The resulting capability may include read, write, and execute permissions.
 *
 * This function violates W^X and its use is discouraged and the reason for
 * use should be documented in a comment when it is used.
 */
void *
_cheri_capability_build_user_rwx(uint32_t perms, ptraddr_t basep, size_t length,
    off_t off, const char* func __unused, int line __unused, bool exact)
{
	void *tmpcap;
#if defined(INVARIANTS) && defined(MAP_RESERVATIONS)
	vm_map_entry_t entry;
	vm_map_t map;
	vm_offset_t reservation;

	if (SV_CURPROC_FLAG(SV_CHERI)) {
		map = &curproc->p_vmspace->vm_map;
		vm_map_lock_read(map);
		KASSERT(vm_map_lookup_entry(map, basep, &entry),
		    ("%s:%d: vm_map does not contain basep 0x%zx "
		    "(length 0x%zu, offset 0x%ju)", func, line,
		    (size_t)basep, length, (uintmax_t)off));
		reservation = entry->reservation;
		for( ; basep + length > entry->end;
		    entry = vm_map_entry_succ(entry)) {
			/*
			 * Check that the created capability is within a
			 * single reservation.  This ensures we don't
			 * make capabilities that might alias with a
			 * later mapping.
			 */
			KASSERT((map->flags & MAP_RESERVATIONS) == 0 ||
			    entry->reservation == reservation,
			    ("Can't create a capability that spans reservations"));

			/*
			 * XXX: Disallow quarantined or abandoned pages?
			 *
			 * XXX: Require page maxprot to be a superset of
			 * perms?
			 */
		}
		vm_map_unlock_read(map);
	}
#endif

	tmpcap = _cheri_capability_build_user_rwx_unchecked(perms, basep,
	    length, off, func, line, exact);

	KASSERT(!exact || cheri_length_get(tmpcap) == length,
	    ("%s:%d: Constructed capability has wrong length 0x%zx != 0x%zx: "
	     "%#lp", func, line, cheri_length_get(tmpcap), length, tmpcap));

	return (tmpcap);
}

void *
_cheri_capability_build_user_rwx_unchecked(uint32_t perms, ptraddr_t basep,
    size_t length, off_t off, const char* func __unused, int line __unused,
    bool exact)
{
	return (cheri_offset_set(cheri_perms_and(cheri_bounds_set(
	    cheri_offset_set(userspace_root_cap, basep), length), perms), off));
}

void
cheri_sysvec_init(struct sysentvec *sv)
{
	ptraddr_t minuser, maxuser, padded_minuser;
	size_t user_length;

	KASSERT(sv->sv_vmspace_cap == 0, ("sv_vmspace_cap already set"));

	minuser = sv->sv_minuser;
	maxuser = sv->sv_maxuser;

	KASSERT(minuser >= VM_MINUSER_ADDRESS,
	    ("sv_minuser < VM_MINUSER_ADDRESS"));
	KASSERT(maxuser <= VM_MAXUSER_ADDRESS,
	    ("sv_maxuser > VM_MAXUSER_ADDRESS"));
	KASSERT("maxuser > minuser", ("sv_maxuser <= sv_minuser"));

	/*
	 * Create a userspace capability for maps created for this
	 * sysvec, nominally bounded by sv_minuser and sv_maxuser.
	 * Allow the lower bound to be imprecise if sv_minuser excludes
	 * the first page.
	 */
	user_length = maxuser - minuser;
	padded_minuser = CHERI_REPRESENTABLE_ALIGN_DOWN(minuser,
	    user_length);
	KASSERT(padded_minuser == minuser ||
	    minuser <= PAGE_SIZE, ("Unrepresentable base"));
	user_length = CHERI_REPRESENTABLE_LENGTH(user_length);
	KASSERT(maxuser - padded_minuser == user_length,
	    ("Unrepresentable length"));
	/*
	 * Use the unchecked version here because we're not in a syscall
	 * and the associated map is probably the kernel map.
	 */
	sv->sv_vmspace_cap = (uintptr_t)
	    cheri_capability_build_user_rwx_unchecked(
	    CHERI_CAP_USER_CODE_PERMS | CHERI_CAP_USER_DATA_PERMS |
	    CHERI_PERMS_SWALL, padded_minuser, user_length,
	    minuser - padded_minuser);
	KASSERT(cheri_tag_get(sv->sv_vmspace_cap),
	    ("sv_vmspace_cap untagged %#lp",
	     (void * __capability)sv->sv_vmspace_cap));
}
