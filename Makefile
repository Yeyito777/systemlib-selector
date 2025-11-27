PREFIX        = /usr/local
INCLUDEDIR    = $(PREFIX)/include
LIBDIR        = $(PREFIX)/lib
DEBUGFLAGS 		= -DDEBUG

CC            = gcc
CFLAGS        = -fPIC -Wall -Wextra `pkg-config --cflags libgit2`
LDFLAGS       = -shared `pkg-config --libs libgit2`

TARGET        = libselector.so
OBJS          = selector.o
HEADER        = selector.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

selector.o: selector.c $(HEADER)
	$(CC) $(CFLAGS) -c selector.c -o selector.o

debug: CFLAGS += -DDEBUG
debug: all

install: $(TARGET)
	install -d $(INCLUDEDIR)
	install -d $(LIBDIR)
	install -m 644 $(HEADER) $(INCLUDEDIR)
	install -m 755 $(TARGET) $(LIBDIR)
	ldconfig

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all install clean
