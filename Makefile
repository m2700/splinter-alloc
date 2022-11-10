include vars.mk

ifneq (,$(ORPHAN_DEPS))

.PHONY: all
all: remove-orphan-files
	@$(MAKE) --no-print-directory $(MAKECMDGOALS)

.PHONY: $(MAKECMDGOALS)
$(MAKECMDGOALS): remove-orphan-files
	@$(MAKE) --no-print-directory $(MAKECMDGOALS)

.PHONY: remove-orphan-files
remove-orphan-files:
	@rm -v '$(ORPHAN_DEPS)'

else # ORPHAN_DEPS

include rules.mk

endif # ORPHAN_DEPS