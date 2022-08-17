CXX     := gcc
C_FLAGS := -ggdb
BIN     := bin
SRC     := cccamtest
INCLUDE := include

LIBRARIES   := -lcrypto -lssl
EXECUTABLE  := cccamtest

$(info $(shell mkdir -p $(BIN)))

all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.c
	$(CXX) $(C_FLAGS) -I$(INCLUDE) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*

lib: $(SRC)/*.c
	$(CXX) $(C_FLAGS) -shared -fPIC -I$(INCLUDE) $^ -o $(BIN)/cccamtest.so $(LIBRARIES)

release : C_FLAGS := -O3 -Wall
release : clean all lib

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: install
install: 
	install -d $(PREFIX)/bin/
	install -m 755 $(BIN)/$(EXECUTABLE) $(PREFIX)/bin/