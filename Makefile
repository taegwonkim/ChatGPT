CC ?= cc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -IApp/Inc
SOURCES := App/Src/app_config.c App/Src/pc_protocol.c App/Src/fpga_parser.c App/Src/wifi_state.c
.PHONY: test clean
test: build/test_main
	./build/test_main
build/test_main: tests/test_main.c $(SOURCES) | build
	$(CC) $(CFLAGS) $^ -o $@
build:
	mkdir -p $@
clean:
	rm -rf build
