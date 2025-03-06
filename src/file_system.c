#include <stdlib.h>
#include <stdio.h>
#include "../include/file_system.h"

#define FREE_BLOCK -1
#define END_OF_CHAIN -2

typedef enum{
    FREE_TYPE,
    FILE_TYPE,
    DIRECTORY_TYPE
}EntryType;

struct DirectoryEntry
{
    char name[MAX_FILENAME_LENGTH];
    int startBlock;
    int size;
    int parentIndex;
};

struct FileSystem{
    int* table;
    DirectoryEntry *entries;
    char (*data)[BLOCK_SIZE];
    int entryCount;
    int maxEntries;
    int TotalBlocks;
    int currentDirIndex;
};

FileSystem *initFileSystem(void* memory, size_t size){

    if(size < sizeof(FileSystem) + sizeof(DirectoryEntry) + BLOCK_SIZE + sizeof(int)){
        //stampa errore
        printf("errore");
        return NULL;
    }

    FileSystem *fs = (FileSystem*) memory;

    

    return fs;
}
