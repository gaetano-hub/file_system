#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
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
    EntryType type;
    int startBlock;
    int size;
    int parentIndex;
    time_t creationTimeStamp;
    time_t lastAccessTimeStamp;
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

    //calcolo delle entries e dei blocchi
    fs->maxEntries = ((size - sizeof(FileSystem)) * PERCENTAGE_OF_ENTRIES / 100) / sizeof(DirectoryEntry);
    if(fs->maxEntries < 1) fs->maxEntries = 1;
    fs->TotalBlocks = (size - sizeof(FileSystem) - fs->maxEntries * sizeof(DirectoryEntry)) / (BLOCK_SIZE + sizeof(int));

    //alloco la FAT table, directory entries e data blocks
    fs->table = (int*)((char*) memory + sizeof(FileSystem));
    fs->entries = (DirectoryEntry*) ((char*)fs->table + fs->TotalBlocks * sizeof(int));
    fs->data = (char(*)[BLOCK_SIZE])((char*)fs->entries + fs->maxEntries * sizeof(DirectoryEntry));

    //printf("%d      %d      %d      ", fs->maxEntries, fs->TotalBlocks, fs->table, fs->entries, fs->data);

    for (int i = 0; i < fs->TotalBlocks; i++)
    {
        fs->table[i] = FREE_BLOCK;
    }
    fs->entryCount = 0;

    //creo la directory root
    DirectoryEntry *root = &(fs->entries[fs->entryCount++]);
    strncpy(root->name, "root/", MAX_FILENAME_LENGTH);
    root->type = DIRECTORY_TYPE;
    root->startBlock = FREE_BLOCK;
    root->size = 0;
    root->parentIndex = -1; //non ha parent

    fs->currentDirIndex = 0; // setto la directory corrente con 0(root)

    return fs;
}
