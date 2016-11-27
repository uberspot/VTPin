# Contains variables exported to all makefiles in VTPin

UNAME := $(shell uname)
export BINDIR=bin
export SRCDIR=src
ifeq ($(UNAME), Linux)
    export OUT_LIBTVPIN=libvtpin.so
    # qt4 only necessary on linux
    export CXXEXTRAS=-I /usr/include/qt4
endif
ifeq ($(UNAME), Darwin)
    export OUT_LIBTVPIN=libvtpin.dylib
    export CXXEXTRAS=-m32
endif

export CXX=clang++
export COMMONFLAGS:=-Wall -Wno-format -Wunused-function -pthread -std=c++11 #-std=c++11
export LDFLAGS=-lpthread

# Targets

debug: export DEBUG=-DDEBUG -g -ggdb -rdynamic -Wextra -Wno-unused-variable -Wno-unused-parameter -Wshadow -Wno-gnu-zero-variadic-macro-arguments -Wnon-virtual-dtor -pedantic 
# -DHANDLE_SEGFAULTS -DDO_GARBAGE_COLLECTION -DTRACE_MEM_TO_FILE
debug: all