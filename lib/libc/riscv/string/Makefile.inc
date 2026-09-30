.if !${MACHINE_ABI:Mpurecap}
MDSRCS+= \
	bcopy.c \
	bzero.c \
	memchr.S \
	memcpy.S \
	memset.S \
	strlen.S \
	strchrnul.S
.endif
