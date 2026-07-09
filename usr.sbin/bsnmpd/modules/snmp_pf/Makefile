#
# Author: Philip Paeps <philip@freebsd.org>

.include <src.opts.mk>

MOD=	pf
SRCS=	pf_snmp.c

XSYM=	begemotPf
DEFS=	${MOD}_tree.def
BMIBS=	BEGEMOT-PF-MIB.txt

CFLAGS+=	-I${SRCTOP}/lib/libpfctl -I${OBJTOP}/lib/libpfctl
LIBADD+=	pfctl

.if ${MACHINE_ABI:Mpurecap}
# ld.lld: error: could not determine size of cap reloc against local object snl_f_p_empty
LD_FATAL_WARNINGS=no
LDFLAGS+=-Wl,--error-limit=0
.endif


.include <bsd.snmpmod.mk>
