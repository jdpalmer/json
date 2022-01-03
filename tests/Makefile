PYTHON3 ?= python3
RM ?= rm -f
CC := $(CXX)
CPPFLAGS += -std=c++17

test: jsontest jsontest.py
	$(PYTHON3) -B -m pytest -p no:cacheprovider jsontest.py

jsontest: jsontest.o

clean:
	$(RM) jsontest *.o
