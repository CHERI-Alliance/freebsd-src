.include <src.opts.mk>

.PATH:	${SRCTOP}/contrib/pf/authpf

PACKAGE=pf
PROG=	authpf
MAN=	authpf.8
BINOWN=	root
BINGRP= authpf
BINMODE= 6555

SRCS=   authpf.c

# XXX ALTQ:
CFLAGS+= -DENABLE_ALTQ
CFLAGS+= -I${SRCTOP}/lib/libpfctl -I${OBJTOP}/lib/libpfctl

LIBADD=	m util pfctl

WARNS?=	3

.if ${MACHINE_ABI:Mpurecap}
# ld.lld: error: could not determine size of cap reloc against local object snl_f_p_empty
LDFLAGS+=-Wl,--error-limit=0
.endif

LINKS=	${BINDIR}/authpf ${BINDIR}/authpf-noip
MLINKS=	authpf.8 authpf-noip.8

.include <bsd.prog.mk>
