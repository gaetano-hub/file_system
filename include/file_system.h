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
typedef struct FileHandle FileHandle;

FileSystem *initFileSystem(void* memory, size_t size);

int createFile(FileSystem *fs, char *fileName);

int eraseFile(FileSystem *fs, char *fileName);

FileHandle *open(FileSystem *fs, char *fileName);

void close(FileHandle *fh);

int write(FileSystem *fs, FileHandle *fh, char* data, int dataLength);

char *read(FileSystem *fs, FileHandle *fh, int maxToRead);

int seek(FileSystem *fs, FileHandle *fh, int offset, int whence);

int createDir(FileSystem *fs, char *dirName);

int eraseDir(FileSystem *fs, char *dirName);

int changeDir(FileSystem *fs, char *dirName);

void listDir(FileSystem *fs);

