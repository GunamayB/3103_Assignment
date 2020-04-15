/*
Written by Gunamay BHARADWAJ. SID:55361670. 
To Compile: g++ 55361670.cpp -lpthread -o 55361670
To Run: ./55361670 <argument-here>
*/
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "generate_frame_vector.cpp"

using namespace std;
#define m 4
#define n 2
#define numQuantize 2

//Defines appending and removal of values from queue
struct camCache {
    double *arr[5];
    unsigned head;
    unsigned tail;
    unsigned size;
    unsigned maxFrame;
    pthread_mutex_t mtx;

    camCache() : head(-1), tail(-1), size(0), maxFrame(5) {}
   
    void append(double *data) {
        while (is_full())
        ;
        int s = pthread_mutex_lock(&mtx);
        if (is_empty()){head = 0;}
        tail = (tail + 1) % maxFrame;
        arr[tail] = data;
        size++;
        s = pthread_mutex_unlock(&mtx);
    }

    void remove(){
        int s = pthread_mutex_lock(&mtx);
        if (head == tail){ head = -1, tail = -1;
        } else { head = (head + 1) % maxFrame;}
        size--;
        s = pthread_mutex_unlock(&mtx);
    }

    double *front() {
        if (size == 0) {return nullptr;}
        return arr[head];
    }

    bool is_empty() {return size == 0;}
    bool is_full() {return size == maxFrame;}

};
camCache cache;
static pthread_mutex_t count_mtx = PTHREAD_MUTEX_INITIALIZER;

void *camera(void *args) {
    double *v;
    int l = m * n;
    int *interval = (int *)args;
    do {
        if (!cache.is_full()) {
            v = generate_frame_vector(l);
            if (v) {cache.append(v);}
        }
        sleep(*interval);
    } while (v);
}

void *quantizer(void *arg) {
    double *t;
    int count = 0;
    do {
        if(!cache.is_empty()) {
            t = cache.front();
            cache.remove();
            int l = m * n;
            for (int i = 0; i < l; i++) {
                t[i] = (t[i] > 0.5) ? 1.0 : 0.0;
            }
            count = 0;
            for (int i = 0; i < l; i++) {
                cout << t[i] << ".0 ";
            }
            cout << endl;
            sleep(3);
        }else {
            count++;
            sleep(1);
        }
    } while (count < 3);
}

int main(int argc, char *argv[]) {

    if(argc == 2) {
        int interval = atoi(argv[1]);
        int createThread, joinThread;

        pthread_t camThread;
        pthread_t quantizeThreads[numQuantize];

        createThread = pthread_create(&camThread, NULL, camera, (void *)&interval);
        
        if (createThread) {
            cout << "Error when creating camera thread!" << endl;
            exit(-1);
        }

        for (int i = 0; i < numQuantize; i++) {
            createThread = pthread_create(&quantizeThreads[i], NULL, quantizer, NULL);
            if (createThread) {
                cout << "Error while creating quantizer thread" << i + 1 << "!" << endl;
                exit(-1);
            }
        }
        joinThread = pthread_join(camThread, NULL);
        
        if (joinThread) {
            cout << "Error while joining camera thread!" << endl;
            exit(-1);
        }

        for (int i = 0; i < numQuantize; i++) {
            joinThread = pthread_join(quantizeThreads[i], NULL);
            if (joinThread) {
                cout << "Error while joining quantizer thread" << i + 1 << "!" << endl;
                exit(-1);
            }
        }
    }
    return 0;
}
