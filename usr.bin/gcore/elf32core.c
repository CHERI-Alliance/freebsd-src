#if __SIZEOF_LONG__ == 4
#error "this file must be compiled for 64-bit architectures."
#endif

#define __ELF_WORD_SIZE 32
#define _MACHINE_ELF_WANT_32BIT

#define	ELFCORE_COMPAT_32	1
#include "elfcore.c"

