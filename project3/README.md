# Project 3 Wiki

2020098240 이현빈

# 명세 요약

In-memory에서 동작하는 Buffer manager를 구현해야 한다. Buffer block은 다음과 같은 구조를 가진다.

![bufferstruct](/project3/resources/img1.png)

Buffer는 LRU(Least Recently Used) 방식으로 동작해야 한다. 이는 최근에 방문한 page에 더 자주 접근할 것으로 기대하는 '시간 지역성' 때문이다.

# Design

Buffer block은 명세에서 주어진 것과 같이 아래와 같이 설계하였다.
```cpp
struct control_block_t
{
    page_t frame;
    int64_t table_id;
    pagenum_t page_num;
    bool is_dirty;
    int pinned;
    control_block_t* next;
    control_block_t* prev;
};
```

Buffer block을 관리하는 Buffer는 아래와 같이 설계하였다.
```cpp
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
};
```
각 멤버변수는 아래와 같은 의미를 가진다.
- `head`: 첫 번째 buffer block
- `tail`: 마지막 buffer block
- `max_size`: buffer의 최대 크기
- `cur_size`: buffer의 현재 크기
- `dic`: `tid`와 `pagenum`을 key로 받아 control block을 가져오는 map
    - 매 접근마다 선형 탐색하는 대신, 블럭에 빠르게 접근하기 위함이다.
- `files`: `tid`를 key로 받아 해당 table의 header를 가져오는 map

첫 번째 원소와 마지막 원소의 삽입/삭제 연산을 중간 원소의 삽입/삭제와 같이 하기 위해, `head`와 `tail`은 아무 정보를 담지 않은 임의의 control block을 할당하여 사용한다. 아래는 `buffer_t`의 생성자이다.
```cpp
buffer_t::buffer_t(int64_t sz)
{
    this->max_size = sz;
    this->cur_size = 0;
    this->head = new control_block_t;
    this->tail = new control_block_t;
    this->head->next = this->tail;
    this->tail->prev = this->head;
}
```

![buffer](/project3/resources/img2.PNG)

따라서 buffer는 위 그림에서 두 번째와 같은 구조를 가진다. 프로젝트 명세에서 `prev of LRU`와 `next of LRU`를 사용해야 한다고 하였으므로, `std::list`를 사용하는 대신 직접 linked list 구조를 구현하였다.

Linked list의 삽입과 삭제는 잘 알려져 있듯이, 아래 그림과 같이 동작한다.

- 삽입
    ![buffer](/project3/resources/img3.PNG)

- 삭제
    ![buffer](/project3/resources/img4.PNG)

이와 같이 buffer를 구현하여 disk에 직접 접근하는 횟수를 획기적으로 줄여 속도 향상을 기대할 수 있다. 특히 buffer에서 파일을 '읽기만 할 경우'를 구분하여 `pinned++`를 하지 않았는데, 이를 통해 pin된 page 수를 줄일 수 있다.

이외의 기능은 명세를 따르면 어렵지 않게 구현할 수 있다.
