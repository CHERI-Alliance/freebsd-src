.include <src.opts.mk>

LIB=		pfctl
INTERNALLIB=	true

SRCS=	libpfctl.c
INCS=	libpfctl.h

CFLAGS+=${PICFLAG}

.if ${MACHINE_ABI:Mpurecap}
# ld.lld: error: could not determine size of cap reloc against local object snl_f_p_empty
LDFLAGS+=-Wl,--error-limit=0
.endif

.include <bsd.lib.mk>
