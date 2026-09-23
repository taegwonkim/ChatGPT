CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic

.PHONY: test clean

test: build/test_esp_at
	./build/test_esp_at

build/test_esp_at: Src/esp_at.c tests/test_esp_at.c Inc/esp_at.h tests/main.h
	mkdir -p build
	$(CC) $(CFLAGS) -Itests -IInc Src/esp_at.c tests/test_esp_at.c -o $@

clean:
	rm -rf build
