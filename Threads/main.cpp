#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>


void* myturn(void * arg){
    for(int i=0; i < 8; i++){
        sleep(1);
        printf("'My turn! %d\n",i);

    }
    //what if we return a value
    return NULL;
}

void yourturn() {
    for (int i = 0; i < 3; ++i) {
        sleep(2);
        printf("Your Turn! %d\n",i);
    }
}


int main() {
    pthread_t newthread;

    pthread_create(&newthread, NULL, myturn, NULL);
    yourturn();

    pthread_join(newthread, NULL);


    return 0;
}
