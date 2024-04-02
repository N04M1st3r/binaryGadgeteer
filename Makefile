CC := gcc
OPT := -O0
DEBUG := -g

BINARY := bin
CDIR = src
ODIR = obj

PROG_INC_DIRS = ./include
PROG_INC_PARAMS = $(foreach d, $(PROG_INC_DIRS), -I$(d))

ZYDIS_DIR := ./zydis

ZYDIS_INC_DIRS = $(ZYDIS_DIR)/dependencies/zycore/include $(ZYDIS_DIR)/include
ZYDIS_INC_PARAMS = $(foreach d, $(ZYDIS_INC_DIRS), -I$(d))

LIB_DIRS = $(ZYDIS_DIR)/build
LIB_PARAMS = $(foreach d, $(LIB_DIRS), -L$(d))

LIBS = -lZydis -lelf
CFILES = $(foreach d, $(CDIR), $(wildcard $(d)/*.c))
OBJECTS = $(patsubst %.c, $(ODIR)/%.o, $(notdir $(CFILES)))

INC_PARAMS = $(ZYDIS_INC_PARAMS) $(PROG_INC_PARAMS)

CFLAGS = -Wall -Wpedantic -Wpedantic -Wextra  $(DEBUG) $(OPT) $(INC_PARAMS)


all: | $(ODIR) $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) -o $@ $^ $(LIB_PARAMS) $(LIBS) -no-pie

$(ODIR)/%.o: $(CDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR):
	mkdir $(ODIR)

clean:
	@rm -rvf $(BINARY) $(ODIR)/*.o

run: all
	$./$(BINARY)


.PHONY: all clean run
