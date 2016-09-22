
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPRO")
endif

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

NO_CTRCOMMON := 1
TOPDIR		?= $(CURDIR)
include $(DEVKITARM)/3ds_rules
include $(TOPDIR)/resources/AppInfo
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

CTRCOMMON			:= $(TOPDIR)/lib/ctrcommon

APP_TITLE			:= $(shell echo "$(APP_TITLE)" | cut -c1-128)
APP_DESCRIPTION		:= $(shell echo "$(APP_DESCRIPTION)" | cut -c1-256)
APP_AUTHOR			:= $(shell echo "$(APP_AUTHOR)" | cut -c1-128)
APP_PRODUCT_CODE	:= $(shell echo $(APP_PRODUCT_CODE) | cut -c1-16)
APP_UNIQUE_ID		:= $(shell echo $(APP_UNIQUE_ID) | cut -c1-7)

BUILD				:= build
SOURCES				:= source 
DATA				:= data
INCLUDES			:= $(SOURCES) include 
ICON				:= resources/icon.png

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

ARCH				:= -march=armv6k -mtune=mpcore -mfloat-abi=hard
COMMON_FLAGS		:= -g -Wall -Wno-strict-aliasing -O3 -mword-relocations \
					-fomit-frame-pointer -ffast-math $(ARCH) $(INCLUDE) -DARM11 -D_3DS $(BUILD_FLAGS)
CXXFLAGS			:= $(COMMON_FLAGS) -std=gnu++11
CFLAGS				:= $(COMMON_FLAGS) -std=gnu99
ASFLAGS				:= -g $(ARCH)
LDFLAGS				:= -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS				:= -lcitro3d -lctru -lm
LIBDIRS				:= $(PORTLIBS) $(CTRULIB) ./lib

RSF_3DS				= $(CTRCOMMON)/tools/template-3ds.rsf
RSF_CIA				= ../template-cia.rsf

ifeq ($(OS),Windows_NT)
	MAKEROM = $(CTRCOMMON)/tools/makerom.exe
	BANNERTOOL = $(CTRCOMMON)/tools/bannertool.exe
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		MAKEROM = $(CTRCOMMON)/tools/makerom-linux
		BANNERTOOL = $(CTRCOMMON)/tools/bannertool-linux
	endif
	ifeq ($(UNAME_S),Darwin)
		MAKEROM = $(CTRCOMMON)/tools/makerom-mac
		BANNERTOOL = $(CTRCOMMON)/tools/bannertool-mac
	endif
endif


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

recurse = $(shell find $2 -type $1 -name '$3' 2> /dev/null)

null				:=
SPACE				:=      $(null) $(null)
export OUTPUT_D		:=	$(CURDIR)/output
export OUTPUT_N		:=	$(subst $(SPACE),,$(APP_TITLE))
export OUTPUT		:=	$(OUTPUT_D)/$(OUTPUT_N)
export TOPDIR		:=	$(CURDIR)

export VPATH		:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir) $(call recurse,d,$(CURDIR)/$(dir),*)) \
						$(foreach dir,$(DATA),$(CURDIR)/$(dir) $(call recurse,d,$(CURDIR)/$(dir),*))

export DEPSDIR		:=	$(CURDIR)/$(BUILD)

CFILES				:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES				:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
BINFILES			:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

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

export OFILES		:=	$(addsuffix .o,$(BINFILES)) \
						$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
						$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE		:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
						$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
						-I$(CURDIR)/$(BUILD)

export LIBPATHS		:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON		:= $(TOPDIR)/$(ICON)

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

icon.icn: $(TOPDIR)/resources/icon.png
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_TITLE)" -p "$(APP_AUTHOR)" -i $(TOPDIR)/resources/icon.png -o icon.icn > /dev/null

$(OUTPUT).elf: $(OFILES)

stripped.elf: $(OUTPUT).elf
	@cp $(OUTPUT).elf stripped.elf
	@$(PREFIX)strip stripped.elf


$(OUTPUT).cia: stripped.elf icon.icn
	@$(MAKEROM) -f cia -o $(OUTPUT).cia -rsf $(RSF_CIA) -target t -exefslogo -elf stripped.elf -icon icon.icn -banner banner.bnr -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(APP_PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(APP_UNIQUE_ID)"
	@echo "built ... $(notdir $@)"

$(OUTPUT).zip: $(OUTPUT_D) $(OUTPUT).elf  $(OUTPUT).smdh  $(OUTPUT).cia
	@cd $(OUTPUT_D); \
	mkdir -p 3ds/$(OUTPUT_N); \
	zip -r $(OUTPUT_N).zip  $(OUTPUT_N).cia  > /dev/null; \
	rm -r 3ds
	@echo "built ... $(notdir $@)"

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

# WARNING: This is not the right way to do this! TODO: Do it right!
#---------------------------------------------------------------------------------
%.vsh.o	:	%.vsh
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@python $(AEMSTRO)/aemstro_as.py $< ../$(notdir $<).shbin
	@bin2s ../$(notdir $<).shbin | $(PREFIX)as -o $@
	@echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(notdir $<).shbin | tr . _)`.h
	@echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(notdir $<).shbin | tr . _)`.h
	@echo "extern const u32" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(notdir $<).shbin | tr . _)`.h
	@rm ../$(notdir $<).shbin

#---------------------------------------------------------------------------------
# rules for assembling GPU shaders
#---------------------------------------------------------------------------------
define shader-as
	$(eval CURBIN := $(patsubst %.shbin.o,%.shbin,$(notdir $@)))
	picasso -o $(CURBIN) $1
	bin2s $(CURBIN) | $(AS) -o $@
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u32" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h
endef

%.shbin.o : %.v.pica %.g.pica
	@echo $(notdir $^)
	@$(call shader-as,$^)

%.shbin.o : %.v.pica
	@echo $(notdir $<)
	@$(call shader-as,$<)

%.shbin.o : %.shlist
	@echo $(notdir $<)
	@$(call shader-as,$(foreach file,$(shell cat $<),$(dir $<)/$(file)))

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
