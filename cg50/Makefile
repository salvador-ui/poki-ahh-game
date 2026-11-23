#---------------------------------------------------------------------------------
# Squad Rush for the Casio fx-CG50 — PrizmSDK (libfxcg) build.
# Based on the PrizmSDK example project Makefile. Run make.bat (Windows).
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(FXCGSDK)),)
export FXCGSDK := C:/Users/salvador/PrizmSDK/PrizmSDK-win-0.6
endif

include $(FXCGSDK)/toolchain/prizm_rules

TARGET		:=	SquadRush
BUILD		:=	build
SOURCES		:=	src
DATA		:=	data
INCLUDES	:=

MKG3AFLAGS := -n basic:SquadRush -i uns:../icon-uns.bmp -i sel:../icon-sel.bmp

CFLAGS	= -O2 -Wall $(MACHDEP) $(INCLUDE) -ffunction-sections -fdata-sections
CXXFLAGS	=	$(CFLAGS) -fno-exceptions
LDFLAGS	= $(MACHDEP) -T$(FXCGSDK)/toolchain/prizm.x -Wl,-static -Wl,-gc-sections
LIBS	:=	 -lc -lfxcg -lgcc
LIBDIRS	:=

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) -I$(LIBFXCG_INC)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBFXCG_LIB)

.PHONY: all clean

all: $(BUILD)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir $@

export CYGWIN := nodosfilewarning
clean:
	$(call rmdir,$(BUILD))
	$(call rm,$(OUTPUT).bin)
	$(call rm,$(OUTPUT).g3a)

else

DEPENDS	:=	$(OFILES:.o=.d)

$(OUTPUT).g3a: $(OUTPUT).bin
$(OUTPUT).bin: $(OFILES)

-include $(DEPENDS)

endif
