#include "file.h"

vector<pair<int64_t, header_page_t>> fds;


// ============================================================
// APIs
// ============================================================

// Open existing database file or create one if it doesn't exist
int64_t file_open_table_file(char* pathname)
{
    /* credit:  https://www.it-note.kr/19?category=1067584
     * O_RDWR:  read & write mode
     * O_CREAT: create file if not exists
     * O_SYNC:  syncronizing disk I/O
     */
    int64_t table_id = open(pathname, O_RDWR | O_CREAT | O_SYNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if (table_id < 0)  return -1;

    header_page_t* cur = new header_page_t;
    int tmp = pread(table_id, cur, PAGE_SIZE, 0);
    if (!cur->num_of_pages || tmp != 4096)
    {
        // initial process
        //// header page * 1
        cur->num_of_pages = INIT_PAGE_NUM;
        cur->free_num = 1;
        write_header_page(table_id, cur);

        //// free page * 2559
        for (int i = 1; i < INIT_PAGE_NUM; i++)
        {
            page_t* tmp = new page_t(-1);
            tmp->header.parent_num = (i == INIT_PAGE_NUM-1 ? 0 : i+1);
            file_write_page(table_id, i, tmp);
        }
        // header 0 -> 1 -> 2 ... -> 2559 -> 0
    }
    fds.push_back({table_id, *cur});

    return table_id;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id)
{
    header_page_t* header_page = new header_page_t;
    read_header_page(table_id, header_page);
    pagenum_t cur_num = header_page->free_num;
    page_t *cur_page = new page_t(-1);

    // no free page exists
    if (!cur_num)
    {
        // cout << "allocating more page: need some time...\n";
        /* if free page not exists,
         * create pages * (#pages)
         * and calculate free_num, num_of_pages
         */
        cur_num = header_page->num_of_pages;
        header_page->num_of_pages <<= 1;
        header_page->free_num = cur_num+1;
        for (int i = cur_num; i < header_page->num_of_pages; i++)
        {
            file_read_page(table_id, i, cur_page);
            cur_page->header.parent_num = i+1;
            file_write_page(table_id, i, cur_page);
        }
        cur_page->header.parent_num = 0;
        file_write_page(table_id, header_page->num_of_pages-1, cur_page);
    }
    else
    {
        /* if free page exists,
         * pop first free page
         * which is next to header page
         * and link next free page
         */
        file_read_page(table_id, cur_num, cur_page);
        header_page->free_num = cur_page->header.parent_num;
        write_header_page(table_id, header_page);
        delete cur_page, header_page;
        return cur_num;
    }

    // save
    write_header_page(table_id, header_page);
    delete cur_page, header_page;

    read_header_page(table_id, header_page);
    return cur_num;
}

// Free an on-disk page to the free page list
void file_free_page(int64_t table_id, pagenum_t page_num)
{
    // load from header page
    header_page_t* header_page = new header_page_t;
    read_header_page(table_id, header_page);

    page_t* cur = new page_t(-1);
    file_read_page(table_id, page_num, cur);

    /*
     * header -> next_free
     * header -> cur -> next_free
     */
    cur->header.parent_num = header_page->free_num;
    header_page->free_num = page_num;

    // save
    cur->header.is_leaf = -1;
    write_header_page(table_id, header_page);
    file_write_page(table_id, page_num, cur);
    delete cur, header_page;
    return ;
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int64_t table_id, pagenum_t page_num, page_t* dest)
{
    if (pread(table_id, dest, PAGE_SIZE, PAGE_SIZE*page_num) < 0)
    {
        puts("Error in file_read_page");
        exit(1);
    }
    return sync();
}

int cnt_write = 0;
// Write an in-memory page(src) to the on-disk page
void file_write_page(int64_t table_id, pagenum_t page_num, const page_t* src)
{
    // cout << __func__ << " " << ++cnt_write << endl;
    if (pwrite(table_id, src, PAGE_SIZE, PAGE_SIZE*page_num) < 0)
    {
        puts("Error in file_write_page");
        exit(1);
    }
    return sync();
}

// Close all database files
void file_close_table_files(void)
{
    for (auto& fd: fds) close(fd.ff);
    return ;
}

// ext
void read_header_page(int64_t table_id, header_page_t* header_page)
{
    if (pread(table_id, header_page, PAGE_SIZE, 0) < 0)
    {
        puts("Error in read_header_page");
        exit(1);
    }
    return ;
}

void write_header_page(int64_t table_id, header_page_t* header_page)
{
    if (pwrite(table_id, header_page, PAGE_SIZE, 0) < 0)
    {
        puts("Error in write_header_page");
        exit(1);
    }
    sync();
    return ;
}

void print_pages(int64_t tid)
{
    cout << "================================================\n";
    header_page_t header;
    read_header_page(tid, &header);
    cout << "header page:\n";
    cout << "\tnum_of_pages = " << header.num_of_pages << endl;
    cout << "\tfree page num = " << header.free_num << endl;
    cout << "\troot page num = " << header.root_num << endl;

    int cnt = header.num_of_pages;
    // cnt = 28;
    for (int i = 1; i < cnt; i++)
    {
        cout << "================================================\n";
        cout << i << "th page: ";
        page_t cur;
        file_read_page(tid, i, &cur);
        if (cur.header.is_leaf == -1)
        {
            cout << "is free page\n";
            cout << "\tnext free page: " << cur.header.parent_num << "\n";
        }
        else if (cur.header.is_leaf == 0)
        {
            cout << "is internal page\n";
            cout << "\tparent: " << cur.header.parent_num << "\n";
            cout << "\tkey & child" << "\n";
            cout << "\t\t\t" << cur.ip.left_num << endl;
            for (int j = 0; j < cur.header.num_of_keys; j++)
                cout << "\t\t" << cur.ip.edges[j].key << "\t" << cur.ip.edges[j].child_num << endl;
        }
        else
        {
            cout << "is leaf page\n";
            cout << "\tparent: " << cur.header.parent_num << endl;
            cout << "\tkey & size & offset" << "\n";
            memory_leaf_t memleaf = diskleaf_to_memleaf(cur);
            for (int j = 0; j < cur.header.num_of_keys; j++)
                cout << "\t\t" << memleaf.slots[j].key << "\t" << memleaf.slots[j].size << "\t" << memleaf.slots[j].offset << endl;
        }
    }
}
