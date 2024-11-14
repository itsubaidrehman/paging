#ifndef __PGM_H__
#define __PGM_H__

struct page_table; // Declares that data type exists, to be defined in pgm.c.
                   // (We can still declare pointers to this type.)


/* Initialization routine, to be called ONCE before using any other pgm functions. 
 */
void pgm_init();


/* Creates a new page table and returns a pointer to it.  This creates a new
 * "virtual address space" that is conceptually filled with zeros. 
 */
struct page_table *pgm_create();


/* Discards a page table that will never be used again. Physical memory pages
 * corresponding to the virtual address space can be reclaimed. 
 */
void pgm_discard(struct page_table *pgtab);


/* Copy bytes into paged memory; start_address gives the virtual memory address where
 * the data will be placed; data_source points to the data to be copied; byte_count
 * is the number of bytes to be copied.  Note that only the lower order bits corresponding
 * to the size of the virtual memory space, are used; high-order bits are discarded.
 * The return value will be the number of bytes actually copied.  A return value less
 * than byte_count indicates an error (presumably, out of space in physical memory), or
 * an illegal parameter such as a bad pointer or a negative byte count.
 */
int pgm_put(struct page_table *pgtab, int start_address, 
                     void *data_source, int byte_count);


/* Copies bytes from paged memory; start_address is the virtual memory address where
 * the data starts; data_destination points to the location into which the data will
 * be copied; byte_count is the number of bytes to copy.  The return value is the
 * actual number of bytes copied.  A return value less than byte_count indicates an
 * error (presumably a bad parameter value).  Note that bytes that have not been
 * previously written with pgm_put will be read as zero.
 */
int pgm_get(struct page_table *pgtab, int start_address,
                    void *data_destination, int byte_count);


/* A convenience routine for copying an int value into paged memory, at the specified
 * virtual memory address.  The return value is the number of bytes copied; a value
 * less than 4 indicates an error.
 */
int pgm_put_int(struct page_table *pgtab, int address, int value);


/* A convenience routine for fetching an int value from paged memory, at the specified
 * virtual memory address.  The return value is the number of bytes copied; a value
 * less than 4 indicates an error.
 */
int pgm_get_int(struct page_table *pgtab, int address, int *pvalue);

#endif

