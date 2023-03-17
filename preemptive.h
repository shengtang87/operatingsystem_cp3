#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 4  /* not including the scheduler */
/* the scheduler does not take up a thread of its own */

#define CNAME(s) _ ## s

#define SemaphoreCreate(s, n)\
            s = n\
                               

#define SemaphoreSignal(s)\
            __asm \
            INC CNAME(s)\
            __endasm;\


#define SemaphoreWaitBody(s, label) \
    { __asm \
        label:\
        MOV A, CNAME(s)\
        JZ  label\
        JB ACC.7, label\
        dec  CNAME(s) \
      __endasm; }

typedef char ThreadID;
typedef void (*FunctionPtr)(void);

ThreadID ThreadCreate(FunctionPtr);
void ThreadYield(void);
void ThreadExit(void);
void myTimer0Handler();



#endif // __PREEMPTIVE_H__