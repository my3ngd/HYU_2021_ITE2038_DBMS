#ifndef __FILE_H__
#define __FILE_H__

#include "page.h"

#define INITIAL_DB_FILE_SIZE (10 * 1024 * 1024)   // 10 MiB
#define PAGE_SIZE 4096                            //  4 KiB
#define HEADER_NUM 0
#define INIT_PAGE_NUM 2560

using std::pair;
#define ff first
#define ss second


/* 6 Important APIs
 * function for:
 * 
 * open file
 * alloc page
 * free page
 * read page
 * write page
 * close file
 */
int64_t file_open_table_file(char*);                    // Open existing database file or create one if it doesn't exist
pagenum_t file_alloc_page(int64_t);                     // Allocate an on-disk page from the free page list
void file_free_page(int64_t, pagenum_t);                // Free an on-disk page to the free page list
void file_read_page(int64_t, pagenum_t, page_t*);       // Read an on-disk page into the in-memory page structure(dest)
void file_write_page(int64_t, pagenum_t, const page_t*);// Write an in-memory page(src) to the on-disk page
void file_close_table_files(void);                      // Close all database files

// ext
void read_header_page(int64_t, header_page_t*);         // read header page
void write_header_page(int64_t, header_page_t*);        // write header page

// Debug
void print_pages(int64_t);
#endif  // __FILE_H__
