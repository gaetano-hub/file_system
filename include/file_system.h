#include <time.h>
#define BLOCK_SIZE 512
#define MAX_FILENAME_LENGTH 64
#define PERCENTAGE_OF_ENTRIES 5

enum{
    SEEK_BEGIN,
    SEEK_CURRENT,
    SEEK_ENDING
};

typedef enum
{
    FREE_TYPE,
    FILE_TYPE,
    DIRECTORY_TYPE
} EntryType;

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

struct FileSystem
{
    int *table;
    struct DirectoryEntry *entries;
    char (*data)[BLOCK_SIZE];
    int entryCount;
    int maxEntries;
    int TotalBlocks;
    int currentDirIndex;
};

struct FileHandle
{
    int fileIndex;
    int currentBlock;
    int currentPosition;
};

typedef struct DirectoryEntry DirectoryEntry;
typedef struct FileSystem FileSystem;
typedef struct FileHandle FileHandle;

FileSystem *initFileSystem(void* memory, size_t size);

int createFile(FileSystem *fs, char *fileName);

int eraseFile(FileSystem *fs, char *fileName);

FileHandle *openFH(FileSystem *fs, char *fileName);

void closeFH(FileHandle *fh);

int writeFH(FileSystem *fs, FileHandle *fh, char* data, int dataLength);

char *readFH(FileSystem *fs, FileHandle *fh, int maxToRead);

int seek(FileSystem *fs, FileHandle *fh, int offset, int whence);

int createDir(FileSystem *fs, char *dirName);

int eraseDir(FileSystem *fs, char *dirName);

int changeDir(FileSystem *fs, char *dirName);

void listDir(FileSystem *fs);

