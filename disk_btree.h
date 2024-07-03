#include <bits/stdc++.h>

using namespace std;

enum NODE_TYPE {
    ROOT = 0b001,
    LEAF = 0b010 // internal is just the lack of this
};

const streampos PAGE_SIZE = 4096;
const int NKEYS = 16; // needed for getting nodes to fit page size

struct HeaderPage {
    int root_index = 0;
    int n_nodes;
    char padding[4096-8];
};

struct Node {
    long node_array_index;
    int node_type;
    int nkeys;
    int curr_capacity; // going to have this double as right most pointer
    int parent_index;
    char keys[NKEYS][64];
    char values[NKEYS][64];
    int children[NKEYS + 1];
    char padding[4096-2144];

    Node() : nkeys(NKEYS), 
                        curr_capacity(0),
                        node_type(0) {}

    Node get_parent(fstream& fs) {
        Node parent = read_node(fs, PAGE_SIZE * (parent_index + 1));
        return parent;
    }

    Node get_child(int i, fstream& fs) {
        int child_index = children[i];
        Node child = read_node(fs, PAGE_SIZE * (child_index + 1));
        return child;
    }

    char* find(char key[64], fstream& fs) {
        for (int i = 0; i < curr_capacity; ++i) {
            int cmp_res = strcmp(key, keys[i]);
            if (cmp_res == 0) {
                return values[i];
            } else if (cmp_res < 0) {
                if ((node_type & NODE_TYPE::LEAF) != 0) {
                    return nullptr;
                } else {
                    Node child = get_child(i, fs);
                    return child.find(key, fs);
                }
            }
        }
        if ((node_type & NODE_TYPE::LEAF) != 0) {
            return nullptr;
        } else {
            Node child = get_child(curr_capacity, fs);
            return child.find(key, fs);
        }
    }

    void split(char key[64], char value[64], int insert_location, int right, HeaderPage& hp, fstream& fs) {
        Node new_node;
        new_node.node_array_index = hp.n_nodes;
        hp.n_nodes++;
        new_node.node_type |= (node_type & NODE_TYPE::LEAF); // takes the leaf/intenal node status of splitting node
        
        // break into left and right nodes
        char tmp_k[64];
        char tmp_v[64];
        if (insert_location < NKEYS / 2) {
            // fill new node
            new_node.children[0] = children[NKEYS / 2];
            for (int i = 0; i < NKEYS / 2; ++i) {
                memcpy(new_node.keys[i], keys[i + NKEYS / 2], 64);
                memcpy(new_node.values[i], values[i + NKEYS / 2], 64);
                new_node.children[i + 1] = children[i + NKEYS / 2 + 1];
            }
            // keep the middle node (that will go up the tree)
            memcpy(tmp_k, keys[NKEYS / 2 - 1], 64);
            memcpy(tmp_v, values[NKEYS / 2 - 1], 64);

            // shift nodes over
            for (int i = NKEYS / 2 - 1; i >= insert_location; --i) {
                memcpy(keys[i + 1], keys[i], 64);
                memcpy(values[i + 1], values[i], 64);
                children[i + 2] = children[i + 1]; // moving right pointers over
            }
            children[insert_location + 1] = right;
            // insert key value pair
            memcpy(keys[insert_location], key, 64);
            memcpy(values[insert_location], value, 64);
            new_node.curr_capacity = NKEYS / 2;
            curr_capacity = NKEYS / 2;
        } else if (insert_location == NKEYS / 2) {
            // keep the middle node (that will go up the tree)
            memcpy(tmp_k, key, 64);
            memcpy(tmp_v, value, 64);
            // fill up the new node
            new_node.children[0] = right;
            for (int i = 0; i < NKEYS / 2; ++i) {
                memcpy(new_node.keys[i], keys[i + NKEYS / 2], 64);
                memcpy(new_node.values[i], values[i + NKEYS / 2], 64);
                new_node.children[i + 1] = children[i + NKEYS / 2 + 1];
            }
            new_node.curr_capacity = NKEYS / 2;
            curr_capacity = NKEYS / 2;
        } else { // insert_location > NKEYS / 2
            // keep the middle node (that will go up the tree)
            memcpy(tmp_k, keys[NKEYS / 2], 64);
            memcpy(tmp_v, values[NKEYS / 2], 64);
            // add nodes less than key to new_node
            for (int i = NKEYS / 2 + 1; i < insert_location; ++i) {
                memcpy(new_node.keys[i - (NKEYS / 2 + 1)], keys[i], 64);
                memcpy(new_node.values[i - (NKEYS / 2 + 1)], values[i], 64);
                new_node.children[i - (NKEYS / 2 + 1)] = children[i];
            }
            // insert key-value pair
            memcpy(new_node.keys[insert_location - (NKEYS / 2 + 1)], key, 64);
            memcpy(new_node.values[insert_location - (NKEYS / 2 + 1)], value, 64);
            new_node.children[insert_location - (NKEYS / 2 + 1)] = children[insert_location];
            new_node.children[insert_location - (NKEYS / 2)] = right;
            // add nodes greater than key to new_node
            for (int i = insert_location; i < NKEYS; ++i) {
                memcpy(new_node.keys[i - (NKEYS / 2)], keys[i], 64);
                memcpy(new_node.values[i - (NKEYS / 2)], values[i], 64);
                new_node.children[i - (NKEYS / 2 + 1)] = children[i + 1];
            }
            new_node.curr_capacity = NKEYS / 2;
            curr_capacity = NKEYS / 2;
        }

        if ((node_type & NODE_TYPE::ROOT) != 0) {
            Node parent;
            parent.node_array_index = hp.n_nodes;
            hp.n_nodes++;
            parent_index = parent.node_array_index;
            parent.node_type |= NODE_TYPE::ROOT;
            node_type &= (~NODE_TYPE::ROOT);

            // set up new root
            memcpy(parent.keys[0], tmp_k, 64);
            memcpy(parent.values[0], tmp_v, 64);
            parent.curr_capacity++;
            parent.children[0] = node_array_index;
            parent.children[1] = new_node.node_array_index;
            new_node.parent_index = parent.node_array_index;

            hp.root_index = parent.node_array_index;
            write_node(fs, PAGE_SIZE * (parent.node_array_index + 1), parent);
            write_node(fs, PAGE_SIZE * (new_node.node_array_index + 1), new_node);
            write_node(fs, PAGE_SIZE * (node_array_index + 1), *this);
        } else {
            Node parent = get_parent(fs);
            new_node.parent_index = parent.node_array_index;
            // get insert location
            int parent_insert_location = 0;
            for (parent_insert_location = 0; parent_insert_location < parent.curr_capacity; ++parent_insert_location) {
                int cmp_res = strcmp(tmp_k, parent.keys[parent_insert_location]);
                assert((cmp_res != 0) && "splitting found existing node");
                if (cmp_res < 0) break;
            }

            // check if parent is full
            if (parent.curr_capacity + 1 > NKEYS) { // if so, recurse the split
                write_node(fs, PAGE_SIZE * (parent.node_array_index + 1), parent);
                write_node(fs, PAGE_SIZE * (new_node.node_array_index + 1), new_node);
                write_node(fs, PAGE_SIZE * (node_array_index + 1), *this);
                parent.split(tmp_k, tmp_v, parent_insert_location, new_node.node_array_index, hp, fs);
            } else { // else, add to the node
                // shift over existing links
                for (int i = parent.curr_capacity - 1; i >= parent_insert_location; --i) {
                    memcpy(parent.keys[i + 1], parent.keys[i], 64);
                    memcpy(parent.values[i + 1], parent.values[i], 64);
                    parent.children[i + 2] = parent.children[i + 1];
                }
                // add new node link
                memcpy(parent.keys[parent_insert_location], tmp_k, 64);
                memcpy(parent.values[parent_insert_location], tmp_v, 64);
                parent.children[parent_insert_location + 1] = new_node.node_array_index;
                parent.curr_capacity++;
                write_node(fs, PAGE_SIZE * (parent.node_array_index + 1), parent);
                write_node(fs, PAGE_SIZE * (new_node.node_array_index + 1), new_node);
                write_node(fs, PAGE_SIZE * (node_array_index + 1), *this);
            }
        }
    }

    char* insert(char key[64], char value[64], HeaderPage& hp, fstream& fs) {
        int insert_location = 0;
        // loop through and update value if exists
        for (insert_location = 0; insert_location < curr_capacity; ++insert_location) {
            int cmp_res = strcmp(key, keys[insert_location]);
            if (cmp_res == 0) {
                char * tmp = values[insert_location]; // WARNING: validate
                memcpy(values[insert_location], value, 64);
                write_node(fs, PAGE_SIZE * (node_array_index + 1), *this);
                return tmp;
            } else if (cmp_res < 0) {
                break; // do not continue for rest of loop
            }
        }
        char * result = nullptr;
        if ((node_type & NODE_TYPE::LEAF) != 0 && curr_capacity + 1 > NKEYS) { // if at full leaf
            split(key, value, insert_location, -1, hp, fs);
        } else {
            if ((node_type & NODE_TYPE::LEAF) == 0) { // if not a leaf, keep searching
                Node child = get_child(insert_location, fs);
                child.parent_index = node_array_index;
                write_node(fs, PAGE_SIZE * (child.node_array_index + 1), child);
                result = child.insert(key, value, hp, fs);
            } else {
                // shift over values
                for (int i = curr_capacity - 1; i >= insert_location; --i) {
                    memcpy(keys[i + 1], keys[i], 64);
                    memcpy(values[i + 1], values[i], 64);
                }
                // perform insertion
                memcpy(keys[insert_location], key, 64);
                memcpy(values[insert_location], value, 64);
                curr_capacity++;
                write_node(fs, PAGE_SIZE * (node_array_index + 1), *this);
            }
        }
        return result;
    }

    void print(int level, fstream& fs, bool recursive=true) {
        cout << string(level * 2, ' ') << "Capacity: " << curr_capacity  << " " << get_type(level) << endl;
        if (((node_type & NODE_TYPE::LEAF) == 0) && recursive) {
            Node child = get_child(0, fs);
            child.print(level+1, fs, recursive);
        }
        for (int i = 0; i < curr_capacity; ++i) {
            cout << string(level * 2, ' ') << keys[i] << " : " << values[i] << " " << get_type(level) << endl;
            if (((node_type & NODE_TYPE::LEAF) == 0) && recursive) {
                Node child = get_child(i + 1, fs);
                child.print(level+1, fs, recursive);
            }
        }
    }

    string get_type(int depth) {
        string res = "";
        if ((node_type & NODE_TYPE::ROOT) != 0) {
            res += "*";
        } 
        if ((node_type & NODE_TYPE::LEAF) == 0) {
            res += "I";
        } 
        if ((node_type & NODE_TYPE::LEAF) != 0) {
            res += "L";
        }
        return "(" + res  + to_string(depth) + ")";
    }

    static Node read_node(fstream& is, streampos addr) {
        assert(sizeof(Node) == PAGE_SIZE && "Node should be the size of a full page");
        Node n;
        is.seekg(addr, ios::beg);
        is.read(reinterpret_cast<char*>(&n), PAGE_SIZE);
        return n;
    }

    static void write_node(fstream& os, streampos addr, Node &n) {
        assert(sizeof(Node) == PAGE_SIZE && "Node should be the size of a full page");
        os.seekp(addr);
        os.write(reinterpret_cast<const char *>(&n), PAGE_SIZE);
        // equivalent of fsync(fd) in C++
        os.flush();
        os.sync(); 
    }
};

HeaderPage read_header_page(fstream& is) {
    assert(sizeof(HeaderPage) == PAGE_SIZE && "Header should be the size of a full page");
    HeaderPage hp;
    is.seekg(0, ios::beg);
    is.read(reinterpret_cast<char*>(&hp), PAGE_SIZE);
    return hp;
}

void write_header_page(fstream& os, HeaderPage &hp) {
    assert(sizeof(HeaderPage) == PAGE_SIZE && "Header should be the size of a full page");
    os.seekp(0);
    os.write(reinterpret_cast<const char *>(&hp), PAGE_SIZE);
    // equivalent of fsync(fd) in C++
    os.flush();
    os.sync();
}

struct DiskBTree {
    string path;
    fstream fs;

    DiskBTree(string path_, bool is_new) {
        path = path_;
        if (is_new) {
            ofstream tmp(path);
            tmp.close();
        }

        fs = fstream(path, ios::binary | ios::in | ios::out);

        if (is_new) {
            HeaderPage hp;
            hp.n_nodes = 1;
            Node root;
            root.node_array_index = 0;
            root.node_type = NODE_TYPE::ROOT | NODE_TYPE::LEAF;
            write_header_page(fs, hp);
            Node::write_node(fs, PAGE_SIZE, root);
        }
    }

    ~DiskBTree() {
        fs.close();
    }

    char* find(char key[64]) {
        HeaderPage hp = read_header_page(fs);
        Node root = Node::read_node(fs, PAGE_SIZE * (hp.root_index + 1));
        return root.find(key, fs);
    }

    char* insert(char key[64], char value[64]) {
        HeaderPage hp = read_header_page(fs);
        Node root = Node::read_node(fs, PAGE_SIZE * (hp.root_index + 1));
        char* result = root.insert(key, value, hp, fs);
        write_header_page(fs, hp);
        return result;
    }

    void print() {
        HeaderPage hp = read_header_page(fs);
        Node root = Node::read_node(fs, PAGE_SIZE * (hp.root_index + 1));
        root.print(0, fs, true);
    }
};