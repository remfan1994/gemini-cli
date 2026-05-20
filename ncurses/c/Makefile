# Makefile for ttychatter, the native C/ncurses Gemini terminal client.
#
# This build is intentionally plain.  No git metadata, generated configure
# scripts, or language-specific package managers are required.  A maintainer can
# build the program with the standard development packages for gcc, make,
# ncurses, libcurl, and json-c.

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
PKG_CONFIG ?= pkg-config
CC ?= gcc

CSTD ?= -std=c11
WARN ?= -Wall -Wextra -Wpedantic
OPT ?= -O2
CPPFLAGS ?= -D_POSIX_C_SOURCE=200809L -Iinclude
CFLAGS ?= $(CSTD) $(WARN) $(OPT)
LDFLAGS ?=

CURL_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcurl 2>/dev/null)
CURL_LIBS := $(shell $(PKG_CONFIG) --libs libcurl 2>/dev/null || echo -lcurl)
JSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags json-c 2>/dev/null)
JSON_LIBS := $(shell $(PKG_CONFIG) --libs json-c 2>/dev/null || echo -ljson-c)
CURSES_CFLAGS := $(shell $(PKG_CONFIG) --cflags ncursesw 2>/dev/null)
CURSES_LIBS := $(shell $(PKG_CONFIG) --libs ncursesw 2>/dev/null || echo -lncursesw)

SRC := src/main.c src/util.c src/config.c src/log.c src/gemini.c src/models.c src/doctor.c src/ui.c
OBJ := $(SRC:.c=.o)
BIN := ttychatter

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(CURL_LIBS) $(JSON_LIBS) $(CURSES_LIBS)

%.o: %.c include/ttychatter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CURL_CFLAGS) $(JSON_CFLAGS) $(CURSES_CFLAGS) -c -o $@ $<

install: $(BIN)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(BIN) $(DESTDIR)$(BINDIR)/$(BIN)
	install -d $(DESTDIR)$(MANDIR)/man1
	install -m 0644 man/ttychatter.1 $(DESTDIR)$(MANDIR)/man1/ttychatter.1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	rm -f $(DESTDIR)$(MANDIR)/man1/ttychatter.1

clean:
	rm -f $(OBJ) $(BIN)
	rm -f ttychatter.zip

check: $(BIN)
	./$(BIN) --version
	./$(BIN) --help >/dev/null

# Build a source archive without relying on git.  The archive contains only the
# C project files and documentation needed to build/install this C port.
dist: clean
	cd .. && zip -r ttychatter.zip ttychatter \
		-x 'ttychatter/*.o' 'ttychatter/ttychatter' 'ttychatter/ttychatter.zip'

.PHONY: all install uninstall clean check dist
