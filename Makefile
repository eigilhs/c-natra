CFLAGS := -O3 -Wall -Wextra -Wno-unused-parameter -Iinclude
VPATH := src
PREFIX ?= /usr/local

.PHONY: clean distclean dist install uninstall

lib/libcnatra.a: c-natra.o cn-trie.o | lib
	$(AR) cvq $@ $^

lib:
	mkdir -p $@

install: lib/libcnatra.a | lib include
	cp -r $| $(PREFIX)

uninstall:
	$(RM) $(addprefix $(PREFIX)/include/, c-natra.h cn-trie.h)
	$(RM) $(PREFIX)/lib/libcnatra.a

dist: lib/libcnatra.a | lib include
	tar cvJf libcnatra.tar.xz --xform "s,^,libcnatra/," $|

.depend: src/* include/*
	$(CC) -Iinclude -MM src/* > $@
	sed -i 's/$$/ $(firstword $(MAKEFILE_LIST))/' $@

clean:
	$(RM) *.o

distclean: clean
	$(RM) *.tar.xz .depend
	$(RM) -r lib

-include .depend
