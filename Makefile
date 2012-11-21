# Makefile for libmsntp
#
# Copyright 2000 N.M. Maclaren
# Copyright 2005 Ryan Barrett <libmsntp@ryanb.org>

VERSION = 1.6a
PKGNAME = libmsntp-$(VERSION)
PREFIX = /usr/local

# Take a look at README.msntp for the various preprocessor symbols, but they
# are extremely unlikely to be needed on newer systems. You may prefer to
# change LOCKNAME and SAVENAME to use /var/run (or even /tmp) rather than /etc.
# Note that not all of the following system settings have been tested recently.

# These options will work on most modern systems.  Start with them, and add
# any necessary options.
CC = cc -fPIC
CFLAGS = -O
LDFLAGS = -lm
LIBS =

# Compiling this sort of ANSI C under SunOS 4.1 is a mug's game, because Sun's
# Unix headers make GNU C vomit even in compatibility mode, but the following
# will compile main.c and unix.c.  At least two people have got it to work.
# CC = gcc -ansi
# CFLAGS = -O -DNONBLOCK_BROKEN
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under SOME versions of Solaris 2, but
# -D_XOPEN_SOURCE should probably be added on versions where setting that
# does not cause it to reject its own headers!  They can also be used under
# UnixWare, probably with similar constraints.
# CC = cc -Xc
# CFLAGS = -O -v
# LDFLAGS =
# LIBS = -lm -lsocket -lnsl

# The following settings can be used under HP-UX 10.0 and later on PA-RISC and
# HP-UX 9.03 and later on 68000.
# CC = cc -Aa -D_HPUX_SOURCE
# CFLAGS = -O
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under HP-UX before 10.0 on PA-RISC.
# CC = cc -Aa -D_HPUX_SOURCE
# CFLAGS = -O -DADJTIME_MISSING
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under Digital Unix (aka DEC OSF/1).
# CC = cc -std1
# CFLAGS = -O
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under DEC Ultrix 4.3 on a MIPS.
# CC = gcc -ansi
# CFLAGS = -O -DNONBLOCK_BROKEN
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under SGI Irix.
# CC = cc -ansi
# CFLAGS = -O
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under Hitachi HI-UX/WE2.
# CC = cc -Aa -D_HIUX_SOURCE
# CFLAGS = -O
# DFLAGS =
# LIBS = -lm

# The following settings can be used under Hitachi HI-OSF/1-MJ and HI-UX/MPP.
# CC = cc
# CFLAGS = -O
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under at least NextStep 3.  cc is a
# wrapper for gcc.
# CC = cc -D_POSIX_SOURCE
# CFLAGS = -O
# LDFLAGS =
# LIBS = -lm

# The following settings can be used under Unicos.
# CC = cc -DNONBLOCK_BROKEN
# CFLAGS = -O
# LDFLAGS = 
# LIBS = -lm

# The following settings can be used under Linux.  While adjtime is present,
# it is completely broken (i.e. it will work only if xntp is running), so it
# is a good idea to add -DADJTIME_MISSING.
# CC = gcc -DADJTIME_MISSING
# CFLAGS = -O>>>
# LDFLAGS =
# LIBS = -lm

# It has been compiled with the following options, though with quite a lot of
# warnings (many due to system header bugs!)  All functions defined without a
# previous declaration should be internal to that file - static is not used
# because it often interferes with debugging.
# CC = gcc -ansi
# CFLAGS = -O -pedantic -Wall -Wtraditional -Wshadow -Wpointer-arith \
# -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Waggregate-return \
# -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
# -Wredundant-decls -Wnested-externs
# LDFLAGS = 
# LIBS = -lm

SRCS = main.c unix.c internet.c socket.c timing.c libmsntp.c
OBJS = $(SRCS:.c=.o)

all: libmsntp example

default: libmsntp

clean:
	rm -f *.o *.a *.so a.out core example $(PKGNAME).tar.gz *~

dist: clean
	ln -s . $(PKGNAME)
	tar czhf $(PKGNAME).tar.gz --exclude $(PKGNAME).tar.gz --exclude .svn \
	  --exclude $(PKGNAME)/$(PKGNAME) --exclude libmsntp.lsm $(PKGNAME)
	rm $(PKGNAME)

install:
	install -b -m 644 libmsntp.h $(PREFIX)/include/libmsntp.h
	install -b -m 755 libmsntp.so $(PREFIX)/lib/libmsntp.so.$(VERSION)
	rm -f $(PREFIX)/lib/libmsntp.so
	ln -s $(PREFIX)/lib/libmsntp.so.$(VERSION) $(PREFIX)/lib/libmsntp.so
	install -b -m 644 libmsntp.a $(PREFIX)/lib/libmsntp.a

example: $(OBJS) example.c
	$(CC) $(CFLAGS) $(LIBS) $(OBJS) -o $@ example.c $(LDFLAGS)

libmsntp.so: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -shared -o $@ $(OBJS) $(LDFLAGS)

libmsntp.a: $(OBJS)
	ar -r $@ $(OBJS)
	ranlib $@

libmsntp : libmsntp.so libmsntp.a
	
