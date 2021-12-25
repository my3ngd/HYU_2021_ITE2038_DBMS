#include "api.h"


/*
int64_t tid;

void* trx_read_only(void* arg)
{
    int trx_id = trx_begin();

    int64_t key;
    char ret_val[150] = {};
    uint16_t val_size;
    int debug;

    // growing phase
    for (int key = 1; key <= 100; key++)
    {
        memset(ret_val, 0, sizeof(ret_val));
        debug = db_find(tid, key, ret_val, &val_size, trx_id);
        cout << "read + | id = " << trx_id << ", key = " << key << ", value = " << ret_val << ", debug = " << debug << endl;
    }

    // shrinking phase
    int res = trx_commit(trx_id);
    cout << "read - | id = " << trx_id << ", res = " << res << endl;
    return nullptr;
}

string value = "::my3ngd's-database-value=after==";

void* trx_write_only(void* arg)
{
    int64_t key;
    uint16_t old_val_size;
    string val = value;
    uint16_t val_size;
    int debug;

    int trx_id = trx_begin();

    // growing phase
    for (int key = 1; key <= 20; key++)
    {
        val = value;
        val += std::to_string(key);
        debug = db_update(tid, key, const_cast<char*>(val.c_str()), val_size, &old_val_size, trx_id);
        cout << "write + | id = " << trx_id << ", key = " << key << endl;
    }

    // shrinking phase
    int res = trx_commit(trx_id);
    cout << "write - | id = " << trx_id << ", res = " << res << endl;

    return nullptr;
}

void* trx_mixed(void* arg)
{
    int trx_id = trx_begin();

    int64_t key;
    char ret_val[150] = {};
    uint16_t val_size;
    int debug;
    string val = value;

    // growing phase
    for (int key = 1; key <= 100; key++)
    {
        // read
        cout << "[ = ";
        memset(ret_val, 0, sizeof(ret_val));
        debug = db_find(tid, key, ret_val, &val_size, trx_id);
        cout << "mix-read + | id = " << trx_id << ", key = " << key << ", value = " << ret_val << ", debug = " << debug << endl;

        // write
        cout << "[ = ";
        val = value;
        while (val.size() < 50) val += std::to_string(key);
        debug = db_update(tid, key, const_cast<char*>(val.c_str()), val_size, &val_size, trx_id);
        cout << "mix-write + | id = " << trx_id << ", key = " << key << endl;

        // read
        cout << "[ = ";
        memset(ret_val, 0, sizeof(ret_val));
        debug = db_find(tid, key, ret_val, &val_size, trx_id);
        cout << "mix-read + | id = " << trx_id << ", key = " << key << ", value = " << ret_val << ", debug = " << debug << endl;

// cout << "\n\n====================================================================================================================\n\n" << endl;
    }

    // shrinking phase
    int res = trx_commit(trx_id);
    cout << "mix-commit - | id = " << trx_id << ", res = " << res << endl;
    return nullptr;
}


int main(int argc, char const *argv[])
{
    init_db(100);
    tid = open_table(const_cast<char*>("my3ngd.db"));
    cout << "opened" << endl;

    for (int i = 1; i <= 100; i++)
    {
        // cout << i;
        if (i%1000 == 0)
            cout << "i = " << i << endl;

        string str = "::my3ngd's-database-value=before=";
        for (int j = 0; j < 50; j++)
            str += std::to_string(i);
        str.resize(50);
        db_insert(tid, i, const_cast<char*>(str.c_str()), 50);
    }
    cout << "insert end" << endl;

    // for (int i = 1; i <= 100; i++)
    // {
    //     uint16_t val_size;
    //     char res[200] = {};
    //     db_find(tid, i, res, &val_size);
    //     cout << i << " | " << res << " | " << val_size << endl;
    // }

    // for (int i = 1; i <= 100; i++)
    // {
    //     char ret_val[150] = {};
    //     uint16_t val_size;
    //     int64_t kk = i*20;
    //     cout << "find key: " << kk << "\t: " << db_find(tid, kk, ret_val, &val_size) << "\t" << ret_val << endl;
    // }








    for (int i = 0; i < 3; i++)
    {
        cout << "for: " << i << endl;
        pthread_t trx1, trx2, trx3, trx4, trx5, trx6, trx7, trx8, trx9;
        pthread_create(&trx1, NULL, trx_read_only, NULL);
        pthread_create(&trx2, NULL, trx_write_only, NULL);
        pthread_create(&trx3, NULL, trx_read_only, NULL);
        pthread_create(&trx4, NULL, trx_mixed, NULL);

        pthread_join(trx1, NULL);
        pthread_join(trx2, NULL);
        pthread_join(trx3, NULL);
        pthread_join(trx4, NULL);
    }


    for (int i = 0; i < 9; i++)
    {
        char ret_val[150] = {};
        uint16_t val_size;
        cout << "find key: " << i+1 << "\t: " << db_find(tid, i+1, ret_val, &val_size) << "\t" << ret_val << endl;
    }

    shutdown_db();
    cout << "end test" << endl;
    return 0;
}
// */

int main(int argc, char const *argv[])
{
    return 0;
}
