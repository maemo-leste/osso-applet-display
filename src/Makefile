OBJS=cpdisplay.o
SOURCES=cpdisplay.c
BUILDDIR=../build
LIB=$(BUILDDIR)/libcpdisplay.so
PKG_FLAGS=$(shell pkg-config glib-2.0 gconf-2.0 hildon-1 libhildondesktop-1 --libs --cflags)
CCFLAGS=-shared
CC=gcc

all:$(LIB)

$(LIB):$(OBJS)
	$(CC) $(CCFLAGS) $(PKG_FLAGS) $(OBJS) -o $(LIB)

.c.o:
	$(CC) -fPIC $(CCFLAGS) $(PKG_FLAGS) -c $< -o $@

.PHONE: clean all

clean:
	rm $(OBJS)
