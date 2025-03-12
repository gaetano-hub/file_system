#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
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

int isValidFilename(const char* filename){
    int length = strlen(filename);
    if(length == 0 || length > MAX_FILENAME_LENGTH){
        return 0;
    }

    if(strcmp(filename, "..") == 0) return 0; // non si può chiamare ".."

    //verifica dei caratteri usabili nel fileName
    for(int i = 0; i < length; i++){
        if(!isalnum(filename[i]) && filename[i] != '.' && filename[i] != '_' && filename[i] != '-'){
            return 0;
        }
    }

    return 1;
}


int createFile(FileSystem *fs, char *fileName){
    if(!isValidFilename(fileName)){
        errno = EINVAL;
        return -1;
    }
    if(fs->entryCount >= fs->maxEntries){
        errno = ENOSPC;
        return -1;
    }

    //fileExists()
    for (int i = 0; i < fs->entryCount; i++)
    {
        if(fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, fileName) == 0 && fs->entries[i].type == FILE_TYPE){
            errno = EEXIST;
            return -1; // il file esiste
        }
    }
    
    DirectoryEntry *file = &(fs->entries[fs->entryCount++]);
    strncpy(file->name, fileName, MAX_FILENAME_LENGTH);
    file->name[MAX_FILENAME_LENGTH - 1] = '\0';

    file->type = FILE_TYPE;
    file->startBlock = FREE_BLOCK;
    file->size = 0;
    file->parentIndex = fs->currentDirIndex;

    return 0;
}


int eraseFile(FileSystem *fs, char *fileName){
    int j = 0;
    for (int i = 0; j < fs->entryCount && i < fs->maxEntries; i++)
    {
        if(fs->entries[i].type == FREE_TYPE)
            continue;
        
        //cerco il file da eliminare
        if(fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, fileName) == 0 && fs->entries[i].type == FILE_TYPE){
            fs->entries[i].type = FREE_TYPE;
            fs->entryCount--;

            return 0;
        }
        
    }

    errno = ENOENT;
    return -1;
    
}