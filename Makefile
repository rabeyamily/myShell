CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = myshell

all: $(TARGET)

$(TARGET): myshell.o parser.o executor.o
	$(CC) $(CFLAGS) -o $(TARGET) myshell.o parser.o executor.o

myshell.o: src/myshell.c src/parser.h src/executor.h
	$(CC) $(CFLAGS) -c src/myshell.c

parser.o: src/parser.c src/parser.h
	$(CC) $(CFLAGS) -c src/parser.c

executor.o: src/executor.c src/executor.h
	$(CC) $(CFLAGS) -c src/executor.c

clean:
	rm -f *.o $(TARGET)