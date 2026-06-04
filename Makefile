CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -mconsole

TARGET = chip8.exe

SRC = src/main.c src/cpu.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(CFLAGS)

clean:
	del /Q src\*.o $(TARGET)