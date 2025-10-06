# Project 5 Wiki

2020098240 이현빈

# 명세 요약

다음과 같은 기능을 지원하는 Lock manageer를 구현해야 한다.

- Conflict-Serializable schedule for transactions
- Strict-2PL (2 phase locking)
- Deadlock 감지
- record-level S/X lock

이를 위해 다음 API를 구현해야 한다.

1. `int trx_begin(void)`
    - 트랜잭션을 할당한다.
    - 성공 시 transaction id(>=0)을 반환, 실패 시 0을 반환한다.
2. `int trx_commit(int trx_id)`
    - 트랜잭션을 commit한다.
    - 성공 시 완료한 transaction id를 반환, 실패 시 0을 반환한다.
3. `int db_find(..., int trx_id)`
4. `int db_update(..., int trx_id)`
5. `int init_lock_table(void)`
6. `lock_t *lock_acquire(int64_t table_id, pagenum_t page_id, int64_t key, int trx_id, int lock_mode)`
    - 새 lock object를 할당하고 연결한다.
    - lock manager에서처럼 동작해야 한다.
    - `lock_mode`: 0 (shared lock) | 1 (exclusive lock)
7. `int lock_release(lock_t *lock_obj)`

또한 트랜잭션이 동작하는 순서가 강제되지 않으므로, 데드락이 발생할 수 있으며 이를 탐지 및 해결해야 한다.

1. Deadlock detected
2. 해당 트랜잭션의 모든 실행된 record를 undo
3. 해당 트랜잭션의 모든 lock objects를 release
4. transaction table에서 transaction entry를 제거

또한 DBMS는 다음을 고려해야 한다.
- Buffer manager에서 multiple access 시에 올바르게 동작해야 한다.
- 이를 위해 Buffer manager latch를 사용해 통째로 잠글 수 있다.
- 그러나 너무 비효율적이므로, page latch를 사용하여 page 단위로 잠글 수 있다.
- page latch를 사용하기 위해 buffer latch를 먼저 사용해야 한다.
    1. Buffer latch acquire
    2. Page latch acquire
    3. Buffer latch release
    4. // Process
    5. Page latch release
- page latch를 곧바로 잡지 않는 것은 LRU로부터 page를 보호하기 위해 buffer를 독점해야 하기 때문이다.
- buffer의 수요는 page보다 높으므로, page latch를 얻은 후 곧바로 해제한다.

마지막으로 DBMS는 deadlock을 탐지하고 rollback해야 하는데, 우선 deadlock detect를 위해 DFS를 고려할 수 있다. deadlock이 발생하였을 경우 wait-for graph에서 사이클이 발생하는데, DFS를 사용하면 쉽게 그래프 내 사이클을 발견할 수 있기 때문이다.

# Design

우선 lock manager와 lock을 구현한다. 각각은 아래와 같다.

```cpp
struct lock_t
{
    lock_manager_entry_t* sentinel;
    pthread_cond_t cond;
    int lock_mode;
    int64_t table_id;
    int64_t record_id;
    int trx_id;
};

struct lock_manager_entry_t
{
    list<lock_t*> locks;
    int64_t table_id;
    pagenum_t page_num;
};

struct lock_manager_t
{
    unordered_map<string, lock_manager_entry_t*> lock_hash_table;
    lock_manager_entry_t* get_lock_manager_entry(int64_t table_id, pagenum_t page_id);
};
```
이는 Project 4에서 구현한 것과 크게 다르지 않다.

Transaction과 transaction manager는 아래와 같이 구현하였다.
```cpp
struct trx_t
{
    int trx_id;
    list<lock_t*> locks;
    bool is_aborted;
    vector<int> wait_for_edges;
    map<pair<int64_t, int64_t>, string> undo_log;
    void add_lock(lock_t* lock);
};

struct trx_manager_t
{
    int trx_counter;
    unordered_map<int, trx_t*> trx_hash_table;
    trx_t* get_trx_entry(int trx_id);
    trx_t* create_trx(void);
};
```
이를 도식화하면 아래 그림과 같다.

![trxmanager](/project5/resources/img1.PNG)


Rollback을 위해서는 트랜잭션이 어떤 변화를 가지는지 알 필요가 있다. 이를 위해 `trx_t`에서는 `undo_log`를 사용하여 rollback에 대비한다.

> 해당 프로젝트에서는 db_update에 의해 record 길이 변화와 그에 따른 side effects(page split 등)을 고려하지 않는다.