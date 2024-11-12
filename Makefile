#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------

# [nitr8]: change this to "boot"
#TARGET		:=	$(notdir $(CURDIR))
TARGET		:=	boot

BUILD		:=	build
SOURCES		:=	src src/hbc src/jpeglib
DATA		:=	data
INCLUDES	:=	include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

# [nitr8]: be really aggressive, so add this - then fix even more compiler warnings
#WARNPLUS = -Wpedantic

# [nitr8]: be more specific, so add this - then fix compiler warnings
WARNFLAGS = -Wextra $(WARNPLUS)

# [nitr8]: Add support for realtime debugging using a USB-Gecko (-g) */
CFLAGS = -g -O2 -Wall $(WARNFLAGS) $(MACHDEP) $(INCLUDE)

# [nitr8]: Reworked
ifndef NO_DOLPHIN_CHECK
# [nitr8]: Reworked
#	CFLAGS = -O2 -Wall $(WARNFLAGS) $(MACHDEP) $(INCLUDE)
	CFLAGS += -DDOLPHIN_CHECK
else
	TARGET = rgdboot-installer_noDolphinCheck
endif

# Skipping the version clear will cause a brick on Wiis with a boot2 version higher than 0 - use for testing if you have a flash programmer only
ifdef NO_VERSION_CLEAR
# [nitr8]: Reworked
#	CFLAGS = -O2 -Wall $(WARNFLAGS) $(MACHDEP) $(INCLUDE)
	CFLAGS += -DNO_VERSION_CLEAR
	TARGET = rgdboot-installer_noVersionClear
endif

CXXFLAGS = $(CFLAGS)

LDFLAGS	= $(MACHDEP) -T$(CURDIR)/../rgdboot.ld -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------

# [nitr8]: Add support for realtime debugging using a USB-Gecko (libdb) */
#LIBS	:=	-lfat -lwiiuse -lbte -logc -lm
LIBS	:=	-lfat -lwiiuse -lbte -logc -ldb -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC) \
					-iquote $(CURDIR)/$(INCLUDES)/hbc \
					-iquote $(CURDIR)/$(INCLUDES)/jpeglib

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:= -L$(LIBOGC_LIB) $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
#buildNumber = $(shell git rev-list --count HEAD)
# [nitr8]: get rid of warnings when variables are not used at all
$(BUILD):
#	@echo "static char *buildNumber = \"$(buildNumber)\";" > $(INCLUDES)/version.h
#	@echo "char *buildNumber = \"$(buildNumber)\";" > $(INCLUDES)/version.h

	@gcc -s -Os -Wall -Wextra -o buildnumber buildNumber.cpp
	@./buildnumber $(INCLUDES)/version.h
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol
	@rm -fr $(BUILD) rgdboot-installer_noDolphinCheck.elf rgdboot-installer_noDolphinCheck.dol

#---------------------------------------------------------------------------------
run:
	wiiload $(TARGET).dol
	

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the .jpg extension
#---------------------------------------------------------------------------------
%.jpg.o	%_jpg.h :	%.jpg
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
