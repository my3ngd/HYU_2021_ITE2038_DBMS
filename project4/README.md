# Project 4 Wiki

2020098240 이현빈

# 명세 요약

Project 4는 Project 2, 3과 무관하지만 Project 5(Concurrency Control)를 이해하기 위한 선행 프로젝트이다.

여기에서는 lock table 모듈을 구현하여 멀티스레드를 관리해야 한다.

아래와 같은 lock table API를 구현해야 한다.
- `int init_lock_table(void)`
- `lock_t* lock_acquire(int64_t table_id, int64_t key)`
- `int lock_release(lock_t* lock_obj)`

# Design

Lock table인 `lock_table_t`은 아래와 같이 구현하였다.

```cpp
struct lock_table_t
{
    unordered_map<string, hash_table_entry_t*> hash_table;
    hash_table_entry_t* get_hash_table_entry(int64_t table_id, int64_t record_id);
};
```
- `hash_table`을 사용하여 `hash_table_entry`에 빠르게 접근할 수 있는 lock table 본체이다.
- `get_hash_table_entry` 멤버함수는 `table_id`와 `record_id`로 `hash_table_entry`을 반환하는 함수이다.

```cpp
hash_table_entry_t* lock_table_t::get_hash_table_entry(int64_t table_id, int64_t record_id)
{
    string key = hashkey_converter(table_id, record_id);
    if (this->hash_table[key] == nullptr)
        this->hash_table[key] = new hash_table_entry_t(table_id, record_id);
    return this->hash_table[key];
}
```
위와 같이 쉽게 구현할 수 있다. `hashkey_converter`는 `table_id`와 `record_id`를 hash의 key로 사용하기 위해 문자열로 조합하는 함수이다.

`hash_table_entry_t`는 아래와 같이 구현되었다.
```cpp
struct hash_table_entry_t
{
    int64_t table_id;
    int64_t record_id;
    lock_t* head;
    lock_t* tail;
};
```
`hash_table_entry_t`에서는 linked list 형태로 lock이 연결된다. 따라서 이를 관리하기 위한 두 포인터 `head`, `tail`이 존재한다.

`lock_t`는 아래와 같이 구현되었다.
```cpp
struct lock_t
{
    lock_t* prev;
    lock_t* next;
    hash_table_entry_t* sentinel;
    pthread_cond_t cond;
};
```
- `prev`와 `next`는 lock을 연결하기 위한 포인터이다.
- `sentinel`은 해당 lock을 포함하는 hash table entry이다.
- `cond`는 condition variable이다.
    - `pthread_cond_wait`: 조건이 참이 될 때까지 대기하도록 한다.
    - `pthread_cond_signal`: 대기중인 스레드에 signal을 보낸다.

이를 이용하여 `lock_acquire()`와 `lock_release()`를 `mutex`로 보호하고, lock을 얻거나 해제할 수 있다.

따라서 전체적인 구조는 아래 그림과 같다.

![lockmanger](/project4/resources/img1.PNG)

하나의 lock entry의 구조는 아래 그림과 같다.

![lockmanger](/project4/resources/img2.PNG)
