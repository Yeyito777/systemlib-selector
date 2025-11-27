# ============================================================
#  Simple Makefile for libselector
# ============================================================

PREFIX        ?= /usr/local
INCLUDEDIR    = $(PREFIX)/include
LIBDIR        = $(PREFIX)/lib

CC            = gcc
CFLAGS        = -fPIC -Wall -Wextra `pkg-config --cflags libgit2`
LDFLAGS       = -shared `pkg-config --libs libgit2`

TARGET        = libselector.so
OBJS          = selector.o
HEADER        = selector.h

# ------------------------------------------------------------
# Default target: build the shared library
# ------------------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

selector.o: selector.c $(HEADER)
	$(CC) $(CFLAGS) -c selector.c -o selector.o

# ------------------------------------------------------------
# Install to /usr/local by default
# ------------------------------------------------------------
install: $(TARGET)
	install -d $(INCLUDEDIR)
	install -d $(LIBDIR)
	install -m 644 $(HEADER) $(INCLUDEDIR)
	install -m 755 $(TARGET) $(LIBDIR)
	ldconfig

# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all install clean
