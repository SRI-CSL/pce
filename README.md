
[![Build Status](https://travis-ci.org/SRI-CSL/pce.svg?branch=master)](https://travis-ci.org/SRI-CSL/pce)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/4869/badge.svg)](https://scan.coverity.com/projects/4869)

Content and Directory Structure
-------------------------------

The source subdirectories include

```
./src:	       source code for MCSAT and PCE
./doc:	       documentation
./examples:    example input files for testing
```

The build subdirectory is organized by platform + compilation mode.
Compilation modes are release, debug, static + others for profiling, etc.

For example, on an intel Mac, there may be three directories below build:

```
./build/i386-apple-darwin8.9.1-release/
./build/i386-apple-darwin8.9.1-debug/
./build/i386-apple-darwin8.9.1-static/
```

Each of these directories contains three subdirectories: for object,
libraries, and binaries, e.g., 

```
./build/i386-apple-darwin8.9.1-release/obj/
./build/i386-apple-darwin8.9.1-release/lib/
./build/i386-apple-darwin8.9.1-release/bin/
```

This is where all object files, libraries, and binaries are located for
that platform and compilation mode.



The rest of this file explains how to configure and compile the package on
UNIX systems. To build under Windows, please check README.windows.




Configuration (for Linux, MacOSX, Solaris)
------------------------------------------

Configuration files are generated by ./configure, and are kept separate for each 
platform. They are stored in the ./configs subdirectory, with a name of the form 
make.include.<platform>. 

The rationale for this organization is to support parallel compilation
on several machines from the same source directory (i.e., without
having to run ./configure every time on each machine).
 
The Makefile in the top-level directory determines the architecture and OS, 
and checks the compilation mode. It sets five variables that are passed
to a recursive Make:

```
  MCSAT_MODE=compilation mode
  ARCH=build/target-platform
  POSIXOS=operating system
  MCSAT_TOP_DIR=top-level directory
  MCSAT_MAKE_INCLUDE=config file to use
```

To see how these variables are set just type make.



To configure for compilation on a new platform:

1) run autoconf

2) run ./configure
   (check the configure options if there's an error).   



Alternative Configurations
--------------------------

1) On Mac OS X/intel platforms, (at least since version 10.5/Leopard), it
is possible to produce objects/libraries/executables either in
32bit or in 64bit mode. The default is 32bit. To override this, build
an alternative configuration file by typing

```
   ./configure --build=x86_64-apple-darwin9.2.2 CFLAGS=-m64
```

(The version number 2.2 changes with each patch/new version of
Darwin. Adjust this as needed).

To use this configuration, add the option ABI=64 to the command line
when using make (e.g., make test MODE=static ABI=64).



2) On Linux/x86_64 platforms, it may be possible to also compile in 32 bit mode.
This depends on libraries and gcc but that can work. The default is to compile
in 64bit. As above, to override this, build an alternative configuration
file:

```
   ./configure --build=i686-unknown-linux-gnu CFLAGS=-m32
```

To use this configuration, add the option ABI=32 when invoking make.




Compilation
-----------

1) To compile the binaries, type

```
   make MODE=<compilation mode> bin
```

The resulting executables are in ./build/<platform>-<mode>/bin

The possible compilation modes are defined in src/Makefile.
Currently the possible compilation modes are:
  
*  release
*  static
*  debug

*  profile
*  valgrind
*  quantify
*  purify


If nothing is specified, the default MODE is release.


2) To compile only the objects files:

```
   make MODE=<compilation mode> objects
```

The objects are in ./build/<platform>-<mode>/obj


3) To compile only the libraries (e.g., libmcsat.a)

```
   make MODE=<compilation mode> lib
```

The library is in ./build/<platform>-<mode>/lib



Getting 64bit code on Mac/Intel
-------------------------------

Add option ABI=64 to all the make commands above.


Getting 32bit code on Linux/x86_64
----------------------------------

Add option ABI=32 to all the make commands above.





Cleanup
-------

1) To cleanup the ./build/<platform>-<mode> directory:

```
   make MODE=<compilation mode> clean
```

this removes libraries, binaries, and object files.

The directories 

```
   ./build/<platform>-<mode>/obj
   ./build/<platform>-<mode>/bin
   ./build/<platform>-<mode>/lib
```

are preserved


2) Deeper cleanup:
 
To remove the build directory for the current platform and mode  (i.e., 
./build/<platform>-<mode> and everything in there):

```
   make build-clean MODE=mode
   make build_clean MODE=mode
```

To remove all build directories for the current platform: ./build/<platform>-*

```
   make arch-clean
   make arch_clean
```

To remove all build directories: ./build/*

```
   make all-clean
   make all_clean
```




Building a tarfile with all source files + common binaries
----------------------------------------------------------

Type 'make distribution' in the top-level directory.

this build a tarfile ./distributions/mcsat-src.tar.gz

The tarfile should contain a full source distribution plus 
precompiled binaries for linux, Mac/intel, and Window XG.




