.PHONY: all
all: $(BINS)

define dir_dependency_rule =
$(call dep2obj,$1): | $(patsubst %/,%,$(dir $1))
endef
$(foreach bdir,$(SUB_BUILD_DIRS) $(OBJS) $(DEPS) $(BINS),$(eval $(call dir_dependency_rule,$(bdir))))

$(BUILD_DIR) $(SUB_BUILD_DIRS):
	@mkdir -v $@

$(OBJS): $(MK_FILES)

-include $(DEPS)

# for not existing dependency files (needed by obj rule)
$(foreach D,$(DEPS),$(eval $D:))

$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.$(SRC_EXT) $(BUILD_DIR)/dep/%.d
ifeq ($(LANG),C)
	$(CC) $(C_FLAGS) -MD -MP -MF$(call src2dep,$<) -MT$@ -c $< -o $@
endif
ifeq ($(LANG),C++)
	$(CXX) $(CXX_FLAGS) -MD -MP -MF$(call src2dep,$<) -MT$@ -c $< -o $@
endif

$(BUILD_DIR)/bin/%: $(BUILD_DIR)/obj/bin/%.o $(LIB_OBJS)
	$(LD) $(LD_FLAGS) $^ -o $@

.PHONY: clean
clean:
	@rm -rfv $(BUILD_DIR)

define bin_rule =
.PHONY: $1
$1: $(call new_bin,$1) 
endef
$(foreach bin,$(BIN_NAMES),$(eval $(call bin_rule,$(bin))))

define run_rule =
run-$1: $1
	@echo running '$1' $(RUN_ARGS) ...
	@$(call new_bin,$1) $(RUN_ARGS) || echo Exit Code: $$$$?
endef
$(foreach bin,$(BIN_NAMES),$(eval $(call run_rule,$(bin))))