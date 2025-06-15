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

struct FileHandle
{
    int fileIndex;
    int currentBlock;
    int currentPosition;
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

FileHandle *open(FileSystem *fs, char *fileName){

    //alloco la memoria per il descrittore che mi servirà per scrivere/leggere un file
    FileHandle *fh = malloc(sizeof(FileHandle));
    if(fh == NULL){
        return NULL;
    }

    fh->fileIndex = -1; //imposto un valore "non valido"

    //cerco il file nella currentDir
    int j = 0;
    for (int i = 0; j < fs->entryCount && i < fs->maxEntries; i++)
    {
        if(fs->entries[i].type == FREE_TYPE)
            continue;
        
        //se trovo il file allora inizializzo *fh
        if(fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, fileName) == 0 && fs->entries[i].type == FILE_TYPE){

            fh->fileIndex = i;
            fh->currentBlock = fs->entries[i].startBlock;
            fh->currentPosition = 0;
            fs->entries[i].creationTimeStamp = time(NULL);
            fs->entries[i].lastAccessTimeStamp = fs->entries[i].creationTimeStamp;

            return fh; 

        }

        j++;
    }

    //non ho trovato il file quindi faccio la free()
    free(fh);
    errno = ENOENT;
    return NULL;
    

}

void close(FileHandle *fh){
    free(fh);
}

int write(FileSystem *fs, FileHandle *fh, char* data, int dataLength){

    int bytesWritten = 0;

    //inizializzo il puntatore al file così da poterne modificare le caratteristiche
    DirectoryEntry *file = &fs->entries[fh->fileIndex];
    //ultimo accesso al file
    file->lastAccessTimeStamp = time(NULL);

    //creo un ciclo che si conclude solo quando si è finito di scrivere
    while (bytesWritten < dataLength){
        //se il blocco corrente è vuoto allora ne alloco uno nuovo
        if(fh->currentBlock == FREE_BLOCK){

            int freeBlock = -1;

            //cerco un blocco vuoto
            for(int i = 0; i < fs->TotalBlocks; i++){
                if(fs->table[i] == FREE_BLOCK){
                    freeBlock = i;

                    //imposto temporaneamente l'i-esimo blocco come fine della catena
                    fs->table[i] = END_OF_CHAIN;
                    
                    //se il file è stato appena creato allora ha come startBlock un free_block e gli imposto l'inizio
                    if(file->startBlock == FREE_BLOCK){
                        file->startBlock = i;
                    }else{

                        //altrimenti se il file ha già dei blocchi allora devo trovare il suo ultimo
                        int lastBlock = file->startBlock;
                        //ciclo finché non trovo l'ultimo blocco
                        while(fs->table[lastBlock] != END_OF_CHAIN){
                            lastBlock = fs->table[lastBlock];
                        }
                        //collego il nuovo blocco alla fine della catena
                        fs->table[lastBlock] = i;
                    }

                    //ho trovato un blocco libero e lo assegno al corrente e quindi esco dal ciclo
                    fh->currentBlock = i;

                    break;


                }
            }

            //spazio esaurito
            if(freeBlock == -1){
                //errore non c'è un freeBlock
                errno = ENOSPC;

                return -1;
            }
        }

        
        //calcolo l'offset del blocco e poi vedo quanto spazio c'è rimasto
        int blockOffset = fh->currentPosition % BLOCK_SIZE;
        int bytesToWrite = BLOCK_SIZE - blockOffset;

        
        //se c'è lo spazio per scrivere i nuovi byte allora imposto il numero di byte da scrivere
        if(bytesToWrite > dataLength - bytesWritten){
            
            bytesToWrite = dataLength - bytesWritten;

        }

        //svolgo la copia dei byte a partire dal blocco corrente(selezionato in precedenza)
        memcpy(fs->data[fh->currentBlock] + blockOffset, data + bytesWritten, bytesToWrite);

        //aggiorno la posizione
        fh->currentPosition += bytesToWrite;
        bytesWritten += bytesToWrite;

        //se si finisce lo spazio nel blocco corrente allora si passa al prossimo
        if (fh->currentPosition % BLOCK_SIZE == 0)
        {
            int nextBlock = fs->table[fh->currentBlock];
            if(nextBlock == END_OF_CHAIN){
                //significa che al prossimo ciclo si allocherà un nuovo blocco
                nextBlock = FREE_BLOCK;
            }
            fh->currentBlock = nextBlock;
        }
        


    }
    
    //aggiorno la dimensione del file e ritorno i byte scritti
    file->size += bytesWritten;

    return bytesWritten;
}

char *read(FileSystem *fs, FileHandle *fh, int maxToBeRead){

    DirectoryEntry *file = &fs->entries[fh->fileIndex];
    file->lastAccessTimeStamp = time(NULL);

    //alloco il "contenitore" per i bytes letti
    char *buffer = (char*)malloc(maxToBeRead + 1);
    int bytesRead = 0;

    //calcolo i maxbytes che posso leggere
    int maxReadable = file->size - fh->currentPosition;
    
    //se si è alla fine del file oppure si è oltre allora ritorno la stringa vuota
    
    if(maxReadable <= 0){
        char *buffer = (char*)malloc(1);
        buffer[0] = '\0';
        
        return buffer;
    }

    

    //se i bytes da leggere sono di più di quelli disponibili allora limita la lettura
    if( maxToBeRead >  maxReadable ){
        maxToBeRead = maxReadable;
    }

    //continuo finché non leggo tutto oppure fin quando non trovo un freeBlock
    while (bytesRead < maxToBeRead && fh->currentBlock != FREE_BLOCK){

        //calcolo l' offset nel blocco e i byte che si possono leggere
        int blockOffset = fh->currentPosition % BLOCK_SIZE;
        int bytesToRead = BLOCK_SIZE - blockOffset;

        //se devo leggere meno byte di quelli che stanno nel blocco allora adatto bytesToRead
        if (bytesToRead > maxToBeRead - bytesRead){

            bytesToRead = maxToBeRead - bytesRead;
        }

        //copio i bytes dal blocco nel buffer
        memcpy(buffer + bytesRead, fs->data[fh->currentBlock] + blockOffset, bytesToRead);

        //aggiorno le posizioni
        fh->currentPosition += bytesToRead;
        bytesRead += bytesToRead;

        //se si finisce un blocco si passa al prossimo
        if(fh->currentPosition % BLOCK_SIZE == 0){
            fh->currentBlock = fs->table[fh->currentBlock];
        }

    }

    // printf("%s debug", buffer);
    buffer[bytesRead] = '\0';

    return buffer;
}