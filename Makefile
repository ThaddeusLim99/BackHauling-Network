CONTIKI_PROJECT = Source-Code
all: $(CONTIKI_PROJECT)

CONTIKI = ../..
CFLAGS += -w

MAKE_NET = MAKE_NET_NULLNET
include $(CONTIKI)/Makefile.include
