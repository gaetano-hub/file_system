CC = gcc
CFLAGS = -g

EXEC_FILES = test test_img
OBJECTS_TEST = main.o file_system.o
OBJECTS_IMG = main_img.o file_system.o

all: $(EXEC_FILES)

test: $(OBJECTS_TEST)
	$(CC) $(CFLAGS) $(OBJECTS_TEST) -o test

test_img: $(OBJECTS_IMG)
	$(CC) $(CFLAGS) $(OBJECTS_IMG) -o test_img

main.o: main/main.c include/file_system.h
	$(CC) $(CFLAGS) -c main/main.c -o main.o

main_img.o: main/main_img.c include/file_system.h
	$(CC) $(CFLAGS) -c main/main_img.c -o main_img.o

file_system.o: src/file_system.c include/file_system.h
	$(CC) $(CFLAGS) -c src/file_system.c -o file_system.o

clean:
	rm -f $(EXEC_FILES) *.o
