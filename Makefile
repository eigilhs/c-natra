APP := example
CFLAGS := -O3
LDLIBS := -levent

.PHONY: clean distclean

$(APP): c-natra.o trie.o
$(APP).o: *.html

.depend: *.[ch]
	$(CC) -MM *.c > $@
	sed -i 's/$$/ $(firstword $(MAKEFILE_LIST))/' $@

clean:
	$(RM) *.o

distclean: clean
	$(RM) $(APP)
	$(RM) .depend

-include .depend
