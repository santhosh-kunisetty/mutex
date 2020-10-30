#include "types.h"
#include "stat.h"
#include "user.h"
#include "qthread.h"

#define MAX_THREADS 10

//#ifndef NULL
//# warning "NULL not defined!"
//# define NULL (void*)0
//#else
//# warning "NULL defined!"
//#endif


void *f1(void *arg);
void *f2(void *v);

int N = 10;
int var = 0;
qthread_cond_t t1;

int mvar= 0;
void *f4(void *v);

qthread_mutex_t m;
int t1rdy;

#define THREADSTACKSIZE 4096

void test1(void){

	qthread_t t[MAX_THREADS];
    qthread_mutex_init(&m);

	int i,j;

    
	for(i = 0; i < MAX_THREADS; i++){
        	int tid = qthread_create(&t[i], f2, (void*)&i);
        	//printf(1, "[%d : %d]\n", tid, t[i]->tid);
            printf(1, "[%d : %d]\n", tid, t[i]);
	}

	for (i = 0; i < MAX_THREADS; i++){
        	//printf(1, "%d\n", t[i]->tid);
            printf(1, "%d\n", t[i]);
    	}

  	for (i = 0; i < MAX_THREADS; i++) {
        	qthread_join(t[i], (void**)&j);
		//        assert(i == j);
    	}
    
   printf(1,"%d\n",mvar);
}


//extern void wrapper(qthread_func_ptr_t func, void *arg);

/*void test1(void)
{
    //qthread_t t[MAX_THREADS] = {0};
    struct qthread t[MAX_THREADS] = {};
    //char stacks[MAX_THREADS][THREADSTACKSIZE];
    int i, j, tid;

#ifndef TEST
    for (i = 0; i < MAX_THREADS; i++){
        //t[i] = malloc(sizeof(struct qthread));
        //tid = t[i]->tid = kthread_create((int)malloc(THREADSTACKSIZE), (int)wrapper, (int)f1, i);
        tid = t[i].tid = kthread_create((int)malloc(THREADSTACKSIZE), (int)wrapper, (int)f1, i);
	//printf(1, "[%s:%d]: tid: %d, %p: TID1: %d\n", __FUNCTION__, __LINE__, tid, t[i], t[i]->tid);
	printf(1, "[%s:%d]: tid: %d, %p: TID1: %d\n", __FUNCTION__, __LINE__, tid, t[i], t[i].tid);
    }
#endif

#ifdef ORIG
    for (i = 0; i < MAX_THREADS; i++){
	tid = qthread_create(&t[i], f1, (void*)&i);
	printf(1, "[%s:%d]: tid: %d, %p: TID1: %d\n", __FUNCTION__, __LINE__, tid, t[i], t[i]->tid);
    }
#endif

#ifdef DEBUG
    for(i=0;i<MAX_THREADS;i++) {
	printf(1, "[%s:%d]: tid: %d, %p: TID1: %d\n", __FUNCTION__, __LINE__, tid, t[i], t[i]->tid);
    }
#endif

    for (i = 0; i < MAX_THREADS; i++) {
	//printf(1,"[%s:%d]: %p: TID3: %d\n", __FUNCTION__, __LINE__, t[i], t[i]->tid);
	printf(1,"[%s:%d]: %p: TID3: %d\n", __FUNCTION__, __LINE__, t[i], t[i].tid);
        //qthread_join(t[i], (void**)&j);
        qthread_join(&t[i], (void**)&j);
//        assert(i == j);
    }

    printf(1, "test 1 OK\n");
}
*/

int main(void){

//test1();
//exit();

     t1rdy = 0;
     qthread_cond_init(&t1);

    qthread_t t[10];
    int i, j;
    for (i = 0; i < 10; i++){
        qthread_create(&t[i], f4, (void*)&i);
    }
    for (i = 0; i < 10; i++) {
        qthread_join(t[i], (void**)&j);
    }
     qthread_cond_destroy(&t1);

     printf(1,"Final Count: %d\n", t1rdy);

     printf(1,"Test 3 OK\n");
     exit();

}

void *f1(void *arg) {

       // printf(1, "My PID: %d\n", getpid());
        return arg;
}

void *f2(void *v)
{
    qthread_mutex_lock(&m);
    mvar++;
    sleep(1);
    qthread_mutex_unlock(&m);

    return 0;
}

void *f4(void *v) {


    qthread_mutex_lock(&m);
    t1rdy++;

    printf(1,"count: %d\n", t1rdy);
    if(t1rdy == N)
        var = 1;

    while(!var) 
        qthread_cond_wait(&t1, &m);

    qthread_cond_signal(&t1);
    printf(1,"count: %d\n", t1rdy);
    t1rdy--;
    qthread_mutex_unlock(&m);
    return 0;
}