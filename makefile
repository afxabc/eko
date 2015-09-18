
# All Target
all: 
	@cd base && $(MAKE)
	@echo ' '
	@cd net && $(MAKE)
	@echo ' '
	@cd test && $(MAKE)
	@echo ' '

clean: 
	@cd base && $(MAKE) clean
	@echo ' '
	@cd net && $(MAKE) clean
	@echo ' '
	@cd test && $(MAKE) clean
	@echo ' '

# Other Targets
base: 
	@cd base && $(MAKE) clean
	@echo ' '
	@cd base && $(MAKE)
	@echo ' '

net: 
	@cd net && $(MAKE) clean
	@echo ' '
	@cd net && $(MAKE)
	@echo ' '

test:
	@cd test && $(MAKE) clean
	@echo ' '
	@cd test && $(MAKE)
	@echo ' '
	
.PHONY: all clean base net test
.SECONDARY:





