include config.mk

ifndef SRC_EXT

SRC_EXT := src_ext

ifeq ($(LANG),C)
SRC_EXT := c
else ifeq ($(LANG),C++)
SRC_EXT := cpp
endif # LANG

endif # SRC_EXT

MK_FILES = Makefile $(wildcard *.mk)

BIN_SRCS = $(wildcard $(SRC_DIR)/bin/*.$(SRC_EXT))
LIB_SRCS = $(wildcard $(SRC_DIR)/*.$(SRC_EXT)) \
		   $(foreach SD,$(SUB_SRC_DIRS),$(wildcard $(SRC_DIR)/$(SD)/*.$(SRC_EXT)))
SRCS = $(BIN_SRCS) $(LIB_SRCS)

src2obj = $(1:$(SRC_DIR)/%.$(SRC_EXT)=$(BUILD_DIR)/obj/%.o)
src2dep = $(1:$(SRC_DIR)/%.$(SRC_EXT)=$(BUILD_DIR)/dep/%.d)
dep2src = $(1:$(BUILD_DIR)/dep/%.d=$(SRC_DIR)/%.$(SRC_EXT))
dep2obj = $(1:$(BUILD_DIR)/dep/%.d=$(BUILD_DIR)/obj/%.o)
obj2bin = $(1:$(BUILD_DIR)/obj/bin/%.o=$(BUILD_DIR)/bin/%)
new_bin = $(BUILD_DIR)/bin/$1
bin2name = $(1:$(BUILD_DIR)/bin/%=%)

BIN_OBJS = $(call src2obj,$(BIN_SRCS))
LIB_OBJS = $(call src2obj,$(LIB_SRCS))
OBJS = $(BIN_OBJS) $(LIB_OBJS)
.SECONDARY: $(OBJS)

DEPS = $(call src2dep,$(SRCS))
EXISTING_BIN_DEPS = $(wildcard $(BUILD_DIR)/dep/bin/*.d)
EXISTING_LIB_DEPS = $(wildcard $(BUILD_DIR)/dep/*.d)
EXISTING_DEPS = $(EXISTING_BIN_DEPS) $(EXISTING_LIB_DEPS)
ORPHAN_DEPS := $(strip $(foreach dep,$(EXISTING_DEPS),$(if $(wildcard $(call dep2src,$(dep))),,$(dep))))

BINS = $(call obj2bin,$(BIN_OBJS))
BIN_NAMES = $(call bin2name,$(BINS))

BUILD_ARTEFACT_CATEGORIES = obj dep
BUILD_CATEGORIES = $(BUILD_ARTEFACT_CATEGORIES) $(BUILD_ARTEFACT_CATEGORIES:%=%/bin) bin
SUB_BUILD_DIRS = $(addprefix $(BUILD_DIR)/,$(BUILD_CATEGORIES) \
				   $(foreach SD,$(SUB_SRC_DIRS),$(BUILD_ARTEFACT_CATEGORIES:%=%/$(SD))))