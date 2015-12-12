
#include <stdlib.h>
#include <stdio.h>
#include "buddy.h"
#include <string.h>

#define MINIMUM_REQUEST_SIZE 256
#define MAXIMUM_REQUEST_SIZE 65536

#define MINIMUM_CHUNK_SIZE 32768
#define MAXIMUM_CHUNK_SIZE (32768 * 1024)

#define MAXIMUM_LEVELS 40



int size; //given size for the contiguous chunk.
long int * startAddress; //starting address of the chunk.
long int freeBlocks [MAXIMUM_LEVELS]; //initialized big enough.
int allocationLimits [MAXIMUM_LEVELS];
int numberOfLevels;

int binit(void *chunkpointer, int chunksize)
{
    size = chunksize * 1024;
    startAddress = chunkpointer;

    //get the number of levels of the tree
    numberOfLevels = getNumberOfLevels(size);
	printf("max number of levels: %d.\n", numberOfLevels);

    //set all the bits in the given chunk to zero
    memset(startAddress, 0, size);

    // knowing the number of levels, we know the allocation limits for each level.
    // e.g. for input 1024 (kb), we know that the first level will be allocated by
    // sizes between 512 - 1024.
    int i;
    int k = MINIMUM_REQUEST_SIZE;
    for(i = 0; i < numberOfLevels; i++ ){
        allocationLimits[numberOfLevels - i - 1] = k;
        k *= 2;
    }

    freeBlocks[0] = (long int) startAddress;
    printf("START ADDRESS: %ld \n", (long int)startAddress);
    //initialize the array pointing to free blocks of each level
    for(i = 1; i < numberOfLevels; i++){
        freeBlocks[i] = (long int) NULL;
    }


	return (0);		// if success
}

void *balloc(int objectsize)
{
	printf("object size: %d\n", objectsize);
    if(objectsize < MINIMUM_REQUEST_SIZE)
        return NULL;    //failure

    objectsize += 2 * sizeof (int); //added one int for FREE/AVAILABLE and one int for LEVEL information.

    if(objectsize > size)
        return NULL;    //failure

    //now find the level for this chunk to be allocated
    int level = 0;
    while(objectsize < allocationLimits[level + 1])
        level++;

    printf("the object with size: %d will be inserted to the level: %d\n", objectsize, level);

    long int allocationAddress = allocateChunkWithLevel(level);
    printf("chunk is allocated to address: %ld .\n", allocationAddress);

    if(allocationAddress < 0)
        return NULL;
    else{
        //success. return the allocation address.
        return (void*) allocationAddress;
    }
}

void bfree(void *objectptr)
{

	printf("bfree called\n");

	return;
}

void bprint(void)
{
	printf("bprint called\n");

	long int* tmp = startAddress;
	if(tmp[0] == 0 && freeBlocks[0] != (long int)NULL){
        printf("The chunk has nothing in it.\n");
        return;
    }

    while( (long int) tmp < (long int) startAddress + size){    //while there is still chunk to go
        int level = tmp[1]; // +1 is for LEVEL
        int isAllocated = tmp[0];
        if(isAllocated)
        {
            printf("ALLOCATED.\t Size of chunk: %d.\t Level: %d\t address: %ld \n", allocationLimits[level], level, ( (long int)tmp -  (long int)startAddress));
        }
        else
        {
            printf("FREE.\t Size of chunk: %d.\t Level: %d\t address: %ld \n", allocationLimits[level], level, ((long int)tmp -  (long int)startAddress));
        }
        tmp = tmp + allocationLimits[level] / sizeof (long int);
    }
}

int getNumberOfLevels(int size){
    int normalizedChunkSize = size / MINIMUM_REQUEST_SIZE;
    int numOfLevels = 0;
    while(normalizedChunkSize != 1){
        normalizedChunkSize /= 2;
        numOfLevels++;
    }
    return numOfLevels;
}

long int allocateChunkWithLevel(int level){

    // EACH CHUNK TO BE ALLOCATED CONSISTS OF THE FOLLOWING:
    // [ FREE/ALLOCATED BIT  |  LEVEL  |  NEXT FREE CHUNK WITH SAME LEVEL  | PREV FREE CHUNK WITH SAME LEVEL | CONTENT ]

    //first of all, level must be sensible
    if (level < 0 || level > numberOfLevels)
		return -1;

    printf("allocating to level: %d\n", level);

    //check the free blocks list. if there is no free blocks, try one level up (and up and up..)
    if(freeBlocks[level] == (long int) NULL ){
        printf("there isn't any free block. adding one to level %d\n", level - 1);
        long int upperBlock = allocateChunkWithLevel(level - 1);

        if(upperBlock < 0)  //means no upper block available. No room to put this chunk. failure.
            return -1;

        int blockSizeToBeAllocated = allocationLimits[level];
        //since now we have the block size to be allocated and the starting point of the block,
        //we know the addresses for left and right block (that are created by splitting upper block.)
        long int leftBlockAddress = upperBlock;
        long int rightBlockAddress = upperBlock + blockSizeToBeAllocated;

        //from the above information, determining the indices of each block, to insert necessary values
        int leftBlockIndex = (leftBlockAddress - (long int)startAddress) / sizeof (long int);
        int rightBlockIndex = (rightBlockAddress - (long int)startAddress) / sizeof (long int);

        //for the left block, insert ALLOCATED and LEVEL info.
        startAddress[leftBlockIndex] = (long int) 1;
        startAddress[leftBlockIndex + 1] = level;

        //for the right block, insert FREE and LEVEL info.
        startAddress[rightBlockIndex] = (long int) 0;
        startAddress[rightBlockIndex + 1] = level;
        //for the right block, also insert the prev and next free blocks information.
        startAddress[rightBlockIndex + 2] = (long int) NULL;    //NEXT
        startAddress[rightBlockIndex + 3] = (long int) NULL;    //PREV

        //since the right block is empty, we should add it to the freeBlocks list.
        freeBlocks[level] = rightBlockAddress;

        return leftBlockAddress;
    }
    else{
        printf("there is a free block.\n");
        //then we need to find the free block in this level.
        long int *blockAddress = (long int*) freeBlocks[level];
        blockAddress[0] = 1; //ALLOCATED

        //now this free block will be used. for our freeBlocks array, we assign a new block, that is,
        //the 'next' block that our assigned block sends us to.
        freeBlocks[level] = blockAddress[2];    //+2 is NEXT

        //if the next is null, we are OK. if not, we need to nullify NEXT's PREV.
        if(freeBlocks[level] != (long int) NULL){
            int nextChunksPrevIndex = ((freeBlocks[level] - (long int)startAddress) / sizeof (long int)) + 3;
            startAddress[nextChunksPrevIndex] = (long int) NULL;
        }


        return (long int) blockAddress;
    }
}
