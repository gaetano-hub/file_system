#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "../include/file_system.h"

#define FILESYSTEM_SIZE 1.5 * 1024 * 1024 * 1024 // 1.5 GB


int main(int argc, char const *argv[])
{
    void *memory = mmap(NULL, FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(memory == MAP_FAILED) return -1;

    printf("memoria allocata\n");

    FileSystem *fs = initFileSystem(memory, FILESYSTEM_SIZE);
    
    if(createFile(fs, "file.txt") == 0){
        printf("creato file.txt\n");
    }

    if(eraseFile(fs, "file.txt") == 0){
        printf("eliminato file.txt\n");
    }


    if(munmap(memory, FILESYSTEM_SIZE) == -1){
        return -1;
    }

    printf("memoria deallocata\n");

    return 0;
}
