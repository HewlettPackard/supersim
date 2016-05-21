#--------------------- Basic Settings -----------------------------------------#
PROGRAM_NAME  := supersim
BINARY_BASE   := bin
BUILD_BASE    := bld
SOURCE_BASE   := src
MAIN_FILE     := src/main.cc

#--------------------- External Libraries -------------------------------------#
LIBS_LOC      := ../
HEADER_DIRS   := \
	$(LIBS_LOC)libprim/inc \
	$(LIBS_LOC)librnd/inc \
	$(LIBS_LOC)libmut/inc \
	$(LIBS_LOC)libbits/inc \
	$(LIBS_LOC)libjson/inc \
	$(LIBS_LOC)libsettings/inc \
	$(LIBS_LOC)libstrop/inc
STATIC_LIBS   := \
	$(LIBS_LOC)libprim/bld/libprim.a \
	$(LIBS_LOC)librnd/bld/librnd.a \
	$(LIBS_LOC)libmut/bld/libmut.a \
	$(LIBS_LOC)libbits/bld/libbits.a \
	$(LIBS_LOC)libjson/bld/libjson.a \
	$(LIBS_LOC)libsettings/bld/libsettings.a \
	$(LIBS_LOC)libstrop/bld/libstrop.a

#--------------------- Cpp Lint -----------------------------------------------#
LINT          := ../makeccpp/cpplint/cpplint.py --filter=-runtime/references
LINT_FLAGS    :=

#--------------------- Unit Tests ---------------------------------------------#
TEST_SUFFIX   := _TEST
GTEST_BASE    := ../makeccpp/gtest

#--------------------- Compilation and Linking --------------------------------#
CXX           := g++
SRC_EXTS      := .cc
HDR_EXTS      := .h .tcc
CXX_FLAGS     := -Wall -Wextra -pedantic -Wfatal-errors -std=c++11
CXX_FLAGS     += -Wno-unused-parameter -Wno-variadic-macros
CXX_FLAGS     += -march=native -g -O3 -flto
LINK_FLAGS    := -lz

#--------------------- Auto Makefile ------------------------------------------#
include ../makeccpp/auto_bin.mk
