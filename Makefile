#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

# Makefile RaceTrack
#------------------------------------------------------------------------------
# useful links: 
# - automatic vars: https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
# - file names: https://www.gnu.org/software/make/manual/html_node/File-Name-Functions.html
#------------------------------------------------------------------------------

################################################################################
# ARCH DEPENDENT VARIABLES
ARCH_FILE ?= make_arch/macos.mak

#only include the ARCH_FILE when we do not clean or destroy
#ifneq ($(MAKECMDGOALS),clean)
include $(ARCH_FILE)
#endif

################################################################################
# FROM HERE, DO NOT CHANGE
#-------------------------------------------------------------------------------
NAME := benchme
TARGET := $(NAME)
#GIT_COMMIT ?= $(shell git describe --always --dirty)

#-------------------------------------------------------------------------------
# get a list of all the source sub-directories + the main one
SRC_DIR := src $(shell find src/** -type d)
OBJ_DIR := build

#-------------------------------------------------------------------------------
# only the source files are needed here, vpath will find the rest
SRC := $(foreach dir,$(SRC_DIR),$(notdir $(wildcard $(dir)/*.cpp)))
# if you need to list the header files, you can use this line
# HEAD := $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.hpp))

## generate object list, the dependence list and the compilation data-base list
OBJ := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEP := $(SRC:%.cpp=$(OBJ_DIR)/%.d)
#CDB := $(SRC:%.cpp=$(OBJ_DIR)/%.o.json)

#-------------------------------------------------------------------------------
# add the folders to the includes and to the vpath
INC := $(foreach dir,$(SRC_DIR),-I$(dir))

# add them to the VPATH as well
# (https://www.gnu.org/software/make/manual/html_node/Selective-Search.html)
vpath %.hpp $(SRC_DIR) $(foreach dir,$(SRC_DIR),$(TEST_DIR)/$(dir))
vpath %.cpp $(SRC_DIR) $(foreach dir,$(SRC_DIR),$(TEST_DIR)/$(dir))

##-------------------------------------------------------------------------------
## LIBRARIES
##---- Google benchmark
#GBENCH_DIR ?= benchmark
#GBENCH_INC ?= ${GBENCH_DIR}/include
#GBENCH_LIB ?= ${GBENCH_DIR}/lib
#GBENCH_LIBNAME ?= -lbenchmark
#
#INC += -I${GBENCH_INC}
#LIB += -L${GBENCH_LIB} ${GBENCH_LIBNAME} -Wl,-rpath,${GBENCH_LIB}

################################################################################
# some usefull variables/functions
comma := ,
empty :=
space := $(empty) $(empty)
nline := \n

to_newline = $(subst $(space),$(nline),$(strip $(1) ))
is_clang := $(shell $(CXX) -v 2>&1 | grep -c "clang")

################################################################################
# mandatory flags
M_FLAGS := -std=c++17 -fPIC -DGIT_COMMIT=\"$(GIT_COMMIT)\"

#-------------------------------------------------------------------------------
# compile + dependence + json file
$(OBJ_DIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(M_FLAGS) -MMD -c $< -o $@

#$(OBJ_DIR)/%.o : %.cpp
#ifeq ($(is_clang), 1)
#	$(CXX) $(CXXFLAGS) $(INC) $(M_FLAGS) -MMD -MF $(OBJ_DIR)/$*.d -MJ $(OBJ_DIR)/$*.o.json -c $< -o $@
#else
#	$(CXX) $(CXXFLAGS) $(INC) $(M_FLAGS) -MMD -c $< -o $@
#endif

## json file
#$(OBJ_DIR)/%.o.json :  %.cpp
#	$(CXX) $(CXXFLAGS) $(INC) $(M_FLAGS) -MJ $@ -E $< -o $(OBJ_DIR)/$*.ii

################################################################################
.PHONY: default
default: all

.PHONY: all
all: ${TARGET}

#-------------------------------------------------------------------------------
# the main target
$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ $(LIB) -o $@

#-------------------------------------------------------------------------------
# clang stuffs

# for the sed command, see https://sarcasm.github.io/notes/dev/compilation-database.html#clang and https://sed.js.org
#.PHONY: compdb
#compdb: $(CDB)
#	sed -e '1s/^/[\n/' -e '$$s/,$$/\n]/' $^ > compile_commands.json

# get the cleaned-up version of the flags
CLANGD_FLAGS := $(call to_newline,$(CXXFLAGS) $(INC) $(M_FLAGS)) 
CLANGD_MPI := $(shell $(CXX) -compile_info) 
CLANGD_MPIF := $(filter -I%,$(shell $(CXX) -compile_info))

.PHONY: flags
flags:
	@echo "$(CLANGD_FLAGS)" > compile_flags.txt ;\
	echo "$(CLANGD_MPIF)" >> compile_flags.txt

#-------------------------------------------------------------------------------
#clean
.PHONY: clean
clean:
	@rm -rf $(TARGET)
	@rm -rf $(OBJ_DIR)/*
	
#-------------------------------------------------------------------------------
.EXPORT_ALL_VARIABLES:

#.PHONY: info
#info: 
#	@$(MAKE) --file=info.mak

# include the needed deps
-include $(DEP)
# mkdir the needed dirs
$(shell   mkdir -p $(OBJ_DIR))



# end of file
