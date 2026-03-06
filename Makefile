CC         = gcc

# from https://stackoverflow.com/questions/154630/recommended-gcc-warning-options-for-c
CFLAGS    += -Wall -Wextra -Wformat=2 -Wswitch-default -Wcast-align \
	     -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Winline \
	     -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wwrite-strings \
	     -Wconversion -Wunreachable-code -Wstrict-aliasing=2 -fno-common \
	     -fstrict-aliasing -std=c99 -pedantic
	     
DBG_FLAGS  = -O0 -g -include stdbool.h
REL_FLAGS  = -O3
PRF_FLAGS  = -pg

LOC       := $(shell readlink -f .)
PRJ_ROOT  := $(LOC)
TGT_DIR   := $(PRJ_ROOT)/target

REL_DIR   := /release
DBG_DIR   := /debug


ifeq ($(DBG),true)
	CFLAGS  += $(DBG_FLAGS)
	OUT_DIR  = $(TGT_DIR)$(DBG_DIR)
else ifeq ($(PRF),true)
	CFLAGS  += $(PRF_FLAGS)
	OUT_DIR  = $(TGT_DIR)$(DBG_DIR)
else
	CFLAGS  += $(REL_FLAGS)
	OUT_DIR  = $(TGT_DIR)$(REL_DIR)
endif

# turn on verbose print debugging if specified in env
ifeq ($(VERBOSE),true)
	CFLAGS  += -D VERBOSE
endif

# turn on pretty printing if specified in env
ifeq ($(PRETTY),true)
	CFLAGS  += -D PRETTY
endif

BIN_DIR   := $(OUT_DIR)/bin
OBJ_DIR   := $(OUT_DIR)/obj
DEP_DIR   := $(OUT_DIR)/dep
SRC_DIR   := $(LOC)

# gcc flags to automatically generate dependencies, from
# https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#tldr
DEP_FLAGS  = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d

NAMES     := life multithreaded_life granular_multithreaded_life
SRCS      := $(wildcard $(SRC_DIR)/*.c)
BINS      := $(NAMES:%=$(BIN_DIR)/%)
OBJS       = $(patsubst $(SRC_DIR)/%.h,$(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.h))


.PHONY: clean all life multi gran list_bins

all: life multi gran


life: $(BIN_DIR)/life


multi: $(BIN_DIR)/multithreaded_life


gran: $(BIN_DIR)/granular_multithreaded_life


$(BIN_DIR) $(OBJ_DIR) $(DEP_DIR): %:
	@mkdir -p $@


DEPS      := $(SRCS:$(SRC_DIR)/%.c=$(DEP_DIR)/%.d)
$(DEPS):


include $(wildcard $(DEPS))


list:
	@echo ""
	@echo "    SRC_DIR = $(SRC_DIR)"
	@echo "    TGT_DIR = $(TGT_DIR)"
	@echo "    OUT_DIR = $(OUT_DIR)"
	@echo "    BIN_DIR = $(BIN_DIR)"
	@echo "    OBJ_DIR = $(OBJ_DIR)"
	@echo "    DEP_DIR = $(DEP_DIR)"
	@echo "    NAMES   = $(NAMES)"
	@echo "    SRCS    = $(SRCS)"
	@echo "    BINS    = $(BINS)"
	@echo "    DEPS    = $(DEPS)"
	@echo "    OBJS    = $(OBJS)"
	@echo ""


# automatically generate dependencies, from
# https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#tldr
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(DEP_DIR)/%.d | $(DEP_DIR)
	@echo "compiling dependency..."
	$(CC) $(DEP_FLAGS) $(CFLAGS) -c $< -o $@
	@echo "...dependency $@ built."



$(BIN_DIR)/%: $(SRC_DIR)/%.c $(DEP_DIR)/%.d $(OBJS) | $(BIN_DIR) $(DEP_DIR) $(OBJ_DIR)
	@echo "building binary..."
	$(CC) $(CFLAGS) $(CMPT_FLAGS) $< $($<:%.c=%.h) $(OBJS) -o $@
	@echo "...binary $@ built."

clean:
	rm -rf $(TGT_DIR)
