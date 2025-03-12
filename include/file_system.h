#define BLOCK_SIZE 512
#define MAX_FILENAME_LENGTH 64
#define PERCENTAGE_OF_ENTRIES 5

enum{
    SEEK_BEGIN,
    SEEK_CURRENT,
    SEEK_ENDING
};

typedef struct DirectoryEntry DirectoryEntry;
typedef struct FileSystem FileSystem;

FileSystem *initFileSystem();

int createFile();

int eraseFile();