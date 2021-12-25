#include "bpt.h"
#include <ctime>
int64_t MAX = 100000;

// #define IFD
#define SHUFFLE
// #define MAKE



void shuffle(vector<int64_t>& a)
{
    a.resize(0);
    for (int i = 0; i < MAX; i++)  a.push_back(i+1);
    #ifdef SHUFFLE
    for (int i = 0; i < MAX*5; i++)
    {
        int x = rand() % MAX,
            y = rand() % MAX;
        swap(a[x], a[y]);
    }
    #endif
}


#ifdef IFD

int main(int argc, char const *argv[])
{
    MAX = 100000;
    string filename = "test.dat";
    // remove(filename.c_str());

    cout << "init db" << endl;
    #ifdef MAKE
    remove(filename.c_str());
    init_db(std::max(4L, MAX/10));
    #else
    init_db(std::min(4L, MAX/30));
    #endif

    cout << "open table" << endl;
    int64_t fd = open_table(const_cast<char*>(filename.c_str()));

    cout << "seed" << endl;
    time_t seed = time(NULL);
    // seed = 1635183145;
    cout << seed << endl;
    srand(seed);
    // gen start

    cout << "shuffle" << endl;
    vector<int64_t> a;
    for (int i = 1; i <= MAX; i++)
        a.push_back(i);
    shuffle(a);
    cout << "shuffle end" << endl;





    int64_t MIN_VAL = 99999999;
    // insert
    string base = string("_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0_________1_x");
// #ifdef MAKE
    for (int i = 0; i < MAX; i++)
    {
        if (MAX >= 100)
        {
            if ((i+1)%(MAX/100) == 0) cout << (i+1)/(MAX/100) << "%\n";
        }
        // ===
        string res = base.substr(0, 50+(a[i]-1)%72);
        char value[120] = {};
        strcpy(value, res.c_str());
        if (db_insert(fd, a[i], value, res.size()))
        {
            cout << i << "\t" << a[i] << "failed\n";
            exit(1);
        }
        MIN_VAL = std::min(MIN_VAL, a[i]);

        uint16_t len;
        if (db_find(fd, MIN_VAL, value, &len))
        {
            cout << i << "\t" << a[i] << " failed\n";
            print_tree(fd);
        }
        // if (empty_buffer_check())
        // {
        //     cout << "buffer assertion failed" << endl;
        //     exit(1);
        // }
    }
    // print_tree(fd);
    cout << "file write success." << endl;

// #else

// cout << "print_tree" << endl;
// print_tree(fd);
    // find
    uint16_t len;
    char value[120];
    shuffle(a);
    cout << "find start\n";
    for (int i = 0; i < MAX; i++)
    {
        if (MAX >= 100)
        {
            if ((i+1)%(MAX/100) == 0) cout << (i+1)/(MAX/100) << "%\n";
        }
        // ===
        string res = base.substr(0, 50+(a[i]-1)%72);
        if (db_find(fd, a[i], value, &len))
        {
            cout << i << "\t" << a[i] << " failed\n";
            exit(1);
        }
        assert(string(value) == res);
    }
    cout << "file read success." << endl;







    // delete & find
    shuffle(a);
    for (int i = 0; i < MAX; i++)
    {
        if (MAX >= 100)
        {
            if ((i+1)%(MAX/100) == 0) cout << (i+1)/(MAX/100) << "%\n";
        }
        // ===
        if (db_delete(fd, a[i]))
        {
            cout << i+1 << " " << a[i] << " failed in delete\n" << endl;
            // print_tree(fd);
            exit(1);
        }
        if (!db_find(fd, a[i], value, &len))
        {
            cout << i+1 << " " << a[i] << " failed in found\n" << endl;
            print_node(fd, find_leaf(fd, a[i]));
            // print_tree(fd);
            exit(1);
        }

        // if (empty_buffer_check())
        // {
        //     cout << "buffer assertion failed" << endl;
        //     exit(1);
        // }
    }
    cout << "file delete success.\n";
    assert(buffer->cur_size == buffer->dic.size());

    print_tree(fd);
    shutdown_db();
    // remove(filename.c_str());
// #endif
    return 0;
}

#endif


int main(int argc, char const *argv[])
{
    MAX = 1000;
    string filename = "test.dat";

    cout << "init db" << endl;
    init_db(MAX);

    cout << "open table" << endl;
    int64_t fd = open_table(const_cast<char*>(filename.c_str()));

    cout << "seed" << endl;
    time_t seed = time(NULL);
    // seed = 1635183145;
    cout << seed << endl;
    srand(seed);
    // gen start

    cout << "shuffle" << endl;
    vector<int64_t> a;
    for (int i = 1; i <= MAX; i++)
        a.push_back(i);
    shuffle(a);
    cout << "shuffle end" << endl;

    int64_t MIN_VAL = 99999999;
    // insert
    string base = string("_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0_________1_x");
    for (int i = 0; i < MAX; i++)
    {
        if (MAX >= 100)
            if ((i+1)%(MAX/100) == 0)
                cout << (i+1)/(MAX/100) << "%\n";
        string res = base.substr(0, 50+(a[i]-1)%72);
        char value[120] = {};
        strcpy(value, res.c_str());
        if (db_insert(fd, a[i], value, res.size()))
        {
            cout << i << "\t" << a[i] << "failed\n";
            // exit(1);
            break;
        }
        MIN_VAL = std::min(MIN_VAL, a[i]);

        uint16_t len;
        if (db_find(fd, MIN_VAL, value, &len))
        {
            cout << i << "\t" << a[i] << " failed\n";
            print_tree(fd);
        }
    }
    cout << "insert success." << endl;

    uint16_t len;
    char value[120];
    shuffle(a);
    cout << "find start\n";
    for (int i = 0; i < MAX; i++)
    {
        if (MAX >= 100)
            if ((i+1)%(MAX/100) == 0)
                cout << (i+1)/(MAX/100) << "%\n";
        string res = base.substr(0, 50+(a[i]-1)%72);
        if (db_find(fd, a[i], value, &len))
        {
            cout << i << "\t" << a[i] << " failed\n";
            exit(1);
        }
        assert(string(value) == res);
    }
    cout << "find success." << endl;

    print_tree(fd);

    if (shutdown_db())
    {
        cout << "shutdown failed" << endl;
        exit(1);
    }
    cout << "shutdown end" << endl;
    if (init_db(4))
    {
        cout << "init failed" << endl;
        exit(1);
    }

    cout << "open table" << endl;
    fd = open_table(const_cast<char*>(filename.c_str()));

    print_tree(fd);
    cout << "print_tree" << endl;
    cout << "delete start" << endl;

    shuffle(a);
    for (int i = 0; i < MAX; i++)
    {
        if (MAX >= 100)
            if ((i+1)%(MAX/100) == 0)
                cout << (i+1)/(MAX/100) << "%\n";

        if (db_delete(fd, a[i]))
        {
            cout << i+1 << " " << a[i] << " failed in delete\n" << endl;
            print_tree(fd);
            exit(1);
        }
        if (!db_find(fd, a[i], value, &len))
        {
            cout << i+1 << " " << a[i] << " failed in found\n" << endl;
            print_node(fd, find_leaf(fd, a[i]));
            exit(1);
        }
    }
    cout << "delete success.\n";
    assert(buffer->cur_size == buffer->dic.size());
    shutdown_db();
}
