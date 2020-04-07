# /****************************************************************
# Copyright (C) Lucent Technologies 1997
# All Rights Reserved
# 
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby
# granted, provided that the above copyright notice appear in all
# copies and that both that the copyright notice and this
# permission notice and warranty disclaimer appear in supporting
# documentation, and that the name Lucent Technologies or any of
# its entities not be used in advertising or publicity pertaining
# to distribution of the software without specific, written prior
# permission.
# 
# LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
# IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
# SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
# IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
# ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
# THIS SOFTWARE.
# ****************************************************************/

# default architecture
ifndef ARCH
ARCH = x64
endif

# compiler(s)
CXX := g++
CPPFLAGS := -std=c++11 -I include/ -Wall -pedantic

CC := gcc 
CFLAGS := -I include/ -Wall -pedantic

# yacc options.  pick one; this varies a lot by system.
#YFLAGS = -d -S
YACC = bison -d -y
#YACC = yacc -d
#		-S uses sprintf in yacc parser instead of sprint

# output folders
OBJDIR := o/awk
LIBDIR := lib

ifeq ($(ARCH), x64)
OBJDIR := $(OBJDIR)/x64
LIBDIR := $(LIBDIR)/x64
else
OBJDIR := $(OBJDIR)/x86
LIBDIR := $(LIBDIR)/x86
CPPFLAGS += -m32
CFLAGS += -m32
endif

ifdef _DEBUG
OBJDIR := $(OBJDIR)/Debug
LIBDIR := $(LIBDIR)/Debug
CPPFLAGS += -g -D_DEBUG=$(_DEBUG) -DYYDEBUG
CFLAGS += -g -D_DEBUG=$(_DEBUG) -DYYDEBUG
else
CPPFLAGS += -O3 -DNDEBUG
CFLAGS += -O3 -DNDEBUG
OBJDIR := $(OBJDIR)/Release
LIBDIR := $(LIBDIR)/Release
endif

# name of output library
LIB = $(LIBDIR)/libawk.a

OFILES = b.o lex.o lib.o libmain.o parse.o run.o tran.o ytab.o

OBJS = $(addprefix $(OBJDIR)/,$(OFILES))

all: $(LIB)

# AWK library target
$(LIB): | $(LIBDIR)

$(LIB): $(OBJS)
	ar rcs  $@ $^

# Object modules
$(OBJS) : | $(OBJDIR)

$(LIBDIR) $(OBJDIR):
	mkdir -p $@

# Grammar files
ytab.h ytab.cpp: awkgram.y awk.h proto.h
	$(YACC) $(YFLAGS) -o ytab.cpp $<
	mv ytab.hpp ytab.h

$(OBJDIR)/%.o: %.cpp proto.h ytab.h awk.h
	$(CXX) $(CPPFLAGS) -c -o $@ $<

# test program
libtest/test1: libtest/libtest.cpp $(LIB)
	$(CXX) $(CPPFLAGS) -o $@ -L $(LIBDIR)/ $< -lutpp -lawk

# Other stuff
clean:
	rm -f *.o *.obj maketab maketab.exe *.bb *.bbg *.da *.gcov *.gcno *.gcda ytab.*

cleaner:
	rm -f a.out *.o *.obj maketab maketab.exe *.bb *.bbg *.da *.gcov *.gcno *.gcda
