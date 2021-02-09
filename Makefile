# Example only; use micropython.mk via USER_C_MODULES in your project

all:
	@echo
	@echo Add: USER_C_MODULES=$(realpath $(PWD)/..) to your mp...mk makefile
	@echo

tags:
	ctags -f .tags *.[ch]
