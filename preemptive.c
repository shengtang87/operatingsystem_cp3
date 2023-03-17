#include <8051.h>

#include "preemptive.h"

/*
 * @@@ [2 pts] declare the static globals here using 
 *        __data __at (address) type name; syntax
 * manually allocate the addresses of these variables, for
 * - saved stack pointers (MAXTHREADS)
 * - current thread ID
 * - a bitmap for which thread ID is a valid thread; 
 *   maybe also a count, but strictly speaking not necessary
 * - plus any temporaries that you need.
 */
__data __at (0x30) char saved_sp[4];
__data __at (0x34) char mask;
__data __at (0x35) char cur_thread;
__data __at (0x36) char i;
__data __at (0x37) char temp;
__data __at (0x38) char new_thread;

/*
 * @@@ [8 pts]
 * define a macro for saving the context of the current thread by
 * 1) push ACC, B register, Data pointer registers (DPL, DPH), PSW
 * 2) save SP into the saved Stack Pointers array
 *   as indexed by the current thread ID.
 * Note that 1) should be written in assembly, 
 *     while 2) can be written in either assembly or C
 */
#define SAVESTATE {\
         __asm \
            PUSH ACC\
            PUSH B\
            PUSH DPL\
            PUSH DPH\
            PUSH PSW\
         __endasm; \
         saved_sp[cur_thread] = SP;\
        }\
         
/*
 * @@@ [8 pts]
 * define a macro for restoring the context of the current thread by
 * essentially doing the reverse of SAVESTATE:
 * 1) assign SP to the saved SP from the saved stack pointer array
 * 2) pop the registers PSW, data pointer registers, B reg, and ACC
 * Again, popping must be done in assembly but restoring SP can be
 * done in either C or assembly.
 */
#define RESTORESTATE {\
            SP = saved_sp[cur_thread];\
            __asm \
               POP PSW\
               POP DPH\
               POP DPL\
               POP B\
               POP ACC\
            __endasm; \
         }


 /* 
  * we declare main() as an extern so we can reference its symbol
  * when creating a thread for it.
  */

extern void main(void);

/*
 * Bootstrap is jumped to by the startup code to make the thread for
 * main, and restore its context so the thread can run.
 */

void Bootstrap(void) {
      /*
       * @@@ [2 pts] 
       * initialize data structures for threads (e.g., mask)
       *
       * optional: move the stack pointer to some known location
       * only during bootstrapping. by default, SP is 0x07.
       *
       * @@@ [2 pts]
       *     create a thread for main; be sure current thread is
       *     set to this thread ID, and restore its context,
       *     so that it starts running main().
       */
      mask = 0;
      TMOD = 0;  // timer 0 mode 0
      IE = 0x82;  // enable timer 0 interrupt; keep consumer polling
                  // EA  -  ET2  ES  ET1  EX1  ET0  EX0
      TR0 = 1; // set bit TR0 to start running timer 0
      cur_thread = ThreadCreate(main);
      RESTORESTATE;
}

/*
 * ThreadCreate() creates a thread data structure so it is ready
 * to be restored (context switched in).
 * The function pointer itself should take no argument and should
 * return no argument.
 */
ThreadID ThreadCreate(FunctionPtr fp) {
        if(mask==15)
         return -1;
        //a,b
        __critical{
         for(i=0;i<4;i++) {
            if((mask & (1 << i)) == 0){
               mask = mask | (1 << i);
               new_thread = i;
               break;
            }
         }
         //c
         temp = SP;
         SP = (0x3F) + (0x10) * new_thread;
         //d
         __asm
            PUSH DPL
            PUSH DPH
         __endasm;
         //e
         __asm 
            ANL A, #0
            PUSH ACC
            PUSH ACC
            PUSH ACC
            PUSH ACC
         __endasm;
         //f
         PSW = new_thread << 3;
         __asm 
            PUSH PSW
         __endasm;
         //g
         saved_sp[new_thread] = SP;
         //h
         SP = temp;
        }
         //i
         return new_thread;         
}



/*
 * this is called by a running thread to yield control to another
 * thread.  ThreadYield() saves the context of the current
 * running thread, picks another thread (and set the current thread
 * ID to it), if any, and then restores its state.
 */

void ThreadYield(void) {
      __critical{
       SAVESTATE;
       do{
           if(cur_thread == 3) cur_thread = 0;
           else cur_thread += 1;

           if(mask & (1<<cur_thread)) break;
        }while(1);
        RESTORESTATE;
      }
}


/*
 * ThreadExit() is called by the thread's own code to terminate
 * itself.  It will never return; instead, it switches context
 * to another thread.
 */
void ThreadExit(void) {
   __critical{
        mask ^= (1<<cur_thread);
        if(mask == 0) cur_thread = -1;
        else{
            while(1){
               if(cur_thread == 3) cur_thread = 0;
               else cur_thread += 1;
               if(mask & (1<<cur_thread)){
                     break;
               }
            }
        }
        RESTORESTATE;
   }
}


void myTimer0Handler(){
      EA = 0;
      SAVESTATE;
      __asm
         MOV A, R0
         PUSH ACC
         MOV A, R1
         PUSH ACC
         MOV A, R2
         PUSH ACC
         MOV A, R3
         PUSH ACC
         MOV A, R4
         PUSH ACC
         MOV A, R5
         PUSH ACC
         MOV A, R6
         PUSH ACC
         MOV A, R7
         PUSH ACC
      __endasm;
      
      do{
         if(cur_thread == 3) cur_thread = 0;
         else cur_thread += 1;
         if( cur_thread == 0 && mask & 1) break;
         else if( cur_thread == 1 && mask & 2) break;
         else if( cur_thread == 2 && mask & 4) break;
         else if( cur_thread == 3 && mask & 8) break;
      }while(1);
      __asm
         POP ACC
         MOV R7, A
         POP ACC
         MOV R6, A
         POP ACC
         MOV R5, A
         POP ACC
         MOV R4, A
         POP ACC
         MOV R3, A
         POP ACC
         MOV R2, A
         POP ACC
         MOV R1, A
         POP ACC
         MOV R0, A
      __endasm;  
      RESTORESTATE;
      EA = 1;
      __asm 
         RETI
      __endasm;       
}