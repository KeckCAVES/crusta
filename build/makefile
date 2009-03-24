#name of your project. Will end up being the name of your binary
PROJECT         = Crusta

#project directories (for now I can only support a single source directory)
SRC_DIR_BASE    = ../source
SRC_DIRS        = ../source/crusta
INCLUDE_DIRS    = ../include/crusta
LIB_DIRS        =

#project preprocessor definitions and dependent libraries
PREPROC         =
LIBS            =

#project flags
CFLAGS          = -pipe -Wall
LFLAGS          = -pipe

#default project configuration
LIB_VRUI = $(HOME)/dev/lib/vrui

#configuration dependent parameters
TMP_DIR_ROOT = tmp

INITIAL_TARGET   =
ifeq ($(cfg),)
  TARGET        :=
  INITIAL_TARGET = debug
endif

ifeq ($(cfg),debug)
  TMP_DIR    = $(TMP_DIR_ROOT)/dbg
  TARGET    := $(PROJECT)
  CFLAGS    += -DDEBUG -g3 -O0
  LFLAGS	+=
  include $(LIB_VRUI)/etc/Vrui.debug.makeinclude
endif

ifeq ($(cfg),profile)
  TMP_DIR    = $(TMP_DIR_ROOT)/prf
  TARGET    := $(PROJECT)
  CFLAGS    += -pg
  LFLAGS    += -pg
  include $(LIB_VRUI)/etc/Vrui.debug.makeinclude
endif

ifeq ($(cfg),release)
  TMP_DIR    = $(TMP_DIR_ROOT)/rel
  TARGET    := $(PROJECT)
  CFLAGS    += -DRELEASE -O2 -floop-optimize -finline-functions\
               -fstrength-reduce
  LFLAGS    +=
  include $(LIB_VRUI)/etc/Vrui.makeinclude
endif

#vrui plugin parameters
TARGET      := $(patsubst %,lib%.$(VRUI_PLUGINFILEEXT),$(TARGET))
CFLAGS      += $(VRUI_PLUGINCFLAGS)
LFLAGS      += $(VRUI_PLUGINLINKFLAGS)
OUTPUT_DIR   = $(VRUI_VISLETSDIR)

#no need to touch anything below this point, unless you know
#what it is you're doing ;)

#generate the complete gcc flags
CC              = g++
C_INCLUDE_DIRS  = $(patsubst %, -I"%", $(INCLUDE_DIRS))
C_LIB_DIRS      = $(patsubst %, -L"%", $(LIB_DIRS))
C_PREPROC       = $(patsubst %, -D%, $(PREPROC))
C_LIBS          = $(patsubst %, -l%, $(LIBS))

#generate object list (out of .cpp and .c files in SRC_DIRS)
SRC_FILES       = $(shell find $(SRC_DIRS) -type f -name '*.cpp')
SRC_FILES      += $(shell find $(SRC_DIRS) -type f -name '*.c')

SRC_OBJS        = $(patsubst $(SRC_DIR_BASE)/%, $(TMP_DIR)/%o, $(SRC_FILES))
SRC_DEPS        = $(patsubst $(SRC_DIR_BASE)/%, $(TMP_DIR)/%d, $(SRC_FILES))

.PHONY: all debug clean superclean debug-clean debug-superclean\
        release release-clean release-superclean\
        profile profile-clean profile-superclean\
        print_header directories internal-clean internal-superclean

all: $(INITIAL_TARGET)$(TARGET)
debug:
	@make cfg=debug 
clean: debug-clean
superclean: debug-superclean
debug-clean:
	@make cfg=debug internal-clean
debug-superclean:
	@make cfg=debug internal-superclean
release:
	@make cfg=release 
release-clean:
	@make cfg=release internal-clean
release-superclean:
	@make cfg=release internal-superclean
profile:
	@make cfg=profile 
profile-clean:
	@make cfg=profile profile-clean
profile-superclean:
	@make cfg=profile profile-superclean


$(TARGET): print_header directories $(SRC_OBJS)
	@echo Linking...
	@$(CC) -o "$(OUTPUT_DIR)/$(TARGET)" $(SRC_OBJS) $(C_LIB_DIRS) $(C_LIBS)\
           $(LFLAGS) $(VRUI_LINKFLAGS)
	@echo done.

print_header:
	@echo ----------Configuration: $(PROJECT) - $(cfg)----------

directories:
	-@if [ ! -d "$(OUTPUT_DIR)" ]; then mkdir -p "$(OUTPUT_DIR)"; fi
	-@if [ ! -d "$(TMP_DIR)" ]; then mkdir -p "$(TMP_DIR)"; fi

internal-superclean: internal-clean
	@echo Deleting all final files for '$(PROJECT) - $(cfg)'
	-@if [ -e "$(OUTPUT_DIR)/$(TARGET)" ]; then rm -f $(OUTPUT_DIR)/$(TARGET);\
         find "$(OUTPUT_DIR)" -depth -type d -empty -exec rmdir {} \;;fi

internal-clean:
	@echo Deleting intermediate files for '$(PROJECT) - $(cfg)'
	-@if [ -d "$(TMP_DIR)" ]; then rm -f $(SRC_OBJS) $(SRC_DEPS); fi
	-@if [ -d "$(TMP_DIR_ROOT)" ]; then\
         find "$(TMP_DIR_ROOT)" -depth -type d -empty -exec rmdir {} \;; fi


#dependency rule
define depend_source
@echo Generating depedency for $<
@if [ ! -d "$(shell dirname $@)" ]; then mkdir -p $(shell dirname $@); fi
@set -e
@$(CC) -MM -MT "$@" $(CFLAGS) $(C_PREPROC) $(C_INCLUDE_DIRS) $(VRUI_CFLAGS)\
       "$<" > "$(patsubst %o,%d,$@)"
endef

#compile rule
define compile_source
@echo Compiling $<
@if [ ! -d "$(shell dirname $@)" ]; then mkdir -p $(shell dirname $@); fi
@$(CC) $(CFLAGS) $(C_PREPROC) $(C_INCLUDE_DIRS) $(VRUI_CFLAGS) -c "$<" -o "$@"
endef

$(TMP_DIR)/%.cppo: $(SRC_DIR_BASE)/%.cpp
	$(depend_source)
	$(compile_source)
$(TMP_DIR)/%.co: $(SRC_DIR_BASE)/%.c
	$(depend_source)
	$(compile_source)

ifeq ($(INITIAL_TARGET),)
  -include $(SRC_DEPS)
endif
