//
// Created by amirsalar on 12/16/23.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
//#include "kernel/proc.h"
typedef unsigned long uint64;



/* Possible states of a thread: */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4
// Saved registers for kernel context switches.
struct context {
    uint64 ra;
    uint64 sp;

    // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};


struct thread{

    char * name;               // thread name
    char stack[STACK_SIZE];     // the thread's stack
    int state;                  // FREE, RUNNING, RUNNABLE
    struct context context;            // swtch() here to run thread
};

struct thread threads[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(struct context *prev, struct context *next);

void f1(void *arg);
void f2(void *arg);
void f3(void *arg);




void
uthread_create(void (*start_routine)(void*), void *arg , char * name)
{
    int i;
    struct thread *t;

    for(i = 0; i < MAX_THREAD; i++){
        t = &threads[i];
        if(t->state == FREE)
            break;
    }
    if(i == MAX_THREAD){
        printf("uthread_create: no available threads\n");
        return ;
    }

    t->state = RUNNABLE;
    t->name = name;
    //print the name
    printf("salam\n");
    printf("%s",t->name);
    memset(&t->context, 0, sizeof(t->context));
    t->context.ra = (uint64)start_routine;
    t->context.sp = (uint64)(t->stack + STACK_SIZE) ;
    printf("khodahafez");



}

void
uthread_scheduler(void)
{
    struct thread *t;
    struct thread *next;
    next = 0;
    t = current_thread + 1;
    for(int i = 0; i < MAX_THREAD; i++){
        if(t >= threads +MAX_THREAD )
            t = threads;
        if(t->state == RUNNABLE){
            next = t;
            break;
        }
        t++;
    }
    if(next == 0){
        printf("uthread_scheduler: no runnable threads\n");
        return;
    }
    if(current_thread != next){
        printf("uthread_scheduler: context switch %s -> %s\n",
               current_thread->name, next->name);
        next->state = RUNNING;
        t = current_thread;
        current_thread = next;
        thread_switch(&t->context, &next->context);
    }else
        printf("uthread_scheduler: no context switch %s\n", current_thread->name);



}

//yield fun
void uthread_yield(void)
{
    current_thread->state = RUNNABLE;
    uthread_scheduler();
}


void
uthread_exit(int thread_id)
{
    struct thread *t;

    t = &threads[thread_id];
    t->state = FREE;
    uthread_scheduler();
}

//init
void
uthread_init(void)
{
    int i;

    for(i = 0; i < MAX_THREAD; i++){
        threads[i].state = FREE;
    }

    current_thread = &threads[0];
    current_thread->state = RUNNING;
    printf("thread library initialized\n");
    //uthread_scheduler();
}




























volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;


int
main(int argc, char *argv[])
{
    // int i;
    // int sum = 0;
    uthread_init();
    uthread_create(f1, (void *)1, "T_1");
    uthread_create(f2, (void *)2, "T_2");
    uthread_create(f3, (void *)3, "T_3");

     uthread_scheduler();
     exit(0);
}
//after each thread has been busy for its allowed 10 ticks, it must change its state and call scheduler
void
f1(void *arg) {
    int up1 = 0;
    int up2 = 0;
    printf("thread_a started\n");
    a_started = 1;
    while(b_started == 0 || c_started == 0)
        uthread_yield();
    while(a_n< 100) {
        while (up2 - up1 < 20) {
            up1 = up2;
            up2 = uptime();
            printf("thread_a %d\n", a_n);
            a_n += 1;
        }
        up1 = 0;
        up2 = 0;
        uthread_yield();
    }

    printf("thread_a: exit after %d\n", a_n);

    current_thread->state = FREE;
    uthread_scheduler();
}

void
f2(void *arg)
{
    int up1 = 0;
    int up2 = 0;
    printf("thread_b started\n");
    b_started = 1;
    while(a_started == 0 || c_started == 0)
        uthread_yield();
    while(b_n< 100) {
        while (up2 - up1 < 10) {
            up1 = up2;
            up2 = uptime();
            printf("thread_b %d\n", b_n);
            b_n += 1;
        }
        up1 = 0;
        up2 = 0;
        uthread_yield();
    }
    printf("thread_b: exit after %d\n", b_n);

    current_thread->state = FREE;
    uthread_scheduler();
}

void
f3(void *arg)
{
    int up1 = 0;
    int up2 = 0;

    printf("thread_c started\n");
    c_started = 1;
    while(a_started == 0 || b_started == 0)
        uthread_yield();
    while(c_n< 100) {
        while (up2 - up1 < 10) {
            up1 = up2;
            up2 = uptime();
            printf("thread_c %d\n", c_n);
            c_n += 1;
        }
        up1 = 0;
        up2 = 0;
        uthread_yield();
    }

    printf("thread_c: exit after %d\n", c_n);
    current_thread->state = FREE;
    uthread_scheduler();
}




