/* Bidirectional list of integers. */
struct bidint {
    int       val;
    struct bidint *prev;
    struct bidint *next;
};
