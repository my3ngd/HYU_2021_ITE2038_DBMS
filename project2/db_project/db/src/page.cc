#include "page.h"

header_page_t::header_page_t(void)
{
    this->free_num = this->num_of_pages = this->root_num = 0;
    memset(this->RESERVED, 0, sizeof(this->RESERVED));
}

slot_t::slot_t(void)
{
    this->key = 0;
    this->size = 0;
    this->offset = 0;
}

slot_t::slot_t(unsigned char* rec, int idx)
{
    #define MEMCPY_USE_SLOT
    #ifdef MEMCPY_USE_SLOT
    memcpy(&(this->key), &rec[12*idx], 8);
    memcpy(&(this->size), &rec[8+12*idx], 2);
    memcpy(&(this->offset), &rec[10+12*idx], 2);
    #else
    // TODO: memcpy
    this->key = 0;
    for (int i = 0; i < 8; i++)
        this->key += rec[i+12*idx]*(1LL << (8*i));

    this->size = rec[8+12*idx] + rec[9+12*idx]*(1 << 8);
    this->offset = rec[10+12*idx] + rec[11+12*idx]*(1 << 8);
    #endif
}

slot_t::slot_t(int64_t key, unsigned short size, unsigned short offset = 0)
{
    this->key = key;
    this->size = size;
    this->offset = offset;
}

record_t::record_t(void)
{
    this->slot = slot_t();
    this->rec = string();
}

record_t::record_t(int64_t key, char* value, uint16_t val_size)
{
    this->slot = slot_t(key, val_size);
    this->rec = string(value);
}


edge_t::edge_t(void)
{
    this->key = 0;
    this->child_num = 0;
}

edge_t::edge_t(int64_t key, pagenum_t child_num)
{
    this->key = key;
    this->child_num = child_num;
}

page_header_t::page_header_t(void)
{
    this->parent_num = 0;
    this->is_leaf = 0;
    this->num_of_keys = 0;
}

internal_page_t::internal_page_t(void)
{
    this->left_num = 0;
}

leaf_page_t::leaf_page_t(void)
{
    this->free_space = INIT_FREE_SPACE;
    this->right_num = 0;
}

page_t::page_t(void)
{
    this->lp.free_space = INIT_FREE_SPACE;
    this->header.is_leaf = -1;
}

page_t::page_t(int x)
{
    this->lp.free_space = INIT_FREE_SPACE;
    this->header.is_leaf = x;
}

/* memory_leaf_t -> page_t(leaf_page_t) converter
 * Built-in features that automatically set offset & free_space.
 * "num_of_keys" not safe
 */
page_t memleaf_to_diskleaf(memory_leaf_t leaf)
{
    page_t res;
    res.header = leaf.header;
    res.lp.right_num = leaf.right_num;

    // num_of_keys adjusting part (may not necessary)
    res.header.num_of_keys = leaf.slots.size();
    // checker
    assert(leaf.slots.size() == leaf.records.size());

    // offset adjusting part
    int offset = 4096;
    for (int i = 0; i < res.header.num_of_keys; i++)
    {
        offset -= leaf.slots[i].size;
        leaf.slots[i].offset = offset;
    }
    offset -= 128 + 12*res.header.num_of_keys;
    // free_space adjusting part
    res.lp.free_space = offset;

    // memcpy
    for (int i = 0; i < res.header.num_of_keys; i++)
    {
        int idx = 12*i;
        memcpy(res.lp.rec+idx, &(leaf.slots[i].key), 8);        // key
        memcpy(res.lp.rec+idx+8, &(leaf.slots[i].size), 2);     // size
        memcpy(res.lp.rec+idx+10, &(leaf.slots[i].offset), 2);  // offset
    }

    // assert(res.header.num_of_keys == leaf.slots.size() && leaf.slots.size() == leaf.records.size());

    int numkeys = res.header.num_of_keys;
    for (int i = 0; i < numkeys; i++)
    {
        memcpy(res.lp.rec-128+leaf.slots[i].offset, leaf.records[i].c_str(), leaf.slots[i].size);
    }
    return res;
}

/* page_t(leaf_page_t) -> memory_leaf_t converter
 * TODO: test that can read unorded data
 */
memory_leaf_t diskleaf_to_memleaf(page_t leaf)
{
    memory_leaf_t res;
    res.header = leaf.header;
    res.free_space = leaf.lp.free_space;
    res.right_num = leaf.lp.right_num;
    for (int i = 0; i < res.header.num_of_keys; i++)
    {
        slot_t slot(leaf.lp.rec, i);
        res.slots.push_back(slot);
    }

    int numkeys = res.header.num_of_keys;
    for (int i = 0; i < numkeys; i++)
    {
        char* tmp = new char[113];
        memset(tmp, 0, sizeof(char)*113);

        memcpy(tmp, leaf.lp.rec-128+res.slots[i].offset, res.slots[i].size);
        res.records.push_back(string(tmp));
        delete tmp;
    }
    return res;
}

page_t meminternal_to_diskinternal(memory_internal_t inter)
{
    page_t res;
    res.header = inter.header;
    res.header.num_of_keys = inter.keys.size();
    res.ip.left_num = inter.childs.front();
    inter.childs.pop_front();
    assert(inter.childs.size() == inter.keys.size());

    for (int i = 0; i < inter.keys.size(); i++)
        res.ip.edges[i] = edge_t(inter.keys[i], inter.childs[i]);

    return res;
}

memory_internal_t diskinternal_to_meminternal(page_t inter)
{
    memory_internal_t res;
    res.header = inter.header;

    res.childs.push_back(inter.ip.left_num);
    for (int i = 0; i < res.header.num_of_keys; i++)
    {
        res.childs.push_back(inter.ip.edges[i].child_num);
        res.keys.push_back(inter.ip.edges[i].key);
    }
    return res;
}
