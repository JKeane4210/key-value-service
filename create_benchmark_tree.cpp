#include <bits/stdc++.h>
#include "disk_btree.h"

using namespace std;

int main(int argc, char** argv) {
    DiskBTree dbt("benchmark_tree.bin", true);
    ofstream fs("benchmark_tree.entries");

    int inserts = 0;
    const int TARGET_INSERTS = 1000000;
    fs << TARGET_INSERTS << endl;

    // I am a lazy coder ... I'm sorry ... but not really :)
    for (int i = 0; i < 26; ++i) {
        for (int j = 0; j < 26; ++j) {
            for (int k = 0; k < 26; ++k) {
                for (int l = 0; l < 26; ++l) {
                    for (int m = 0; m < 26; ++m) {
                        char key[64];
                        key[0] = 'a' + i;
                        key[1] = 'a' + j;
                        key[2] = 'a' + k;
                        key[3] = 'a' + l;
                        key[4] = 'a' + m;
                        key[5] = 0;
                        fs << key << endl;
                        dbt.insert(key, key);
                        inserts++;
                        if (inserts == TARGET_INSERTS) {
                            return 0;
                        }
                    }
                }
            }
        }
    }

    return 0;
}