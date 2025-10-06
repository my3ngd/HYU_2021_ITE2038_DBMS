# Project 2 Wiki

2020098240 이현빈

# 명세 요약

## Milestone 1

DBMS의 상위 계층에서 사용할 아래 6가지 API를 구현해야 한다.

1. `int file_open_database_file(const char *pathname)`
    - DB 파일을 연다.
    - `pathname`에 해당하는 파일이 없으면 만든다.
    - `fd`를 반환한다.
    - 다른 5개의 API는 해당 API 사용 이후에 사용한다.

2. `uint64_t file_alloc_page(int fd)`
    - 페이지를 할당한다.
    - free page list에서 새 page를 가져오고 번호를 반환한다.
    - free page list가 비어있다면, DB file을 키우고 free page를 만들어 새 free page 번호를 반환한다.

3. `void file_free_page(int fd, uint64_t page_number)`
    - 할당된 page를 free한다.

4. `void file_read_page(int fd, uint64_t page_number, char *dest)`
    - disk에 있는 page를 in-memory page로 가져온다.

5. `void file_write_page(int fd, uint64_t page_number, const char *src)`
    - `src`에 있는 in-memory page를 disk에 기록한다.

6. `void file_close_database_file(void)`
    - DB 파일을 닫는다.

Milestone 1에서 모든 page는 4096Byte의 크기를 가지며, 종류에 따라 아래와 같은 구조를 가진다. 4096Byte의 크기를 가지도록 설정한 것은 현대 운영체제의 페이지 단위와 일치하도록 설정하여 빠른 속도를 기대한 것이다.
- Header page
    - Free page number (0 - 7)
    - Number of pages (8 - 15)
    - Reserved (16 - 4095)

- Free page
    - Next free page number (0 - 7)
    - Reserved (8 - 4095)

Header page는 DB 파일의 metadate를 가진다.

Free page는 Linked list 형태로 관리되며, 사용되는 방식은 stack과 동일하다.

## Milestone 2

Milestone 1에서 구현한 6개의 API를 활용하여, Disk-based B+tree를 구현해야 한다. 상위 계층에서 사용하기 위해 구현해야 하는 API는 아래와 같다.

1. `int64_t open_table(char *pathname)`
    - Milestone 1의 `open_table`과 동일

2. `int db_insert(int64_t table_id, int64_t key, char *value, uint16_t val_size)`
    - key-value 쌍(record)를 '올바른 위치'에 삽입해야 한다.
    - 성공 시 `0`을, 실패 시 non-zero를 반환한다.

3. `int db_find(int64_t table_id, int64_t key, char *ret_val, uint16_t *val_size)`
    - `key`를 이용하여 record를 찾아야 한다.
    - `key`를 찾으면, `ret_val`과 `val_size`에 적절한 값을 넣어야 한다.
    - 성공 시 `0`을, 실패 시 non-zero를 반환한다.
    - caller에서 메모리를 할당해야 한다.(`ret_val`)

4. `int db_delete(int64_t table_id, int64_t key)`
    - `key`를 이용하여 record를 제거한다.
    - 성공 시 `0`을, 실패 시 non-zero를 반환한다.

5. `int init_db(void)`
    - DBMS를 시작한다.
    - 성공 시 `0`을, 실패 시 non-zero를 반환한다.

6. `int shutdown_db(void)`
    - DBMS를 종료한다.
    - 성공 시 `0`을, 실패 시 non-zero를 반환한다.

Milestone 2에서 모든 page는 4096 Byte의 크기를 가지며, 종류에 따라 아래와 같은 구조를 가진다. 
- Header page
    - Free page number (0 - 7)
    - Number of pages (8 - 15)
    - Root page number (16 - 23)
- Free page
    - Next free page number (0 - 7)
- Leaf page
    - Page header (0 - 111)
    - Amount of free space (112 - 119)
    - Right sibling page number (120 - 127)
    - slot & value (128 - 4095)
        - slot: key(8 Byte), size(2 Byte), offset(2 Byte)
        - value: 50 - 112 Byte
- internal page
    - Page header (0 - 111)
    - Reserved (112 - 119)
    - One more page number (120 - 127)
    - Key + page number (128 - 4095)
        - key: 8 Byte
        - value: 8 Byte

그림으로 나타내면 아래와 같다.

![pagelayout](/project2/resources/img1.png)

---

B+tree rule

- Insertion rule for leaf pages
    - Free space가 충분한 경우: 삽입 외에 작업이 필요 없다.
    - Free space가 충분하지 않을 경우: split을 해야 한다. 이때 전체 크기의 절반 지점을 record 분할 지점으로 설정한다.

- Deletion rule for leaf pages
    - Free space가 threshold(2500 Byte)보다 작은 경우: 삭제 이외에 작업이 필요 없다.
    - Free space가 threshold(2500 Byte)보다 크거나 같은 경우: 삭제 대상이 있는 page가 merge되거나 재분배되어야 한다.
        - Merge: sibling에 충분한 공간이 있으면 실행한다.
        - Redistribute: sibling과 merge가 불가능할 경우 실행한다.

- Rules for internal pages
    - 동적 길이가 존재하지 않으므로, 일반적인 B+tree와 같이 동작한다.

몇 가지 예시를 그림으로 나타내면 아래와 같다.

![bptrule](/project2/resources/img2.png)

Milestone 1에서 작성한 6개의 API를 사용하여, 아래 6개 API를 구현해야 한다.
1. `int64_t file_open_table_file (const char *pathname)`
2. `uint64_t file_alloc_page (int64_t table_id)`
    - 새 page를 할당받아 해당 page의 page number를 반환한다.
3. `void file_free_page (int64_t table_id, uint64_t page_number)`
    - 사용중인 page를 free한다.
4. `void file_read_page (int64_t table_id, uint64_t page_number, char *dest)`
    - Disk에 있는 page를 memory로 가져온다.
5. `void file_write_page (int64_t table_id, uint64_t page_number, const char *src)`
    - Memory에 있는 page를 disk로 보낸다.
6. `void file_close_table_files(void)`

# Design

명세에 작성된대로 다음과 같은 구조체를 작성한다.

``` cpp
struct slot_t
{
    int64_t key;
    unsigned short size;
    unsigned short offset;
    // 생성자
};

struct record_t
{
    slot_t slot;
    string rec;
    // 생성자
};

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
    // 생성자
};

struct internal_page_t
{
    unsigned char RESERVED[120 - 112];
    pagenum_t left_num;
    edge_t edges[248];
    // 생성자
};

struct leaf_page_t
{
    int64_t free_space;
    pagenum_t right_num;
    unsigned char rec[4096 - 128];
    // 생성자
};

struct page_t
{
    page_header_t header;
    union
    {
        internal_page_t ip;
        leaf_page_t     lp;
    };
    // 생성자
};
```

`page_t`는 leaf page와 internal page로 사용될 수 있으므로, `union`을 사용하여 같은 메모리를 공유하도록 한다. (`page_t`의 크기를 4906B로 유지하기 위함)

Page를 메모리에서 쉽게 관리하기 위해 아래 구조체들을 설계하였다. 메모리에서 관리를 쉽게 하기 위한 용도로 설계되었으므로, `page_t`와의 전환이 자연스럽도록 추가로 함수를 설계할 필요가 있다.

```cpp
struct memory_leaf_t
{
    page_header_t header;
    pagenum_t right_num;
    int64_t free_space;

    deque<slot_t> slots;
    deque<string> records;
};

struct memory_internal_t
{
    page_header_t header;
    deque<pagenum_t> childs;
    deque<int64_t> keys;
};

memory_leaf_t diskleaf_to_memleaf(page_t);
page_t memleaf_to_diskleaf(memory_leaf_t);
memory_internal_t diskinternal_to_meminternal(page_t);
page_t meminternal_to_diskinternal(memory_internal_t);
```

B+tree의 주요 연산을 그림으로 나타내면 아래와 같다.

- merge
    ![merge](/project2/resources/img3.png)

- redistribute
    ![redistribute](/project2/resources/img4.png)

- split
    ![split](/project2/resources/img5.png)

따라서, 여러 경우를 고려하여 다음과 같이 함수들을 구현하였다.
- find
- insert
    - leaf에 insert한 이후 split이 발생하지 않는 경우
    - leaf에 insert한 이후 split이 발생하는 경우
    - parent에 새로운 key를 insert한 이후 split이 발생하지 않는 경우
    - parent에 새로운 key를 insert한 이후 split이 발생하는 경우
    - 새로운 root node가 생기는 경우
    - 새 B+tree를 시작하는 경우
- delete
    - node에서 record를 제거하는 경우
    - root node를 재조정하는 경우
    - leaf node를 rerdistribute하는 경우
    - internal node를 rerdistribute하는 경우
    - leaf node를 merge하는 경우
    - internal node를 merge하는 경우

B+tree에서 insert, delete 연산은 find 연산을 전제로 가지므로 find -> insert -> delete 순서로 구현하였다.

해당 프로젝트에서는 page에서 key를 찾을 때 이분 탐색을 활용하였으나, 하나의 leaf/internal page에 존재할 수 있는 key의 개수는 매우 한정적이므로 적절한 key를 찾는 연산을 선형 탐색으로 구현하여도 성능상으로 크게 문제되지 않는다. 이는 disk와 memory간에 page를 이동하는 것이 훨씬 느리기 때문에 전체에서 key를 찾는 연산이 차지하는 시간이 적게 차지하기 때문이다.

