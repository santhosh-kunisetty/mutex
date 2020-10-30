/*
 * file:        threadtests.c
 * description: test file for qthreads (homework 3)
 *
 */

#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"

#include "qthread.h"

#define NULL 0
const int stdout = 1;

/* 1. create and join. Create N threads, which don't do anything
 * except return a value. (and possibly print something) Call
 * qthread_join() to wait for each of them.
 */
qthread_mutex_t m1;
void *f1(void *arg) {
    int x = (int)arg;
    qthread_mutex_lock(&m1);
    printf(stdout, "thread %d\n", x);
    qthread_mutex_unlock(&m1);
    return arg;
}

void test1(void)
{
    qthread_t t[10];
    int i, j;
    qthread_mutex_init(&m1, NULL);
    for (i = 0; i < 10; i++)
        qthread_create(&t[i], NULL, f1, (void*)i);
    for (i = 0; i < 10; i++) {
        qthread_join(t[i], (void**)&j);
        if (i != j) {
            printf(stdout, "thread join value error: %d != %d\n", i, j);
            exit();
        }
    }
    printf(stdout, "test 1 OK\n");
}

/* 2. mutex and sleep.
 * initialize a mutex
 * Create thread 1, which locks a mutex and goes to sleep for a
 * second or two using qthread_usleep.
 *   (you can wait for this by calling qthread_yield repeatedly,
 *    waiting for thread 1 to set a global variable)
 * threads 2..N try to lock the mutex
 * after thread 1 wakes up it releases the mutex; 
 * threads 2..N each release the mutex after getting it.
 * run with N=2,3,4
 */
// ASSUMPTIONS: mutex_t is an int. (initializes it to -1)
int t1rdy;
qthread_mutex_t m = -1;
void *f2(void *v)
{
    qthread_mutex_lock(&m);
    t1rdy = 1;
    sleep(2);
    qthread_mutex_unlock(&m);

    return 0;
}

void *f22(void *v)
{
    qthread_mutex_lock(&m);
    printf(stdout, "f22\n");
    qthread_mutex_unlock(&m);
    return 0;
}
    
void test2(void)
{
    qthread_t t0, t[10];
    int i;
    
    qthread_mutex_init(&m, NULL);
    qthread_create(&t0, NULL, f2, NULL);
    while (!t1rdy)
        sleep(1);
    for (i = 0; i < 4; i++)
        qthread_create(&t[i], NULL, f22, NULL);

    void *val;
    qthread_join(t0, &val);
    for (i = 0; i < 4; i++)
        qthread_join(t[i], &val);

    qthread_mutex_destroy(&m);
    printf(stdout, "test 2 done\n");
}

    /* 3. condvar and sleep.
     * initialize a condvar and a mutex
     * create N threads, each of which locks the mutex, increments a
     *   global count, and waits on the condvar; after the wait it
     *   decrements the count and releases the mutex.
     * call qthread_yield until count indicates all threads are waiting
     * call qthread_signal, qthread_yield until count indicates a
     *   thread has left. repeat.
     */
qthread_cond_t c = -1;
int count;
void *f3(void *arg)
{
    qthread_mutex_lock(&m);
    count++;
    qthread_cond_wait(&c, &m);
    count--;
    qthread_mutex_unlock(&m);
    return 0;
}

void test3(void)
{
    int i;
    qthread_t t[10];

    qthread_mutex_init(&m, NULL);
    qthread_cond_init(&c, NULL);
    
    for (i = 0; i < 10; i++)
        qthread_create(&t[i], NULL, f3, NULL);
    while (count < 10)
        sleep(1);


    printf(1, "GOING TO SIGNAL!!!\n");
    for (i = 0; i < 10; i++) {
        sleep(1);
        qthread_cond_signal(&c);
    }
    sleep(1);
    if (count != 0) {
        printf(stdout, "Test 3a failed: count %d\n", count);
	for(i=0;i<10;i++)
        	qthread_join(t[i], NULL);
        exit();
    }

    for(i=0;i<10;i++)
        qthread_join(t[i], NULL);

    printf(stdout, "Test 3a OK\n");

    for (i = 0; i < 10; i++)
        qthread_create(&t[i], NULL, f3, NULL);
    while (count < 10)
        sleep(1);
    for (i = 0; i < 10; i++) {
        qthread_cond_signal(&c);
    }
    int t0 = uptime();
    while (count != 0)
        if (uptime() > t0+20) {
            printf(stdout, "Test 3b: timeout\n");
	    for(i=0;i<10;i++)
        	qthread_join(t[i], NULL);
            exit();
        }

    for(i=0;i<10;i++)
        qthread_join(t[i], NULL);

    printf(stdout, "Test 3b OK\n");
    
    for (i = 0; i < 10; i++)
        qthread_create(&t[i], NULL, f3, NULL);
    while (count < 10)
        sleep(1);
    qthread_cond_broadcast(&c);
    t0 = uptime();
    while (count != 0)
        if (uptime() > t0+2)
            break;
    if (count != 0) {
        printf(stdout, "Test 3c failed: count=%d\n", count);
	for(i=0;i<10;i++)
        	qthread_join(t[i], NULL);
        exit();
    }

    for(i=0;i<10;i++)
	qthread_join(t[i], NULL);

    qthread_mutex_destroy(&m);
    printf(1, "Mutex Destroyed\n");
    qthread_cond_destroy(&c);
    m = c = -1;                 // ASSUMPTION: mutex_t, cond_t = int
    
    printf(1, "condvar destroyed\n");
    printf(stdout, "Test 3 OK\n");
}

    /* 4. read/write
     * create a pipe ('int fd[2]; pipe(fd);')
     * create 2 threads:
     * one sleeps and then writes to the pipe
     * one reads from the pipe. [this tests blocking read]
     */
int a_pipe[2];
qthread_t t1, t2;

void *f41(void* val)
{
    char *data = "1234567890";
    sleep(1);
    write(a_pipe[1], data, 10);
    qthread_join(t2, &val);
    printf(stdout, "f41 done\n");
    close(a_pipe[1]);
    return 0;
}

void *f42(void* arg)
{
    char buf[16];
    int n = read(a_pipe[0], buf, 10);
    if (n != 10) {
        printf(stdout, "Test 4 failed: %d bytes\n", n);
        exit();
    }
    close(a_pipe[0]);
    printf(stdout, "f42 done\n");
    return 0;
}

void test4(void)
{
    void *val;

    pipe(a_pipe);
    qthread_create(&t1, NULL, f41, NULL);
    qthread_create(&t2, NULL, f42, NULL);

    sleep(5);
    qthread_join(t1, &val);

    // ASSUMPTION: qthread_t is the same as pid
    if (kill(*t1) >= 0 || kill(*t2) >= 0) {
        printf(stdout, "Test 4: threads still alive\n");
        exit();
    }
    printf(stdout, "Test 4 OK\n");
}

/* Test that exit works properly (UNIX semantics - kills all threads)
 * OK if it doesn't pass.
 */
void *f5(void *arg) {
    if (arg != 0) {
        sleep(10);
        exit();
    }
    sleep(100);
    return 0;
}
void test5(void)
{
    int i, detached = 1;
    qthread_t t[10];
    
    if (fork() == 0) {
        for (i = 0; i < 10; i++)
            qthread_create(&t[i], NULL, f5, (void*)i);
        sleep(100);
        exit();
    }

    wait();
    printf(stdout, "Test 5a OK\n");

    if (fork() == 0) {
        for (i = 0; i < 10; i++)
            qthread_create(&t[i], &detached, f5, (void*)i);
        sleep(100);

        exit();
    }
    wait();
    printf(stdout, "Test 5b OK\n");

    if (fork() == 0) {
        for (i = 0; i < 10; i++)
            qthread_create(&t[i], NULL, f5, (void*)(i == 5));
        sleep(1000);
    }
    wait();
    printf(stdout, "Test 5c OK\n");
}

int main(int argc, char **argv)
{
    /* Here are some suggested tests to implement. You can either run
     * them one after another in the program (cleaning up threads
     * after each test) or pass a command-line argument indicating
     * which test to run.
     * This may not be enough tests to fully debug your assignment,
     * but it's a good start.
     */

    test1();
    test2();
    
    /* 3. condvar and sleep.
     * initialize a condvar and a mutex
     * create N threads, each of which locks the mutex, increments a
     *   global count, and waits on the condvar; after the wait it
     *   decrements the count and releases the mutex.
     * call qthread_yield until count indicates all threads are waiting
     * call qthread_signal, qthread_yield until count indicates a
     *   thread has left. repeat.
     */
    test3();
    
    /* 4. read/write
     * create a pipe ('int fd[2]; pipe(fd);')
     * create 2 threads:
     * one sleeps and then writes to the pipe
     * one reads from the pipe. [this tests blocking read]
     */
    test4();
    
    /* test 5 - exit
     * create 10 threads, have them each sleep, then call exit
     */
    test5();
    
    exit();
    return 0;
}
