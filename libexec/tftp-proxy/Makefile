.include <src.opts.mk>

.PATH:	${SRCTOP}/contrib/pf/tftp-proxy

PACKAGE= pf
PROG=	tftp-proxy
SRCS=	tftp-proxy.c filter.c
MAN=	tftp-proxy.8

CFLAGS+= -I${SRCTOP}/lib/libpfctl -I${OBJTOP}/lib/libpfctl
LIBADD= pfctl

WARNS?=	3

.if ${MACHINE_ABI:Mpurecap}
# ld.lld: error: could not determine size of cap reloc against local object snl_f_p_empty
LDFLAGS+=-Wl,--error-limit=0
.endif

.include <bsd.prog.mk>
