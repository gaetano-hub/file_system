#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/file_system.h"

#define FILESYSTEM_SIZE 1024 * 1024 // 1MB

//ho deciso di inserire i test_ok e i test che "falliscono" rinominandoli nulli così da far risaltare la prensenza di alcuni file/directory nel FS fin dall'inizio
//poi svolgo una scrittura e una lettura ad alcuni file, e se si lancia più volte ./test_img si può notare che la dimensione di tali file aumenta

int test_ok = 0;
int risultato_test = 0;



void test(int condition, char *message){
    if(condition){
        test_ok++;
        printf("\n[TEST SUPERATO]: %s\n", message);
    }else{
        risultato_test++;
        printf("\n---[Risultati test nullo]: %s\n", message);
    }

    
}

void inizioTest(void* memory, int flag){

    FileSystem *fs = NULL;
    if (!flag)
    {
        fs = initFileSystem(memory, FILESYSTEM_SIZE);
        if(!fs){
            perror("init failed");
            return;
        }
    }

    //ricollego i vari puntatori alla memoria così da ristabilire il filesystem
    fs = (FileSystem *)memory;
    fs->table = (int *)((char *)fs + sizeof(FileSystem));
    fs->entries = (DirectoryEntry *)((char *)fs->table + fs->TotalBlocks * sizeof(int));
    fs->data = (char (*)[BLOCK_SIZE])((char *)fs->entries + fs->maxEntries * sizeof(DirectoryEntry));

    printf("\t\t\t\t    FileSystem pronto per l'uso...\n");
    printf("\t\t\tRecupero la memoria da FileSystem.img...\n\n");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(createFile(fs, "file.txt") == 0, "Creazione file.txt");

    FileHandle *fh = openFH(fs, "file.txt");
    test(fh != NULL, "FileHandle open() riuscita");

    //da notare che in questi file si continua a scrivere e a leggere(infatti la loro dimensiona aumenta se viene avviato fs più volte)
    if(fh){
        //imposto il descrittore alla fine così da poter continuare a scrivere
        test(seek(fs, fh, 0, SEEK_ENDING) == 0, "Seek ending riuscita");
        test(writeFH(fs, fh, "file", 4) == 4, "Scrittura di 'file' avvenuta");
        test(seek(fs, fh, 0, SEEK_BEGIN) == 0, "Seek begin riuscita");
        test(strcmp(readFH(fs, fh, 4), "file") == 0, "Lettura di 'file'");
    }
    

    test(createDir(fs, "dir1") == 0, "Creazione dir1");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(changeDir(fs, "dir1") == 0, "cd dir1");
    test(createFile(fs, "file2.txt") == 0, "Creazione file2.txt");

    FileHandle *fh2 = openFH(fs, "file2.txt");

    test(fh2 != NULL, "FileHandle open() riuscita");

    if(fh2){
        //imposto il descrittore alla fine così da poter continuare a scrivere
        test(seek(fs, fh2, 0, SEEK_ENDING) == 0, "Seek ending riuscita");
        test(writeFH(fs, fh2, "file", 4) == 4, "Scrittura di 'file2' avvenuta");
        test(seek(fs, fh2, 0, SEEK_BEGIN) == 0, "Seek begin riuscita");
        test(strcmp(readFH(fs, fh2, 4), "file") == 0, "Lettura di 'ile'");
    }


    test(createDir(fs, "dir2") == 0, "Creazione dir2");
    test(createFile(fs, "file4.txt") == 0, "Creazione file4.txt");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    test(changeDir(fs, "..") == 0, "cd ..");


    printf("\n\n___Eseguo listDir()...\n\n");
    listDir(fs);
    printf("\n\n");

    closeFH(fh);
    closeFH(fh2);
}



int main(int argc, char const *argv[])
{
    int fileExists = (access("FileSystem.img", F_OK) == 0);

    int fd = open("FileSystem.img", O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        printf("Errore open()\n");
        return -1;
    }

    if (!fileExists) {
        if(ftruncate(fd, FILESYSTEM_SIZE) == -1){
            printf("Errore ftruncate");
            close(fd);
            return -1;
        }
    }


    void *memory = mmap(NULL, FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(memory == MAP_FAILED){
        perror("mmap failed"); 
        return -1;
    }


    printf("\t\t\t\t========= MEMORIA MMAPPATA =========\n");
    
    printf("\t\t\t\t    Inizio fase di testing...\n\n");

    inizioTest(memory, fileExists);

    printf("\n\ntest_ok: %d\n", test_ok);
    printf("\nRisultati test nulli perché in memoria già sono presenti file/directory: %d\n\n", risultato_test);

    printf("\t\t\t\t    Fine fase di testing...\n\n");

    if (msync(memory, FILESYSTEM_SIZE, MS_SYNC) == -1) {
        perror("msync");
    }


    if(munmap(memory, FILESYSTEM_SIZE) == -1){
        close(fd);
        return -1;
    }

    close(fd);

    printf("\t\t\t\t========= MEMORIA MUNMAP =========\n");

    return 0;
}
