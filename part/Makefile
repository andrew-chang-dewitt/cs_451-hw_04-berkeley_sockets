CC         = gcc

# from https://stackoverflow.com/questions/154630/recommended-gcc-warning-options-for-c
CFLAGS    += -Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align \
	     -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Winline \
	     -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wwrite-strings \
	     -Wconversion -Wunreachable-code -Wstrict-aliasing=2 -fno-common \
	     -fstrict-aliasing -std=c99 -pedantic
	     
CMPT_FLAGS = -DINCLUDE_PID -I../libcompart/

ifeq ($(LC_LIB),dynamic)
	CMPT_FLAGS += -L../libcompart/ -lcompart
else
	CMPT_FLAGS += ../libcompart/compart.o
endif

DBG_FLAGS  = -O0 -g -include stdbool.h
REL_FLAGS  = -O3
PRF_FLAGS  = -pg

LOC       := $(shell readlink -f .)
PRJ_ROOT  := $(LOC)/..

TGT_DIR   := $(PRJ_ROOT)/target/part

REL_DIR   := $(TGT_DIR)/release
DBG_DIR   := $(TGT_DIR)/debug


ifeq ($(DBG),true)
	CFLAGS  += $(DBG_FLAGS)
	CMPT_FLAGS += -DPITCHFORK_DBGSTDOUT 
	OUT_DIR := $(DBG_DIR)
else ifeq ($(PRF),true)
	CFLAGS  += $(PRF_FLAGS)
	CMPT_FLAGS += -DPITCHFORK_DBGSTDOUT 
	OUT_DIR := $(DBG_DIR)
else
	CFLAGS  += $(REL_FLAGS)
	OUT_DIR := $(REL_DIR)
endif

BIN_DIR   := $(OUT_DIR)/bin
OBJ_DIR   := $(OUT_DIR)/obj
SRC_DIR   := $(LOC)

DEPS      := world.c interface.c
OBJS      := $(DEPS:%.c=$(OBJ_DIR)/%.o)

.PHONY: clean part


part: $(BIN_DIR)/partitioned_life


$(BIN_DIR)/partitioned_life: $(SRC_DIR)/partitioned_life.c $(OBJS) | $(BIN_DIR)
	@echo "building binary $@ from $< ..."
	$(CC) $(CFLAGS) $(CMPT_FLAGS) $< $(OBJS) -o $@
	@echo "... $@ built."

$(OBJ_DIR)/world.o: $(SRC_DIR)/world.c | $(OBJ_DIR)
	@echo "building object $@ from $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "... $@ built."

$(OBJ_DIR)/interface.o: $(SRC_DIR)/interface.c | $(OBJ_DIR)
	@echo "building object $@ from $< ..."
	$(CC) $(CFLAGS) $(CMPT_FLAGS) -c $< -o $@
	@echo "... $@ built."

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	rm -rf $(TGT_DIR)
