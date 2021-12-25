#ifndef __API_H__
#define __API_H__

#include "concurrency_control.h"
#include "log.h"

int init_db(int num_buf, int flag, int log_num, char* log_path, char* logmsg_path);
int shutdown_db(void);

#endif  // __API_H__
