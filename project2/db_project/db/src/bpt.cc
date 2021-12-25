#include "bpt.h"
#include <iostream>
#include <queue>
#include <assert.h>
#include <fstream>
#define endl '\n'

// for print
using std::queue;
// =================================================================
// Print
// =================================================================

/* print leaves
 * print all leaf nodes
 */
void print_leaves(int64_t tid)
{
    std::ofstream fs;
    fs.open("output.txt", std::ios::app);

    header_page_t header;
    read_header_page(tid, &header);
    page_t cur;
    pagenum_t cur_num = header.root_num;
    file_read_page(tid, cur_num, &cur);
    // goto bottom-leftmost (leaf) node
    while (!cur.header.is_leaf)
    {
        cur_num = cur.ip.left_num;
        file_read_page(tid, cur_num, &cur);
    }
    std::set<pagenum_t> s;
    // now, cur.header.is_leaf = 1
    // print and go right
    for (memory_leaf_t ml; cur_num; file_read_page(tid, cur_num, &cur))
    {
        // print page data simply
        fs << "[" << cur.header.parent_num << "\\" << cur_num << "\\" << cur.header.num_of_keys << "]:";
        ml = diskleaf_to_memleaf(cur);

        assert(ml.slots.size() == ml.header.num_of_keys);
        for (int i = 0; i < ml.header.num_of_keys; i++)
        {
            fs << ml.slots[i].key << " ";
            // need extra? () options!
        }
        cur_num = cur.lp.right_num;
        file_read_page(tid, cur_num, &cur);
        if (s.count(cur_num))   assert(false);
        s.insert(cur_num);
        fs << endl;
    }
    fs << "\n";
}

/* get_height
 * get the number of edges
 * to current page to root page
 * ex: get_height(tid, root_page) = 0
 */
int get_height(int64_t tid, pagenum_t page_num)
{
    int h = 0; // return value
    page_t cur;
    file_read_page(tid, page_num, &cur);
    // while existing parent node
    for (; cur.header.parent_num; h++)
    {
        page_num = cur.header.parent_num;
        file_read_page(tid, page_num, &cur);
    }
    return h;
}

/* print_tree
 * print whole tree
 */
void print_tree(int64_t tid)
{
    header_page_t header;
    read_header_page(tid, &header);
    if (!header.root_num)
        return ;
    page_t cur;

    queue<pagenum_t> q;
    q.push(header.root_num);

    int tmp_h = 0, cur_h;
    while (q.size())
    {
        pagenum_t cur_num = q.front();
        q.pop();
        file_read_page(tid, cur_num, &cur);
        cur_h = get_height(tid, cur_num);
        if (cur_h != tmp_h)     tmp_h = cur_h;
        if (cur.header.is_leaf) return print_leaves(tid);

        q.push(cur.ip.left_num);
        for (int i = 0; i < cur.header.num_of_keys; i++)
            q.push(cur.ip.edges[i].child_num);
    }
}

// =================================================================
// Find
// =================================================================

/* find_leaf
 * find the leaf node
 * which "may" contain [key]
 * find_leaf dosn't check
 * whether leaf has key or not.
 * (and db_find does.)
 */
pagenum_t find_leaf(int64_t tid, int64_t key)
{
    header_page_t header;
    read_header_page(tid, &header);
    page_t cur;

    // start with root node,
    pagenum_t cur_num = header.root_num;
    file_read_page(tid, cur_num, &cur);

    // go down using binary search
    while (!cur.header.is_leaf)
    {
        // leftmost case
        if (key < cur.ip.edges[0].key)
            cur_num = cur.ip.left_num;
        // rightmost case
        else if (cur.ip.edges[cur.header.num_of_keys-1].key <= key)
            cur_num = cur.ip.edges[cur.header.num_of_keys-1].child_num;
        // binary search
        else
        {
            int LL = 0,
                RR = cur.header.num_of_keys-2,
                MM;
            while (LL <= RR)
            {
                MM = LL + RR >> 1;
                // target
                if (cur.ip.edges[MM].key <= key && key < cur.ip.edges[MM+1].key)
                {
                    cur_num = cur.ip.edges[MM].child_num;
                    break;
                }
                else if (key < cur.ip.edges[MM].key)
                    RR = MM;
                else
                    LL = MM + 1;
            }
        }
        file_read_page(tid, cur_num, &cur);
    }
    assert(cur.header.is_leaf);
    return cur_num;
}

/* db_find
 * master function for [find] operation
 * The “caller” should allocate memory for a record structure (ret_val).
 */
int db_find(int64_t tid, int64_t key, char *ret_val, uint16_t* val_size)
{
    page_t leaf;
    pagenum_t leaf_num = find_leaf(tid, key);
    file_read_page(tid, leaf_num, &leaf);
    memory_leaf_t mleaf = diskleaf_to_memleaf(leaf);
    // empty page
    if (!mleaf.header.num_of_keys)  return 1;

    // binary search
    int LL = 0, MM, RR = mleaf.header.num_of_keys-1;
    // rightmost case
    if (mleaf.slots[RR].key == key)
    {
        memset(ret_val, 0, mleaf.records[RR].size()+1);
        memcpy(ret_val, mleaf.records[RR].c_str(), mleaf.records[RR].size());
        *val_size = mleaf.records[RR].size();
        return 0;
    }
    while (LL < RR)
    {
        MM = LL + RR >> 1;
        if (mleaf.slots[MM].key == key)
        {
            memset(ret_val, 0, mleaf.records[MM].size()+1);
            memcpy(ret_val, mleaf.records[MM].c_str(), mleaf.records[MM].size());
            *val_size = mleaf.records[MM].size();
            return 0;
        }
        else if (mleaf.slots[MM].key > key) RR = MM;
        else                                LL = MM + 1;
    }
    return 1;
}
// =================================================================
// Insert
// =================================================================
/* get_insert_idx
 * helper function for insert_into_parent
 */
int get_insert_idx(int64_t tid, page_t& pa, pagenum_t le_num)
{
    int ret = 0;
    if (le_num == pa.ip.left_num)
        return 0;
    for (int i = 0; i < pa.header.num_of_keys; i++)
        if (le_num == pa.ip.edges[i].child_num)
            return i+1;
    assert(false);
}

/* insert_into_leaf
 * insert to leaf node WITHOUT split
 * the calling condition is (val_size >= free_space)
 */
void insert_into_leaf(int64_t tid, pagenum_t leaf_num, page_t& leaf, record_t rec)
{
    memory_leaf_t ml = diskleaf_to_memleaf(leaf);
    ml.free_space -= rec.slot.size + 12;

    // now insert
    ml.slots.push_back(rec.slot);
    ml.records.push_back(rec.rec);
    for (int i = ml.header.num_of_keys; i > 0; i--)
    {
        if (ml.slots[i].key < ml.slots[i-1].key)
        {
            swap(ml.slots[i], ml.slots[i-1]);
            swap(ml.records[i], ml.records[i-1]);
        }
        else break;
    }
    ml.header.num_of_keys++;

    // insertion end
    leaf = memleaf_to_diskleaf(ml);
    file_write_page(tid, leaf_num, &leaf);

    return ;
}

/* insert_into_leaf_after_splitting
 * insert to leaf node WITH split
 */
void insert_into_leaf_after_splitting(int64_t tid, pagenum_t lleaf_num, page_t& lleaf, record_t rec)
{
    page_t rleaf(1);
    pagenum_t rleaf_num = file_alloc_page(tid);
    memory_leaf_t lml = diskleaf_to_memleaf(lleaf),
                  rml = diskleaf_to_memleaf(rleaf);
    // insert
    lml.slots.push_back(rec.slot);
    lml.records.push_back(rec.rec);
    // sort
    for (int i = lml.header.num_of_keys; i > 0; i--)
    {
        if (lml.slots[i].key < lml.slots[i-1].key)
        {
            swap(lml.slots[i], lml.slots[i-1]);
            swap(lml.records[i], lml.records[i-1]);
        }
    }
    lml.header.num_of_keys++;
    // split point
    int space = 0, split = 0, max_space = 1984;
    for (int i = 0; space < max_space; i++, split++)
    {
        // space + lml.slots[i].size == max_space -> break!
        if (space + lml.slots[i].size < max_space)  space += lml.slots[i].size;
        else break;
    }
    // about rml (rleaf)
    for (int i = split; i < lml.header.num_of_keys; i++)
    {
        rml.slots.push_back(lml.slots[i]);
        rml.records.push_back(lml.records[i]);
        rml.header.num_of_keys++;
    }
    // about lml (lleaf)
    for (int i = 0; i < rml.header.num_of_keys; i++)
    {
        lml.slots.pop_back();
        lml.records.pop_back();
        lml.header.num_of_keys--;
    }
    int64_t key_up = rml.slots.front().key;

    // connections
    rml.header.is_leaf = 1;
    rml.header.parent_num = lml.header.parent_num;
    rml.right_num = lml.right_num;
    lml.right_num = rleaf_num;

    lleaf = memleaf_to_diskleaf(lml);
    rleaf = memleaf_to_diskleaf(rml);
    file_write_page(tid, lleaf_num, &lleaf);
    file_write_page(tid, rleaf_num, &rleaf);

    header_page_t header;
    read_header_page(tid, &header);

    return insert_into_parent(tid, lml.header.parent_num, lleaf_num, rleaf_num, lleaf, rleaf, key_up);
}

/* insert_into_parent
 * switch function
 * which choose recursive or not
 */
void insert_into_parent(int64_t tid, pagenum_t pa_num, pagenum_t le_num, pagenum_t ri_num, page_t& le, page_t& ri, int64_t key)
{
    // case 0: new root
    if (!pa_num)
        return insert_into_new_root(tid, le_num, ri_num, le, ri, key);
    
    page_t pa(0);
    file_read_page(tid, pa_num, &pa);
    int insert_idx = get_insert_idx(tid, pa, le_num);

    if (pa.header.num_of_keys < BRANCHING_FACTOR-1)
        insert_into_internal(tid, pa_num, ri_num, pa, ri, key, insert_idx);
    else
        insert_into_internal_after_splitting(tid, pa_num, ri_num, pa, ri, key, insert_idx);
}

/* insert_into_internal
 * not work recursivly
 */
void insert_into_internal(int64_t tid, pagenum_t pa_num, pagenum_t ri_num, page_t& pa, page_t& ri, int64_t key, int idx)
{
    // shift
    for (int i = pa.header.num_of_keys; i > idx; i--)
        pa.ip.edges[i] = pa.ip.edges[i-1];
    // insert
    pa.header.num_of_keys++;
    pa.ip.edges[idx] = edge_t(key, ri_num);
    // save
    file_write_page(tid, pa_num, &pa);
    file_write_page(tid, ri_num, &ri);

    return ;
}

/* insert_into_internal_after_splitting
 * not work recursivly
 */
void insert_into_internal_after_splitting(int64_t tid, pagenum_t pa_num, pagenum_t ri_num, page_t& pa, page_t& ri, int64_t key, int idx)
{
    file_write_page(tid, ri_num, &ri);

    page_t ne(0),  // must be internal
           ch(0);  // i don't know too (but not matter!)
    edge_t tmp_edge[250];
    memset(tmp_edge, 0, sizeof(tmp_edge));

    // insert point
    for (int i = 0, j = 0; i < pa.header.num_of_keys; i++, j++)
    {
        if (j == idx)   j++;
        tmp_edge[j] = pa.ip.edges[i];
    }
    // insert
    tmp_edge[idx] = edge_t(key, ri_num);

    // split
    // about pa
    int split = BRANCHING_FACTOR >> 1;
    pa.header.num_of_keys = split;
    for (int i = 0; i < BRANCHING_FACTOR-1; i++)
        pa.ip.edges[i] = (i < split ? tmp_edge[i] : edge_t());
    // about ne
    ne.ip.left_num = tmp_edge[split].child_num;
    ne.header.num_of_keys = BRANCHING_FACTOR - split - 1;
    for (int i = split+1, j = 0; tmp_edge[i].child_num; i++, j++)
        ne.ip.edges[j] = tmp_edge[i];

    // connections
    ne.header.parent_num = pa.header.parent_num;

    // save
    pagenum_t ch_num = ne.ip.left_num,
              ne_num = file_alloc_page(tid);
    file_write_page(tid, pa_num, &pa);
    file_write_page(tid, ne_num, &ne);

    // about childs
    // ne's leftmost child
    file_read_page(tid, ch_num, &ch);
    ch.header.parent_num = ne_num;
    file_write_page(tid, ch_num, &ch);
    // ne's other children
    for (int i = 0; i < ne.header.num_of_keys; i++)
    {
        ch_num = ne.ip.edges[i].child_num;
        file_read_page(tid, ch_num, &ch);
        ch.header.parent_num = ne_num;
        file_write_page(tid, ch_num, &ch);
    }

    return insert_into_parent(tid, pa.header.parent_num, pa_num, ne_num, pa, ne, tmp_edge[split].key);
}
/* insert_into_new_root
 * called when need to split old-node
 */
void insert_into_new_root(int64_t tid, pagenum_t le_num, pagenum_t ri_num, page_t& le, page_t& ri, int64_t key)
{
    page_t pa(0);   // must be internal node
    pagenum_t pa_num = file_alloc_page(tid);
    header_page_t header;
    read_header_page(tid, &header);
    // about header page
    header.root_num = pa_num;

    // about pa (== new root)
    pa.header.parent_num = 0;
    pa.header.num_of_keys = 1;
    pa.ip.left_num = le_num;
    pa.ip.edges[0] = edge_t(key, ri_num);

    // about le, ri
    le.header.parent_num = pa_num;
    ri.header.parent_num = pa_num;

    // save
    write_header_page(tid, &header);
    file_write_page(tid, pa_num, &pa);
    file_write_page(tid, le_num, &le);
    file_write_page(tid, ri_num, &ri);

    read_header_page(tid, &header);

    return ;
}

/* start_tree
 * if tree not exists, create a new tree
 */
void start_tree(int64_t tid, record_t rec)
{    // about root page
    page_t root(1);  // it is also leaf page
    pagenum_t root_num = file_alloc_page(tid);   // alloc page
    insert_into_leaf(tid, root_num, root, rec);  // write & save data

    // about header page
    header_page_t header;
    read_header_page(tid, &header);
    header.root_num = root_num;

    // save header page
    write_header_page(tid, &header);

    return ;
}

/* db_insert()
 * master function for [insert] operation
 */
int db_insert(int64_t tid, int64_t key, char* value, uint16_t val_size)
{
    header_page_t header;
    read_header_page(tid, &header);

    // case 0: key already exists
    char tmp[112];
    uint16_t tmp_size;
    if (!db_find(tid, key, tmp, &tmp_size)) return 1;

    // case 1: tree not exists
    if (!header.root_num)
    {
        start_tree(tid, record_t(key, value, val_size));
        return 0;
    }

    page_t leaf;
    pagenum_t leaf_num = find_leaf(tid, key);
    file_read_page(tid, leaf_num, &leaf);

    memory_leaf_t mleaf = diskleaf_to_memleaf(leaf);

    // case 2: can insert to that leaf node
    if (val_size+12 <= mleaf.free_space) // 12: slot size
        insert_into_leaf(tid, leaf_num, leaf, record_t(key, value, val_size));
    // case 3: or not
    else
        insert_into_leaf_after_splitting(tid, leaf_num, leaf, record_t(key, value, val_size));
    return 0;
}

// =================================================================
// Delete
// =================================================================

/* helper function for delete_entry
 * get caller's index
 * return -1                 if caller is the leftmost page
 * return non-negative value if caller is not leftmost page
 * assertion false           if catch error
 */
int get_my_idx(int64_t tid, pagenum_t page_num, page_t& page)
{
    page_t pa;
    file_read_page(tid, page.header.parent_num, &pa);
    if (pa.ip.left_num == page_num)
        return -1;
    for (int i = 0; i < pa.header.num_of_keys; i++)
        if (pa.ip.edges[i].child_num == page_num)
            return i;
    assert(false);
}

/* remove_entry_from_node
 * remove entry from one node
 * not call recursivly itself
 * (because it don't have to be.)
 */
void remove_entry_from_node(int64_t tid, pagenum_t page_num, page_t& page, int64_t key)
{
    int idx = 0;
    // leaf case
    if (page.header.is_leaf)
    {
        memory_leaf_t mleaf = diskleaf_to_memleaf(page);
        // remove record if found
        deque<slot_t>::iterator s = mleaf.slots.begin();
        deque<string>::iterator r = mleaf.records.begin();
        for (; idx < mleaf.header.num_of_keys; idx++, s++, r++)
            if (mleaf.slots[idx].key == key)
                break;
        // target exists
        if (idx < mleaf.header.num_of_keys)
        {
            mleaf.slots.erase(s);
            mleaf.records.erase(r);
            mleaf.header.num_of_keys--;
        }
        // autoset offset, free_space
        page = memleaf_to_diskleaf(mleaf);
    }
    // internal case
    else
    {
        // move data
        for (; page.ip.edges[idx].key != key; idx++);
        for (idx++; idx < page.header.num_of_keys; idx++)
            page.ip.edges[idx-1] = page.ip.edges[idx];
        page.header.num_of_keys--;
        // remove
        int& back = page.header.num_of_keys;
        page.ip.edges[back] = edge_t();
    }
    // save & end
    file_write_page(tid, page_num, &page);
    return ;
}

/* adjust_root
 * called when root node touched
 */
void adjust_root(int64_t tid, pagenum_t root_num, page_t& root)
{
    // case 0: normal case (tree exists, root node not changes)
    if (root.header.num_of_keys > 0)
        return file_write_page(tid, root_num, &root);

    header_page_t header;
    read_header_page(tid, &header);

    // case 1: tree exists, remove root; root changes
    pagenum_t new_root_num = 0;
    if (!root.header.is_leaf)
    {
        /* in this case, root node has
         * only one child (== leftmost child)
         */
        page_t new_root;
        new_root_num = root.ip.left_num;
        file_read_page(tid, new_root_num, &new_root);
        new_root.header.parent_num = 0;
        file_write_page(tid, new_root_num, &new_root);
    }
    // case2: empty tree; run below code
    header.root_num = new_root_num;
    write_header_page(tid, &header);
    file_free_page(tid, root_num);
    return ;
}

/* redistribute_leaf
 * called when cannot merge(my, ne)
 * this function for only leaf pages
 */
void redistribute_leaf(int64_t tid, pagenum_t my_num, pagenum_t ne_num, page_t& my, page_t& ne, int my_idx, int ne_idx)
{
    page_t pa;
    pagenum_t pa_num = my.header.parent_num;
    file_read_page(tid, pa_num, &pa);

    memory_leaf_t mmy = diskleaf_to_memleaf(my),
                  mne = diskleaf_to_memleaf(ne);
    // my | ne -> my.push_back(ne.front())
    if (my_idx < 0)
    {
        mmy.slots.push_back(mne.slots.front());
        mmy.records.push_back(mne.records.front());
        mne.slots.pop_front();
        mne.records.pop_front();
        pa.ip.edges[0].key = mne.slots.front().key;
    }
    // ne | my -> my.push_front(ne.back())
    else
    {
        mmy.slots.push_front(mne.slots.back());
        mmy.records.push_front(mne.records.back());
        mne.slots.pop_back();
        mne.records.pop_back();
        pa.ip.edges[my_idx].key = mmy.slots.front().key;
    }
    mmy.header.num_of_keys++;
    mne.header.num_of_keys--;
    // save
    file_write_page(tid, pa_num, &pa);
    my = memleaf_to_diskleaf(mmy);  file_write_page(tid, my_num, &my);
    ne = memleaf_to_diskleaf(mne);  file_write_page(tid, ne_num, &ne);
    if (mmy.free_space >= THRESHOLD)
        return redistribute_leaf(tid, my_num, ne_num, my, ne, my_idx, ne_idx);
    return ;
}

/* redistribute_internal
 * called when cannot merge(my, ne)
 * this function for only internal pages
 */
void redistribute_internal(int64_t tid, pagenum_t my_num, pagenum_t ne_num, page_t& my, page_t& ne, int my_idx, int ne_idx)
{
    page_t pa, ch;
    pagenum_t pa_num = my.header.parent_num,
              ch_num;
    file_read_page(tid, pa_num, &pa);

    memory_internal_t mmy = diskinternal_to_meminternal(my),
                      mne = diskinternal_to_meminternal(ne),
                      mpa = diskinternal_to_meminternal(pa);

    // my | ne -> my.push_back(ne.front())
    if (my_idx < 0)
    {
        ch_num = mne.childs.front();
        mmy.keys.push_back(mpa.keys.front());
        mmy.childs.push_back(mne.childs.front());
        mpa.keys.front() = mne.keys.front();
        mne.keys.pop_front();
        mne.childs.pop_front();
    }
    // ne | my -> my.push_front(ne.back())
    else
    {
        ch_num = mne.childs.back();
        mmy.keys.push_front(mpa.keys[my_idx]);
        mmy.childs.push_front(mne.childs.back());
        mpa.keys[my_idx] = mne.keys.back();
        mne.keys.pop_back();
        mne.childs.pop_back();
    }
    mmy.header.num_of_keys++;
    mne.header.num_of_keys--;

    file_read_page(tid, ch_num, &ch);
    ch.header.parent_num = my_num;
    file_write_page(tid, ch_num, &ch);

    pa = meminternal_to_diskinternal(mpa);  file_write_page(tid, pa_num, &pa);
    my = meminternal_to_diskinternal(mmy);  file_write_page(tid, my_num, &my);
    ne = meminternal_to_diskinternal(mne);  file_write_page(tid, ne_num, &ne);

    return ;
}

/* merge_leaf
 * "comment"
 */
void merge_leaf(int64_t tid, pagenum_t my_num, pagenum_t ne_num, page_t& my, page_t& ne, int my_idx, int ne_idx)
{
    page_t pa;
    pagenum_t pa_num = my.header.parent_num;
    file_read_page(tid, pa_num, &pa);

    int64_t key_up;
    memory_leaf_t mmy = diskleaf_to_memleaf(my),
                  mne = diskleaf_to_memleaf(ne);
    // my | ne -> [ne] is absorbed by [my]
    if (my_idx < 0)
    {
        while (mne.slots.size() || mne.records.size())
        {
            // move all data from [ne] -> [my]
            mmy.slots.push_back(mne.slots.front());
            mmy.records.push_back(mne.records.front());
            mne.slots.pop_front();
            mne.records.pop_front();
        }
        key_up = pa.ip.edges[0].key;  // ne_idx == 0
        my = memleaf_to_diskleaf(mmy);
        my.lp.right_num = ne.lp.right_num;
        file_write_page(tid, my_num, &my);
        file_free_page(tid, ne_num);
    }
    // ne | my -> [my] is absorbed by [ne]
    else
    {
        while (mmy.slots.size() || mmy.records.size())
        {
            // move all data from [my] -> [ne]
            mne.slots.push_back(mmy.slots.front());
            mne.records.push_back(mmy.records.front());
            mmy.slots.pop_front();
            mmy.records.pop_front();
        }
        key_up = pa.ip.edges[my_idx].key;
        ne = memleaf_to_diskleaf(mne);
        ne.lp.right_num = my.lp.right_num;
        file_write_page(tid, ne_num, &ne);
        file_free_page(tid, my_num);
    }
    return delete_entry(tid, pa_num, key_up);
}

/* merge_internal
 * "comment"
 */
void merge_internal(int64_t tid, pagenum_t my_num, pagenum_t ne_num, page_t& my, page_t& ne, int my_idx, int ne_idx)
{
    page_t pa, ch;
    pagenum_t ch_num, pa_num = my.header.parent_num;
    file_read_page(tid, pa_num, &pa);
    int64_t key_up;

    memory_internal_t mmy = diskinternal_to_meminternal(my),
                      mne = diskinternal_to_meminternal(ne),
                      mpa = diskinternal_to_meminternal(pa);

    // my | ne -> [ne] is absorbed by [my]
    if (my_idx < 0)
    {
        key_up = mpa.keys.front();
        mmy.keys.push_back(mpa.keys.front());
        while (mne.childs.size())
        {
            mmy.childs.push_back(mne.childs.front());
            mne.childs.pop_front();
            mmy.header.num_of_keys++;
            // change child's parent
            ch_num = mmy.childs.back();
            file_read_page(tid, ch_num, &ch);
            ch.header.parent_num = my_num;
            file_write_page(tid, ch_num, &ch);
        }
        while (mne.keys.size())
        {
            mmy.keys.push_back(mne.keys.front());
            mne.keys.pop_front();
        }
        // save
        pa = meminternal_to_diskinternal(mpa);  file_write_page(tid, pa_num, &pa);
        my = meminternal_to_diskinternal(mmy);  file_write_page(tid, my_num, &my);
        file_free_page(tid, ne_num);
    }
    // ne | my -> [my] is absorbed by [ne]
    else
    {
        key_up = mpa.keys[my_idx];
        mne.keys.push_back(key_up);
        while (mmy.childs.size())
        {
            mne.childs.push_back(mmy.childs.front());
            mmy.childs.pop_front();
            mne.header.num_of_keys++;
            // change child's parent
            ch_num = mne.childs.back();
            file_read_page(tid, ch_num, &ch);
            ch.header.parent_num = ne_num;
            file_write_page(tid, ch_num, &ch);
        }
        while (mmy.keys.size())
        {
            mne.keys.push_back(mmy.keys.front());
            mmy.keys.pop_front();
        }
        // save
        pa = meminternal_to_diskinternal(mpa);  file_write_page(tid, pa_num, &pa);
        ne = meminternal_to_diskinternal(mne);  file_write_page(tid, ne_num, &ne);
        file_free_page(tid, my_num);
    }
    // pa.edge will removed by remove_entry_from_node

    return delete_entry(tid, pa_num, key_up);
}

/* delete_entry
 * remove key & record from leaf
 * and makes all appropriate
 * changes to preserve the B+ tree
 * properties.
 */
void delete_entry(int64_t tid, pagenum_t page_num, int64_t key)
{
    page_t my;
    file_read_page(tid, page_num, &my);
    remove_entry_from_node(tid, page_num, my, key);

    header_page_t header;
    read_header_page(tid, &header);

    // case 0: delete from root node
    if (page_num == header.root_num)
        return adjust_root(tid, page_num, my);

    // parent node
    page_t pa;
    pagenum_t pa_num = my.header.parent_num;
    file_read_page(tid, pa_num, &pa);

    // get neighbor node
    int my_idx = get_my_idx(tid, page_num, my),
        ne_idx = (my_idx == -1 ? 0 : my_idx-1);
    page_t ne;
    pagenum_t ne_num = (ne_idx == -1 ? pa.ip.left_num : pa.ip.edges[ne_idx].child_num);
    file_read_page(tid, ne_num, &ne);

    // case 1: leaf case
    if (my.header.is_leaf)
    {
        /* in leaf case, we need to know
         * the free space sizes of my/ne page.
         * if my.free_space + ne.free_space < INIT_FREE_SPACE
         * then merge; else redistribute
         */
        int64_t my_free = my.lp.free_space,
                ne_free = ne.lp.free_space;
        // case 1-0: Nothing to do
        if (my.lp.free_space < THRESHOLD)
            return file_write_page(tid, page_num, &my);
        // case 1-1: merge
        else if (INIT_FREE_SPACE - my_free <= ne_free)
            return merge_leaf(tid, page_num, ne_num, my, ne, my_idx, ne_idx);
        // case 1-2: redistribution
        else
            return redistribute_leaf(tid, page_num, ne_num, my, ne, my_idx, ne_idx);
    }
    // case 2: internal case
    else
    {
        int my_key = my.header.num_of_keys,
            ne_key = ne.header.num_of_keys;
        // case 2-0: Nothing to do
        if (my.header.num_of_keys >= (BRANCHING_FACTOR >> 1))
            return file_write_page(tid, page_num, &my);
        // case 2-1: merge
        else if (my_key + ne_key < BRANCHING_FACTOR-1)
            return merge_internal(tid, page_num, ne_num, my, ne, my_idx, ne_idx);
        // case 2-2: redistribution
        else
            return redistribute_internal(tid, page_num, ne_num, my, ne, my_idx, ne_idx);
    }
}

/* db_delete
 * master delete operation function
 */
int db_delete(int64_t tid, int64_t key)
{
    char tmp[120] = {};
    uint16_t tmp_size;
    // key not found
    if (db_find(tid, key, tmp, &tmp_size))
        return 1;
    // key found
    delete_entry(tid, find_leaf(tid, key), key);
    return 0;
}

// =================================================================
// Open & Init & Shutdown
// =================================================================
/* open table
 * 제곧내
 */
int64_t open_table(char* pathname)
{
    return file_open_table_file(pathname);
}

/* init_db
 * initialize DBMS
 * initialize and allocate anything you need
 * Nothing to do in this project yet.
 */
int init_db(void)
{
    return 0;
}

/* shutdown_db
 * Shutdown DBMS
 * clean up everything
 */
int shutdown_db(void)
{
    file_close_table_files();
    return 0;
}

// =================================================================
// DEBUG
// =================================================================

pagenum_t detect_zero_error(int64_t tid)
{
    header_page_t header;
    read_header_page(tid, &header);
    page_t cur;
    pagenum_t cur_num = header.root_num;
    queue<pagenum_t> q;
    q.push(cur_num);
    while (q.size())
    {
        cur_num = q.front();
        q.pop();
        file_read_page(tid, cur_num, &cur);

        if (cur.header.is_leaf)
        {
            memory_leaf_t leaf = diskleaf_to_memleaf(cur);
            for (auto x: leaf.slots)
                if (x.key == 0)
                    return cur_num;
        }
        else
        {
            memory_internal_t inter = diskinternal_to_meminternal(cur);
            for (auto x: inter.keys)
                if (x == 0)
                    return cur_num;
            for (pagenum_t pn: inter.childs)
                q.push(pn);
        }
    }
    return 0;
}

void print_node(int64_t tid, pagenum_t num)
{
    cout << "pagenum = " << num << endl;

    page_t cur;
    file_read_page(tid, num, &cur);
    if (cur.header.is_leaf == 1)
    {
        memory_leaf_t leaf = diskleaf_to_memleaf(cur);
        cout << "free space:" << leaf.free_space << endl;
        cout << "\tkeys:\n";
        for (int i = 0; i < leaf.header.num_of_keys; i++)
            cout << "\t" << leaf.slots[i].key << "\n";
    }
    else if (cur.header.is_leaf == 0)
    {
        memory_internal_t inter = diskinternal_to_meminternal(cur);
        cout << "\tkeys\tnums\n";
        cout << "\t\t" << inter.childs.front() << endl;
        for (int i = 0; i < inter.header.num_of_keys; i++)
            cout << "\t" << inter.keys[i] << "\t" << inter.childs[i+1] << "\n";
    }
    else
        cout << "free page\n";
    return ;
}
