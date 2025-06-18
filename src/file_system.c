#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include "../include/file_system.h"

#define FREE_BLOCK -1
#define END_OF_CHAIN -2

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

FileSystem *initFileSystem(void *memory, size_t size)
{

    if (size < sizeof(FileSystem) + sizeof(DirectoryEntry) + BLOCK_SIZE + sizeof(int))
    {
        // stampa errore
        errno = ENOMEM;
        return NULL;
    }

    FileSystem *fs = (FileSystem *)memory;

    // calcolo delle entries e dei blocchi
    fs->maxEntries = ((size - sizeof(FileSystem)) * PERCENTAGE_OF_ENTRIES / 100) / sizeof(DirectoryEntry);
    if (fs->maxEntries < 1)
        fs->maxEntries = 1;

    fs->TotalBlocks = (size - sizeof(FileSystem) - fs->maxEntries * sizeof(DirectoryEntry)) / (BLOCK_SIZE + sizeof(int));

    // alloco la FAT table, directory entries e data blocks
    fs->table = (int *)((char *)memory + sizeof(FileSystem));
    fs->entries = (DirectoryEntry *)((char *)fs->table + fs->TotalBlocks * sizeof(int));
    fs->data = (char (*)[BLOCK_SIZE])((char *)fs->entries + fs->maxEntries * sizeof(DirectoryEntry));


    for (int i = 0; i < fs->TotalBlocks; i++)
    {
        fs->table[i] = FREE_BLOCK;
    }
    fs->entryCount = 0;

    // creo la directory root
    DirectoryEntry *root = &(fs->entries[fs->entryCount++]);
    strncpy(root->name, "root/", MAX_FILENAME_LENGTH);
    root->type = DIRECTORY_TYPE;
    root->startBlock = FREE_BLOCK;
    root->size = 0;
    root->parentIndex = -1; // non ha parent
    root->creationTimeStamp = time(NULL);

    fs->currentDirIndex = 0; // setto la directory corrente con 0(root)

    return fs;
}

int isValidFilename(const char *filename)
{
    int length = strlen(filename);
    if (length == 0 || length > MAX_FILENAME_LENGTH)
    {
        return 0;
    }

    if (strcmp(filename, "..") == 0)
        return 0; // non si può chiamare ".."

    // verifica dei caratteri usabili nel fileName
    for (int i = 0; i < length; i++)
    {
        if (!isalnum(filename[i]) && filename[i] != '.' && filename[i] != '_' && filename[i] != '-')
        {
            return 0;
        }
    }

    return 1;
}

int createFile(FileSystem *fs, char *fileName)
{
    if (!isValidFilename(fileName))
    {
        errno = EINVAL;
        return -1;
    }
    if (fs->entryCount >= fs->maxEntries)
    {
        errno = ENOSPC;
        return -1;
    }

    // fileExists()
    for (int i = 0; i < fs->entryCount; i++)
    {
        if (fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, fileName) == 0 && fs->entries[i].type == FILE_TYPE)
        {
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
    file->creationTimeStamp = time(NULL);
    file->lastAccessTimeStamp = time(NULL);

    return 0;
}

int eraseFile(FileSystem *fs, char *fileName)
{
    int j = 0;
    for (int i = 0; j < fs->entryCount && i < fs->maxEntries; i++)
    {

        if (fs->entries[i].type == FREE_TYPE)
            continue;

        // cerco il file da eliminare
        if (fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, fileName) == 0 && fs->entries[i].type == FILE_TYPE)
        {
            fs->entries[i].type = FREE_TYPE;
            fs->entryCount--;

            return 0;
        }
        j++;
    }
    errno = ENOENT;
    return -1;
}

FileHandle *open(FileSystem *fs, char *fileName)
{

    // alloco la memoria per il descrittore che mi servirà per scrivere/leggere un file
    FileHandle *fh = malloc(sizeof(FileHandle));
    if (fh == NULL)
    {
        return NULL;
    }

    fh->fileIndex = -1; // imposto un valore "non valido"

    // cerco il file nella currentDir
    int j = 0;
    for (int i = 0; j < fs->entryCount && i < fs->maxEntries; i++)
    {
        if (fs->entries[i].type == FREE_TYPE)
            continue;

        // se trovo il file allora inizializzo *fh
        if (fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, fileName) == 0 && fs->entries[i].type == FILE_TYPE)
        {

            fh->fileIndex = i;
            fh->currentBlock = fs->entries[i].startBlock;
            fh->currentPosition = 0;
            fs->entries[i].lastAccessTimeStamp = time(NULL);

            return fh;
        }

        j++;
    }

    // non ho trovato il file quindi faccio la free()
    free(fh);
    errno = ENOENT;
    return NULL;
}

void close(FileHandle *fh)
{
    free(fh);
}

int write(FileSystem *fs, FileHandle *fh, char *data, int dataLength)
{

    int bytesWritten = 0;

    // inizializzo il puntatore al file così da poterne modificare le caratteristiche
    DirectoryEntry *file = &fs->entries[fh->fileIndex];
    // ultimo accesso al file
    file->lastAccessTimeStamp = time(NULL);

    // creo un ciclo che si conclude solo quando si è finito di scrivere
    while (bytesWritten < dataLength)
    {
        // se il blocco corrente è vuoto allora ne alloco uno nuovo
        if (fh->currentBlock == FREE_BLOCK)
        {

            int freeBlock = -1;

            // cerco un blocco vuoto
            for (int i = 0; i < fs->TotalBlocks; i++)
            {
                if (fs->table[i] == FREE_BLOCK)
                {
                    freeBlock = i;

                    // imposto temporaneamente l'i-esimo blocco come fine della catena
                    fs->table[i] = END_OF_CHAIN;

                    // se il file è stato appena creato allora ha come startBlock un free_block e gli imposto l'inizio
                    if (file->startBlock == FREE_BLOCK)
                    {
                        file->startBlock = i;
                    }
                    else
                    {

                        // altrimenti se il file ha già dei blocchi allora devo trovare il suo ultimo
                        int lastBlock = file->startBlock;
                        // ciclo finché non trovo l'ultimo blocco
                        while (fs->table[lastBlock] != END_OF_CHAIN)
                        {
                            lastBlock = fs->table[lastBlock];
                        }
                        // collego il nuovo blocco alla fine della catena
                        fs->table[lastBlock] = i;
                    }

                    // ho trovato un blocco libero e lo assegno al corrente e quindi esco dal ciclo
                    fh->currentBlock = i;

                    break;
                }
            }

            // spazio esaurito
            if (freeBlock == -1)
            {
                // errore non c'è un freeBlock
                errno = ENOSPC;
                return -1;
            }
        }

        // calcolo l'offset del blocco e poi vedo quanto spazio c'è rimasto
        int blockOffset = fh->currentPosition % BLOCK_SIZE;
        int bytesToWrite = BLOCK_SIZE - blockOffset;

        // se c'è lo spazio per scrivere i nuovi byte allora imposto il numero di byte da scrivere
        if (bytesToWrite > dataLength - bytesWritten)
        {

            bytesToWrite = dataLength - bytesWritten;
        }

        // svolgo la copia dei byte a partire dal blocco corrente(selezionato in precedenza)
        memcpy(fs->data[fh->currentBlock] + blockOffset, data + bytesWritten, bytesToWrite);

        // aggiorno la posizione
        fh->currentPosition += bytesToWrite;
        bytesWritten += bytesToWrite;

        // se si finisce lo spazio nel blocco corrente allora si passa al prossimo
        if (fh->currentPosition % BLOCK_SIZE == 0)
        {
            int nextBlock = fs->table[fh->currentBlock];
            if (nextBlock == END_OF_CHAIN)
            {
                // significa che al prossimo ciclo si allocherà un nuovo blocco
                nextBlock = FREE_BLOCK;
            }
            fh->currentBlock = nextBlock;
        }
    }

    // aggiorno la dimensione del file e ritorno i byte scritti
    file->size += bytesWritten;

    return bytesWritten;
}

char *read(FileSystem *fs, FileHandle *fh, int maxToBeRead)
{

    DirectoryEntry *file = &fs->entries[fh->fileIndex];
    file->lastAccessTimeStamp = time(NULL);

    // alloco il "contenitore" per i bytes letti
    char *buffer = (char *)malloc(maxToBeRead + 1);
    int bytesRead = 0;

    // calcolo i maxbytes che posso leggere
    int maxReadable = file->size - fh->currentPosition;

    // se si è alla fine del file oppure si è oltre allora ritorno la stringa vuota

    if (maxReadable <= 0)
    {
        char *buffer = (char *)malloc(1);
        buffer[0] = '\0';

        return buffer;
    }

    // se i bytes da leggere sono di più di quelli disponibili allora limita la lettura
    if (maxToBeRead > maxReadable)
    {
        maxToBeRead = maxReadable;
    }

    // continuo finché non leggo tutto oppure fin quando non trovo un freeBlock
    while (bytesRead < maxToBeRead && fh->currentBlock != FREE_BLOCK)
    {

        // calcolo l' offset nel blocco e i byte che si possono leggere
        int blockOffset = fh->currentPosition % BLOCK_SIZE;
        int bytesToRead = BLOCK_SIZE - blockOffset;

        // se posso leggere massimo x byte allora adatto bytesToRead
        if (bytesToRead > maxToBeRead - bytesRead)
        {

            bytesToRead = maxToBeRead - bytesRead;
        }

        // copio i bytes dal blocco nel buffer
        memcpy(buffer + bytesRead, fs->data[fh->currentBlock] + blockOffset, bytesToRead);

        // aggiorno le posizioni
        fh->currentPosition += bytesToRead;
        bytesRead += bytesToRead;

        // se si finisce un blocco si passa al prossimo
        if (fh->currentPosition % BLOCK_SIZE == 0)
        {
            fh->currentBlock = fs->table[fh->currentBlock];
        }
    }

    
    buffer[bytesRead] = '\0';

    return buffer;
}

int seek(FileSystem *fs, FileHandle *fh, int offset, int whence)
{
    int newPos;

    DirectoryEntry *file = &fs->entries[fh->fileIndex];
    file->lastAccessTimeStamp = time(NULL);

    if (file->type != FILE_TYPE)
    {
        // errore
        // non è un file
        errno = EISDIR;
        return -1;
    }

    if (whence == SEEK_BEGIN)
    {
        newPos = offset;
    }
    else if (whence == SEEK_CURRENT)
    {
        newPos = fh->currentPosition + offset;
    }
    else if (whence == SEEK_ENDING)
    {
        newPos = file->size + offset; //ricordarsi di inserire valori negativi di offset
    }
    else
    {
        // errore
        errno = EINVAL;
        return -1;
    }

    // controllo della posizione se è valida
    if (newPos < 0 || newPos > file->size)
    {
        errno = EINVAL;
        return -1;
    }

    // aggiorno la posizione
    fh->currentPosition = newPos;
    fh->currentBlock = file->startBlock;

    // calcolo numero blocchi per arrivare alla newPosizione
    int blocksToSeek = newPos / BLOCK_SIZE;
    for (int i = 0; i < blocksToSeek; i++)
    {
        fh->currentBlock = fs->table[fh->currentBlock];
    }
    
    return 0;
}

int createDir(FileSystem *fs, char *dirName)
{
    if (!isValidFilename(dirName))
    {
        errno = EINVAL;
        return -1;
    }

    if (fs->entryCount >= fs->maxEntries)
    {
        errno = ENOSPC;
        return -1;
    }

    // cerco se già esiste una dir nella currentDir
    for (int i = 0; i < fs->entryCount; i++)
    {

        if (fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, dirName) == 0 && fs->entries[i].type == DIRECTORY_TYPE)
        {
            errno = EEXIST;
            return -1;
        }
    }

    // creo la nuova directory
    DirectoryEntry *dir = &(fs->entries[fs->entryCount++]);
    strncpy(dir->name, dirName, MAX_FILENAME_LENGTH - 1);
    dir->name[MAX_FILENAME_LENGTH - 1] = '\0';
    dir->type = DIRECTORY_TYPE;
    dir->creationTimeStamp = time(NULL);
    dir->size = 0;
    dir->startBlock = FREE_BLOCK;
    dir->parentIndex = fs->currentDirIndex;
    dir->lastAccessTimeStamp = dir->creationTimeStamp;

    return 0;
}

int eraseDir(FileSystem *fs, char *dirName)
{
    // cerco la directory nella directory corrente
    int j = 0;
    for (int i = 0; j < fs->entryCount && i < fs->maxEntries; i++)
    {
        if (fs->entries[i].type == FREE_TYPE)
            continue;

        
        if (fs->entries[i].parentIndex == fs->currentDirIndex && strcmp(fs->entries[i].name, dirName) == 0 && fs->entries[i].type == DIRECTORY_TYPE)
        {

            int dirIndex = i;

            // effettuo due volte il controllo perché con un solo ciclo qualche file/dir potrebbe sfuggire visto che si modifica fs->entries e fs->entryCount
            for (int pass = 0; pass < 2; pass++)
            {
                for (int k = 0; k < fs->maxEntries; k++)
                {
                    if (fs->entries[k].type == FREE_TYPE)
                        continue;

                    if (fs->entries[k].parentIndex == dirIndex)
                    {
                        // c'è bisogno di cambiare fs->currentDirIndex perché sennò non è possibile eliminare(per es. visto che eraseFile l'ho strutturata che
                        //  che bisogna stare nel parentIndex per eliminare)
                        int temp = fs->currentDirIndex;
                        fs->currentDirIndex = dirIndex;

                        // uso chiamate ricorsive
                        if (fs->entries[k].type == DIRECTORY_TYPE)
                        {
                            
                            eraseDir(fs, fs->entries[k].name);
                        }
                        else if (fs->entries[k].type == FILE_TYPE)
                        {
                            
                            eraseFile(fs, fs->entries[k].name);
                        }

                        fs->currentDirIndex = temp;
                    }
                }
            }

            // elimino
            fs->entries[dirIndex].type = FREE_TYPE;
            fs->entryCount--;
            

            return 0;
        }
        j++;
    }
    errno = ENOENT;
    return -1; // directory non trovata
}

int changeDir(FileSystem *fs, char *dirName)
{
    // caso in cui torno indietro
    if (strcmp(dirName, "..") == 0)
    {
        // nel caso si è già in root
        if (fs->currentDirIndex == 0)
        {
            return 0;
        }
        fs->currentDirIndex = fs->entries[fs->currentDirIndex].parentIndex;
        return 0;
    }

    int j = 0;
    for (int i = 0; j < fs->entryCount && i < fs->maxEntries; i++)
    {
        if (fs->entries[i].type == FREE_TYPE)
            continue;
        j++;

        // cerco la directory nella directory corrente e sposto currentDirIndex
        if (strcmp(fs->entries[i].name, dirName) == 0 && fs->currentDirIndex == fs->entries[i].parentIndex && fs->entries[i].type == DIRECTORY_TYPE)
        {
            fs->currentDirIndex = i;
            fs->entries[i].lastAccessTimeStamp = time(NULL);
            return 0;
        }
    }
    errno = ENOENT;
    return -1;
}

void listDir(FileSystem *fs)
{
    int j = 0;
    printf("------------------------------------------  Directory corrente: %s\n", fs->entries[fs->currentDirIndex].name);
    fs->entries[fs->currentDirIndex].lastAccessTimeStamp = time(NULL);
    for (int i = 0; j < fs->entryCount && i < fs->maxEntries; i++)
    {
        if (fs->entries[i].type == FREE_TYPE)
            continue;

        if (fs->currentDirIndex == fs->entries[i].parentIndex)
        {

            if (fs->entries[i].type == DIRECTORY_TYPE)
            {
                printf("--- DIR: %s\n--- Creazione: %s--- UltimoAccesso: %s\n", fs->entries[i].name, ctime(&fs->entries[i].creationTimeStamp), ctime(&fs->entries[i].lastAccessTimeStamp));
            }
            else if (fs->entries[i].type == FILE_TYPE)
            {
                printf("--- FILE: %s\n--- Dimensione: %d\n--- Creazione: %s--- UltimoAccesso: %s\n", fs->entries[i].name, fs->entries[i].size, ctime(&fs->entries[i].creationTimeStamp), ctime(&fs->entries[i].lastAccessTimeStamp));
            }
        }

        j++;
    }
}