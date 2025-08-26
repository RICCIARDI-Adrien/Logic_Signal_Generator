PATH_BINARIES = $(shell realpath .)/Binaries
PATH_INCLUDES = $(shell realpath .)/Includes
PATH_OBJECTS = $(shell realpath .)/Objects
PATH_SOURCES = $(shell realpath .)/Sources

FIRMWARE_VERSION=1.0

BINARY_NAME = Logic_Signal_Generator.hex
SOURCES = \
	$(PATH_SOURCES)/Log.c \
	$(PATH_SOURCES)/Main.c \
	$(PATH_SOURCES)/MSSP.c \
	$(PATH_SOURCES)/Shell.c \
	$(PATH_SOURCES)/Shell_Command_Help.c \
	$(PATH_SOURCES)/Shell_Command_I2C.c \
	$(PATH_SOURCES)/Shell_Commands.c \
	$(PATH_SOURCES)/UART.c \
	$(PATH_SOURCES)/USB_Communications.c \
	$(PATH_SOURCES)/USB_Core.c

CC = xc8-cc
# The -Wa,-a argument tells the assembler to generate a listing file
CFLAGS = -mcpu=18F25K50 -Xparser -W -Xparser -Wall -D_XTAL_FREQ=48000000 -DMAKEFILE_FIRMWARE_VERSION=\\\"$(FIRMWARE_VERSION)\\\" -Wa,-a -Wl,-Map=Logic_Signal_Generator.map

all: $(PATH_BINARIES) $(PATH_OBJECTS)
	cd $(PATH_OBJECTS) && $(CC) $(CFLAGS) -I$(PATH_INCLUDES) $(SOURCES) -o $(BINARY_NAME)
	mv $(PATH_OBJECTS)/$(BINARY_NAME) $(PATH_BINARIES)

debug: CFLAGS += -DLOG_ENABLE_LOGGING
debug: all

$(PATH_BINARIES):
	mkdir -p $(PATH_BINARIES)

$(PATH_OBJECTS):
	mkdir -p $(PATH_OBJECTS)

clean:
	rm -rf $(PATH_BINARIES) $(PATH_OBJECTS)

cppcheck:
	cppcheck -I $(PATH_INCLUDES) --platform=pic8-enhanced --check-level=exhaustive $(PATH_SOURCES)
