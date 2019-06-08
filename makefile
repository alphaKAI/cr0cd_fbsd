SRCS=cr0cd.c
KMOD=cr0cd

.include <bsd.kmod.mk>

CFLAGS += -std=gnu11