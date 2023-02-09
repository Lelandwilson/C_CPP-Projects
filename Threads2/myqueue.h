#ifndef THREADS2_MYQUEUE_H
#define THREADS2_MYQUEUE_H



struct node {
    struct node* next;
    int *client_socket;
};
typedef struct node node_t;

void enqueue(int *client_socket);
int* dequeue();

#endif //THREADS2_MYQUEUE_H