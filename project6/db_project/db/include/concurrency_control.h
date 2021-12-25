#ifndef __LOCK_H__
#define __LOCK_H__

#include "bpt.h"
#include <set>
#include <list>
#include <stack>
#include <pthread.h>
#include <unordered_map>

using std::set;
using std::list;
using std::stack;
using std::unordered_map;


// lock state
const int S_LOCK = 0;
const int X_LOCK = 1;

/* **********************************************************
 * lock manager part
********************************************************** */
// lock manager
struct lock_manager_t;
struct lock_manager_entry_t;
struct lock_t;

extern pthread_mutex_t lock_manager_latch;
extern lock_manager_t* lock_manager;

struct lock_t
{
    lock_manager_entry_t* sentinel;
    pthread_cond_t cond;
    int lock_mode;
    int64_t table_id;
    int64_t record_id;
    int trx_id;

    lock_t(void);
    lock_t(int64_t table_id, int64_t key, lock_manager_entry_t* sentinel, int lock_mode, int trx_id);
};

struct lock_manager_entry_t
{
    list<lock_t*> locks;
    int64_t table_id;
    pagenum_t page_num;

    lock_manager_entry_t(int64_t table_id, pagenum_t page_num);
    ~lock_manager_entry_t();
};

struct lock_manager_t
{
    unordered_map<string, lock_manager_entry_t*> lock_hash_table;

    lock_manager_entry_t* get_lock_manager_entry(int64_t table_id, pagenum_t page_id);
};

int init_lock_manager(void);
lock_t *lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode);
int lock_release(lock_t *lock);

bool is_deadlocked(lock_t *lock);

/* **********************************************************
 * transaction manager part
********************************************************** */

// transaction manager
struct trx_manager_t;
struct trx_t;

extern pthread_mutex_t trx_manager_latch;
extern trx_manager_t* trx_manager;
extern int trx_id_counter;

struct trx_t
{
    int trx_id;
    list<lock_t*> locks;
    bool is_aborted;

    vector<int> wait_for_edges;
    map<pair<int64_t, int64_t>, string> undo_log;

    trx_t(void);
    void add_lock(lock_t* lock);
};

struct trx_manager_t
{
    int trx_counter;
    unordered_map<int, trx_t*> trx_hash_table;

    trx_manager_t(void);
    trx_t* get_trx_entry(int trx_id);
    trx_t* create_trx(void);
};

int init_trx_manager(void);

int trx_begin(void);
int trx_commit(int trx_id);
void trx_abort(int trx_id);

int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size, int trx_id);
int db_update(int64_t table_id, int64_t key, char* values, uint16_t new_val_size, uint16_t* old_val_size, int trx_id);


#endif  // __LOCK_H__
