#include <8051.h>
#include "preemptive.h"

__data __at (0x3A) char buffer;
__data __at (0x3B) char character;
__data __at (0x3C) char full;
__data __at (0x3D) char mutex;
__data __at (0x3E) char empty;

#define L(x) LABEL(x)
#define LABEL(x) x##$


void Producer(void) {
	character = 'A';
        while (1) {
                SemaphoreWaitBody(empty, L(__COUNTER__) );
                SemaphoreWaitBody(mutex, L(__COUNTER__) );
                buffer = character;
                if(character == 'Z') character = 'A';
                else character +=  1;
                SemaphoreSignal(mutex);
                SemaphoreSignal(full);                
        }
}

void Consumer(void) {
        EA = 0;
        TMOD |= 0x20; // TMOD is also assigned by the (modified) Bootstrap code to set up the timer interrupt in timer-0 for preemption 
        TH1 = -6;
        SCON = 0x50;
        TR1 = 1;
        EA = 1;
        while (1) {
                
                SemaphoreWaitBody(full,  L(__COUNTER__));
                SemaphoreWaitBody(mutex,  L(__COUNTER__));
                SBUF = buffer;
                SemaphoreSignal(mutex);
                SemaphoreSignal(empty);

                while(!TI){}
                TI=0;
        }
}

/* [5 pts for this function]
 * main() is started by the thread bootstrapper as thread-0.
 * It can create more thread(s) as needed:
 * one thread can act as producer and another as consumer.
 */
void main(void) {
        SemaphoreCreate(mutex, 1);
        SemaphoreCreate(full, 0);
        SemaphoreCreate(empty,1);
        ThreadCreate(Producer);
        Consumer();
}

void _sdcc_gsinit_startup(void) {
        __asm
                ljmp  _Bootstrap
        __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}
void timer0_ISR(void) __interrupt(1) {
        __asm
            ljmp _myTimer0Handler
        __endasm;
}