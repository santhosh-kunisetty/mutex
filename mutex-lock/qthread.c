#include "types.h"
#include "qthread.h"
#include "user.h"

#define THREADSTACKSIZE 4096

void wrapper(qthread_func_ptr_t func, void *arg) {

    void *ptr = func(arg);
    kthread_exit((int)ptr);
    //exit();
}

int qthread_create(qthread_t *thread,qthread_attr_t *attr, qthread_func_ptr_t my_func, void *arg) {

    *thread = (qthread_t)malloc(sizeof(int));
    **thread = kthread_create((int)((char*)malloc(THREADSTACKSIZE) + THREADSTACKSIZE), *attr, (int)wrapper, (int)my_func, (int)arg);

    if(**thread == -1)
    	return **thread;
    else
       return 0;
}

int qthread_join(qthread_t thread, void **retval){


    int val = kthread_join(*thread);

    if(val != -1)
	if((val  = kthread_fetchretval(*thread)) != -1)
		*retval = (void*)val;	

    if(val != -1)
	val = 0;

    return val;
}

int qthread_mutex_init(qthread_mutex_t *mutex, qthread_mutexattr_t *attr){
	*mutex = kthread_mutex_init();
	if (*mutex > 0){
		return 0;
	}

	return *mutex;
}

int qthread_mutex_destroy(qthread_mutex_t *mutex){
    int val = kthread_mutex_destroy(*mutex);
    if (val < 0){
    	return -1;
    }
    return 0;
}

int qthread_mutex_lock(qthread_mutex_t *mutex){

    int val = kthread_mutex_lock(*mutex);
    if (val < 0){
    	return -1;
    }
    return 0;
}

int qthread_mutex_unlock(qthread_mutex_t *mutex){

    int val = kthread_mutex_unlock(*mutex);
    if (val < 0){
    	return -1;
    }
    return 0;
}

int qthread_cond_init(qthread_cond_t *cond, qthread_condattr_t *attr){
    *cond = kthread_cond_init();
    if (*cond > 0){
        return 0;
    }
    return *cond;
}

int qthread_cond_destroy(qthread_cond_t *cond){
    int val = kthread_cond_destroy(*cond);
    if (val < 0){
        return -1;
    }
    return 0;
}

int qthread_cond_signal(qthread_cond_t *cond){

    int val = kthread_cond_signal(*cond);
    if (val < 0){
        return -1;
    }
    return 0;
}

int qthread_cond_broadcast(qthread_cond_t *cond){

    int val = kthread_cond_broadcast(*cond);
	if (val < 0){
        return -1;
    }
    return 0;
    
}

int qthread_cond_wait(qthread_cond_t *cond, qthread_mutex_t *mutex){

    int val = kthread_cond_wait(*cond, *mutex);
	if (val < 0){
        return -1;
    }
    return 0;
    
}

void qthread_exit(void *retval){
    kthread_exit((int)retval);
}
