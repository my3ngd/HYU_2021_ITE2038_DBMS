#ifndef __BPT_H__
#define __BPT_H__
#include "file.h"
#include <set>

// internal page branching factor
#define BRANCHING_FACTOR 249
#define THRESHOLD 2500
// leaf page

// print
void print_leaves(int64_t);
int get_height(int64_t, pagenum_t);
void print_tree(int64_t);

// find
pagenum_t find_leaf(int64_t, int64_t);
int db_find(int64_t, int64_t, char*, uint16_t*);

// insert
int get_insert_idx(int64_t, page_t&, pagenum_t);
void insert_into_leaf(int64_t, pagenum_t, page_t&, record_t);
void insert_into_leaf_after_splitting(int64_t, pagenum_t, page_t&, record_t);
void insert_into_parent(int64_t, pagenum_t, pagenum_t, pagenum_t, page_t&, page_t&, int64_t);
void insert_into_internal(int64_t, pagenum_t, pagenum_t, page_t&, page_t&, int64_t, int);
void insert_into_internal_after_splitting(int64_t, pagenum_t, pagenum_t, page_t&, page_t&, int64_t, int);
void insert_into_new_root(int64_t, pagenum_t, pagenum_t, page_t&, page_t&, int64_t);
void start_tree(int64_t, record_t);
int db_insert(int64_t, int64_t, char*, uint16_t);

// delete
void remove_entry_from_node(int64_t, pagenum_t, page_t&, int64_t);
void adjust_root(int64_t, pagenum_t, page_t&);
void redistribute_leaf(int64_t, pagenum_t, pagenum_t, page_t&, page_t&, int, int);
void redistribute_internal(int64_t, pagenum_t, pagenum_t, page_t&, page_t&, int, int);
void merge_leaf(int64_t, pagenum_t, pagenum_t, page_t&, page_t&, int, int);
void merge_internal(int64_t, pagenum_t, pagenum_t, page_t&, page_t&, int, int);
void delete_entry(int64_t, pagenum_t, int64_t);
int db_delete(int64_t, int64_t);

// open, init/shutdown
int64_t open_table(char*);
int init_db(void);
int shutdown_db(void);

// Debug
pagenum_t detect_zero_error(int64_t);
void print_node(int64_t, pagenum_t);

#endif
