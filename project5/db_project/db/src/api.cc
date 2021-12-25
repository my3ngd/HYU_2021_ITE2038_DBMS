#include "api.h"


int init_db(int num_buf)
{
    if (init_buffer(num_buf))   return 1;
    if (init_lock_manager())    return 1;
    if (init_trx_manager())     return 1;
    return 0;
}

/* shutdown db
 * flush all pages
 * close all files
 * free all resources
 */
int shutdown_db(void)
{
    if (!buffer_exist)
        return -1;

    buffer_exist = false;

    // about header page
    for (auto& x: buffer->files)
        write_header_page(x.ff, &(x.ss));

    // flush all pages
    for (auto& x: buffer->dic)
    {
        file_write_page(x.ff.ff, x.ff.ss, &(x.ss->frame));
    }

    // about B+ tree pages
    delete buffer;
    file_close_table_files();
    return 0;
}

