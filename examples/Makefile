APP := example
CFLAGS := -O3 -I../include
LDLIBS := -lh2o -L../lib -lcnatra

.PHONY: clean distclean all

all: $(APP)
$(APP).o: *.html

clean:
	$(RM) *.o

distclean: clean
	$(RM) $(APP)
