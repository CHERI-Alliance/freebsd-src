.include <src.opts.mk>

.PATH:	${SRCTOP}/contrib/pf/ftp-proxy

PACKAGE=	pf
PROG=	ftp-proxy
MAN=	ftp-proxy.8

SRCS=	ftp-proxy.c filter.c

CFLAGS+=-I${SRCTOP}/contrib/pf/libevent
CFLAGS+= -I${SRCTOP}/lib/libpfctl -I${OBJTOP}/lib/libpfctl

LIBADD=	event1 pfctl

WARNS?=	3

.if ${MACHINE_ABI:Mpurecap}
# ld.lld: error: could not determine size of cap reloc against local object snl_f_p_empty
LD_FATAL_WARNINGS=no
LDFLAGS+=-Wl,--error-limit=0
.endif

.include <bsd.prog.mk>
