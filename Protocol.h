const int KEY_SIZE = 64;

enum REQUEST_TYPE {
    PUT,
    GET,
    CONTAINS
};

struct Protocol {
    REQUEST_TYPE request_type;
    bool in_database;
    char key[KEY_SIZE];
    char value[KEY_SIZE];
    char padding[256 - 136];
};