
#include "pgm.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

// (SPECIFICATIONS FOR FUNCTIONS ARE IN pgm.h !!)

// Physical memory is 2^24 (0x1000000 OR 16777216) bytes, with 2^12 (4096) physical pages.

static const int physical_page_count = 4096;  // Number of physical pages.
static const int physical_page_size = 4096;   // Size of each page.

static char physmem[4096*4096];  // Storage for physical memory.

// Virtual addresses are 22 bits, 10 bits for page number, 12 bits for offset in page.
// The macros PAGE(n) and OFFSET(n) extract the page number and the offset within the
// page from a virtual address.

#define PHYS_ADDR(phys_page, offset) (((phys_page) << 12) + (offset))
#define OFFSET(n) ((n) & 0xFFF)
#define PAGE(n) ( ((n) >> 12) & 0x3FF )

struct page_table {
   int *virtual_to_physical;  // Maps virtual page numbers to physical page numbers
   int virtual_pages;
   char *page_allocated;
      
};

static char *ppn_used;   // to check if physical page number is used or not
static char *permission;  // permission whether the page is read only or read write - 1 means you can write else read only
static pthread_mutex_t physmem_lock;
void pgm_init() {
   pthread_mutex_init(&physmem_lock, NULL);
   // calloc to initialize the ppn used with 0 to keep track if physical page in use or not
   ppn_used = (char *)calloc(physical_page_count, sizeof(char));
   permission = (char *)malloc(physical_page_count * sizeof(char));
   for (int i = 0; i < physical_page_count; i++) {
      if (physical_page_count < 500) {
         permission[i] = 0;
      }
      else {
         permission[i] = 1;
      }
   }
}


struct page_table *pgm_create() {
   struct page_table *pt = (struct page_table *)malloc(sizeof(struct page_table));
   if (pt == NULL) {
      return NULL;
   }


   // lock the section of initializing the page table and its allocation
   pthread_mutex_lock(&physmem_lock);
   pt -> virtual_pages = 1024;   // because of 22 bits virtual address, 12 for offset and 10 for virtual pages so 1024 virtual pages in total
   pt -> virtual_to_physical = (int *)malloc(sizeof(int) * pt -> virtual_pages);  // since we can map only 1024 virtual pages to 4096 physical pages so we just need space for 1024 locations
   pt -> page_allocated = (char *)malloc(sizeof(char) * pt -> virtual_pages);
   //return pt;
   if (pt -> virtual_to_physical == NULL || pt -> page_allocated == NULL) {
      free(pt -> virtual_to_physical);
      free(pt -> page_allocated);
      free(pt);
      return NULL;
   }

   // Initialize the page table to map all virtual pages to the first physical page.
   for (int i = 0; i < pt -> virtual_pages; i++) {
      pt -> virtual_to_physical[i] = -1;     // -1 means No Virtual to Physical mapping yet
      pt -> page_allocated[i] = 0;           // page is not allocated
   }
   pthread_mutex_unlock(&physmem_lock);
   return pt;
}


void pgm_discard(struct page_table *pgtab) {
   if (pgtab == NULL) {
      return;
   }

   pthread_mutex_lock(&physmem_lock);
   // Free the allocated page tables as well
   for (int i = 0; i < pgtab -> virtual_pages; i++) {
      
      if (pgtab -> page_allocated[i]) {
         ppn_used[pgtab -> virtual_to_physical[i]] = 0;
         pgtab -> page_allocated[i] = 0;
         pgtab -> virtual_to_physical[i] = -1;
         
      }
   }

   pthread_mutex_unlock(&physmem_lock);
   

  // free(pgtab -> virtual_to_physical);
   //free(pgtab -> page_allocated);
   free(pgtab);
}


int pgm_put(struct page_table *pgtab, int start_address, void *data_source, int byte_count) {

   if (pgtab == NULL || data_source == NULL || byte_count < 0) {
      return 0;
      
   }
   char *data   = (char *)data_source;
   int bytes_written = 0;
   
   while (bytes_written < byte_count) {
      int vpn = PAGE(start_address + bytes_written);
      int offset = OFFSET(start_address + bytes_written);

      // check if the virtual page is greater than the Page table entries
      if (vpn >= pgtab -> virtual_pages) {
         //printf("Virtaul address is not valid\n");
         break;
      }

      // lock this section as we dont want that other thread interfere at the same time page is being allocated for one thread
      //pthread_mutex_lock(&physmem_lock);
      // check if the page is not allocated in the page table
      if (pgtab -> page_allocated[vpn] == 0) {
         pthread_mutex_lock(&physmem_lock);
         int new_page = -1;
         for (int i = 0; i < physical_page_count; i++) {
            if (!ppn_used[i] && permission[i] == 1) {
               new_page = i;
               ppn_used[i] = 1;
               break;
            }
            
         }

         if (new_page != -1) {
            pgtab -> virtual_to_physical[vpn] = new_page;
            pgtab -> page_allocated[vpn] = 1;
            
         }
         pthread_mutex_unlock(&physmem_lock);
        

         if (new_page == -1) {
            //printf("Not enough physical pages");
            break;
         }
         



      }
      //pthread_mutex_unlock(&physmem_lock);
      


      int bytes_to_write = physical_page_size - offset;
      if (bytes_to_write + bytes_written > byte_count) {
         bytes_to_write = byte_count - bytes_written;
      }

      int physical_address = PHYS_ADDR(pgtab -> virtual_to_physical[vpn], offset);
      memcpy(&physmem[physical_address], &data[bytes_written], bytes_to_write);
      bytes_written = bytes_written + bytes_to_write;

      
   }

   return bytes_written;
}


int pgm_get(struct page_table *pgtab, int start_address, void *data_destination, int byte_count) {
   if (pgtab == NULL || data_destination == NULL || byte_count < 0) {
      return 0;
      
   }
   char *data   = (char *)data_destination;
   int bytes_read = 0;

   
   //pthread_mutex_lock(&physmem_lock);
   while (bytes_read < byte_count) {
      int vpn = PAGE(start_address + bytes_read);
      int offset = OFFSET(start_address + bytes_read);

      // if the page is not allocated, data should not be read
      if (pgtab -> page_allocated[vpn] == 0) {
         return 0;
      }

      // check if the virtual page is greater than the Page table entries
      if (vpn >= pgtab -> virtual_pages) {
         //printf("Virtaul address is not valid\n");
         break;
      }

      int read_page_bytes = physical_page_size - offset;    // No of bytes remained in the page to be copied
      if (read_page_bytes + bytes_read > byte_count) {   // check if no of bytes to be written is greater than the requested bytes
         read_page_bytes = byte_count - bytes_read;

      }

      int phys_addr = PHYS_ADDR(pgtab->virtual_to_physical[vpn], offset);
      memcpy(data + bytes_read, &physmem[phys_addr], read_page_bytes);
      

      bytes_read = bytes_read + read_page_bytes;

      
   }
   //pthread_mutex_unlock(&physmem_lock);

   return bytes_read;

   
}


int pgm_put_int(struct page_table *pgtab, int address, int value) {
   return pgm_put(pgtab, address, &value, sizeof(int));
}


int pgm_get_int(struct page_table *pgtab, int address, int *pvalue) {
   return pgm_get(pgtab, address, pvalue, sizeof(int));
}



