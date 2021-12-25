#include "bpt.h"
#include <ctime>
const int64_t MAX = 5000;

// #define FILE_GENERATOR
// #define FIND_ONLY_TEST
// #define PRINT_ONLY
// #define FIND_1000000
#define IFD
// #define MULTI_TABLE

void shuffle(vector<int64_t>& a)
{
    a.resize(0);
    for (int i = 0; i < MAX; i++)  a.push_back(i+1);
    for (int i = 0; i < MAX*5; i++)
    {
        int x = rand() % MAX,
            y = rand() % MAX;
        swap(a[x], a[y]);
    }
}


#ifdef FILE_GENERATOR

int main(int argc, char const *argv[])
{
    string filename = "order5_debug";

    remove(filename.c_str());
    cout << "Make " << filename << ":" << endl;

    time_t seed = time(NULL);
    srand(seed);
    int64_t fd = open_table(filename.c_str());
    // gen start

    vector<int64_t> a;
    for (int i = 0; i < MAX; i++)  a.push_back(i+1);
    // shuffle
    for (int i = 0; i < MAX*5; i++)
    {
        int x = rand() % MAX,
            y = rand() % MAX;
        swap(a[x], a[y]);
    }
    cout << "insert\n";

    // insert
    string base = string("_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0_________1_x");
    for (int i = 0; i < MAX; i++)
    {
        // if (i%10 == 0)  cout << i+1 << endl;
        if ((i+1)%100 == 0)   cout << "=";
        // cout << i+1 << "\t" << a[i] << "\t" << get_height(fd, 1) << endl;
        if ((i+1)%1000== 0)  cout << "| " << (i+1)/1000 << "%\n";

        string res = base.substr(0, 50+(a[i]-1)%72);
        char value[120] = {};
        strcpy(value, res.c_str());
        if (db_insert(fd, a[i], value, res.size()))
        {
            continue;
            cout << i << "\t" << a[i] << "failed\n";
            exit(1);
        }
        // print_tree(fd);
    }
    cout << "file write end." << endl;
    // print_tree(fd);

    // gen end
    file_close_database_files();
    return 0;
}


#endif
#ifdef FIND_ONLY_TEST

int main(int argc, char const *argv[])
{
    time_t seed = time(NULL);
    srand(seed);

    int64_t fd = open_table("find_test");

    // test start ------------------------------------------------

    string base = string("_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0_________1_x");

    char tmp[120];
    uint16_t len;
    for (int64_t i = 1; i <= MAX; i++)
    {
        if (db_find(fd, i, tmp, &len))
        {
            cout << "[find] operation fails when i == " << i << endl;
            exit(1);
        }
        string cmp = string(tmp),
               may = base.substr(0, 50+(i-1)%72);
        if (cmp != may)
        {
            cout << "[find] operation fails when i == " << i << endl;
            cout << "expect \"" << may << "\"\n";
            cout << "[find] \"" << cmp << "\"\n";
            exit(1);
        }
    }
    cout << "[find] test successful\nGG\n";

    // test end   ------------------------------------------------

    file_close_database_files();
    return 0;
}

#endif
#ifdef PRINT_ONLY

int main(int argc, char const *argv[])
{
    int64_t fd = open_table("order5.dat");

    print_tree(fd);
    // print_node(fd, 81921);

    file_close_database_files();
    return 0;
}

#endif

#ifdef FIND_1000000

int main(int argc, char const *argv[])
{
    int64_t fd = open_table("insertTest.dat");

    // print_pages(fd);

    page_t node;
    puts("============================================================================================================================================");
    for (int i = 1; i <= 1000000; i++)
    {
        // if ((i+1)%100 == 0) cout << '=';
        // if ((i+1)%1000 == 0) cout << '|';
        if (i%1000 == 0)cout << i << endl;
        if (db_delete(fd, i))
        {
            cout << i << "failed" << endl;
            exit(1);
        }
    }
    puts("============================================================================================================================================");
    print_pages(fd);
    file_close_database_files();
    return 0;
}

#endif

#ifdef IFD

int main(int argc, char const *argv[])
{
    string filename = "test.dat";
    remove(filename.c_str());
    init_db();
    int64_t fd = open_table(const_cast<char*>(filename.c_str()));

    time_t seed = time(NULL);
    srand(seed);
    // gen start

    vector<int64_t> a;
    for (int i = 1; i <= MAX; i++)
        a.push_back(i);
    shuffle(a);

    // insert
    string base = string("_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0_________1_x");
    for (int i = 0; i < MAX; i++)
    {
        if ((i+1)%(MAX/100) == 0)  cout << "insert\t" << (i+1)/(MAX/100) << "%\n";

        string res = base.substr(0, 50+(a[i]-1)%72);
        char value[120] = {};
        strcpy(value, res.c_str());
        if (db_insert(fd, a[i], value, res.size()))
        {
            cout << i << "\t" << a[i] << "failed\n";
            exit(1);
        }
    }
    cout << "file write success." << endl;

    cout << "get_height = " << get_height(fd, 1) << endl;

    // find
    uint16_t len;
    char value[120];
    shuffle(a);
    for (int i = 0; i > -100; i--)
        if (!db_find(fd, i, value, &len))
            cout << "error: " << i << endl;
    for (int i = 0; i < MAX; i++)
    {
        if ((i+1)%(MAX/100) == 0)  cout << "find\t" << (i+1)/(MAX/100) << "%\n";

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

    // uint16_t len;
    // char value[120];
    for (int i = 0; i < MAX; i++)
    {
        // print_tree(fd);
        if ((i+1)%(MAX/100) == 0)  cout << "delete\t" << (i+1)/(MAX/100) << "%\n";

        // cout << i+1 << " " << a[i] << "\n";

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
            print_tree(fd);
            exit(1);
        }
        // print_tree(fd);
    }
    cout << "file delete success.\n";

    shutdown_db();
    remove(filename.c_str());

    return 0;
}

#endif
