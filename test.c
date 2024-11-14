
/* Tests that pgm.c works when multiple threads are continually reading
 * writing memory.  Should be compiled with:  
 *           gcc -o test2 test2.c pgm.c -pthread
 */

#include "pgm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

const int MEM_SIZE = 1 << 22;  // maximum memory that a thread can have.

struct input {
   int id;  
   int size;  // amount of memory to allocate, fill with data and read
   int table_ct;  // number of page tables to create
   int write_ct;  // number of times to fill memory and read it back
};

struct output {
   int page_faults;  // number of times that writing an int failed
   int read_faults;  // number of times that reading an int failed
   int bad_reads;   // number of times that a read got a bad value
};

void *thread_func(void *arg) {
   struct input *input = (struct input *)arg;
   struct output *output = malloc( sizeof(struct output) );
   output->page_faults = 0;
   output->read_faults = 0;
   output->bad_reads = 0;
   for (int k = 0; k < input->table_ct; k++) {
      struct page_table *mem = pgm_create();
      for (int j = 0; j < input->write_ct; j++) {
          int x = input->id*10000 + k * 1000 + j * 100;
          int top = input->size;
          for (int i = 0; i < top; i += 4) {
              if ( pgm_put_int(mem, i, x+i)  != 4 ) {
                  output->page_faults++;
                  top = i;
                  break;
              }
          }
          int read;
          for (int i = 0; i < top; i += 4) {
              if ( pgm_get_int(mem, i, &read)  != 4 ) {
                  output->read_faults++;
                  break;
              }
              else if ( read != x+i) {
                  output->bad_reads++;
              }
          }
       }
       pgm_discard(mem);
   }
   return output;
}


void run_test( int thread_count, int size, int table_ct, int write_ct ) {
   struct input thread_arg[thread_count];
   pthread_t thread[thread_count];
   int ok[thread_count];
   int ct = 0;
   for (int i = 0; i < thread_count; i++) {
      thread_arg[i].id = i;
      thread_arg[i].size = size;
      thread_arg[i].table_ct = table_ct;
      thread_arg[i].write_ct = write_ct;
      ok[i] = pthread_create(&thread[i], NULL, thread_func, &thread_arg[i]) == 0;
      if (!ok[i]) {
         printf("   Could not start thread %d!!\n",i);
      }
      else {
         ct++;
      }
   }
   printf("\n   Started %d threads.\n\n", ct);
   for (int i = 0; i < thread_count; i++) {
      if (ok[i]) {
         struct output *out;
         if (pthread_join(thread[i], (void**)&out)) {
            printf("   Error while joining thread.\n");
         }
         else {
            printf("   Thread %2d finished with %2d page faults, %2d read faults, %2d bad reads\n",
                       i, out->page_faults, out->read_faults, out->bad_reads);
         }
      }
   }
}

int main() {
   pgm_init();
   
   printf("\nNote: in these tests, a 'page fault' means that a put request did not write\n");
   printf("as many bytes as requested.  A 'read fault' means that a get request did not\n");
   printf("read as many bytes as requested.  A 'bad read' means that a get read the right\n");
   printf("numbers of bytes, but the value that was read was not the same as what was written.\n");

   printf("\n\n\nRunning 4 threads, each writing all of their memory 5 times with each\n");
   printf("of 5 page tables.  Since this can fill but never overfill physical memory,\n");
   printf("there should be NO page faults, read faults, or bad reads.\n\n");
   run_test( 4, MEM_SIZE, 5, 5 );

   printf("\n\n\nNow run 16 threads, each writing 1/4 of their memory 5 times with each\n");
   printf("of 10 page tables.  Since this can fill but never overfill physical memory,\n");
   printf("there should again be NO page faults, read faults, or bad reads.\n\n");
   run_test( 16, MEM_SIZE/4, 5, 10 );

   printf("\n\n\nNow repeat with 17 threads.  Since this can overfill overfill physical\n");
   printf("memory, there WILL LIKELY be page faults, but there should still be NO\n");
   printf("read faults, or bad reads.\n\n");
   run_test( 17, MEM_SIZE/4, 5, 10 );
   
   printf("\n\n");
}



