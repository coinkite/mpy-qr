# desktop executable that can do "import uqr"
MPY_EXEC = ../../../simulator

.PHONEY: test
test:
	$(MPY_EXEC) mpy_test_code.py
	py.test test_uqr.py -vx

build: $(MPY_EXEC)
	(cd ../../..; make simulator)
