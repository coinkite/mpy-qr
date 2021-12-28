# Example only; use micropython.mk via USER_C_MODULES in your project

all:
	@echo
	@echo Version 1.13
	@echo Add: USER_C_MODULES=$(realpath $(PWD)/..) to your mp...mk makefile
	@echo
	@echo Version 1.17
	@echo Add: USER_C_MODULES=$(realpath $(PWD))/micropython.cmake after "make" command
	@echo Example: "make USER_C_MODULES=path/to/mpy-qr/micropython.cmake"
	@echo

tags:
	ctags -f .tags *.[ch]
