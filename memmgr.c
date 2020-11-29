//
//  memmgr.c
//  memmgr
//
//  Created by William McCarthy on 17/11/20.
//  Copyright © 2020 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256

unsigned page_table[256];
int memoryFull = 0;
char memory[256][256];

//-------------------------------------------------------------------
void pageFault(unsigned page, char buf[]){
  FILE* fback = fopen("BACKING_STORE.bin", "r");    // open file BACKING_STORE.bin (represents data on disk)
  if(fback == NULL) { fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");  exit(FILE_ERROR); }

  if(fseek(fback, 256*page, SEEK_SET) != 0){
    printf("Page not found!");
    return;
  }

  fread(buf, BUFLEN, 1, fback);

  for(int i=0; i<256; i++){
    memory[memoryFull][i] = buf[i];
  }

  page_table[page] = memoryFull;
  memoryFull++;

  fclose(fback);
}

unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt
  memset(page_table, -1, sizeof(page_table));

  printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");

  while (frame < 1000) {

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(logic_add);
    offset = getoffset(logic_add);

    // If the page number is not in the page table
    if(page_table[page] == -1){
      pageFault(page, buf);
    }
    
    physical_add = page_table[page] * FRAME_SIZE + offset; 
    assert(physical_add == phys_add);
    
    // todo: read BINARY_STORE and confirm value matches read value from correct.txt
    
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    frame++;
    if (frame % 5 == 0) { printf("\n"); }
  }
  fclose(fcorr);
  fclose(fadd);
  printf("\n\t\t...done.\n");
  return 0;
}
