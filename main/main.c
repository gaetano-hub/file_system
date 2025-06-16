#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "../include/file_system.h"

#define FILESYSTEM_SIZE 10 * 1024 * 1024 // 10MB


int main(int argc, char const *argv[])
{
    void *memory = mmap(NULL, FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(memory == MAP_FAILED){
        perror("mmap failed"); 
        return -1;
    }

    printf("memoria allocata\n");

    FileSystem *fs = initFileSystem(memory, FILESYSTEM_SIZE);

    if(!fs){
        perror("init failed");
        return -1;
    }
    
    if(createFile(fs, "file.txt") == 0){
        printf("creato file.txt\n");
    }

    FileHandle *fh = open(fs, "file.txt");
    if(!fh){ 
        printf("errore open()\n");
        return -1;
    }

    if(write(fs, fh, "file", 4) == 4){
        printf("scrittura avvenuta\n");
    }else{
        printf("errore\n");
    }

    close(fh);

    FileHandle *fh1 = open(fs, "file.txt");


    int pos = seek(fs, fh1, 2, SEEK_BEGIN);
    if(pos == 0){
        printf("seek ok\n");
    }

    

    
    char *string = read(fs, fh, 5);

    if(string != NULL){
        printf("lettura avvenuta\n");
        printf("stringa: %s\n", string);
    }else{
        printf("errore lettura\n");
    }

    free(string);
    close(fh1);



    

    if(eraseFile(fs, "file.txt") == 0){
        printf("eliminato file.txt\n");
    }

    if(createDir(fs, "dir1") == 0){
        // printf("creazione dir1 riuscita\n");
    }else{
        // printf("errore creazione dir1\n");
    }

    if (createFile(fs, "file2.txt") == 0)
    {
        // printf("file2 creato\n");
    }else{
        // printf("errore creazione file2\n");
    }

    if (changeDir(fs, "dir1") == 0)
    {
        printf("change riuscito root/dir1\n");
    }

    if(createFile(fs, "file66.txt") == 0){
        // printf("creato file.txt\n");
    }

    if(createFile(fs, "file67.txt") == 0){
        // printf("creato file.txt\n");
    }

    if(createDir(fs, "dir2") == 0){
        // printf("creazione dir2 riuscita\n");
    }else{
        // printf("errore creazione dir1\n");
    }

    if (changeDir(fs, "dir2") == 0)
    {
        printf("change riuscito root/dir2\n");
    }

    if(createDir(fs, "dir2") == 0){
        // printf("creazione dir2 riuscita\n");
    }else{
        // printf("errore creazione dir1\n");
    }

    if(createFile(fs, "file.txt") == 0){
        // printf("creato file.txt\n");
    }

    if (changeDir(fs, "dir2") == 0)
    {
        printf("change riuscito root/dir2/dir2\n");
    }

    if(createFile(fs, "file.txt") == 0){
        // printf("creato file.txt\n");
    }

    if(createFile(fs, "file6.txt") == 0){
        // printf("creato file.txt\n");
    }

    if (changeDir(fs, "..") == 0)
    {
        printf("change riuscito root/dir2\n");
    }

    listDir(fs);

    if (changeDir(fs, "..") == 0)
    {
        printf("change riuscito root/dir1\n");
    }

    if(createFile(fs, "file.txt") == 0){
        // printf("creato file.txt\n");
    }

    listDir(fs);

    if (changeDir(fs, "..") == 0)
    {
        printf("change riuscito  root/\n");
    }

    

    

    if(eraseDir(fs, "dir1") == 0){
        printf("eliminato dir1\n");
    }
    
    if(eraseFile(fs, "file2.txt") == 0){
        printf("eliminato file2.txt\n");
    }

    // if(createDir(fs, "dir1") == 0){
    //     printf("creazione dir1 riuscita\n");
    // }else{
    //     printf("errore creazione dir1\n");
    // }

    // stampaFS(fs);


    if(munmap(memory, FILESYSTEM_SIZE) == -1){
        return -1;
    }

    printf("memoria deallocata\n");

    return 0;
}
