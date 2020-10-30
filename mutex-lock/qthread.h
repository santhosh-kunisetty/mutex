struct qthread;
struct qthread_mutex;
struct qthread_cond;
struct qthreadList;

/* function pointer w/ signature 'void *f(void*)'
 */
typedef void *(*qthread_func_ptr_t)(void*);  

//typedef struct qthread *qthread_t;
typedef int qthread_attr_t;       /* need to support detachstate */
typedef void qthread_mutexattr_t; /* no mutex attributes needed */
typedef void qthread_condattr_t;  /* no cond attributes needed */
typedef int *qthread_t;
typedef int qthread_mutex_t;
typedef int qthread_cond_t;

int  qthread_create(qthread_t *thread, qthread_attr_t *attr, qthread_func_ptr_t start, void *arg);
int  qthread_join(qthread_t thread, void **retval);
void qthread_exit(void *val);

int qthread_mutex_init(qthread_mutex_t *mutex, qthread_mutexattr_t *attr);
int qthread_mutex_destroy(qthread_mutex_t *mutex);
int qthread_mutex_lock(qthread_mutex_t *mutex);
int qthread_mutex_unlock(qthread_mutex_t *mutex);

int qthread_cond_init(qthread_cond_t *cond, qthread_condattr_t *attr);
int qthread_cond_destroy(qthread_cond_t *cond);
int qthread_cond_wait(qthread_cond_t *cond, qthread_mutex_t *mutex);
int qthread_cond_signal(qthread_cond_t *cond);
int qthread_cond_broadcast(qthread_cond_t *cond);




