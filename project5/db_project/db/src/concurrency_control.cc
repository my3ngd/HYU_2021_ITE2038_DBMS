#include "concurrency_control.h"

/* ********************************************************************************************************************
 * lock manager part
******************************************************************************************************************** */

pthread_mutex_t lock_manager_latch;
lock_manager_t* lock_manager;

/* lock_t base constructor */
lock_t::lock_t(void)
{
    this->sentinel = nullptr;
    this->cond = PTHREAD_COND_INITIALIZER;
    this->lock_mode = S_LOCK;
    this->table_id = -1;
    this->record_id = -1;
    this->trx_id = 0;
}

lock_t::lock_t(int64_t table_id, int64_t key, lock_manager_entry_t* sentinel, int lock_mode, int trx_id)
{
    this->sentinel = sentinel;
    this->cond = PTHREAD_COND_INITIALIZER;
    this->lock_mode = lock_mode;
    this->table_id = table_id;
    this->record_id = key;
    this->trx_id = trx_id;
}



/* lock_manager_entry_t constructor */
lock_manager_entry_t::lock_manager_entry_t(int64_t table_id, pagenum_t page_num)
{
    this->table_id = table_id;
    this->page_num = page_num;
}

/* lock manager */
lock_manager_entry_t* lock_manager_t::get_lock_manager_entry(int64_t table_id, pagenum_t page_id)
{
    string key = std::to_string(table_id) + ":" + std::to_string(page_id);
    if (this->lock_hash_table[key] == nullptr)
        this->lock_hash_table[key] = new lock_manager_entry_t(table_id, page_id);
    return this->lock_hash_table[key];
}


int init_lock_manager(void)
{
    lock_manager = new lock_manager_t();
    return 0;
}



lock_t* lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode)
{
    pthread_mutex_lock(&lock_manager_latch);
    lock_manager_entry_t* sentinel = lock_manager->get_lock_manager_entry(table_id, page_id);
    list<lock_t*>& locks = sentinel->locks;
    trx_t* trx = trx_manager->get_trx_entry(trx_id);

    bool EXIST = false;
    for (lock_t*& cur: locks)
    {
        if (cur->table_id == table_id && cur->record_id == key)
        {
            EXIST = true;
            break;
        }
    }

    if (!EXIST)
    {
        #ifndef __MILESTONE_2_IMPLICIT_LOCK__
        lock_t* lock = new lock_t(table_id, key, sentinel, lock_mode, trx_id);
        locks.push_back(lock);
        trx->add_lock(lock);
        pthread_mutex_unlock(&lock_manager_latch);
        return lock;
        #else  // implement of implicit lock (milestone 2)

        /* lock compression
         * compress X-lock if there is no lock in record
         * performance improvement when "disjoint X lock test"
         * not implemented because problem of latches:
         * after commit a transaction, line 109? cause problem.
         * (not changed "page.cc" and "page.h")
         */

        // get page latch
        pthread_mutex_lock(&buffer_manager_latch);
        buffer->load_page(table_id, page_id);
        pthread_mutex_lock(&(buffer->dic[{table_id, page_id}]->page_latch));
        buffer->dic[{table_id, page_id}]->pinned = true;
        pthread_mutex_unlock(&buffer_manager_latch);
        // get trx latch
        memory_leaf_t leaf = diskleaf_to_memleaf(buffer->dic[{table_id, page_id}]->frame);
        int key_idx = key_exists(leaf, key);
        pthread_mutex_lock(&trx_manager_latch); // <- (may cause problem in milestone 2! 😥 I have no idea how to solve in this design)
        // implicit lock's trx not activate
        if (trx_manager->trx_hash_table.find(leaf.slots[key_idx].trx_id) == trx_manager->trx_hash_table.end())
        {
            // set implicit lock again
            if (lock_mode == X_LOCK)
            {
                leaf.slots[key_idx].trx_id = trx_id;
                pthread_mutex_unlock(&trx_manager_latch);
                buffer->dic[{table_id, page_id}]->dirty = true;
                pthread_mutex_unlock(&(buffer->dic[{table_id, page_id}]->page_latch));
                buffer->dic[{table_id, page_id}]->pinned = false;
                pthread_mutex_unlock(&lock_manager_latch);
                return tmp_lock; // not nullptr (return nullptr means 'deadlock')
            }
            // just connect
            else
            {
                pthread_mutex_unlock(&(buffer->dic[{table_id, page_id}]->page_latch));
                buffer->dic[{table_id, page_id}]->pinned = false;
                lock_t* lock = new lock_t(table_id, key, sentinel, lock_mode, trx_id);
                locks.push_back(lock);
                trx->add_lock(lock);
                pthread_mutex_unlock(&trx_manager_latch);
                pthread_mutex_unlock(&lock_manager_latch);
                return lock;
            }
        }
        // implicit lock's trx active
        else
        {
            lock_t* impl = new lock_t(table_id, key, sentinel, X_LOCK, leaf.slots[key_idx].trx_id);
            lock_t* lock = new lock_t(table_id, key, sentinel, lock_mode, trx_id);
            locks.push_back(impl);
            trx_manager->get_trx_entry(leaf.slots[key_idx].trx_id)->add_lock(impl);
            locks.push_back(lock);
            trx->add_lock(lock);
            pthread_mutex_unlock(&trx_manager_latch);
            pthread_mutex_unlock(&(buffer->dic[{table_id, page_id}]->page_latch));
            buffer->dic[{table_id, page_id}]->pinned = false;
            pthread_mutex_unlock(&lock_manager_latch);
            return lock;
        }

        #endif // __MILESTONE_2_IMPLICIT_LOCK__
    }

    bool conflict = lock_mode;
    for (lock_t*& cur: locks)
    {
        if (cur->table_id != table_id || cur->record_id != key)
            continue;
        if (cur->trx_id == trx_id)
        {
            if (cur->lock_mode >= lock_mode)
            {
                pthread_mutex_unlock(&lock_manager_latch);
                return cur;
            }
            else
            {
                bool OTHER_TRX_EXIST = false;
                // check if it is only trx
                for (lock_t* tmp: locks)
                {
                    if (tmp->table_id != table_id || tmp->record_id != key)
                        continue;
                    if (tmp->trx_id != trx_id)
                    {
                        OTHER_TRX_EXIST = true;
                        break;
                    }
                }
                // wait or not
                if (OTHER_TRX_EXIST)
                {
                    lock_t* new_lock = new lock_t(table_id, key, sentinel, lock_mode, trx_id);
                    locks.push_back(new_lock);
                    trx->add_lock(new_lock);
                    // drow graph
                    if (lock_mode == S_LOCK)
                    {
                        auto iter = sentinel->locks.rbegin();
                        for (iter++; iter != sentinel->locks.rend(); iter++)
                        {
                            if ((*iter)->table_id == table_id && (*iter)->record_id == key && (*iter)->lock_mode == X_LOCK)
                            {
                                if ((*iter)->trx_id == trx_id)
                                    continue;
                                trx->wait_for_edges.push_back((*iter)->trx_id);
                                break;
                            }
                        }
                    }
                    else // X_LOCK
                    {
                        auto iter = sentinel->locks.rbegin();
                        for (iter++; iter != sentinel->locks.rend(); iter++)
                        {
                            if ((*iter)->table_id == table_id && (*iter)->record_id == key)
                            {
                                if ((*iter)->trx_id == trx_id)
                                    continue;
                                trx->wait_for_edges.push_back((*iter)->trx_id);
                                if ((*iter)->lock_mode == X_LOCK)
                                    break;
                            }
                        }
                    }
                    // deadlock check
                    if (is_deadlocked(new_lock))
                    {
                        pthread_mutex_unlock(&lock_manager_latch);
                        return nullptr;
                    }
                    pthread_cond_wait(&new_lock->cond, &lock_manager_latch);
                    pthread_mutex_unlock(&lock_manager_latch);
                    return new_lock;
                }
                else
                {
                    cur->lock_mode = X_LOCK;
                    pthread_mutex_unlock(&lock_manager_latch);
                    return cur;
                }
            }
        }
        else if (cur->lock_mode == X_LOCK)
        {
            conflict = true;
        }
    }
    lock_t* new_lock = new lock_t(table_id, key, sentinel, lock_mode, trx_id);
    locks.push_back(new_lock);
    trx->add_lock(new_lock);
    // conflict
    if (conflict)
    {
        if (lock_mode == S_LOCK)
        {
            auto iter = sentinel->locks.rbegin();
            for (iter++; iter != sentinel->locks.rend(); iter++)
            {
                if ((*iter)->table_id == table_id && (*iter)->record_id == key && (*iter)->lock_mode == X_LOCK)
                {
                    if ((*iter)->trx_id == trx_id)
                        continue;
                    trx->wait_for_edges.push_back((*iter)->trx_id);
                    break;
                }
            }
        }
        else // lock_mode == X_LOCK
        {
            auto iter = sentinel->locks.rbegin();
            for (iter++; iter != sentinel->locks.rend(); iter++)
            {
                if ((*iter)->table_id == table_id && (*iter)->record_id == key)
                {
                    if ((*iter)->trx_id == trx_id)
                        continue;
                    trx->wait_for_edges.push_back((*iter)->trx_id);
                    if ((*iter)->lock_mode == X_LOCK)
                        break;
                }
            }
        }
        if (is_deadlocked(new_lock))
        {
            pthread_mutex_unlock(&lock_manager_latch);
            // cout << "deadlocked!" << endl;
            return nullptr;
        }
        pthread_cond_wait(&new_lock->cond, &lock_manager_latch);
        pthread_mutex_unlock(&lock_manager_latch);
        return new_lock;
    }
    // not conflict
    else
    {
        pthread_mutex_unlock(&lock_manager_latch);
        return new_lock;
    }
}

/*
 * Credit: Ingyu Bang
 * He pointed out that
 * unlike the previous design,
 * it can be implemented
 * regardless of the location
 * of the lock to be deleted.
 */
int lock_release(lock_t* lock)
{
    pthread_mutex_lock(&lock_manager_latch);
    lock_manager_entry_t* sentinel = lock->sentinel;
    list<lock_t*>& locks = sentinel->locks;

    // remove part
    locks.remove(lock);

    lock_t* first_lock = nullptr;
    lock_t* second_lock = nullptr;
    for (lock_t*& cur: locks)
    {
        if (lock->table_id == cur->table_id && lock->record_id == cur->record_id)
        {
            if (first_lock == nullptr)
                first_lock = cur;
            else
            {
                second_lock = cur;
                break;
            }
        }
    }

    // no lock after
    if (first_lock == nullptr)
    {
        pthread_mutex_unlock(&lock_manager_latch);
        return 0;
    }
    else if (second_lock == nullptr)
    {
        pthread_cond_signal(&first_lock->cond);
        pthread_mutex_unlock(&lock_manager_latch);
        return 0;
    }
    // 2+ lock after
    // X > ? > ?
    if (first_lock->lock_mode == X_LOCK)
    {
        pthread_cond_signal(&first_lock->cond);
        pthread_mutex_unlock(&lock_manager_latch);
        return 0;
    }
    // S > X > ?
    else if (second_lock->lock_mode == X_LOCK)
    {
        // S->trx_id == X->trx_id: wake up two
        if (first_lock->trx_id == second_lock->trx_id)
        {
            pthread_cond_signal(&first_lock->cond);
            pthread_cond_signal(&second_lock->cond);
            pthread_mutex_unlock(&lock_manager_latch);
            return 0;
        }
        else
        {
            pthread_cond_signal(&first_lock->cond);
            pthread_mutex_unlock(&lock_manager_latch);
            return 0;
        }
    }
    // S > S > ...
    else
    {
        for (lock_t*& cur: locks)
        {
            if (cur->table_id == lock->table_id && cur->record_id == lock->record_id)
            {
                if (cur->lock_mode == S_LOCK)
                    pthread_cond_signal(&cur->cond);
                else
                    break;
            }
        }
    }

    pthread_mutex_unlock(&lock_manager_latch);
    locks.remove(lock);
    return 0;
}


/* ********************************************************************************************************************
 * transaction manager part
******************************************************************************************************************** */

pthread_mutex_t trx_manager_latch;
trx_manager_t* trx_manager = nullptr;

trx_t::trx_t(void)
{
    this->trx_id = 0;
    this->is_aborted = false;
}


/* add lock to trx node */
void trx_t::add_lock(lock_t* lock)
{
    this->locks.push_back(lock);
    return ;
}

trx_manager_t::trx_manager_t(void)
{
    this->trx_counter = 1;
}

trx_t* trx_manager_t::get_trx_entry(int trx_id)
{
    if (this->trx_hash_table[trx_id] == nullptr)
        this->trx_hash_table[trx_id] = new trx_t();
    return this->trx_hash_table[trx_id];
}

trx_t* trx_manager_t::create_trx(void)
{
    trx_t* trx = new trx_t();
    if (this->trx_counter < 0)
        this->trx_counter = 1;
    trx->trx_id = this->trx_counter++;
    this->trx_hash_table[trx->trx_id] = trx;
    return trx;
}


// binary search
int key_exists(memory_leaf_t leaf, int64_t key)
{
    // binary search
    int LL = 0, MM, RR = leaf.header.num_of_keys-1;
    // rightmost case
    if (leaf.slots[RR].key == key)
        return RR;
    while (LL < RR)
    {
        MM = LL + RR >> 1;
        if      (leaf.slots[MM].key == key) return MM;
        else if (leaf.slots[MM].key >  key) RR = MM;
        else                                LL = MM + 1;
    }
    return -1;
}



/* trx manager initailizer */
int init_trx_manager(void)
{
    trx_manager = new trx_manager_t();
    trx_manager_latch = PTHREAD_MUTEX_INITIALIZER;
    return 0;
}

/* trx begin
 * make a trx node
 */
int trx_begin(void)
{
    pthread_mutex_lock(&trx_manager_latch);
    trx_t* trx = trx_manager->create_trx();
    pthread_mutex_unlock(&trx_manager_latch);
    return trx->trx_id;
}

/* trx commit
 * release all locks (by strict-2PL policy)
 */
int trx_commit(int trx_id)
{
    pthread_mutex_lock(&trx_manager_latch);
    trx_t* trx = trx_manager->get_trx_entry(trx_id);

    lock_t* lock;
    for (lock_t*& cur: trx->locks)
        lock_release(cur);

    while (trx->locks.size())
    {
        lock = trx->locks.back();
        trx->locks.pop_back();
    }

    trx_manager->trx_hash_table.erase(trx_id);

    pthread_mutex_unlock(&trx_manager_latch);
    return 0;
}


/* ********************************************************************************************************************
 * deadlock detection & abort part
******************************************************************************************************************** */

/* trx rollback
 * rollback records
 * rollback by page-unit
 */
void trx_rollback(trx_t* trx)
{
    for (auto& log: trx->undo_log)
    {
        int64_t table_id = log.ff.ff;
        int64_t key = log.ff.ss;
        string log_value = log.ss;

        // rollback part
        pthread_mutex_lock(&buffer_latch);
        pagenum_t leaf_num = find_leaf(table_id, key);
        pthread_mutex_lock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
        buffer->dic[{table_id, leaf_num}]->pinned = true;
        pthread_mutex_unlock(&buffer_latch);

        memory_leaf_t leaf = diskleaf_to_memleaf(buffer->dic[{table_id, leaf_num}]->frame);
        int key_idx = key_exists(leaf, key);// get index
        leaf.records[key_idx] = log_value;  // rollback value
        buffer->dic[{table_id, leaf_num}]->frame = memleaf_to_diskleaf(leaf);
        buffer->dic[{table_id, leaf_num}]->is_dirty = true;
        pthread_mutex_unlock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
        buffer->dic[{table_id, leaf_num}]->pinned = false;
    }
}

/* trx abort
 * if deadlock detected, remove all locks
 */
void trx_abort(int trx_id)
{
    trx_t* trx = trx_manager->get_trx_entry(trx_id);
    // rollback records
    trx_rollback(trx);
    // release all locks
    lock_t* lock;
    for (lock_t* cur: trx->locks)
        lock_release(cur);

    trx_manager->trx_hash_table.erase(trx_id);
    delete trx;

    return ;
}


/* deadlock detection
 * search cycle in "wait-for"
 * graph. it is called only
 * if "conflicted" condition.
 * 
 * Credit: Yeonjin Park
 * Suggested a method
 * for statistically generating
 * a wait-for graph.
 */
bool DFS(unordered_map<int, bool>& visited, trx_t* trx, int start_trx_id)
{
    bool deadlock = false;
    for (auto iter = trx->wait_for_edges.begin(); iter != trx->wait_for_edges.end(); )
    {
        auto cur = trx_manager->trx_hash_table.find(*iter);
        if (cur == trx_manager->trx_hash_table.end())
        {
            iter = trx->wait_for_edges.erase(iter++);
            continue;
        }
        if (start_trx_id == cur->ss->trx_id)
        {
            return true;
        }
        if (visited[cur->ss->trx_id])
        {
            iter++;
            continue;
        }
        visited[cur->ss->trx_id] = true;
        deadlock |= DFS(visited, cur->ss, start_trx_id);
        iter++;
    }
    return deadlock;
}


bool is_deadlocked(lock_t* lock)
{
    unordered_map<int, bool> visited;
    visited[lock->trx_id] = true;
    return DFS(visited, trx_manager->get_trx_entry(lock->trx_id), lock->trx_id);
}



int db_find(int64_t table_id, int64_t key, char* ret_val, uint16_t* val_size, int trx_id)
{
    pthread_mutex_lock(&trx_manager_latch);
    trx_t* trx = trx_manager->get_trx_entry(trx_id);
    pthread_mutex_unlock(&trx_manager_latch);
    if (trx->is_aborted)
    {
        *val_size = 0;
        ret_val = nullptr;
        return -1;
    }

    pthread_mutex_lock(&buffer_latch);
    pagenum_t leaf_num = find_leaf(table_id, key);
    // leaf not exist
    if (leaf_num == 0)
    {
        *val_size = 0;
        ret_val = nullptr;
        pthread_mutex_unlock(&buffer_latch);
        return -1;
    }
    pthread_mutex_lock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    buffer->dic[{table_id, leaf_num}]->pinned = true;   // no evict
    pthread_mutex_unlock(&buffer_latch);

    // get page latch only, check if key exist
    memory_leaf_t leaf = diskleaf_to_memleaf(buffer->dic[{table_id, leaf_num}]->frame);
    int key_idx = key_exists(leaf, key);
    if (key_idx == -1)  // key not exist
    {
        *val_size = 0;
        ret_val = nullptr;
        pthread_mutex_unlock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
        return -1;
    }

    pthread_mutex_unlock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    buffer->dic[{table_id, leaf_num}]->pinned = false;

    lock_t* lock = lock_acquire(table_id, leaf_num, key, trx_id, S_LOCK);
    // deadlocked
    if (lock == nullptr)
    {
        *val_size = 0;
        ret_val = nullptr;
        trx->is_aborted = true;
        trx_abort(trx_id);
        return -1;
    }

    pthread_mutex_lock(&buffer_latch);
    // load page to buffer
    leaf_num = find_leaf(table_id, key);
    // leaf must exist
    pthread_mutex_lock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    leaf = diskleaf_to_memleaf(buffer->dic[{table_id, leaf_num}]->frame);
    buffer->dic[{table_id, leaf_num}]->pinned = true;   // no evict
    pthread_mutex_unlock(&buffer_latch);

    // get page latch only, find value
    memset(ret_val, 0, leaf.slots[key_idx].size+1);
    memcpy(ret_val, leaf.records[key_idx].c_str(), leaf.slots[key_idx].size);
    *val_size = leaf.slots[key_idx].size;

    pthread_mutex_unlock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    buffer->dic[{table_id, leaf_num}]->pinned = false;

    return 0;
}


int db_update(int64_t table_id, int64_t key, char* values, uint16_t new_val_size, uint16_t* old_val_size, int trx_id)
{
    pthread_mutex_lock(&trx_manager_latch);
    trx_t* trx = trx_manager->get_trx_entry(trx_id);
    pthread_mutex_unlock(&trx_manager_latch);
    if (trx->is_aborted)
    {
        *old_val_size = 0;
        return -1;
    }

    pthread_mutex_lock(&buffer_latch);
    pagenum_t leaf_num = find_leaf(table_id, key);
    // leaf not exist
    if (leaf_num == 0)
    {
        *old_val_size = 0;
        pthread_mutex_unlock(&buffer_latch);
        return -1;
    }
    pthread_mutex_lock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    buffer->dic[{table_id, leaf_num}]->pinned = true;   // no evict
    pthread_mutex_unlock(&buffer_latch);

    // get page latch only, check if key exist
    memory_leaf_t leaf = diskleaf_to_memleaf(buffer->dic[{table_id, leaf_num}]->frame);
    int key_idx = key_exists(leaf, key);
    if (key_idx == -1)  // key not exist
    {
        *old_val_size = 0;
        pthread_mutex_unlock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
        return -1;
    }

    pthread_mutex_unlock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    buffer->dic[{table_id, leaf_num}]->pinned = false;

    lock_t* lock = lock_acquire(table_id, leaf_num, key, trx_id, X_LOCK);
    // deadlocked
    if (lock == nullptr)
    {
        *old_val_size = 0;
        trx->is_aborted = true;
        trx_abort(trx_id);
        return -1;
    }

    pthread_mutex_lock(&buffer_latch);
    // load page to buffer
    leaf_num = find_leaf(table_id, key);
    // leaf must exist
    pthread_mutex_lock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    leaf = diskleaf_to_memleaf(buffer->dic[{table_id, leaf_num}]->frame);
    buffer->dic[{table_id, leaf_num}]->pinned = true;   // no evict
    pthread_mutex_unlock(&buffer_latch);

    // map<pair<int64_t, int64_t>, string> undo_log;
    if (trx->undo_log.find({table_id, key}) == trx->undo_log.end())
        trx->undo_log.insert({{table_id, key}, leaf.records[key_idx]});

    // get page latch only, update value
    *old_val_size = leaf.slots[key_idx].size;
    leaf.records[key_idx] = string(values);
    leaf.slots[key_idx].size = new_val_size;
    buffer->dic[{table_id, leaf_num}]->frame = memleaf_to_diskleaf(leaf);
    buffer->dic[{table_id, leaf_num}]->is_dirty = true;

    pthread_mutex_unlock(&(buffer->dic[{table_id, leaf_num}]->page_latch));
    buffer->dic[{table_id, leaf_num}]->pinned = false;

    return 0;
}

