APP := example
CFLAGS := -O3 -I../include
LDLIBS := -levent -lcnatra -L../lib

.PHONY: clean distclean all

all: $(APP)
$(APP).o: *.html

clean:
	$(RM) *.o

distclean: clean
	$(RM) $(APP)
