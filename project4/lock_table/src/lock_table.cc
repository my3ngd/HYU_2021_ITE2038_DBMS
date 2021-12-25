#include "lock_table.h"

string hashkey_converter(int64_t table_id, int64_t record_id)
{
    return std::to_string(table_id) + ":" + std::to_string(record_id);
}

// variables
pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;
lock_table_t* lock_table = nullptr;

/* lock_t
 * lock block constructor
 */
lock_t::lock_t(void)
{
    this->next = nullptr;
    this->prev = nullptr;
    this->sentinel = nullptr;
    this->cond = PTHREAD_COND_INITIALIZER;
}

lock_t::lock_t(hash_table_entry_t* sentinel)
{
    this->next = nullptr;
    this->prev = nullptr;
    this->sentinel = sentinel;
    this->cond = PTHREAD_COND_INITIALIZER;
}

/* hash_table_entry_t
 * constructor
 */
hash_table_entry_t::hash_table_entry_t(int64_t table_id, int64_t record_id)
{
    this->table_id = table_id;
    this->record_id = record_id;
    this->head = new lock_t();
    this->tail = new lock_t();

    // linked list connection
    this->head->next = this->tail;
    this->tail->prev = this->head;
}

/* lock_table_t
 * function
 */
hash_table_entry_t* lock_table_t::get_hash_table_entry(int64_t table_id, int64_t record_id)
{
    string key = hashkey_converter(table_id, record_id);
    if (this->hash_table[key] == nullptr)
        this->hash_table[key] = new hash_table_entry_t(table_id, record_id);
    return this->hash_table[key];
}

/* APIs for lock table */

int init_lock_table(void)
{
    lock_table = new lock_table_t();
    return 0;
}

// ------------------------------------------------------- Preserve Line -------------------------------------------------------

lock_t* lock_acquire(int64_t table_id, int64_t key)
{
    pthread_mutex_lock(&lock_table_latch);
    // Thread-safe zone start

    hash_table_entry_t* hash_entry = lock_table->get_hash_table_entry(table_id, key);
    // new lock
    lock_t* lock = new lock_t(hash_entry);

    // first lock_t
    if (hash_entry->head->next == hash_entry->tail)
    {
        // connection
        hash_entry->head->next = lock;
        lock->prev = hash_entry->head;
        lock->next = hash_entry->tail;
        hash_entry->tail->prev = lock;
        // no wait (prev mistake)
    }
    // not first lock_t
    else
    {
        lock_t* prev_lock = hash_entry->tail->prev;
        // connection
        prev_lock->next = lock;
        lock->prev = prev_lock;
        lock->next = hash_entry->tail;
        hash_entry->tail->prev = lock;
        // wait
        pthread_cond_wait(&lock->cond, &lock_table_latch);
    }

    // Thread-safe zone end
    pthread_mutex_unlock(&lock_table_latch);
    return lock;
};


int lock_release(lock_t* lock_obj)
{
    pthread_mutex_lock(&lock_table_latch);
    // Thread-safe zone start

    hash_table_entry_t* hash_entry = lock_obj->sentinel;

    // not first entry
    if (hash_entry->head->next != lock_obj)
    {
        pthread_mutex_unlock(&lock_table_latch);
        return -1;
    }
    assert(lock_obj->prev == hash_entry->head);

    // only one lock_t in the list -> no signal
    if (lock_obj->next == hash_entry->tail)
    {
        // connection
        hash_entry->head->next = hash_entry->tail;
        hash_entry->tail->prev = hash_entry->head;
        // no signal (prev mistake)
    }
    // not only one lock_t in the list -> signal
    else
    {
        lock_t* next_lock = lock_obj->next;
        // connection
        hash_entry->head->next = next_lock;
        next_lock->prev = hash_entry->head;
        // signal
        pthread_cond_signal(&next_lock->cond);
    }
    delete lock_obj;

    // Thread-safe zone end
    pthread_mutex_unlock(&lock_table_latch);
    return 0;
}
