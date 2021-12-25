/* TODO
 * May need operator= overloading
 */

#ifndef __PAGE_H__
#define __PAGE_H__

#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <deque>

using std::endl;
using std::cin;
using std::cout;
using std::swap;
using std::string;
using std::vector;
using std::deque;

using pagenum_t = int64_t;

#define INIT_FREE_SPACE 3968

/* slot_t
 * leaf page's slot
 * has constant size
 * 12 = 8 + 2 + 2
 * (slot_t is not directly
 *  writed to disk)
 */
struct slot_t
{
    int64_t key;
    unsigned short size;
    unsigned short offset;

    slot_t(void);
    slot_t(unsigned char* rec, int idx);
    slot_t(int64_t key, unsigned short size, unsigned short offset);
};

/* record_t
 * temp structure for records
 * defined by slot and std::string
 */
struct record_t
{
    slot_t slot;
    string rec;

    record_t(void);
    record_t(int64_t, char*, uint16_t);
};

/* edge_t
 * internal page's edge
 */
struct edge_t
{
    int64_t key;
    pagenum_t child_num;

    edge_t(void);
    edge_t(int64_t key, pagenum_t child_num);

    bool operator==(const edge_t&);
    bool operator!=(const edge_t&);
};

/* header page
 * #header page == 1
 * for all DB
 */
struct header_page_t
{
    pagenum_t free_num;
    int64_t num_of_pages;
    pagenum_t root_num;
    unsigned char RESERVED[4096 - 24];

    header_page_t(void);
};

/* page header
 * page_header must be 128 Byte,
 * but it is designed 112 Byte
 * for convenience.
 */
struct page_header_t
{
    pagenum_t parent_num;
    /* is_leaf
     * -1: free
     *  0: internal
     *  1: leaf
     */
    int is_leaf;
    int num_of_keys;
    unsigned char RESERVED[112 - 16];

    page_header_t(void);
    bool operator==(const page_header_t&);
    bool operator!=(const page_header_t&);
};

/* internal page
 * left_num: leftmost child's page number
 * edges:    {key, child_num} pairs
 */
struct internal_page_t
{
    unsigned char RESERVED[120 - 112];
    pagenum_t left_num;
    edge_t edges[248];

    internal_page_t(void);
};

/* leaf page
 * free_space: free space of [rec]
 * right_num:  right sibling of itself
 * rec[~~]:    space for [slot_t] and [record]
 * because of their # are not constant and
 * can violate each other's memory,
 * rec must be defined with byte only.
 */
struct leaf_page_t
{
    int64_t free_space;
    pagenum_t right_num;
    unsigned char rec[4096 - 128];

    leaf_page_t(void);
};

/* page_t
 * page_t is the main page structure
 * which defines leaf/internal page,
 * not header page, free page.
 * (free page is not defined)
 */
struct page_t
{
    page_header_t header;
    union
    {
        internal_page_t ip;
        leaf_page_t     lp;
    };

    page_t(void);
    page_t(int);
    bool operator==(const page_t&);
};

/* memory_leaf_t
 * Important structure for leaf node.
 * because of leaf_page_t::rec is defined
 * by char*, memory_leaf_t is defined
 * by vector of [slot_t] and [string]
 */
struct memory_leaf_t
{
    page_header_t header;
    pagenum_t right_num;
    int64_t free_space;

    deque<slot_t> slots;
    deque<string> records;
};

/*
 */
struct memory_internal_t
{
    page_header_t header;
    deque<pagenum_t> childs;
    deque<int64_t> keys;
};

/* we MUST NOT write memory_leaf_t
 * to disk directly.
 * we MUST write page_t::leaf_page_t,
 * use under functions.
 */
memory_leaf_t diskleaf_to_memleaf(page_t);

page_t memleaf_to_diskleaf(memory_leaf_t);

memory_internal_t diskinternal_to_meminternal(page_t);

page_t meminternal_to_diskinternal(memory_internal_t);

#endif  // __PAGE_H__
