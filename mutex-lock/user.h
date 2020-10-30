struct stat;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int kthread_create(int,int, int, int, int);
int kthread_join(int);
int kthread_mutex_init(void);
int kthread_mutex_destroy(int);
int kthread_mutex_lock(int);
int kthread_mutex_unlock(int);
int kthread_cond_init(void);
int kthread_cond_destroy(int);
int kthread_cond_wait(int,int);
int kthread_cond_signal(int);
int kthread_cond_broadcast(int);
int kthread_exit(int) __attribute__((noreturn));
int kthread_saveretval(int);
int kthread_fetchretval(int);


// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
