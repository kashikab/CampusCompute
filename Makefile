CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -pthread
LDFLAGS = -pthread -lrt
SRC = src/main.c src/scheduler.c src/banker.c src/demos.c
TARGET = campuscompute
.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(SRC) src/campus.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)
test: all
	bash scripts/test.sh
clean:
	rm -f $(TARGET) logs/events.log
