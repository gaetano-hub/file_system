#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

#include "../include/file_system.h"

#define FILESYSTEM_SIZE 10 * 1024 * 1024 // 10MB

int test_ok = 0;
int test_failed = 0;

void test(int condition, char *message){
    if(condition){
        test_ok++;
        printf("[TEST SUPERATO]: %s\n", message);
    }else{
        test_failed++;
        printf("----------------TEST FALLITO %s\n", message);
    }

    
}

void inizioTest(void* memory){

    FileSystem *fs = initFileSystem(memory, FILESYSTEM_SIZE);
    if(!fs){
        perror("init failed");
        return;
    }

    printf("\t\t\t\t    FileSystem inizializzato...\n");

    //=============================== Inizio test sui file(compreso write, seek, read)...==============================
    printf("\t\tInizio test sui file(compreso write, seek, read)...\n\n");

    test(createFile(fs, "file.txt") == 0, "Creato file.txt");
    test(createFile(fs, "file.txt") == -1, "File.txt non creato");
    test(createFile(fs, "file2.txt") == 0, "Creato file.txt");
    test(createFile(fs, "file3.txt") == 0, "Creato file.txt");
    
    FileHandle *fh = open(fs, "file.txt");
    FileHandle *fh2 = open(fs, "file2.txt");
    FileHandle *fh3 = open(fs, "file3.txt");
    test(fh != NULL, "FileHandle open() riuscita");
    test(fh2 != NULL, "FileHandle open() riuscita");
    test(fh3 != NULL, "FileHandle open() riuscita");
    if(fh){
        test(write(fs, fh, "file", 4) == 4, "Scrittura di 'file' avvenuta");
        test(seek(fs, fh, 0, SEEK_BEGIN) == 0, "Seek begin riuscita");
        test(strcmp(read(fs, fh, 5), "file") == 0, "Lettura di 'file'");
    }

    if(fh2){
        test(write(fs, fh2, "file2", 4) == 4, "Scrittura di 'file2' avvenuta");
        test(seek(fs, fh2, 0, SEEK_BEGIN) == 0, "Seek begin riuscita");
        test(seek(fs, fh2, 1, SEEK_CUR) == 0, "Seek current riuscita");
        test(strcmp(read(fs, fh2, 4), "ile") == 0, "Lettura di 'ile'");
    }

    if(fh3){
        test(write(fs, fh3, "file3", 4) == 4, "Scrittura di 'file3' avvenuta");
        test(seek(fs, fh3, 0, SEEK_BEGIN) == 0, "Seek begin riuscita");
        test(seek(fs, fh3, -2, SEEK_END) == 0, "Seek ending riuscita");
        test(strcmp(read(fs, fh3, 2), "le") == 0, "Lettura di 'e'");
    }

    test(eraseFile(fs, "file.txt") == 0, "Eliminazione file.txt");
    test(eraseFile(fs, "file2.txt") == 0, "Eliminazione file2.txt");
    test(eraseFile(fs, "file3.txt") == 0, "Eliminazione file3.txt");

    close(fh);
    close(fh2);
    close(fh3);

    //========================= Inizio test sulle directory...=======================
    printf("\n\t\tInizio test sulle directory...\n\n");
    

    test(createDir(fs, "dir1") == 0, "Creazione dir1");
    test(createFile(fs, "file1.txt") == 0, "Creazione file1.txt");
    test(createFile(fs, "file2.txt") == 0, "Creazione file2.txt");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(changeDir(fs, "dir1") == 0, "cd dir1");
    test(createFile(fs, "file2.txt") == 0, "Creazione file2.txt");
    test(createFile(fs, "file3.txt") == 0, "Creazione file3.txt");


    test(createDir(fs, "dir2") == 0, "Creazione dir2");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(changeDir(fs, "dir2") == 0, "cd dir2");

    test(createFile(fs, "file4.txt") == 0, "Creazione file4.txt");

    test(createDir(fs, "dir3") == 0, "Creazione dir3");
    

    test(eraseFile(fs, "file4.txt") == 0, "Eliminazione file4.txt");

    

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(changeDir(fs, "dir3") == 0, "cd dir3");

    test(eraseFile(fs, "file.txt") == -1, "Errore eliminazione file.txt");

    test(eraseFile(fs, "file6.txt") == -1, "Errore eliminazione file6.txt");

    test(eraseDir(fs, "dir6") == -1, "Errore eliminazione dir6");

    test(changeDir(fs, "..") == 0, "cd ..");
    test(changeDir(fs, "..") == 0, "cd ..");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(changeDir(fs, "..") == 0, "cd ..");

    test(createFile(fs, "file10.txt") == 0, "Creato file10.txt");

    FileHandle *fh4 = open(fs, "file10.txt");
    test(fh4 != NULL, "FileHandle open() riuscita");
    if(fh4){
        test(write(fs, fh4, "file", 4) == 4, "Scrittura di 'file' avvenuta");
        test(seek(fs, fh4, 0, SEEK_BEGIN) == 0, "Seek begin riuscita");
        test(strcmp(read(fs, fh4, 5), "file") == 0, "Lettura di 'file'");
    }

    close(fh4);

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(eraseFile(fs, "file.txt") == -1, "Errore eliminazione file.txt");

    test(eraseFile(fs, "file10.txt") == 0, "Errore eliminazione file10.txt");

    test(changeDir(fs, "..") == 0, "cd ..");
    
    test(eraseDir(fs, "dir1") == 0, "Eliminazione dir1");
    
    test(eraseFile(fs, "file2.txt") == 0, "Eliminazione file2.txt");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(eraseFile(fs, "file1.txt") == 0, "Eliminazione file1.txt");
}



int main(int argc, char const *argv[])
{
    void *memory = mmap(NULL, FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(memory == MAP_FAILED){
        perror("mmap failed"); 
        return -1;
    }
    printf("\t\t\t\t========= MEMORIA ALLOCATA =========\n");
    
    printf("\t\t\t\t    Inizio fase di testing...\n\n");

    inizioTest(memory);

    printf("\n\ntest_ok: %d\n", test_ok);
    printf("test_failed: %d\n", test_failed);

    printf("\t\t\t\t    Fine fase di testing...\n\n");

    if(munmap(memory, FILESYSTEM_SIZE) == -1){
        return -1;
    }

    printf("\t\t\t\t========= MEMORIA DEALLOCATA =========\n");

    return 0;
}
