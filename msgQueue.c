/*
 * Name: Preston Tighe
 * 5343 Operating Systems
 * Program 4
 *
 * Command: gcc -std=c99 -D _XOPEN_SOURCE -pthread -o msgQueue msgQueue.c -lm && ./msgQueue
 *
 * Mutual Exclusion: This is made possible by a queue that has sending & receiving functions.
 * When a thread calls msgrcv, the thread is put into a blocked state until a real messages appears in the queue.
 * Only 1 thread is allowed to enter the critical section to read a message from the queue (ex: prevent 3 threads
 * from reading the same queue message at the same time)
 *
 * Synchronization: The msgrv function is a blocking function so the producer/consumer problem is solved by preventing
 * consumers from receiving messages from the queue until a message exists.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <math.h>


void createProducer(int, int, int);
void createConsumers(int, int, int);

typedef struct {
    int numItems, threadNum, msqId, msg_size;
} thread_args;

struct rand_msgbuf {
    long msgType;
    int rand;
};

void *threadFunc(void* args) {
    thread_args* arguments = args;
    int numItems = arguments->numItems;
    int threadNum = arguments->threadNum;
    int msqId = arguments->msqId;
    int msg_size = arguments->msg_size;
    struct rand_msgbuf incomingMSG = {1, 0};

    printf("Thread %d Created\n", threadNum);

    int C, total = 0;

    for(int i = 0; i < numItems; i++) {
        // Read incoming message
        msgrcv(msqId, &incomingMSG, msg_size, 1, 0);
        C = incomingMSG.rand;

        // Consumer running total
        total += C;

        // Print current state
        printf("\tConsumer %d thread consumed a %d\n", threadNum, C);

        // Consumer thread 1-3 sleep
        sleep(rand() % 3 + 1);
    }

    printf("Total consumed by consumer thread %d = %d\n", threadNum, total);
    printf("Terminating Thread %d\n", threadNum);

    pthread_exit(NULL);
}


int main(int argc, char *argv[ ]) {
    int numItems;
    if (argc == 2) {
        numItems = atoi(argv[1]);
    } else {
        numItems = 5;
    }

    srand(time(NULL));

    // Queue key
    key_t key = ftok("", 'a');

    // Creates a message queue
    int msqId = msgget(key, 0666 | IPC_CREAT);

    // Queue message size
    int msg_size = sizeof(int);

    // Create child process
    pid_t childPID = fork();

    if(childPID == 0){
        createConsumers(numItems, msqId, msg_size);
    } else {
        createProducer(numItems, msqId, msg_size);
    }
    msgctl(msqId, IPC_RMID, NULL);
}



void createProducer(int numItems, int msqId, int msg_size) {
    printf("Producer Created\n");
    int R, total = 0, status;

    struct rand_msgbuf outgoingMSG = {1, 0};

    for(int i = 0; i < numItems; i++) {
        // 0-999 random number
        R = rand() % 1000;

        // Send random number into queue from consumers
        outgoingMSG.rand = R;
        msgsnd(msqId, &outgoingMSG, msg_size, 0);

        // Producer total
        total += R;

        // Print current state
        printf("Producer produced a %d\n", R);

        // 0-1 second sleep
        sleep(rand() % 2);
    }

    // Exit when all child processes are complete
    wait(&status);

    // Print current state
    printf("Total produced = %d\n\n", total);
}

void createConsumers(int numItems, int msqId, int msg_size) {
    // Create consumer threads
    int numThreads = 3;
    int threadNumItems;
    int itemsLeft = numItems;

    // Store all the threads
    pthread_t threads[numThreads];

    // pthread setup
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Create threads
    thread_args threadArgs[numThreads];
    for(int i = 0; i < numThreads; i++) {
        // Calculate num of items for i thread
        if (itemsLeft <= 0) {
            threadNumItems = 0;
        } else {
            threadNumItems = (int) ceil((float) numItems / numThreads);
            if (itemsLeft - threadNumItems < 0) {
                threadNumItems = itemsLeft;
                itemsLeft = 0;
            } else {
                itemsLeft -= threadNumItems;
            }
        }

        // Create thread args
        thread_args newThreadArgs = {threadNumItems, i, msqId, msg_size};
        threadArgs[i] = newThreadArgs;

        // Create single thread
        pthread_create(&threads[i], &attr, threadFunc, &threadArgs[i]);
    }

    // Destroy thread
    pthread_attr_destroy(&attr);

    // Join threads
    for(int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    exit(1);
}
