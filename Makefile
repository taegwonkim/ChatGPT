CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic
INCLUDES = -ICore/Inc
SRC = Core/Src/surge_config.c Core/Src/fpga_protocol.c Core/Src/wifi_state.c
.PHONY: test clean
test: build/test_protocol
	./build/test_protocol
build/test_protocol: tests/test_protocol.c $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@
clean:
	rm -rf build
