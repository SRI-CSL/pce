#
# Top-level Makefile
#
# Determine the architecture and top directory
# Import architecture include files from top-dir/configs/make.include.$(ARCH)
#

SHELL=/bin/sh

MCSAT_TOP_DIR=$(PWD)

#
# Find platform
#
ARCH=$(shell ./config.sub `./config.guess`)
POSIXOS=$(shell ./autoconf/os)

ifeq (,$(POSIXOS))
  $(error "Problem running ./autoconf/os")
else
ifeq (unknown,$(POSIXOS))
  $(error "Unknown OS")
endif
endif


#
# Alternative ABI
#
# 1) On Mac OS X/intel, we compile in 32 bit mode by default.
# It's possible to build in 64 bit mode on the same machine.
# - To select 64bit mode, type 'Make MODE=... ABI=64'
# - This changes ARCH from 
#     i386-apple-darwin9.x.y to x86_64-apple-darwin9.x.y
#   and the corresponding make.include should be constructed using 
#   ./configure --build=x86_64-apple-darwin.x.y CFLAGS=-m64
#
# 2) On Linux/x86_64, we compile in 64 bit mode by default.
# It may be possible to build in 32 bit mode on the same machine.
# - To select 32bit mode, type 'Make MODE=... ABI=32'
# - This changes ARCH from
#     x86_64-unknown-linux-gnu to i686-unknown-linux-gnu
#   and there must be a corresponding make.include file in ./configs
# - To construct that file use
#   ./configure --build=i686-unknown-linux-gnu CFLAGS=-m32
#
ifneq ($(ABI),)
ifeq ($(ABI),32)
  newarch=$(subst x86_64,i686,$(ARCH))
else
ifeq ($(ABI),64) 
  newarch=$(subst i386,x86_64,$(ARCH))
else
  $(error "Invalid ABI option: $(ABI)")
endif
endif
ifeq ($(newarch),$(ARCH))
  $(error "ABI option $(ABI) not supported on platform $(ARCH)")
endif
ARCH := $(newarch)
endif


#
# Check whether make.include exists
#
# Note: we don't want to run ./configure from here.
# The user may need to give options to the ./configure 
# script.
#
make_include = configs/make.include.$(ARCH)
known_make_includes = $(filter-out %.in, $(wildcard configs/make.include.*))

MCSAT_MAKE_INCLUDE := $(findstring $(make_include), $(known_make_includes))

ifeq (,$(MCSAT_MAKE_INCLUDE))
  $(error Could not find $(make_include). Run ./configure)
endif


#
# Check build mode
#
default_mode=release
allowed_modes=release static debug profile valgrind purify quantify
MODE ?= $(default_mode)

MCSAT_MODE := $(filter $(allowed_modes), $(MODE))

ifeq (,$(MCSAT_MODE))
  $(error "Invalid build mode: $(MODE)")
endif


#
# Source distribution
#
distdir=./distributions
tmpdir=./mcsat
srctarfile=$(distdir)/mcsat-src.tar.gz
timestamp=./distributions/timestamp

#
# Just print configuration
#
check: checkgmake
	@ echo "ARCH is $(ARCH)"
	@ echo "POSIXOS is $(POSIXOS)"
	@ echo "MCSAT_TOP_DIR is $(MCSAT_TOP_DIR)"
	@ echo "MCSAT_MAKE_INCLUDE is $(MCSAT_MAKE_INCLUDE)"
	@ echo "MCSAT_MODE is $(MCSAT_MODE)"

checkgmake:
	@ ./gmaketest --make=$(MAKE) || \
	  (echo "GNU-Make is required to compile MCSAT. Aborting."; exit1)



#
# Invoke submake that will do the real work
#
.DEFAULT: checkgmake
	@ echo "Mode:     $(MCSAT_MODE)"
	@ echo "Platform: $(ARCH)"
	@ $(MAKE) -f Makefile.build \
	MCSAT_MODE=$(MCSAT_MODE) \
	ARCH=$(ARCH) \
	POSIXOS=$(POSIXOS) \
	MCSAT_TOP_DIR=$(MCSAT_TOP_DIR) \
	MCSAT_MAKE_INCLUDE=$(MCSAT_MAKE_INCLUDE) \
	$@


#
# Build the tar file
#
source-distribution:
	rm -f -r $(tmpdir)
	mkdir $(tmpdir)
	mkdir $(tmpdir)/autoconf
	mkdir $(tmpdir)/configs
	mkdir $(tmpdir)/src
	mkdir $(tmpdir)/tests
	mkdir $(tmpdir)/doc
	mkdir $(tmpdir)/examples
	cp install-sh config.guess configure configure.ac config.sub gmaketest $(tmpdir)
	cp README Makefile Makefile.build make.include.in $(tmpdir)
	cp autoconf/* $(tmpdir)/autoconf
	cp src/Makefile src/*.h src/*.c src/mcsat_keywords.txt src/smt_keywords.txt $(tmpdir)/src
	cp tests/Makefile tests/*.c $(tmpdir)/tests
	cp doc/NOTES doc/SMT-LIB-LANGUAGE doc/smt_parser.txt doc/mcsat_parser.txt doc/table_builder.c \
	  doc/smt-grammar $(tmpdir)/doc
	cp examples/*.ys $(tmpdir)/examples
	chmod -R og+rX $(tmpdir)
	mkdir -p $(distdir)
	tar -czf $(srctarfile) $(tmpdir)
	chmod -R og+rX $(distdir)
	rm -f -r $(tmpdir)


.PHONY: checkgmake check source-distribution
