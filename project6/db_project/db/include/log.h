#ifndef __LOG_H__
#define __LOG_H__
#include "concurrency_control.h"


struct log_header_t
{
    uint16_t log_size;
    uint64_t LSN;
    uint64_t prev_LSN;
    int trx_id;
    int type;
};

struct log_body_t
{
    int64_t table_id;
    pagenum_t page_num;
    int16_t offset;
    int16_t length;
};


struct log_manager_t
{
    ;
};

#endif  // __LOG_H__
