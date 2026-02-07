TARGET = a.out

SRC = main.c

CC = gcc

CFLAGS = $(shell pkg-config --cflags gtk4 gtk4-layer-shell-0 gio-unix-2.0)
LIBS = $(shell pkg-config --libs gtk4 gtk4-layer-shell-0 gio-unix-2.0)

all:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

