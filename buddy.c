/*
Alican Büyükçakır
Emrehan Tüzün

CS342 - Bilkent Fall 2015
*/

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
	//printf("max number of levels: %d.\n", numberOfLevels);

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
    //initialize the array pointing to free blocks of each level
    for(i = 1; i < numberOfLevels; i++){
        freeBlocks[i] = (long int) NULL;
    }


	return (0);		// if success
}

void *balloc(int objectsize)
{
    if(objectsize < MINIMUM_REQUEST_SIZE)
        return NULL;    //failure

    objectsize += 2 * sizeof (int); //added one int for FREE/AVAILABLE and one int for LEVEL information.

    if(objectsize > size)
        return NULL;    //failure

    //now find the level for this chunk to be allocated
    int level = 0;
    while(objectsize < allocationLimits[level + 1])
        level++;

    long int allocationAddress = allocateChunkWithLevel(level);
    printf("chunk with the size: %d is allocated to address: %lx .\n", objectsize ,allocationAddress);

    if(allocationAddress < 0)
        return NULL;
    else{
        //success. return the allocation address.
        return (void*) allocationAddress;
    }
}

void bfree(void *objectptr)
{
    long int addr = (long int) objectptr;

    //check if the object is in the boundaries
    if(addr < (long int) startAddress || addr > (long int ) startAddress + size){
        return;
    }

    //instead of taking the address as a value, now taking it as a pointer.
    long int* temp = (long int *) addr;
    //level of the chunk to be deleted:
    int level = temp[1];

    if (level == 0){
		freeBlocks[level] = addr;
		temp[0] = 0; //since it's free now.
		return;
	}

	//now, we need to find the address of the sibling block.
	//it can be either to the left or to the right of this current block 'temp'.
	int levelMultiplier = 1 << (level + 1);
    int indexAtCurrentLevel = (addr - (long int) startAddress) /  (size / levelMultiplier);
    //knowing the indexAtCurrentLevel, we can now know the sibling's address.
    //If the index is even, then it's the left chunk. The sibling is to the right of that and vice versa.
    int siblingIndex;
    if(indexAtCurrentLevel % 2 == 0)
        siblingIndex = indexAtCurrentLevel+1;
    else{
        siblingIndex = indexAtCurrentLevel-1;
    }

    long int siblingAddress = (long int) startAddress + siblingIndex * ( (long int) size / levelMultiplier );
    //now we have our possible sibling's address.
    long int* siblingPointer = (long int*)siblingAddress;


    if(siblingPointer[0] == 1 ) //if the sibling is not free. Easy case.
    {
        //then set your free/allocated bit to FREE
        temp[0] = 0;
        temp[2] = freeBlocks[level]; //NEXT FREE is assigned as the first one known in our free list.
        freeBlocks[level] = addr;   //assign the first free of that level as this one.
        temp[3] = (long int ) NULL; //no prev since this is assigned as the first free block.

        //now, the chunk that is previously pointed by the freeBlocks 's prev is NULL. change it.
        if( temp[2] != (long int) NULL)
        {
            long int * formerFirstChunk = (long int*) temp[2];
            formerFirstChunk[2] = addr;
        }

    }
    else{   //sibling was also free. Now we need to combine the chunks while freeing too.
        temp[0] = 0;    //set the free/allocated bit to FREE
        if(siblingPointer[1] == level){ //if the sibling's level is the same, we got some things to do.
            //now, we need to remove the entries associated with our sibling from freeBlocks
            //and rearrange next and prev pointers.

            if(freeBlocks[level] == (long int) siblingAddress){ //sibling is the head of the free list.
                freeBlocks[level] = siblingPointer[2]; // freeBlocks[level] = sibling->nextFree

                if(siblingPointer[2] != (long int) NULL){   //if the next is not null, then arrange prev too.
                    long int* nextChunk = (long int*) siblingPointer[2];
                    nextChunk[3] = (long int) NULL; // next->prevFree = NULL
                }
            }

            else{   //sibling is not the head of the free list. It is somewhere in the middle.
                long int* prev = (long int* ) siblingPointer[3];    //prev = sibling->prev

                if(prev != NULL){
                    prev[2] = (long int) siblingPointer[2];    //prev->next = sibling->next



                    if(siblingPointer[2] != (long int) NULL){   // if the next is not null, then arrange prev. of that too
                        long int* nextChunk = (long int*) siblingPointer[2];
                        nextChunk[3] = (long int) prev;
                    }
                }

            }

            //now, to put this combined chunk to one level up,
            //we need to know which one (current or sibling) is on the left and decrement the level of that chunk.
            //when the level is decreased, recursively call the bfree function to take care of the higher levels as well.
            if(indexAtCurrentLevel % 2 == 0)    //then we are on the left.
            {
                temp[1] = level - 1;
                bfree( (void*) temp);


            }
            else{   //sibling is on the left.
                siblingPointer[1] = level - 1;
                bfree( (void*) siblingPointer);
            }
        }
    }

	return;
}

void bprint(void)
{
	long int* tmp = startAddress;
	if(tmp[0] == 0 && freeBlocks[0] == (long int)startAddress){
        printf("The chunk has nothing in it.\n");
        return;
    }
    printf("--------------------------------------------------------------\n");
    printf("%-20s %-20s %-5s %-20s %-20s\n", "[FREE/ALLOCATED]", "SIZE", "LEVEL", "ChunkStart", "Address");
    //printf("FREE/ALLOCATED\t size\t level\t ChunkStart\t Address\n");

    while( (long int) tmp < (long int) startAddress + size){    //while there is still chunk to go
        int level = tmp[1]; // +1 is for LEVEL
        int isAllocated = tmp[0];
        if(isAllocated)
        {
            printf("%-20s %-20d %-5d %-20ld %-20lx\n", "[ALLOCATED]" ,allocationLimits[level], level, ( (long int)tmp -  (long int)startAddress), (long int ) tmp);
        }
        else
        {
            printf("%-20s %-20d %-5d %-20ld %-20lx\n", "[FREE]" ,allocationLimits[level], level, ((long int)tmp -  (long int)startAddress), (long int ) tmp);
        }
        tmp = tmp + allocationLimits[level] / sizeof (long int);
    }

    printf("--------------------------------------------------------------\n");
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

    //check the free blocks list. if there is no free blocks, try one level up (and up and up..)
    if(freeBlocks[level] == (long int) NULL ){
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
        //then we need to find the free block in this level.
        long int *blockAddress = (long int*) freeBlocks[level];
        blockAddress[0] = 1; //ALLOCATED
        blockAddress[1] = level;

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
