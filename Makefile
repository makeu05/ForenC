CC = gcc
TARGET = forenc

SRC = src/main.c src/hash.c src/scanner.c src/manifest.c src/verify.c src/diff.c
OBJ = $(SRC:.c=.o)

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS = -Iinclude $(shell pkg-config --cflags openssl json-c)
LDLIBS = $(shell pkg-config --libs openssl json-c)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET) --help
	./$(TARGET) --version
	./tests/test_forenc.sh
clean:
	rm -f $(OBJ) $(TARGET) test_input.txt
