CC ?= gcc
ATFSROOT ?= /opt/ATFS
ATFSARCH ?= i386
ATFSOS ?= linux
ATFSSYS ?= ATFSsys-4.05F1
ATFSSYSTEM ?= $(ATFSROOT)/$(ATFSARCH)/$(ATFSOS)
ATFS_INCLUDE_DIR ?= $(ATFSSYSTEM)/$(ATFSSYS)/include
ATFS_LIB_DIR ?= $(ATFSSYSTEM)/$(ATFSSYS)/lib
ATFS_STATIC_LIBS ?= $(ATFS_LIB_DIR)/libatfshn.a
CFLAGS ?= -m32 -O2 -Wall -std=c99 -D_POSIX_C_SOURCE=200112L
CPPFLAGS ?= -Iinclude -I$(ATFS_INCLUDE_DIR)
LDFLAGS ?= -m32 -Wl,-E -Wl,--whole-archive $(ATFS_STATIC_LIBS) -Wl,--no-whole-archive -ldl -lrt
TARGET = gpib-test
SOURCES = \
	src/main.c \
	src/config.c \
	src/gpib_runtime.c \
	src/gpib_command.c \
	src/cli.c \
	src/interactive.c \
	src/output.c \
	src/text_utils.c \
	src/shutdown.c

.PHONY: all clean print-atfs-build verify-atfs-symbols

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

print-atfs-build:
	@echo "ATFSROOT=$(ATFSROOT)"
	@echo "ATFSARCH=$(ATFSARCH)"
	@echo "ATFSOS=$(ATFSOS)"
	@echo "ATFSSYS=$(ATFSSYS)"
	@echo "ATFSSYSTEM=$(ATFSSYSTEM)"
	@echo "ATFS_INCLUDE_DIR=$(ATFS_INCLUDE_DIR)"
	@echo "ATFS_LIB_DIR=$(ATFS_LIB_DIR)"
	@echo "ATFS_STATIC_LIBS=$(ATFS_STATIC_LIBS)"

verify-atfs-symbols: $(TARGET)
	nm -D $(TARGET) | grep UTHN_Handle_FindArea

clean:
	rm -f $(TARGET)
