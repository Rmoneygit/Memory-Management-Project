//
//  memmgr.c
//  memmgr
//

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256

unsigned page_table[256];
int TLBsize = 0;
unsigned TLB[16][2];
int memoryMax = 0;
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
    memory[memoryMax][i] = buf[i];
  }

  page_table[page] = memoryMax;
  memoryMax++;

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
  unsigned   page, offset, physical_add, address_value, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt
  unsigned TLBhitCount = 0;
  unsigned pageFaultCount = 0;
  memset(page_table, -1, sizeof(page_table));

  while (frame < 1000) {

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(logic_add);
    offset = getoffset(logic_add);

    bool TLBhit = false;

    // Try to find the page in the TLB
    for(int i = 0; i < TLBsize; i++){
      if(TLB[i][0] == page){
        TLBhit = true;
        address_value = memory[TLB[i][1]][offset];
        physical_add = TLB[i][1]*256 + offset;
        TLBhitCount++;
        break;
      }
    }

    // Do this stuff if we didn't find it in the TLB
    if(!TLBhit){
      // If the page number is not in the page table
      if(page_table[page] == -1){
        pageFault(page, buf);
        pageFaultCount++;
      }

      physical_add = page_table[page] * FRAME_SIZE + offset;
      address_value = memory[page_table[page]][offset];

      // If the TLB still has room, append new entry
      if(TLBsize != 16){
        TLB[TLBsize][0] = page;
        TLB[TLBsize][1] = page_table[page];
        TLBsize++;        
      }
      else{ // If its full, overwrite the oldest entry. (FIFO)
        for(int i=0; i<15; i++){
          TLB[i][0] = TLB[i+1][0];
          TLB[i][1] = TLB[i+1][1];
        }
        TLB[15][0] = page;
        TLB[15][1] = page_table[page];
      }

    }

    assert(physical_add == phys_add);
    assert(address_value == value);
    
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u (value: %4d) -- passed\n", logic_add, page, offset, physical_add, value);
    frame++;
    if (frame % 5 == 0) { printf("\n"); }
  }
  fclose(fcorr);
  fclose(fadd);
  printf("Page fault rate: %.2f %%\n", (1.0*pageFaultCount)/(1000.0)*100.0);
  printf("TLB hit rate: %.2f %%\n", (1.0*TLBhitCount) / (1000.0)*100.0);
  printf("\n\t\t...done.\n");
  return 0;
}
