/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2002 Doug Rabson
 * All rights reserved.
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

#include <sys/systm.h>
#include <sys/event.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallsubr.h>

#include <compat/freebsd32/freebsd32.h>
#include <compat/freebsd32/freebsd32_signal.h>
#include <compat/freebsd32/freebsd32_proto.h>

CTASSERT(sizeof(struct sigaction32) == 24);

int
convert_sigevent32(struct sigevent32 *sig32, struct sigevent *sig)
{

	CP(*sig32, *sig, sigev_notify);
	switch (sig->sigev_notify) {
	case SIGEV_NONE:
		break;
	case SIGEV_THREAD_ID:
		CP(*sig32, *sig, sigev_notify_thread_id);
		/* FALLTHROUGH */
	case SIGEV_SIGNAL:
		CP(*sig32, *sig, sigev_signo);
		PTRIN_CP(*sig32, *sig, sigev_value.sival_ptr);
		break;
	case SIGEV_KEVENT:
		CP(*sig32, *sig, sigev_notify_kqueue);
		CP(*sig32, *sig, sigev_notify_kevent_flags);
		PTRIN_CP(*sig32, *sig, sigev_value.sival_ptr);
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

int
freebsd32_sigaction(struct thread *td, struct freebsd32_sigaction_args *uap)
{
	struct sigaction32 s32;
	struct sigaction sa, osa, *sap;
	int error;

	if (uap->act) {
		error = copyin(uap->act, &s32, sizeof(s32));
		if (error)
			return (error);
		sa.sa_handler = PTRIN(s32.sa_u);
		CP(s32, sa, sa_flags);
		CP(s32, sa, sa_mask);
		sap = &sa;
	} else
		sap = NULL;
	error = kern_sigaction(td, uap->sig, sap, &osa, 0);
	if (error == 0 && uap->oact != NULL) {
		s32.sa_u = PTROUT(osa.sa_handler);
		CP(osa, s32, sa_flags);
		CP(osa, s32, sa_mask);
		error = copyout(&s32, uap->oact, sizeof(s32));
	}
	return (error);
}

#ifdef COMPAT_FREEBSD4
int
freebsd4_freebsd32_sigaction(struct thread *td,
			     struct freebsd4_freebsd32_sigaction_args *uap)
{
	struct sigaction32 s32;
	struct sigaction sa, osa, *sap;
	int error;

	if (uap->act) {
		error = copyin(uap->act, &s32, sizeof(s32));
		if (error)
			return (error);
		sa.sa_handler = PTRIN(s32.sa_u);
		CP(s32, sa, sa_flags);
		CP(s32, sa, sa_mask);
		sap = &sa;
	} else
		sap = NULL;
	error = kern_sigaction(td, uap->sig, sap, &osa, KSA_FREEBSD4);
	if (error == 0 && uap->oact != NULL) {
		s32.sa_u = PTROUT(osa.sa_handler);
		CP(osa, s32, sa_flags);
		CP(osa, s32, sa_mask);
		error = copyout(&s32, uap->oact, sizeof(s32));
	}
	return (error);
}
#endif

#ifdef COMPAT_43
struct osigaction32 {
	uint32_t	sa_u;
	osigset_t	sa_mask;
	int		sa_flags;
};

#define	ONSIG	32

int
ofreebsd32_sigaction(struct thread *td,
			     struct ofreebsd32_sigaction_args *uap)
{
	struct osigaction32 s32;
	struct sigaction sa, osa, *sap;
	int error;

	if (uap->signum <= 0 || uap->signum >= ONSIG)
		return (EINVAL);

	if (uap->nsa) {
		error = copyin(uap->nsa, &s32, sizeof(s32));
		if (error)
			return (error);
		sa.sa_handler = PTRIN(s32.sa_u);
		CP(s32, sa, sa_flags);
		OSIG2SIG(s32.sa_mask, sa.sa_mask);
		sap = &sa;
	} else
		sap = NULL;
	error = kern_sigaction(td, uap->signum, sap, &osa, KSA_OSIGSET);
	if (error == 0 && uap->osa != NULL) {
		s32.sa_u = PTROUT(osa.sa_handler);
		CP(osa, s32, sa_flags);
		SIG2OSIG(osa.sa_mask, s32.sa_mask);
		error = copyout(&s32, uap->osa, sizeof(s32));
	}
	return (error);
}

struct sigvec32 {
	uint32_t	sv_handler;
	int		sv_mask;
	int		sv_flags;
};

int
ofreebsd32_sigvec(struct thread *td,
			  struct ofreebsd32_sigvec_args *uap)
{
	struct sigvec32 vec;
	struct sigaction sa, osa, *sap;
	int error;

	if (uap->signum <= 0 || uap->signum >= ONSIG)
		return (EINVAL);

	if (uap->nsv) {
		error = copyin(uap->nsv, &vec, sizeof(vec));
		if (error)
			return (error);
		sa.sa_handler = PTRIN(vec.sv_handler);
		OSIG2SIG(vec.sv_mask, sa.sa_mask);
		sa.sa_flags = vec.sv_flags;
		sa.sa_flags ^= SA_RESTART;
		sap = &sa;
	} else
		sap = NULL;
	error = kern_sigaction(td, uap->signum, sap, &osa, KSA_OSIGSET);
	if (error == 0 && uap->osv != NULL) {
		vec.sv_handler = PTROUT(osa.sa_handler);
		SIG2OSIG(osa.sa_mask, vec.sv_mask);
		vec.sv_flags = osa.sa_flags;
		vec.sv_flags &= ~SA_NOCLDWAIT;
		vec.sv_flags ^= SA_RESTART;
		error = copyout(&vec, uap->osv, sizeof(vec));
	}
	return (error);
}

struct sigstack32 {
	uint32_t	ss_sp;
	int		ss_onstack;
};

int
ofreebsd32_sigstack(struct thread *td,
			    struct ofreebsd32_sigstack_args *uap)
{
	struct sigstack32 s32;
	struct sigstack nss, oss;
	int error = 0, unss;

	if (uap->nss != NULL) {
		error = copyin(uap->nss, &s32, sizeof(s32));
		if (error)
			return (error);
		nss.ss_sp = PTRIN(s32.ss_sp);
		CP(s32, nss, ss_onstack);
		unss = 1;
	} else {
		unss = 0;
	}
	oss.ss_sp = td->td_sigstk.ss_sp;
	oss.ss_onstack = sigonstack(cpu_getstack(td));
	if (unss) {
		td->td_sigstk.ss_sp = nss.ss_sp;
		td->td_sigstk.ss_size = 0;
		td->td_sigstk.ss_flags |= (nss.ss_onstack & SS_ONSTACK);
		td->td_pflags |= TDP_ALTSTACK;
	}
	if (uap->oss != NULL) {
		s32.ss_sp = PTROUT(oss.ss_sp);
		CP(oss, s32, ss_onstack);
		error = copyout(&s32, uap->oss, sizeof(s32));
	}
	return (error);
}
#endif

void
siginfo_to_siginfo32(const siginfo_t *src, struct __siginfo32 *dst)
{
	bzero(dst, sizeof(*dst));
	dst->si_signo = src->si_signo;
	dst->si_errno = src->si_errno;
	dst->si_code = src->si_code;
	dst->si_pid = src->si_pid;
	dst->si_uid = src->si_uid;
	dst->si_status = src->si_status;
	dst->si_addr = (uintptr_t)src->si_addr;
	dst->si_value.sival_int = src->si_value.sival_int;
	dst->si_timerid = src->si_timerid;
	dst->si_overrun = src->si_overrun;
}

int
freebsd32_sigtimedwait(struct thread *td, struct freebsd32_sigtimedwait_args *uap)
{
	struct timespec32 ts32;
	struct timespec ts;
	struct timespec *timeout;
	sigset_t set;
	ksiginfo_t ksi;
	struct __siginfo32 si32;
	int error;

	if (uap->timeout) {
		error = copyin(uap->timeout, &ts32, sizeof(ts32));
		if (error)
			return (error);
		ts.tv_sec = ts32.tv_sec;
		ts.tv_nsec = ts32.tv_nsec;
		timeout = &ts;
	} else
		timeout = NULL;

	error = copyin(uap->set, &set, sizeof(set));
	if (error)
		return (error);

	error = kern_sigtimedwait(td, set, &ksi, timeout);
	if (error)
		return (error);

	if (uap->info) {
		siginfo_to_siginfo32(&ksi.ksi_info, &si32);
		error = copyout(&si32, uap->info, sizeof(struct __siginfo32));
	}

	if (error == 0)
		td->td_retval[0] = ksi.ksi_signo;
	return (error);
}

int
freebsd32_sigwaitinfo(struct thread *td, struct freebsd32_sigwaitinfo_args *uap)
{
	ksiginfo_t ksi;
	struct __siginfo32 si32;
	sigset_t set;
	int error;

	error = copyin(uap->set, &set, sizeof(set));
	if (error)
		return (error);

	error = kern_sigtimedwait(td, set, &ksi, NULL);
	if (error)
		return (error);

	if (uap->info) {
		siginfo_to_siginfo32(&ksi.ksi_info, &si32);
		error = copyout(&si32, uap->info, sizeof(struct __siginfo32));
	}	
	if (error == 0)
		td->td_retval[0] = ksi.ksi_signo;
	return (error);
}

#ifndef _FREEBSD32_SYSPROTO_H_
struct freebsd32_sigqueue_args {
        pid_t pid;
        int signum;
        /* union sigval32 */ int value;
};
#endif
int
freebsd32_sigqueue(struct thread *td, struct freebsd32_sigqueue_args *uap)
{
	union sigval sv;

	/*
	 * On 32-bit ABIs, sival_int and sival_ptr are the same.
	 * On 64-bit little-endian ABIs, the low bits are the same.
	 * In 64-bit big-endian ABIs, sival_int overlaps with
	 * sival_ptr's HIGH bits.  We choose to support sival_int
	 * rather than sival_ptr in this case as it seems to be
	 * more common.
	 */
	bzero(&sv, sizeof(sv));
	sv.sival_int = (uint32_t)(uint64_t)uap->value;

	return (kern_sigqueue(td, uap->pid, uap->signum, &sv));
}
