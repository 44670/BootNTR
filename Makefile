NO_CTRCOMMON := 1

#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------



ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPRO")
endif

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

CTRCOMMON = $(TOPDIR)/lib/ctrcommon

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

include $(TOPDIR)/resources/AppInfo

APP_TITLE := $(shell echo "$(APP_TITLE)" | cut -c1-128)
APP_DESCRIPTION := $(shell echo "$(APP_DESCRIPTION)" | cut -c1-256)
APP_AUTHOR := $(shell echo "$(APP_AUTHOR)" | cut -c1-128)
APP_PRODUCT_CODE := $(shell echo $(APP_PRODUCT_CODE) | cut -c1-16)
APP_UNIQUE_ID := $(shell echo $(APP_UNIQUE_ID) | cut -c1-7)

BUILD := build
SOURCES := source
DATA := data
INCLUDES := $(SOURCES) include
ICON := resources/icon.png

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard

COMMON_FLAGS := -g -Wall -Wno-strict-aliasing -O3 -mword-relocations -fomit-frame-pointer -ffast-math $(ARCH) $(INCLUDE) -DARM11 -D_3DS $(BUILD_FLAGS)

CXXFLAGS := $(COMMON_FLAGS) -std=gnu++11
ifeq ($(ENABLE_EXCEPTIONS),)
	CXXFLAGS += -fno-rtti -fno-exceptions
endif

CFLAGS := $(COMMON_FLAGS) -std=gnu99

ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lctru -lm
LIBDIRS	:= $(PORTLIBS) $(CTRULIB) ./lib
ifeq ($(NO_CTRCOMMON),)
	LIBS	:= -lctrcommon -lctru -lm
	LIBDIRS	:= $(CTRCOMMON) $(PORTLIBS) $(CTRULIB) ./lib
endif

RSF_3DS = $(CTRCOMMON)/tools/template-3ds.rsf
RSF_CIA = ../template-cia.rsf

ifneq ("$(wildcard $(TOPDIR)/resources/banner.cgfx)","")
	BANNER_IMAGE := $(TOPDIR)/resources/banner.cgfx
	BANNER_IMAGE_ARG := -ci $(BANNER_IMAGE)
else
	BANNER_IMAGE := $(TOPDIR)/resources/banner.png
	BANNER_IMAGE_ARG := -i $(BANNER_IMAGE)
endif

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

recurse = $(shell find $2 -type $1 -name '$3' 2> /dev/null)

null            :=
SPACE           :=      $(null) $(null)
export OUTPUT_D	:=	$(CURDIR)/output
export OUTPUT_N	:=	$(subst $(SPACE),,$(APP_TITLE))
export OUTPUT	:=	$(OUTPUT_D)/$(OUTPUT_N)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir) $(call recurse,d,$(CURDIR)/$(dir),*)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir) $(call recurse,d,$(CURDIR)/$(dir),*))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(call recurse,f,$(dir),*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(call recurse,f,$(dir),*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(call recurse,f,$(dir),*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(call recurse,f,$(dir),*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON := $(TOPDIR)/$(ICON)

export _3DSXFLAGS += --smdh=$(OUTPUT).smdh

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:

	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT_D)


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
.PHONY: all
all: $(OUTPUT).zip

$(OUTPUT_D):
	@[ -d $@ ] || mkdir -p $@

banner.bnr: $(BANNER_IMAGE) $(TOPDIR)/resources/audio.wav
	@bannertool makebanner $(BANNER_IMAGE_ARG) -a $(TOPDIR)/resources/audio.wav -o banner.bnr > /dev/null

icon.icn: $(TOPDIR)/resources/icon.png
	@bannertool makesmdh -s "$(APP_TITLE)" -l "$(APP_TITLE)" -p "$(APP_AUTHOR)" -i $(TOPDIR)/resources/icon.png -o icon.icn > /dev/null

$(OUTPUT).elf: $(OFILES)

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh

$(OUTPUT).cia: $(OUTPUT).elf banner.bnr icon.icn
	@makerom -f cia -o $(OUTPUT).cia -rsf $(RSF_CIA) -target t -exefslogo -elf $(OUTPUT).elf -icon icon.icn -banner banner.bnr -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(APP_PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(APP_UNIQUE_ID)"
	@echo "built ... $(notdir $@)"

$(OUTPUT).zip: $(OUTPUT_D) $(OUTPUT).cia $(OUTPUT).3dsx
	@cd $(OUTPUT_D); \
	mkdir -p 3ds/$(OUTPUT_N); \
	zip -r $(OUTPUT_N).zip  $(OUTPUT_N).3dsx $(OUTPUT_N).cia  > /dev/null; \
	rm -r 3ds
	@echo "built ... $(notdir $@)"

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
