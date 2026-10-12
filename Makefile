# ---------------------------------------------------------------------------
# Makefile for MPLAB XC8 (xc8-cc) command-line builds
# Target: PIC12F675
# ---------------------------------------------------------------------------

XC8      := xc8-cc

TARGET   := main
SRC      := $(TARGET).c
BUILDDIR := build
HEADERS  := uart_transmitter.h timer1_timebase.h led_indicator.h button_input.h \
	clock_calendar.h clock_display.h
ROOT_BUILD_FILES := \
	__eeprom.d __eeprom.p1 \
	$(TARGET).cmf $(TARGET).d $(TARGET).elf $(TARGET).hex $(TARGET).hxl \
	$(TARGET).o $(TARGET).p1 $(TARGET).s $(TARGET).sdb $(TARGET).sym \
	startup.lst startup.o startup.rlf startup.s

# Device part number (no "PIC" prefix)
MCU      := 12F675

# XC8 v3.x+ needs an explicit Device Family Pack (DFP). Set this to the
# path found via:
#   Get-ChildItem "C:\Program Files\Microchip\MPLABX" -Recurse -Filter "pic12f675.h"
#   Get-ChildItem "$env:USERPROFILE\.mchp_packs" -Recurse -Filter "pic12f675.h"
# strip off "\pic\include\proc\pic12f675.h" -- what's left (ending in
# "...\xc8") is the DFP path below.
DFP     := C:\Program Files\Microchip\MPLABX\v6.35\packs\Microchip\PIC10-12Fxxx_DFP\1.9.189\xc8

CFLAGS   := -mcpu=$(MCU) -mdfp='$(DFP)' -O2 -o $(BUILDDIR)/$(TARGET).elf

.PHONY: all clean program info

all: $(BUILDDIR) $(BUILDDIR)/$(TARGET).hex

ifeq ($(OS),Windows_NT)
$(BUILDDIR):
	powershell -NoProfile -Command "New-Item -ItemType Directory -Path '$(BUILDDIR)' -Force | Out-Null"

$(BUILDDIR)/$(TARGET).hex: $(SRC) $(HEADERS) | $(BUILDDIR)
	powershell -NoProfile -Command "& '$(XC8)' $(CFLAGS) $(SRC); if ($$LASTEXITCODE -ne 0) { exit $$LASTEXITCODE }"
	powershell -NoProfile -Command "foreach ($$file in @('$(TARGET).hex','$(TARGET).p1','$(TARGET).d','$(TARGET).obj','$(TARGET).sym','$(TARGET).lst')) { if (Test-Path -LiteralPath $$file) { Move-Item -LiteralPath $$file -Destination '$(BUILDDIR)' -Force -ErrorAction Stop } }"
else
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(TARGET).hex: $(SRC) $(HEADERS) | $(BUILDDIR)
	$(XC8) $(CFLAGS) $(SRC)
	@# xc8-cc doesn't always respect -o for every side-file it emits;
	@# sweep any stragglers left in the project root into build/.
	@for f in $(TARGET).hex $(TARGET).p1 $(TARGET).d $(TARGET).obj $(TARGET).sym $(TARGET).lst; do \
		[ -f "$$f" ] && mv "$$f" $(BUILDDIR)/ || true; \
	done
endif

info:
	$(XC8) --version

ifeq ($(OS),Windows_NT)
clean:
	powershell -NoProfile -Command "foreach ($$file in '$(ROOT_BUILD_FILES)'.Split(' ')) { if (Test-Path -LiteralPath $$file) { Remove-Item -LiteralPath $$file -Force -ErrorAction Stop } }"
	powershell -NoProfile -Command "foreach ($$dir in @('$(BUILDDIR)','_build','out')) { if (Test-Path -LiteralPath $$dir) { Remove-Item -LiteralPath $$dir -Recurse -Force -ErrorAction Stop } }"
else
clean:
	rm -f $(ROOT_BUILD_FILES)
	rm -rf $(BUILDDIR) _build out
endif

# Programs the chip via CCSLOAD/Mach X once the hex is built.
# Confirm CCSLOAD's exact CLI flags with `ccsload /?` first.
program: all
	ccsload $(BUILDDIR)/$(TARGET).hex