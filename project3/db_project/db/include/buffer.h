#ifndef __BUFFER_H__
#define __BUFFER_H__
#include "file.h"
#include <map>
using std::map;

int64_t open_table(char*);
int init_db(int);
int shutdown_db(void);

struct control_block_t
{
    page_t frame;
    int64_t table_id;
    pagenum_t page_num;
    bool is_dirty;
    int pinned;
    control_block_t* next;
    control_block_t* prev;

    control_block_t(void);
};

struct buffer_t
{
    // pointers
    control_block_t* head;
    control_block_t* tail;

    // maximum control_block count
    uint64_t max_size, cur_size;
    // tid, pagenum -> control_block pointer
    map<pair<int64_t, pagenum_t>, control_block_t*> dic;
    // table id, header page pair
    map<int64_t, header_page_t> files;
    vector<string> names;

    buffer_t(int64_t);
    ~buffer_t();
    void load_page(int64_t, pagenum_t);
    void remove_back(void);
};

// page -> buffer block
void file_buffer_read(int64_t, pagenum_t, control_block_t*);

// header pages
void read_buffer_header(int64_t);
void write_buffer_header(int64_t);
void buffer_get_header(int64_t, header_page_t&);
void buffer_set_header(int64_t, header_page_t&);

// buffer alloc/free/read
pagenum_t buffer_alloc_page(int64_t);
void buffer_free_page(int64_t, pagenum_t);
void buffer_read_page(int64_t, pagenum_t, page_t&);
void buffer_read_only_page(int64_t, pagenum_t, page_t&);
void buffer_write_page(int64_t, pagenum_t, page_t&);

// DEBUG
void print_buffer(void);
void print_map(void);
int count_pin(void);

// variables
extern bool buffer_exist;
extern buffer_t* buffer;

#endif // __BUFFER_H__
