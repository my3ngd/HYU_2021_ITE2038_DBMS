#include "buffer.h"

bool buffer_exist;
buffer_t* buffer;

// open table
int64_t open_table(char* pathname)
{
    // too many tables
    if (buffer->files.size() == 19)  return -1;
    // name duplicated
    for (int i = 0; i < buffer->names.size(); i++)
        if (string(pathname) == buffer->names[i])
            return -1;
    int64_t fd = file_open_table_file(pathname);
    // open error
    if (fd < 0)
        return -1;

    // no problem
    buffer->names.push_back(string(pathname));
    header_page_t header;
    read_header_page(fd, &header);
    buffer->files[fd] = header;
    return fd;
}

/* init_db
 * initialize DBMS
 * initialize and allocate anything you need
 */
int init_db(int num_buf)
{
    buffer = new buffer_t(num_buf);
    if (buffer_exist)   return 1;
    buffer_exist = true;
    return 0;
}

/* shutdown_db
 * shutdown DBMS
 * write all pages
 */
int shutdown_db(void)
{
    if (!buffer_exist)
        return -1;

    buffer_exist = false;

    // about header page
    for (auto& x: buffer->files)
        write_header_page(x.ff, &(x.ss));

    // about B+ tree pages
    delete buffer;
    file_close_table_files();
    return 0;
}


// constructors, structure operators

control_block_t::control_block_t(void)
{
    this->frame = page_t();
    this->table_id = 0;
    this->page_num = 0;
    this->is_dirty = 0;
    this->pinned = 0;
    this->next = nullptr;
    this->prev = nullptr;
}

buffer_t::buffer_t(int64_t sz)
{
    this->max_size = sz;
    this->cur_size = 0;
    this->head = new control_block_t;
    this->tail = new control_block_t;
    this->head->next = this->tail;
    this->tail->prev = this->head;
}

buffer_t::~buffer_t()
{
    control_block_t* cur = this->head;
    control_block_t* pre = nullptr;
    while (cur != nullptr)
    {
        // cout << __func__ << " " << cur->page_num << endl;
        if (cur->page_num && cur->is_dirty)
            file_write_page(cur->table_id, cur->page_num, &(cur->frame));
        pre = cur;
        cur = cur->next;
        delete pre;
    }
}

/* load page
 * put {tid, pnum} to first
 */
void buffer_t::load_page(int64_t tid, pagenum_t pnum)
{
    // target page not found in buffer
    if (this->dic.find({tid, pnum}) == this->dic.end())
    {
        // need to pop LRU block
        if (this->cur_size == this->max_size)
            this->remove_back();

        // create new ctrl block
        control_block_t* block = new control_block_t;
        file_buffer_read(tid, pnum, block);
        // connect front (head <-> block <-> old ...)
        control_block_t* old = this->head->next; // old front
        this->head->next = block;
        block->prev = this->head;
        // about old
        block->next = old;
        old->prev = block;
        // about dic
        this->dic[{tid, pnum}] = block;
        this->cur_size++;
    }
    // target found in buffer
    else
    {
        control_block_t* block = this->dic[{tid, pnum}];

        // cut block
        control_block_t* ll = block->prev;
        control_block_t* rr = block->next;
        ll->next = rr;
        rr->prev = ll;
        control_block_t* old = buffer->head->next;
        buffer->head->next = block;
        block->prev = buffer->head;
        block->next = old;
        old->prev = block;
    }
    assert(buffer->dic.size() == buffer->cur_size);
    return ;
}

/*
 * not just about pointers,
 * it includes about
 * map, size...
 */
void buffer_t::remove_back(void)
{
    // cout << __func__ << " " << __LINE__ << endl;
    assert(this->dic.size() == this->cur_size);

    control_block_t* cur = this->tail->prev;
    for (; cur != this->head && cur->pinned; cur = cur->prev);

    if (cur == this->head)
    {
        cout << "All control blocks are pinned. Check buffer size. Program terminated." << endl;
        exit(1);
    }

    assert(cur->pinned == 0);

    control_block_t* le = cur->prev;
    control_block_t* ri = cur->next;

    le->next = ri;
    ri->prev = le;

    this->dic.erase({cur->table_id, cur->page_num});
    this->cur_size--;

    file_write_page(cur->table_id, cur->page_num, &(cur->frame));

    delete cur;

    assert(this->dic.size() == this->cur_size);
    // cout << __func__ << " " << __LINE__ << endl;
}


// buffer functions

// page(tid, pnum) to ctrl_block(dest)
void file_buffer_read(int64_t tid, pagenum_t pnum, control_block_t* dest)
{
    file_read_page(tid, pnum, &(dest->frame));
    dest->table_id = tid;
    dest->page_num = pnum;
}

// write header page, buffer -> disk
void write_buffer_header(int64_t tid)
{
    write_header_page(tid, &(buffer->files[tid]));
}

// read header page, buffer <- disk
void read_buffer_header(int64_t tid)
{
    read_header_page(tid, &(buffer->files[tid]));
}

// get tid -> header page
void buffer_get_header(int64_t tid, header_page_t& header)
{
    header = buffer->files[tid];
}

void buffer_set_header(int64_t tid, header_page_t& header)
{
    buffer->files[tid] = header;
}


// buffer alloc/free/read

/* buffer_alloc_page
 * allocate new page
 * replace "file_alloc_page"
 * one by one
 */
pagenum_t buffer_alloc_page(int64_t tid)
{
    write_buffer_header(tid);
    pagenum_t res = file_alloc_page(tid);
    read_buffer_header(tid);
    // cout << res << " -> res" << endl;

    // buffer full
    if (buffer->cur_size == buffer->max_size)
        buffer->remove_back();

    control_block_t* block = new control_block_t;
    control_block_t* old = buffer->head->next;
    buffer->dic[{tid, res}] = block;
    buffer->cur_size++;

    block->table_id = tid;
    block->page_num = res;

    // new first block
    buffer->head->next = block;
    block->prev = buffer->head;
    // old first block
    block->next = old;
    old->prev = block;

    // allocated block may dirty
    block->is_dirty = true;
    // allocated block may pinned
    block->pinned++;

    return res;
}

/* buffer_free_page
 * free page
 */
void buffer_free_page(int64_t tid, pagenum_t page_num)
{
    assert(buffer->dic.size() == buffer->cur_size);
    buffer->load_page(tid, page_num);
    assert(buffer->dic.size() == buffer->cur_size);

    write_buffer_header(tid);
    file_free_page(tid, page_num);
    read_buffer_header(tid);

    // delete control block
    control_block_t* target = buffer->head->next;
    control_block_t* first = target->next;
    // connection
    buffer->head->next = first;
    first->prev = buffer->head;
    buffer->cur_size--;
    buffer->dic.erase({tid, page_num});

    delete target;
    assert(buffer->dic.size() == buffer->cur_size);
}

/* buffer_read_page
 * includes ::pined++
 * buffer -> index (page)
 */
void buffer_read_page(int64_t tid, pagenum_t page_num, page_t& page)
{
    assert(buffer->dic.size() == buffer->cur_size);
    buffer->load_page(tid, page_num);
    control_block_t* block = buffer->head->next;
    page = block->frame;
    block->pinned++;
    assert(buffer->dic.size() == buffer->cur_size);
}

/* buffer_read_only_page
 * not pinned because not necessary
 * buffer -> index (page, read only)
 */
void buffer_read_only_page(int64_t tid, pagenum_t page_num, page_t& page)
{
    assert(buffer->dic.size() == buffer->cur_size);
    buffer->load_page(tid, page_num);
    control_block_t* block = buffer->head->next;
    page = block->frame;
    assert(buffer->dic.size() == buffer->cur_size);
}

/* buffer_write_page
 * includes ::pinned--
 * index -> buffer (page)
 */
void buffer_write_page(int64_t tid, pagenum_t page_num, page_t& page)
{
    assert(buffer->dic.size() == buffer->cur_size);
    buffer->load_page(tid, page_num);
    control_block_t* block = buffer->head->next;
    block->frame = page;
    block->pinned--;
    // must not happen
    if (block->pinned < 0)
        block->pinned = 0;
    block->is_dirty = true;
    assert(buffer->dic.size() == buffer->cur_size);
}

// ============================================================================================================
// Debug

void print_buffer(void)
{
    cout << "====================================================================================" << endl;
    cout << "size = " << buffer->cur_size << endl;
    control_block_t* cur = buffer->head->next;
    for (int cnt = 0; cur->next != nullptr; cur = cur->next)
    {
        if (cur->next == cur)assert(false);
        if (cnt > 5) return ;
        cout << "-------------------------------------------" << endl;
        cout << "buffer \t" << cnt++ << endl;
        cout << "dirty  \t" << cur->is_dirty << endl;
        cout << "pg num \t" << cur->page_num << endl;
        cout << "tid    \t" << cur->table_id << endl;
        cout << "pinned \t" << cur->pinned   << endl;
    }
    cout << "====================================================================================" << endl;
}

void print_map(void)
{
    cout << "====================================================================================" << endl;
    cout << buffer->dic.size() << endl;
    for (auto& x: buffer->dic)
    {
        cout << x.ff.ff << " " << x.ff.ss << " = " << x.ss->page_num << endl;
    }
    cout << "====================================================================================" << endl;
}

int count_pin(void)
{
    int res = 0;
    control_block_t* cur = buffer->head->next;
    for (int i = 0; i < buffer->cur_size; i++)
    {
        if (cur->pinned)
        {
            cout << cur->table_id << "\t" << cur->page_num << "\t" << cur->pinned << endl;
            res++;
        }
        cur = cur->next;
    }
    if (res)
        exit(1);
    return res;
}