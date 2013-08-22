#
# src/Makefile
#
# Must be invoked with the following variables set
#
#   MCSAT_TOP_DIR = top-level directory for Mcsat
#   MCSAT_MODE = build mode
#   MCSAT_MAKE_INCLUDE = configuration file to include
#   ARCH = architecture (e.g, i686-pc-linux-gnu)
#   POSIXOS = OS (e.g., linux)
#   BUILD = target build director (normally build/$(ARCH)-$(MCSAT_MODE))
#
# Config variables are imported by including the file
#   $(MCSAT_TOP_DIR)/$(MCSAT_MAKE_INCLUDE)
#


SHELL=/bin/sh

conf=$(MCSAT_TOP_DIR)/$(MCSAT_MAKE_INCLUDE)

ifeq ($conf,"/")
$(error "MCSAT_TOP_DIR and MCSAT_MAKE_INCLUDE are undefined")
else
include $(conf)
endif

LIBS := $(LIBS) -lm
YFLAGS := $(YFLAGS) -d -t
CFLAGS := $(CFLAGS) -DMEXP=11213

#
# build subdirectories
#
objdir := $(BUILD)/obj
libdir := $(BUILD)/lib
bindir := $(BUILD)/bin

#
# source files for the library
#
src_c := array_hash_map.c cnf.c gcd.c hash_functions.c input.c \
         int_array_sort.c integer_stack.c int_stack.c mcsat.c memalloc.c \
         memsize.c mcmc.c samplesat.c ground.c buffer.c string_heap.c tables.c \
         symbol_tables.c utils.c vectors.c parser.c print.c yacc.tab.c SFMT.c \
         clause_list.c
ifneq ($(XMLRPC_SERVER_LIBS),)
src_c := $(src_c) xpce.c test-xpce.c test-xpce2.c xpce-client.c
endif


#
# additional source files for the binary
#
bin_src_c := 

#
# Auto-generated version file
#
#mcsat_version_c := mcsat_version.c
#mcsat_version_o := $(objdir)/mcsat_version.o

#
# list of exported symbols for dynamic library
# this is used on cygwin/mingw only
# (We don't use this. It's simpler to export all symbols)
## mcsat_exports := mcsat_exports.def
#

# dependencies and object files to build
dep := $(src_c:%.c=$(objdir)/%.d)
obj := $(src_c:%.c=$(objdir)/%.o)

bin_dep := $(bin_src_c:%.c=$(objdir)/%.d)
bin_obj := $(bin_src_c:%.c=$(objdir)/%.o)

# static library
libmcsat := $(libdir)/libmcsat.a

# dynamic libraries
libmcsat_obj := $(objdir)/allmcsat.o
libmcsat_so := $(libdir)/libmcsat.so
libmcsat_dylib := $(libdir)/libmcsat.dylib
libmcsat_dll := $(libdir)/cygmcsat.dll
libmcsat_mingw_dll := $(libdir)/libmcsat.dll

# related files for cygwin and mingw
libmcsat_implib=$(libdir)/libmcsat.dll.a
libmcsat_def=$(libdir)/libmcsat.def


# binaries
binaries := $(bindir)/mcsat$(EXEEXT)
ifneq ($(XMLRPC_SERVER_LIBS),)
binaries := $(binaries) $(bindir)/xpce$(EXEEXT) $(bindir)/test-xpce$(EXEEXT) \
            $(bindir)/test-xpce2$(EXEEXT) $(bindir)/xpce-client$(EXEEXT)
endif


#
# OS-dependent compilation flags + which dynamic libraries to build
# -fPIC: PIC is the default on Darwin/Cygwin/Mingw (and causes 
#  compilation warning if present)
# -static: is not supported by Darwin or our Solaris2.10 machine
#
ifeq ($(POSIXOS),cygwin)
  CPPFLAGS := $(CPPFLAGS) -DCYGWIN
  PIC=
  STATIC=-static
  libmcsat_dynamic=$(libmcsat_dll)
else
ifeq ($(POSIXOS),mingw)
  CPPFLAGS := $(CPPFLAGS) -DMINGW
  PIC=-fPIC
  STATIC=-static
  libmcsat_dynamic=$(libmcsat_mingw_dll)
else
ifeq ($(POSIXOS),darwin)
  CPPFLAGS := $(CPPFLAGS) -DMACOSX
  CFLAGS += -fvisibility=hidden
  PIC=-fPIC
  STATIC=
  RANLIBFLAGS=-s
  libmcsat_dynamic=$(libmcsat_dylib)
else
ifeq ($(POSIXOS),sunos)
  PIC=-fpic
  STATIC=
  CPPFLAGS := $(CPPFLAGS) -DSOLARIS
  CFLAGS += -fvisibility=hidden
  libmcsat_dynamic=$(libmcsat_so)
else
  PIC=-fPIC
  STATIC=-static
  CFLAGS += -fvisibility=hidden
  libmcsat_dynamic=$(libmcsat_so)
endif
endif
endif
endif


#
# Warning levels
#
CFLAGS += -Wall -Winline -Wredundant-decls

# 
# for l-bfgs library
#
#LIBS += -L$(MCSAT_TOP_DIR)/lib -llbfgs


#
# Compilation flags dependent on MODE
#
# Option -fomit-frame-pointer confuses the Mac OS profiling tools
# (don't use it if MODE=profile)
#
ifeq ($(MCSAT_MODE),release) 
CFLAGS := $(CFLAGS) -O3 -fomit-frame-pointer $(PIC)
CPPFLAGS := $(CPPFLAGS) -DNDEBUG
else 
ifeq ($(MCSAT_MODE),static)
CFLAGS := $(CFLAGS) -DNDEBUG -O3 $(STATIC)
else
ifeq ($(MCSAT_MODE),profile)
CFLAGS := $(CFLAGS) -O3 $(PIC) -pg
CPPFLAGS := $(CPPFLAGS) -DNDEBUG
else
ifeq ($(findstring $(MCSAT_MODE),valgrind quantify purify),$(MCSAT_MODE))
CFLAGS := $(CFLAGS) -O3 -fomit-frame-pointer -g $(PIC) 
CPPFLAGS := $(CPPFLAGS) -DNDEBUG
else
ifeq ($(MCSAT_MODE),debug) 
CFLAGS := $(CFLAGS) -ggdb
else 
CFLAGS := $(CFLAGS) -g
endif
endif
endif
endif
endif


#
# Link command for purify/quantify
#
ifeq ($(MCSAT_MODE),purify)
LNK := purify $(CC)
else
ifeq ($(MCSAT_MODE),quantify)
LNK := quantify $(CC)
else
LNK := $(CC)
endif
endif


#
# Which libraries to build
#
ifeq ($(MCSAT_MODE),release)
lib_targets=$(libmcsat)  $(libmcsat_dynamic)
else
lib_targets=$(libmcsat)
endif




################
#  Make rules  #
################

#
# Binaries: force generation of mcsat_version.c on every make bin
# 
#bin: $(binaries) $(mcsat_version_o)
#	rm -f mcsat_version.c
bin:  $(binaries)

# All objects
# NOTE: removed bin_obj for now. The bin_src code does not compile yet.
# objects: $(obj) $(bin_obj)
objects: $(obj) 

# Static library
lib: $(lib_targets)

# Dependency files
$(objdir)/%.d: %.c
	@ echo Building dependency file $@
	$(CC) -MM -MG $(CPPFLAGS) $< > $@.tmp
	@ $(SED) 's,\($*\).o[ :]*,$(objdir)/\1.o $@ : , g' < $@.tmp > $@
	@ rm -f $@.tmp

ifneq ($@,clean)
include $(dep)
###include $(bin_dep)
endif

# Yacc files
yacc.tab.c yacc.tab.h: yacc.y
	@ echo Building YACC parser
	$(YACC) $(YFLAGS) $<

# Object files
$(objdir)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# static Library
$(libmcsat): $(obj)
	@ rm -f $(libmcsat)
	$(AR) cr $(libmcsat) $(obj)
	$(RANLIB) $(RANLIBFLAGS) $(libmcsat)


#
# We combine all objects into allmcsat.o then build the dynamic library 
# from allmcsat.o
#
$(libmcsat_obj): $(obj)
	$(LD) -r $(LDFLAGS) -o $(libmcsat_obj) $(obj)


#
# For dynamic libraries, the rules are platform dependent.
#

#
# linux + solaris
#
$(libmcsat_so): $(libmcsat_obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $(libmcsat_so) $(libmcsat_obj) $(LIBS)
	$(STRIP) -x $(libmcsat_so)

#
# DLL on cygwin
# the DLL is called cygmcsat.dll
# the linker creates libmcsat.dll.a (import library)
# the flag --no-whole-archive suppresses inclusion of all objects
# stored in $(LIBS) in cygmcsat.dll
#
# To use these files on cygwin
# copy libmcsat.dll.a in /lib (or /usr/lib)
# copy cygmcsat.dll in /bin (or /usr/bin)
# link the code using the flags -lmcsat
#
$(libmcsat_dll): $(libmcsat_obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $(libmcsat_dll) \
	-Wl,--out-implib=$(libmcsat_implib) --no-whole-archive $(libmcsat_obj) \
	$(LIBS)
	$(STRIP) $(libmcsat_dll)

#
# DLL on minnw: more-or-less like cygwin
# the DLL is called libmcsat.dll
# the linker creates libmcsat.dll.a (import library) and libmcsat.def
#
# To use libmcsat.dll on windows
# construct libmcsat.lib from def using the Microsoft lib tool
#    lib /machine:i386 /def:libmcsat.def
#
$(libmcsat_mingw_dll): $(libmcsat_obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $(libmcsat_mingw_dll) \
	-Wl,--out-implib=$(libmcsat_implib) -Wl,--output-def,$(libmcsat_def) \
	$(libmcsat_obj) $(LIBS)
	$(STRIP) $(libmcsat_mingw_dll)


#
# Special dynamic library tricks for Mac OS X:
# - give libmcsat.dylib as the install_name
# - for this to work, the dynamic library has to be copied into
#   the /usr/local/lib directory, with that exact name.
# - the user can change this by using install_name_tool
# - we give option -headerpad_max_install_names so that user can safely
# change the install name using install_name_tool.
# 
$(libmcsat_dylib): $(libmcsat_obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -dynamiclib -current_version 2.0 -compatibility_version 2.0 \
	$(libmcsat_obj) $(LIBS) -o $(libmcsat_dylib) \
	-Wl,-install_name,libmcsat.dylib
	$(STRIP) -x $(libmcsat_dylib)



#
# Executables
#
#$(bindir)/%$(EXEEXT): $(objdir)/%.o $(mcsat_version_o) $(libmcsat)
#	$(LNK) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(mcsat_version_o) $(libmcsat) $(LIBS)
$(bindir)/%$(EXEEXT): $(objdir)/%.o $(libmcsat)
	$(LNK) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(libmcsat) $(LIBS)

ifneq ($(XMLRPC_SERVER_LIBS),)
$(bindir)/xpce$(EXEEXT): $(objdir)/xpce.o $(libmcsat)
	$(LNK) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(libmcsat) $(LIBS) $(XMLRPC_SERVER_LIBS)
$(bindir)/test-xpce$(EXEEXT): $(objdir)/test-xpce.o $(libmcsat)
	$(LNK) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(libmcsat) $(LIBS) $(XMLRPC_CLIENT_LIBS)
$(bindir)/test-xpce2$(EXEEXT): $(objdir)/test-xpce2.o $(libmcsat)
	$(LNK) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(libmcsat) $(LIBS) $(XMLRPC_CLIENT_LIBS)
$(bindir)/xpce-client$(EXEEXT): $(objdir)/xpce-client.o $(libmcsat)
	$(LNK) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(libmcsat) $(LIBS) $(XMLRPC_CLIENT_LIBS)
endif

#
# Gperf generated tables
# - we need to give different names to the mcsat and smt lookup functions
#
mcsat_hash_keywords.h: mcsat_keywords.txt
	$(GPERF) -C -G -L ANSI-C -W mcsat_kw --output-file=mcsat_hash_keywords.h \
	--lookup-function-name=in_mcsat_kw mcsat_keywords.txt

smt_hash_keywords.h: smt_keywords.txt
	$(GPERF) -C -G -L ANSI-C -W smt_kw --output-file=smt_hash_keywords.h \
	--lookup-function-name=in_smt_kw smt_keywords.txt


#
# Compilation date
#
now := $(shell date)

#mcsat_version.c:
#	$(SED) 's/MCSAT_COMPILATION_DATE/$(now)/g' mcsat_version_template.txt > $@



#
# rule to avoid triggering the .DEFAULT rule if .h or .c files have been deleted
#
%.h:
	@ echo "$@ missing"

%.c:
	@ echo
	@ echo "Missing source file: $@"
	@ echo


#
# For debugging of Makefile and configuration:
# print the options as set by this Makefile
#
.DEFAULT:
	@ echo
	@ echo "*** src/Mafefile ***"
	@ echo
	@ echo "target is $@"
	@ echo
	@ echo "ARCH is $(ARCH)"
	@ echo "POSIXOS is $(POSIXOS)"
	@ echo "MCSAT_TOP_DIR is $(MCSAT_TOP_DIR)"
	@ echo "MCSAT_MAKE_INCLUDE is $(MCSAT_MAKE_INCLUDE)"
	@ echo "MCSAT_MODE is $(MCSAT_MODE)"
	@ echo "BUILD is $(BUILD)"
	@ echo
	@ echo "Configuration"
	@ echo "  EXEEXT   = $(EXEEXT)"
	@ echo "  SED      = $(SED)"
	@ echo "  LN_S     = $(LN_S)"
	@ echo "  MKDIR_P  = $(MKDIR_P)"
	@ echo "  CC       = $(CC)"
	@ echo "  CPPFLAGS = $(CPPFLAGS)"
	@ echo "  LIBS     = $(LIBS)"
	@ echo "  LDFLAGS  = $(LDFLAGS)"
	@ echo "  libmcsat  = $(libmcsat)"
	@ echo "  LD       = $(LD)"
	@ echo "  AR       = $(AR)"
	@ echo "  RANLIB   = $(RANLIB)"
	@ echo "  STRIP    = $(STRIP)"
	@ echo



.PHONY: objects lib bin
