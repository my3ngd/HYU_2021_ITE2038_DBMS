#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include <cassert>  // debug
#include <stdint.h>
#include <pthread.h>
#include <string>
#include <utility>
#include <unordered_map>

using std::string;
using std::pair;
using std::unordered_map;

#define ff first
#define ss second

struct hash_table_entry_t;

struct lock_t
{
    lock_t* prev;
    lock_t* next;
    hash_table_entry_t* sentinel;
    pthread_cond_t cond;

    lock_t(hash_table_entry_t* sentinel);
    lock_t(void);
};

struct hash_table_entry_t
{
    int64_t table_id;
    int64_t record_id;
    lock_t* head;
    lock_t* tail;

    hash_table_entry_t(int64_t table_id, int64_t record_id);
};

struct lock_table_t
{
    unordered_map<string, hash_table_entry_t*> hash_table;

    hash_table_entry_t* get_hash_table_entry(int64_t table_id, int64_t record_id);
};


/* APIs for lock table */
int init_lock_table(void);
lock_t *lock_acquire(int64_t table_id, int64_t key);
int lock_release(lock_t* lock_obj);

#endif /* __LOCK_TABLE_H__ */
