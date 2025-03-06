CC = gcc
EXEC_FILE = test
CFLAGS = -g
OBJECTS = main.o file_system.o

all: $(EXEC_FILE)

$(EXEC_FILE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

main.o: main/main.c include/file_system.h
	$(CC) $(CFLAGS) -c main/main.c -o $@

file_system.o: src/file_system.c include/file_system.h
	$(CC) $(CFLAGS) -c src/file_system.c -o $@

clean:
	rm -f $(EXEC_FILE) $(OBJECTS)
