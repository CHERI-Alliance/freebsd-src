/*-
 * Copyright (c) 2011-2018 Robert N. M. Watson
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
#include <sys/signal.h>
#include <sys/systm.h>

#include <cheri/cheri.h>

#include <machine/riscvreg.h>

static const char *cheri_fault_type_descr[] = {
	[CHERI_EXCTYPE_FETCH_FAULT] = "pcc fault",
	[CHERI_EXCTYPE_DATA_FAULT] = " access fault",
	[CHERI_EXCTYPE_BRANCH_FAULT] = "branch fault",
};

static const char *cheri_fault_cause_descr[] = {
	[CHERI_EXCCODE_TAG] = "tag violation",
	[CHERI_EXCCODE_SEAL] = "seal violation",
	[CHERI_EXCCODE_PERMS] = "permission violation",
	[CHERI_EXCCODE_ADDRESS] = "address violation",
	[CHERI_EXCCODE_BOUNDS] = "bounds violation",
};

const char *
cheri_exccode_string(uint8_t fault_type, uint8_t cause)
{
	static char buf[64];

	if (fault_type >= nitems(cheri_fault_type_descr) ||
	    cheri_fault_type_descr[fault_type] == NULL ||
	    cause >= nitems(cheri_fault_cause_descr) ||
	    cheri_fault_cause_descr[cause] == NULL) {
		snprintf(buf, sizeof(buf), "exception type=%#x cause=%#x",
		    fault_type, cause);
	} else {
		snprintf(buf, sizeof(buf), "exception %s: %s",
		    cheri_fault_type_descr[fault_type],
		    cheri_fault_cause_descr[cause]);
	}
	return (buf);
}

int
cheri_stval_to_sicode(register_t stval)
{
	uint8_t exccode;

	exccode = TVAL_CAP_CAUSE(stval);
	switch (exccode) {
	case CHERI_EXCCODE_TAG:
		return (PROT_CHERI_TAG);
	case CHERI_EXCCODE_SEAL:
		return (PROT_CHERI_SEALED);
	case CHERI_EXCCODE_PERMS:
		return (PROT_CHERI_PERM);
	case CHERI_EXCCODE_ADDRESS:
		return (PROT_CHERI_INVALID_ADDRESS);
	case CHERI_EXCCODE_BOUNDS:
		return (PROT_CHERI_BOUNDS);
	default:
		printf(
		    "%s: Warning: Unknown exccode %u, returning si_code 0\n",
		    __func__, exccode);
		return (0);
	}
}
