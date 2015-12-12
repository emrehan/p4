#ifndef BUDDY_H
#define BUDDY_H

int binit(void *, int);
void *balloc(int);
void bfree(void *);
void bprint(void);
int getNumberOfLevels(int);
long int allocateChunkWithLevel(int level);

#endif
