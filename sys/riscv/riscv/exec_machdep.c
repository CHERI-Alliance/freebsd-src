/*-
 * Copyright (c) 2014 Andrew Turner
 * Copyright (c) 2015-2026 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Portions of this software were developed by SRI International and the
 * University of Cambridge Computer Laboratory under DARPA/AFRL contract
 * FA8750-10-C-0237 ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Portions of this software were developed by the University of Cambridge
 * Computer Laboratory as part of the CTSRD Project, with support from the
 * UK Higher Education Innovation Fund (HEIF).
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
#include <sys/systm.h>
#include <sys/exec.h>
#include <sys/imgact.h>
#include <sys/kdb.h>
#include <sys/kernel.h>
#include <sys/ktr.h>
#include <sys/limits.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/signalvar.h>
#include <sys/syscallsubr.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/ucontext.h>

#include <machine/cpu.h>
#include <machine/fpe.h>
#include <machine/kdb.h>
#include <machine/pcb.h>
#include <machine/pte.h>
#include <machine/riscvreg.h>
#include <machine/sbi.h>
#include <machine/trap.h>
#include <machine/vector.h>
#include <machine/md_var.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>

#ifdef __CHERI__
#include <cheri/cheri.h>
#endif

static void get_fpcontext(struct thread *td, mcontext_t *mcp);
static void set_fpcontext(struct thread *td, mcontext_t *mcp);

#define	CTX_SIZE_VS(buf_size)					\
    roundup2(sizeof(struct vector_context) + (buf_size),	\
    _Alignof(struct vector_context))

#ifdef __CHERI__
_Static_assert(sizeof(mcontext_t) == 1152, "mcontext_t size incorrect");
_Static_assert(sizeof(ucontext_t) == 1248, "ucontext_t size incorrect");
_Static_assert(sizeof(siginfo_t) == 112, "siginfo_t size incorrect");
#else
_Static_assert(sizeof(mcontext_t) == 864, "mcontext_t size incorrect");
_Static_assert(sizeof(ucontext_t) == 936, "ucontext_t size incorrect");
_Static_assert(sizeof(siginfo_t) == 80, "siginfo_t size incorrect");
#endif

/*
 * XXX: CHERI TODO: Eventually 'struct reg' should use capregs for purecap
 * which would make this much cleaner.
 */
int
fill_regs(struct thread *td, struct reg *regs)
{
	struct trapframe *frame;
#ifdef __CHERI__
	u_int i;
#endif

	frame = td->td_frame;
	regs->sepc = (__cheri_addr register_t)frame->tf_sepc;
	regs->sstatus = frame->tf_sstatus;
	regs->ra = (__cheri_addr register_t)frame->tf_ra;
	regs->sp = (__cheri_addr register_t)frame->tf_sp;
	regs->gp = (__cheri_addr register_t)frame->tf_gp;
	regs->tp = (__cheri_addr register_t)frame->tf_tp;

#ifdef __CHERI__
	for (i = 0; i < nitems(regs->t); i++)
		regs->t[i] = (register_t)frame->tf_t[i];
	for (i = 0; i < nitems(regs->s); i++)
		regs->s[i] = (register_t)frame->tf_s[i];
	for (i = 0; i < nitems(regs->a); i++)
		regs->a[i] = (register_t)frame->tf_a[i];
#else
	memcpy(regs->t, frame->tf_t, sizeof(regs->t));
	memcpy(regs->s, frame->tf_s, sizeof(regs->s));
	memcpy(regs->a, frame->tf_a, sizeof(regs->a));
#endif

	return (0);
}

int
set_regs(struct thread *td, struct reg *regs)
{
	struct trapframe *frame;
#ifdef __CHERI__
	u_int i;
#endif

	frame = td->td_frame;
#ifdef __CHERI__
	frame->tf_sepc = cheri_address_set(frame->tf_sepc, regs->sepc);
#else
	frame->tf_sepc = regs->sepc;
#endif
	frame->tf_ra = (uintptr_t)regs->ra;
	frame->tf_sp = (uintptr_t)regs->sp;
	frame->tf_gp = (uintptr_t)regs->gp;
	frame->tf_tp = (uintptr_t)regs->tp;

#ifdef __CHERI__
	for (i = 0; i < nitems(regs->t); i++)
		frame->tf_t[i] = (uintptr_t)regs->t[i];
	for (i = 0; i < nitems(regs->s); i++)
		frame->tf_s[i] = (uintptr_t)regs->s[i];
	for (i = 0; i < nitems(regs->a); i++)
		frame->tf_a[i] = (uintptr_t)regs->a[i];
#else
	memcpy(frame->tf_t, regs->t, sizeof(frame->tf_t));
	memcpy(frame->tf_s, regs->s, sizeof(frame->tf_s));
	memcpy(frame->tf_a, regs->a, sizeof(frame->tf_a));
#endif

	return (0);
}

int
fill_fpregs(struct thread *td, struct fpreg *regs)
{
	struct pcb *pcb;

	pcb = td->td_pcb;

	if ((pcb->pcb_fpflags & PCB_FP_STARTED) != 0) {
		/*
		 * If we have just been running FPE instructions we will
		 * need to save the state to memcpy it below.
		 */
		if (td == curthread)
			fpe_state_save(td);

		memcpy(regs->fp_x, pcb->pcb_x, sizeof(regs->fp_x));
		regs->fp_fcsr = pcb->pcb_fcsr;
	} else
		memset(regs, 0, sizeof(*regs));

	return (0);
}

int
set_fpregs(struct thread *td, struct fpreg *regs)
{
	struct trapframe *frame;
	struct pcb *pcb;

	frame = td->td_frame;
	pcb = td->td_pcb;

	memcpy(pcb->pcb_x, regs->fp_x, sizeof(regs->fp_x));
	pcb->pcb_fcsr = regs->fp_fcsr;
	pcb->pcb_fpflags |= PCB_FP_STARTED;
	frame->tf_sstatus &= ~SSTATUS_FS_MASK;
	frame->tf_sstatus |= SSTATUS_FS_CLEAN;

	return (0);
}

int
fill_dbregs(struct thread *td, struct dbreg *regs)
{

	panic("fill_dbregs");
}

int
set_dbregs(struct thread *td, struct dbreg *regs)
{

	panic("set_dbregs");
}

#ifdef __CHERI__
/* Number of capability registers in 'struct capreg'. */
#define	NCAPREGS	(offsetof(struct capreg, tagmask) / sizeof(uintptr_t))

/*
 * These assume that the capabilities in struct trapframe
 * follow the same layout as struct capreg.
 */
_Static_assert(offsetof(struct trapframe, tf_ra) ==
    offsetof(struct capreg, cra), "cra mismatch");
_Static_assert(offsetof(struct trapframe, tf_sp) ==
    offsetof(struct capreg, csp), "csp mismatch");
_Static_assert(offsetof(struct trapframe, tf_gp) ==
    offsetof(struct capreg, cgp), "cgp mismatch");
_Static_assert(offsetof(struct trapframe, tf_tp) ==
    offsetof(struct capreg, ctp), "ctp mismatch");
_Static_assert(offsetof(struct trapframe, tf_t) ==
    offsetof(struct capreg, ct), "ct[] mismatch");
_Static_assert(offsetof(struct trapframe, tf_s) ==
    offsetof(struct capreg, cs), "cs[] mismatch");
_Static_assert(offsetof(struct trapframe, tf_a) ==
    offsetof(struct capreg, ca), "ca[] mismatch");
_Static_assert(offsetof(struct trapframe, tf_a) ==
    offsetof(struct capreg, ca), "ca[] mismatch");
_Static_assert(offsetof(struct trapframe, tf_sepc) ==
    offsetof(struct capreg, sepcc), "sepcc mismatch");
_Static_assert(offsetof(struct trapframe, tf_ddc) ==
    offsetof(struct capreg, ddc), "ddc mismatch");
#endif

void
exec_setregs(struct thread *td, struct image_params *imgp, uintptr_t stack)
{
	struct trapframe *tf;
	struct pcb *pcb;

	tf = td->td_frame;
	pcb = td->td_pcb;

	memset(tf, 0, sizeof(struct trapframe));

#ifdef __CHERI__
	if (SV_PROC_FLAG(td->td_proc, SV_CHERI)) {
		tf->tf_a[0] = (uintptr_t)imgp->auxv;
		tf->tf_sp = stack;
		tf->tf_sepc = (uintptr_t)cheri_exec_pcc(td, imgp);
		td->td_proc->p_md.md_sigcode = cheri_sigcode_capability(td);
	} else
#endif
	{
		tf->tf_a[0] = (ptraddr_t)stack;
		tf->tf_sp = STACKALIGN((ptraddr_t)stack);
#ifdef __CHERI__
		legacyabi_thread_setregs(td, imgp->entry_addr);
#else
		tf->tf_sepc = imgp->entry_addr;
#endif
	}
	tf->tf_ra = tf->tf_sepc;

	pcb->pcb_fpflags &= ~PCB_FP_STARTED;
}

/* Sanity check these are the same size, they will be memcpy'd to and fro */
CTASSERT(sizeof(((struct trapframe *)0)->tf_a) ==
    sizeof((struct gpregs *)0)->gp_a);
CTASSERT(sizeof(((struct trapframe *)0)->tf_s) ==
    sizeof((struct gpregs *)0)->gp_s);
CTASSERT(sizeof(((struct trapframe *)0)->tf_t) ==
    sizeof((struct gpregs *)0)->gp_t);
#ifndef __CHERI__
CTASSERT(sizeof(((struct trapframe *)0)->tf_a) ==
    sizeof((struct reg *)0)->a);
CTASSERT(sizeof(((struct trapframe *)0)->tf_s) ==
    sizeof((struct reg *)0)->s);
CTASSERT(sizeof(((struct trapframe *)0)->tf_t) ==
    sizeof((struct reg *)0)->t);
#endif

int
get_mcontext(struct thread *td, mcontext_t *mcp, int clear_ret)
{
	struct trapframe *tf = td->td_frame;

	memcpy(mcp->mc_gpregs.gp_t, tf->tf_t, sizeof(mcp->mc_gpregs.gp_t));
	memcpy(mcp->mc_gpregs.gp_s, tf->tf_s, sizeof(mcp->mc_gpregs.gp_s));
	memcpy(mcp->mc_gpregs.gp_a, tf->tf_a, sizeof(mcp->mc_gpregs.gp_a));

	if (clear_ret & GET_MC_CLEAR_RET) {
		mcp->mc_gpregs.gp_a[0] = 0;
		mcp->mc_gpregs.gp_t[0] = 0; /* clear syscall error */
	}

	mcp->mc_gpregs.gp_ra = tf->tf_ra;
	mcp->mc_gpregs.gp_sp = tf->tf_sp;
	mcp->mc_gpregs.gp_gp = tf->tf_gp;
	mcp->mc_gpregs.gp_tp = tf->tf_tp;
	mcp->mc_gpregs.gp_sepc = tf->tf_sepc;
	mcp->mc_gpregs.gp_sstatus = tf->tf_sstatus;
#ifdef __CHERI__
	mcp->mc_gpregs.gp_ddc = tf->tf_ddc;
#endif
	get_fpcontext(td, mcp);

	return (0);
}

static int
restore_vector_state(struct pcb *pcb, struct riscv_reg_context *ctx,
    vm_offset_t addr)
{
	struct vector_context vs_ctx;
	size_t buf_size;
	int error;

	/* Ensure vector engine present. */
	if (!has_vector)
		return (EINVAL);

	/* Ensure vector state is initialized. */
	if (pcb->pcb_vsaved == NULL)
		return (EINVAL);

	buf_size = vector_get_size();
	if (ctx->ctx_size != CTX_SIZE_VS(buf_size))
		return (EINVAL);

	/* Copy the vector registers. */
	error = copyin((const void *)addr, &vs_ctx, sizeof(vs_ctx));
	if (error != 0)
		return (error);

	/* Copy the vector data. */
	if (copyin((void *)(addr + sizeof(vs_ctx)), pcb->pcb_vsaved,
	    buf_size) != 0)
		return (EINVAL);

	/* Restore pcb registers. */
	pcb->pcb_vstart = vs_ctx.vs_vstart;
	pcb->pcb_vl = vs_ctx.vs_vl;
	pcb->pcb_vtype = vs_ctx.vs_vtype;
	pcb->pcb_vcsr = vs_ctx.vs_vcsr;
	pcb->pcb_vsflags |= PCB_VS_STARTED;

	return (0);
}

int
set_mcontext(struct thread *td, mcontext_t *mcp)
{
	struct trapframe *tf;
	struct riscv_reg_context ctx;
	struct pcb *pcb;
	vm_offset_t addr;
	int error, seen_types;
	bool done;
	register_t new_sstatus;

	tf = td->td_frame;

	new_sstatus = mcp->mc_gpregs.gp_sstatus;

	/*
	 * Permit changes to the USTATUS bits of SSTATUS.
	 *
	 * Ignore writes to read-only bits (SD, XS).
	 *
	 * Ignore writes to the FS field as set_fpcontext() will set
	 * it explicitly.
	 */
	if (((new_sstatus ^ tf->tf_sstatus) &
	    ~(SSTATUS_SD | SSTATUS_XS_MASK | SSTATUS_FS_MASK | SSTATUS_UPIE |
	    SSTATUS_UIE)) != 0)
		return (EINVAL);

	memcpy(tf->tf_t, mcp->mc_gpregs.gp_t, sizeof(tf->tf_t));
	memcpy(tf->tf_s, mcp->mc_gpregs.gp_s, sizeof(tf->tf_s));
	memcpy(tf->tf_a, mcp->mc_gpregs.gp_a, sizeof(tf->tf_a));

	tf->tf_ra = mcp->mc_gpregs.gp_ra;
	tf->tf_sp = mcp->mc_gpregs.gp_sp;
	tf->tf_gp = mcp->mc_gpregs.gp_gp;
	tf->tf_sepc = mcp->mc_gpregs.gp_sepc;
	tf->tf_sstatus = mcp->mc_gpregs.gp_sstatus;
#ifdef __CHERI__
	tf->tf_ddc = mcp->mc_gpregs.gp_ddc;
#endif

	set_fpcontext(td, mcp);

	if (mcp->mc_ptr == 0)
		return (0);

	/* Read any register contexts we find */
	addr = mcp->mc_ptr;
	pcb = td->td_pcb;

#define	CTX_TYPE_VS	(1 << 0)

	seen_types = 0;
	done = false;
	do {
		if (!__is_aligned(addr, _Alignof(struct riscv_reg_context)))
			return (EINVAL);

		error = copyin((const void *)addr, &ctx, sizeof(ctx));
		if (error != 0)
			return (error);

		switch (ctx.ctx_id) {
		case RISCV_CTX_MAGIC_VS:
			if ((seen_types & CTX_TYPE_VS) != 0)
				return (EINVAL);
			seen_types |= CTX_TYPE_VS;
			error = restore_vector_state(pcb, &ctx, addr);
			if (error)
				return (EINVAL);
			break;
		case RISCV_CTX_MAGIC_END:
			done = true;
			break;
		default:
			return (EINVAL);
		}
		addr += ctx.ctx_size;
	} while (!done);

#undef	CTX_TYPE_VS

	return (0);
}

static void
get_fpcontext(struct thread *td, mcontext_t *mcp)
{
	struct pcb *curpcb;

	critical_enter();

	curpcb = curthread->td_pcb;

	KASSERT(td->td_pcb == curpcb, ("Invalid fpe pcb"));

	if ((curpcb->pcb_fpflags & PCB_FP_STARTED) != 0) {
		/*
		 * If we have just been running FPE instructions we will
		 * need to save the state to memcpy it below.
		 */
		fpe_state_save(td);

		KASSERT((curpcb->pcb_fpflags & ~PCB_FP_USERMASK) == 0,
		    ("Non-userspace FPE flags set in get_fpcontext"));
		memcpy(mcp->mc_fpregs.fp_x, curpcb->pcb_x,
		    sizeof(mcp->mc_fpregs.fp_x));
		mcp->mc_fpregs.fp_fcsr = curpcb->pcb_fcsr;
		mcp->mc_fpregs.fp_flags = curpcb->pcb_fpflags;
		mcp->mc_flags |= _MC_FP_VALID;
	}

	critical_exit();
}

static void
set_fpcontext(struct thread *td, mcontext_t *mcp)
{
	struct pcb *curpcb;

	td->td_frame->tf_sstatus &= ~SSTATUS_FS_MASK;
	td->td_frame->tf_sstatus |= SSTATUS_FS_OFF;

	critical_enter();

	if ((mcp->mc_flags & _MC_FP_VALID) != 0) {
		curpcb = curthread->td_pcb;
		/* FPE usage is enabled, override registers. */
		memcpy(curpcb->pcb_x, mcp->mc_fpregs.fp_x,
		    sizeof(mcp->mc_fpregs.fp_x));
		curpcb->pcb_fcsr = mcp->mc_fpregs.fp_fcsr;
		curpcb->pcb_fpflags = mcp->mc_fpregs.fp_flags & PCB_FP_USERMASK;
		td->td_frame->tf_sstatus |= SSTATUS_FS_CLEAN;
	}

	critical_exit();
}

int
sys_sigreturn(struct thread *td, struct sigreturn_args *uap)
{
	ucontext_t uc;
	int error;

	if (copyinptr(uap->sigcntxp, &uc, sizeof(uc)))
		return (EFAULT);

	error = set_mcontext(td, &uc.uc_mcontext);
	if (error != 0)
		return (error);

	/* Restore signal mask. */
	kern_sigprocmask(td, SIG_SETMASK, &uc.uc_sigmask, NULL, 0);

	return (EJUSTRETURN);
}

static bool
sendsig_ctx_end(struct thread *td, vm_offset_t *addrp)
{
	struct riscv_reg_context end_ctx;
	vm_offset_t ctx_addr;

	*addrp -= sizeof(end_ctx);
	ctx_addr = *addrp;

	memset(&end_ctx, 0, sizeof(end_ctx));
	end_ctx.ctx_id = RISCV_CTX_MAGIC_END;
	end_ctx.ctx_size = sizeof(end_ctx);

	if (copyout(&end_ctx, (void *)ctx_addr, sizeof(end_ctx)) != 0)
		return (false);
	return (true);
}

static bool
sendsig_ctx_vector(struct thread *td, vm_offset_t *addrp)
{
	struct vector_context vs_ctx;
	struct pcb *pcb;
	size_t buf_size, ctx_size;
	vm_offset_t vs_ctx_addr;

	pcb = td->td_pcb;
	/* Do nothing if vector hasn't started */
	if (pcb->pcb_vsaved == NULL)
		return (true);

	MPASS(pcb->pcb_vsaved != NULL);

	buf_size = vector_get_size();
	ctx_size = CTX_SIZE_VS(buf_size);

	/* Address for the full context. */
	*addrp -= ctx_size;
	vs_ctx_addr = *addrp;

	memset(&vs_ctx, 0, sizeof(vs_ctx));
	vs_ctx.ctx.ctx_id = RISCV_CTX_MAGIC_VS;
	vs_ctx.ctx.ctx_size = ctx_size;
	vs_ctx.vs_vstart = pcb->pcb_vstart;
	vs_ctx.vs_vl = pcb->pcb_vl;
	vs_ctx.vs_vtype = pcb->pcb_vtype;
	vs_ctx.vs_vcsr = pcb->pcb_vcsr;

	/* Copy out the header and data */
	if (copyout(&vs_ctx, (void *)vs_ctx_addr, sizeof(vs_ctx)) != 0)
		return (false);
	if (copyout(pcb->pcb_vsaved, (void *)(vs_ctx_addr + sizeof(vs_ctx)),
	    buf_size) != 0)
		return (false);

	return (true);
}

typedef bool(*ctx_func)(struct thread *, vm_offset_t *);
static const ctx_func ctx_funcs[] = {
	sendsig_ctx_end,	/* Must go first. */
	sendsig_ctx_vector,
	NULL,
};

void
sendsig(sig_t catcher, ksiginfo_t *ksi, sigset_t *mask)
{
	struct sigframe *fp, frame;
#ifndef __CHERI__
	struct sysentvec *sysent;
#endif
	struct trapframe *tf;
	struct sigacts *psp;
	struct thread *td;
	struct proc *p;
	vm_offset_t addr;
	int onstack;
	int sig;
	int i;

	td = curthread;
	p = td->td_proc;
	PROC_LOCK_ASSERT(p, MA_OWNED);

	sig = ksi->ksi_signo;
	psp = p->p_sigacts;
	mtx_assert(&psp->ps_mtx, MA_OWNED);

	tf = td->td_frame;

	/*
	 * XXXCHERI: We make an on-stack determination using the
	 * virtual address associated with the stack pointer, rather
	 * than using the full capability.  Should we compare the
	 * entire capability...?  Just pointer and bounds...?
	 */
	onstack = sigonstack(tf->tf_sp);

	CTR4(KTR_SIG, "sendsig: td=%p (%s) catcher=%p sig=%d", td, p->p_comm,
	    (__cheri_addr ptraddr_t)catcher, sig);

	/* Allocate and validate space for the signal handler context. */
	if ((td->td_pflags & TDP_ALTSTACK) != 0 && !onstack &&
	    SIGISMEMBER(psp->ps_sigonstack, sig)) {
		addr = ((uintptr_t)td->td_sigstk.ss_sp +
		    td->td_sigstk.ss_size);
	} else {
		addr = td->td_frame->tf_sp;
	}

	/* Fill in the frame to copy out */
	bzero(&frame, sizeof(frame));
	get_mcontext(td, &frame.sf_uc.uc_mcontext, 0);
	frame.sf_si = ksi->ksi_info;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_stack = td->td_sigstk;
	frame.sf_uc.uc_stack.ss_flags = (td->td_pflags & TDP_ALTSTACK) != 0 ?
	    (onstack ? SS_ONSTACK : 0) : SS_DISABLE;
	mtx_unlock(&psp->ps_mtx);
	PROC_UNLOCK(td->td_proc);

	for (i = 0; ctx_funcs[i] != NULL; i++) {
		if (!ctx_funcs[i](td, &addr)) {
			CTR4(KTR_SIG,
			    "sendsig: frame sigexit td=%p fp=%#lx func[%d]=%p",
			    td, addr, i, ctx_funcs[i]);
			PROC_LOCK(p);
			sigexit(td, SIGILL);
			/* NOTREACHED */
		}
	}

	/* Point at the first context */
	frame.sf_uc.uc_mcontext.mc_ptr = addr;

	/* Make room, keeping the stack aligned */
	fp = (struct sigframe *)addr;
	fp--;
	fp = (struct sigframe *)STACKALIGN(fp);

	/* Copy the sigframe out to the user's stack. */
	if (copyoutptr(&frame, fp, sizeof(*fp)) != 0) {
		/* Process has trashed its stack. Kill it. */
		CTR2(KTR_SIG, "sendsig: sigexit td=%p fp=%p", td,
				(__cheri_addr ptraddr_t)fp);
		PROC_LOCK(p);
		sigexit(td, SIGILL);
	}

	tf->tf_a[0] = sig;
#ifdef __CHERI__
	tf->tf_a[1] = (uintptr_t)cheri_bounds_set(&fp->sf_si,
	    sizeof(fp->sf_si));
	tf->tf_a[2] = (uintptr_t)cheri_bounds_set(&fp->sf_uc,
	    sizeof(fp->sf_uc));
#else
	tf->tf_a[1] = (register_t)&fp->sf_si;
	tf->tf_a[2] = (register_t)&fp->sf_uc;
#endif

	tf->tf_sepc = (uintptr_t)catcher;
	tf->tf_sp = (uintptr_t)fp;

#ifdef __CHERI__
	tf->tf_ra = (uintptr_t)p->p_md.md_sigcode;
#else
	sysent = p->p_sysent;
	if (PROC_HAS_SHP(p))
		tf->tf_ra = (register_t)PROC_SIGCODE(p);
	else
		tf->tf_ra = (register_t)(PROC_PS_STRINGS(p) -
		    *(sysent->sv_szsigcode));
#endif

	CTR3(KTR_SIG, "sendsig: return td=%p pc=%#x sp=%#x", td, tf->tf_sepc,
	    tf->tf_sp);

	PROC_LOCK(p);
	mtx_lock(&psp->ps_mtx);
}
