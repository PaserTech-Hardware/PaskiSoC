TARGET := hello_demo
BUILDDIR := ./build
OUTPUTDIR := ./output
SOURCEDIR := src

CC := /opt/rv32im/bin/riscv32-unknown-elf-gcc
OBJCOPY := /opt/rv32im/bin/riscv32-unknown-elf-objcopy
PYTHON := python2.7

VPATH = $(dir $(wildcard ./$(SOURCEDIR)/)) $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))
VPATH += $(dir $(wildcard ./$(SOURCEDIR)/*/*/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/*/*/))

OBJS = $(addprefix $(BUILDDIR)/$(TARGET)/, \
        start.o \
	main.o \
	uart.o \
	logging.o \
	sd.o \
)

ARCH := -march=rv32i -mabi=ilp32 -mcmodel=medany -static
CFLAGS = $(ARCH) -Os -g -nostdlib -ffunction-sections -fdata-sections -fomit-frame-pointer -std=gnu11 -Wall $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections -Xlinker

.PHONY: all clean

all: $(TARGET).bin
	@echo "--------------------------------------"
	@echo -n "Firmware size:     "
	$(eval BIN_SIZE = $(shell wc -c < $(OUTPUTDIR)/$(TARGET).bin))
	@echo $(BIN_SIZE)" Bytes"
	@echo "--------------------------------------"

clean:
	@rm -rf $(OBJS)
	@rm -rf $(BUILDDIR)
	@rm -rf $(OUTPUTDIR)

$(TARGET).bin: $(BUILDDIR)/$(TARGET)/$(TARGET).elf
	$(OBJCOPY) -R .readwrite -S -O binary $< $(OUTPUTDIR)/temp1
	$(OBJCOPY) -j .readwrite -S -O binary $< $(OUTPUTDIR)/temp2
	@cat $(OUTPUTDIR)/temp1 $(OUTPUTDIR)/temp2 > $(OUTPUTDIR)/$@
	@rm -rf $(OUTPUTDIR)/temp1
	@rm -rf $(OUTPUTDIR)/temp2

$(BUILDDIR)/$(TARGET)/$(TARGET).elf: $(OBJS)
	@$(CC) $(LDFLAGS) -T ./link.ld $^ -o $@
	@echo "Firmware was built with the following flags:\nCFLAGS:  "$(CFLAGS)"\nLDFLAGS: "$(LDFLAGS)

$(BUILDDIR)/$(TARGET)/%.o: %.c
	@echo Building $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.S
	@echo Building $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET):
	@mkdir -p "$(BUILDDIR)"
	@mkdir -p "$(BUILDDIR)/$(TARGET)"
	@mkdir -p "$(OUTPUTDIR)"
