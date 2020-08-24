// os.c
// Runs on LM4F120/TM4C123/MSP432
// Lab 3 starter file.
// Daniel Valvano
// May 24, 2020

#include <stdint.h>
#include "os.h"
#include "CortexM.h"
#include "BSP.h"



// function definitions in osasm.s
void StartOS(void);

void (*myPeriodicTask)(void); //EDN pointer to user function

void (*function1)(void); //Periodic function1, EDN pointer to user function
void (*function2)(void); //Periodic function2, EDN pointer to user function


uint32_t period1;

void runperiodicevents();

#define NUMTHREADS  6        // maximum number of threads
#define NUMPERIODIC 20        // maximum number of periodic threads
#define STACKSIZE   100      // number of 32-bit words in stack per thread

//EDN: TCB DEFINITION
struct threadblk{
  int32_t *sp;       // pointer to stack (valid for threads not running
  struct threadblk *next;  // linked-list pointer
											// nonzero if blocked on this semaphore
											// nonzero if this thread is sleeping
//*FILL THIS IN****
	int32_t *blocked; 
	int32_t sleep;
};

typedef struct threadblk threadblkType;

threadblkType threadblocks[NUMTHREADS];
threadblkType *RunPt;
int32_t Stacks[NUMTHREADS][STACKSIZE];

struct periodic_threadblk{
  void (*thread)();       // pointer to stack (valid for threads not running
	uint32_t pcounter;
	uint32_t psleep;
	uint32_t period;
};

uint32_t pevents =0;
typedef struct periodic_threadblk pthreadblkType;
pthreadblkType pthreadblocks[NUMPERIODIC];

// ******** OS_Init ************
// Initialize operating system, disable interrupts
// Initialize OS controlled I/O: periodic interrupt, bus clock as fast as possible
// Initialize OS global variables
// Inputs:  none
// Outputs: none
void OS_Init(void){
  DisableInterrupts();
  BSP_Clock_InitFastest();// set processor clock to fastest speed
  // perform any initializations needed
}

void SetInitialStack(int i){
//***YOU IMPLEMENT THIS FUNCTION*****
 threadblocks[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer 
  Stacks[i][STACKSIZE-1] = 0x01000000; // Thumb bit 
  Stacks[i][STACKSIZE-3] = 0x14141414; // R14 
  Stacks[i][STACKSIZE-4] = 0x12121212; // R12 
  Stacks[i][STACKSIZE-5] = 0x03030303; // R3 
  Stacks[i][STACKSIZE-6] = 0x02020202; // R2 
  Stacks[i][STACKSIZE-7] = 0x01010101; // R1 
  Stacks[i][STACKSIZE-8] = 0x00000000; // R0 
  Stacks[i][STACKSIZE-9] = 0x11111111; // R11 
  Stacks[i][STACKSIZE-10] = 0x10101010; // R10 
  Stacks[i][STACKSIZE-11] = 0x09090909; // R9 
  Stacks[i][STACKSIZE-12] = 0x08080808; // R8 
  Stacks[i][STACKSIZE-13] = 0x07070707; // R7 
  Stacks[i][STACKSIZE-14] = 0x06060606; // R6 
  Stacks[i][STACKSIZE-15] = 0x05050505; // R5 
  Stacks[i][STACKSIZE-16] = 0x04040404; // R4 
 
}

//******** OS_AddThreads ***************
// Add six main threads to the scheduler
// Inputs: function pointers to six void/void main threads
// Outputs: 1 if successful, 0 if this thread can not be added
// This function will only be called once, after OS_Init and before OS_Launch
int OS_AddThreads(void(*funcThread0)(void),
                  void(*funcThread1)(void),
                  void(*funcThread2)(void),
                  void(*funcThread3)(void),
                  void(*funcThread4)(void),
                  void(*funcThread5)(void)){
  // **similar to Lab 2. initialize as not blocked, not sleeping****
int32_t status; 
  status = StartCritical(); 
  threadblocks[0].next = &threadblocks[1]; // 0 points to 1 
  threadblocks[1].next = &threadblocks[2]; // 1 points to 2 
  threadblocks[2].next = &threadblocks[3]; // 2 points to 3 
  threadblocks[3].next = &threadblocks[4]; // 3 points to 4 
  threadblocks[4].next = &threadblocks[5]; // 4 points to 5 
  threadblocks[5].next = &threadblocks[0]; // 5 points to 0 
  
										
//EDN: Specially added for the blocked variable of each thread i.e setting them all to zero means non is blocked initially  
threadblocks[0].blocked=threadblocks[1].blocked=threadblocks[2].blocked=threadblocks[3].blocked=threadblocks[4].blocked=threadblocks[5].blocked = 0;

//EDN: Specially added for the "sleep" variable of each thread i.e setting them all to zero means non is sleeping initially  
threadblocks[0].sleep=threadblocks[1].sleep=threadblocks[2].sleep=threadblocks[3].sleep=threadblocks[4].sleep=threadblocks[5].sleep = 0;
										
	SetInitialStack(0); Stacks[0][STACKSIZE-2] = (int32_t)(funcThread0); // PC
  SetInitialStack(1); Stacks[1][STACKSIZE-2] = (int32_t)(funcThread1); // PC
  SetInitialStack(2); Stacks[2][STACKSIZE-2] = (int32_t)(funcThread2); // PC
  SetInitialStack(3); Stacks[3][STACKSIZE-2] = (int32_t)(funcThread3); // PC
  SetInitialStack(4); Stacks[4][STACKSIZE-2] = (int32_t)(funcThread4); // PC		
  SetInitialStack(5); Stacks[5][STACKSIZE-2] = (int32_t)(funcThread5); // PC										
  RunPt = &threadblocks[0];        // thread 0 will run first 
  EndCritical(status); 
								
  return 1;               // successful
}

//******** OS_Launch ***************
// Start the scheduler, enable interrupts
// Inputs: number of clock cycles for each time slice
// Outputs: none (does not return)
// Errors: theTimeSlice must be less than 16,777,216

void OS_Launch(uint32_t theTimeSlice){
  STCTRL = 0;                  // disable SysTick during setup
  STCURRENT = 0;               // any write to current clears it
  SYSPRI3 =(SYSPRI3&0x00FFFFFF)|0xE0000000; // priority 7
  STRELOAD = theTimeSlice - 1; // reload value
  STCTRL = 0x00000007;         // enable, core clock and interrupt arm
	//uint32_t frequency = 1000/period;
	//BSP_PeriodicTask_InitC(Thread, 1000, 5);
  //BSP_PeriodicTask_InitB(&runperiodicevents, 1000, 6);  //EDN: THIS WAS NEEDED FOR STEP 4 ONLY
	StartOS();                   // start on the first task
}
// runs every ms


void Scheduler(void){ // every time slice
// ROUND ROBIN, skip blocked and sleeping threads
  RunPt = RunPt -> next;    // run next thread not blocked
  while((RunPt -> blocked)||(RunPt -> sleep)){  // skip if blocked or sleeping
    RunPt = RunPt -> next;
  } 

}

//******** OS_Suspend ***************
// Called by main thread to cooperatively suspend operation
// Inputs: none
// Outputs: none
// Will be run again depending on sleep/block status
void OS_Suspend(void){
  STCURRENT = 0;        // any write to current clears it
  INTCTRL = 0x04000000; // trigger SysTick
// next thread gets a full time slice
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// OS_Sleep(0) implements cooperative multitasking

void OS_Sleep(uint32_t sleepTime){
// set sleep parameter in TCB
	RunPt->sleep = sleepTime;
// suspend, stops running
	OS_Suspend();
}

//******** OS_AddPeriodicEventThread ***************
// Add one background periodic event thread
// Typically this function receives the highest priority
// Inputs: pointer to a void/void event thread function
//         period given in units of OS_Launch (Lab 3 this will be msec)
// Outputs: 1 if successful, 0 if this thread cannot be added
// It is assumed that the event threads will run to completion and return
// It is assumed the time to run these event threads is short compared to 1 msec
// These threads cannot spin, block, loop, sleep, or kill
// These threads can call OS_Signal
// In Lab 3 this will be called exactly twice

int OS_AddPeriodicEventThread(void(*thread)(void), uint32_t period){
// ****IMPLEMENT THIS****
    pthreadblocks[pevents].thread = thread; //assign pointer
    pthreadblocks[pevents].period = period; //assing rate
    pthreadblocks[pevents].pcounter = 0;    //counter initialy 0
    pevents++;
	
  return 1;
}

void static runperiodicevents(void){
// ****IMPLEMENT THIS****
// **RUN PERIODIC THREADS, DECREMENT SLEEP COUNTERS
DisableInterrupts();
uint32_t i;
    for(i = 0; i < pevents; i++)
    {
        pthreadblocks[i].pcounter++;//increase counter by 1
        if(pthreadblocks[i].pcounter % pthreadblocks[i].period == 0)//run 
        {
            pthreadblocks[i].thread();
        }
    }
   
    for (i = 0; i < NUMTHREADS; i ++){//check sleeping threads
        if (threadblocks[i].sleep > 0)
            {
            (threadblocks[i].sleep) =  (threadblocks[i].sleep) - 1;//reduce sleeptime by 1
        }
    }
EnableInterrupts();
}

// ******** OS_InitSemaphore ************
// Initialize counting semaphore
// Inputs:  pointer to a semaphore
//          initial value of semaphore
// Outputs: none
void OS_InitSemaphore(int32_t *s, int32_t value){
//***IMPLEMENT THIS***
	*s = value;
}

// ******** OS_Wait ************
// Decrement semaphore and block if less than zero
// Lab2 spinlock (does not suspend while spinning)
// Lab3 block if less than zero
// Inputs:  pointer to a counting semaphore
// Outputs: none
	void OS_Wait(int32_t *s){//***IMPLEMENT THIS***
	  DisableInterrupts();
  (*s) = (*s) - 1;
  if((*s) < 0){
    RunPt -> blocked = s; // reason it is blocked
    EnableInterrupts();
    OS_Suspend();       // run thread switcher
  }
  EnableInterrupts();
}

// ******** OS_Signal ************
// Increment semaphore
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate
// Inputs:  pointer to a counting semaphore
// Outputs: none
	void OS_Signal(int32_t *s){
//***IMPLEMENT THIS***
	threadblkType *pt;
  DisableInterrupts();
  (*s) = (*s) + 1;
  if((*s) <= 0)
		{
			pt = RunPt->next;   // search for a thread blocked on this semaphore
			while(pt->blocked != s)
			{
				pt = pt->next;
			}
			pt->blocked = 0;    // wakeup this one
  }
  EnableInterrupts();
}


// ******** OS_FIFO_Init ************
// Initialize FIFO.  
// One event thread producer, one main thread consumer
// Inputs:  none
// Outputs: none
#define FIFOSIZE 10  // can be any size
uint32_t PutI;       // index of where to put next
uint32_t GetI;       // index of where to get next
uint32_t Fifo[FIFOSIZE];
int32_t CurrentSize; // 0 means FIFO empty, FIFOSIZE means full
uint32_t LostData;   // number of lost pieces of data

void OS_FIFO_Init(void){
//***IMPLEMENT THIS***
	PutI = GetI = 0;   // Empty
  OS_InitSemaphore(&CurrentSize, 0);
  LostData = 0;
}

// ******** OS_FIFO_Put ************
// Put an entry in the FIFO.  
// Exactly one event thread puts,
// do not block or spin if full
// Inputs:  data to be stored
// Outputs: 0 if successful, -1 if the FIFO is full

int OS_FIFO_Put(uint32_t data){
//***IMPLEMENT THIS***
if(CurrentSize == FIFOSIZE){
    LostData++;
    return -1;         // full
  } else{
    Fifo[PutI] = data; // Put
    PutI = (PutI+1)%FIFOSIZE;
    OS_Signal(&CurrentSize);
  return 0;   // success
}
	
}
// ******** OS_FIFO_Get ************
// Get an entry from the FIFO.   
// Exactly one main thread get,
// do block if empty
// Inputs:  none
// Outputs: data retrieved
uint32_t OS_FIFO_Get(void){uint32_t data;
//***IMPLEMENT THIS***
	
  OS_Wait(&CurrentSize);    // block if empty
  data = Fifo[GetI];        // get
  GetI = (GetI+1)%FIFOSIZE; // place to get next
  return data;
}
