CC=gcc
CFLAGS=`sdl2-config --cflags` -Wall -O2
LDFLAGS=`sdl2-config --libs` -lSDL2_image
TARGET=fruitmachine
SRCS=main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
