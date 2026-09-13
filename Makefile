CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iinclude -D_POSIX_C_SOURCE=200809L
LDLIBS ?= -pthread

TARGET := netprobe
SRC := src/main.c src/stats.c
OBJ := $(SRC:.c=.o)
TEST_TARGET := tests/test_stats

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDLIBS) -o $@

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): tests/test_stats.c src/stats.c include/stats.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_stats.c src/stats.c -lm -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(OBJ) $(TEST_TARGET)
