// %%%%%%%%%%%%%%%%%%%%%% EE442 PA2 %%%%%%%%%%%%%%%%%%%%%%//

// Please note that the input is taken from the file input.txt
// The input file format is given in the InputFormat.txt

// In the code, there are multiple usage of IA code generators 
// accompanying with different open source repositories. The used
// repositories are given in the reference at the end.

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#define pREARDY 1
#define pFINISHED 4
#define pRUNNING 2
#define pIO 3
#define pEMPTY 0
#define pMAX_NUM_THREAD 7
#define pMAX_NUM_TICKET 500
#define pSTACK_SIZE 4096
#define pBREAK_TIME 3

struct ThreadInfo {
  ucontext_t context; // Context of thread
  int BurstOfCpu[2]; // 2 CPU burst times
  int state; // State of Thread
  int BurstOfIo[2];  // 2 I/O burst times
  int NumberOfTickets; //Total Number of Tickets
  int AllBurst; // Total Burst Time of the Thread
  int arrival_time;
};

struct ThreadInfo threads[pMAX_NUM_THREAD];
ucontext_t main_uc; // Context


// Arrays to hold burst times for all threads
int BurstOfCpu[pMAX_NUM_THREAD][2];
int BurstOfIo[pMAX_NUM_THREAD][2];

// Arrays to hold tickets
int lottery_tickets[pMAX_NUM_TICKET];
int TicketIndex;
int TotalBurst;
int TotoalNumberOfTickets;
int all_finished;

void exitThread(int id);

// Print status of threads
void printStatus(int TypeOfPrint) {
  if (TypeOfPrint == 0) {
    printf("TID\tBursts\tState\tTickets\tCPU1\tIO1\tCPU2\tIO2\n"); // Adjust headers
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
      printf("T%d\t", i);
      printf("%d\t", threads[i].AllBurst);
      printf("%d\t", threads[i].state);
      printf("%d/%d\t", threads[i].NumberOfTickets, TotoalNumberOfTickets);
      for (int j = 0; j < 2; j++) { 
        printf("%d\t", threads[i].BurstOfCpu[j]); // CPU Burst Times
        printf("%d\t", threads[i].BurstOfIo[j]); // IO Burst Times
      }

      printf("\n");
    }
    printf("\n");
  } else if (TypeOfPrint == 1) {
    int states[pMAX_NUM_THREAD] = {0, 0, 0, 0, 0, 0, 0};

    // Fill states array with current thread states
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
      states[i] = threads[i].state;
    }

    printf("running>");
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
        switch(states[i]) {
            case pRUNNING:
                printf("T%d", i);
                break;
            default:
                break;
        }
    }

    int ready = 0;
    int FirstPrintedCase = 0;
    printf("\tready>");
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
      if (states[i] == pREARDY) {
        ready++;
        if (FirstPrintedCase) {
          printf(",");
        }
        printf("T%d", i);
        FirstPrintedCase = 1;
      }
    }

    for (int i = 0; i < pMAX_NUM_THREAD - ready; i++) {
      printf("   ");
    }

    int finished = 0;
    FirstPrintedCase = 0;
    printf("\tfinished>");
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
      if (states[i] == pFINISHED) {
        finished++;
        if (FirstPrintedCase) {
          printf(",");
        }
        printf("T%d", i);
        FirstPrintedCase = 1;
      }
    }

    for (int i = 0; i < pMAX_NUM_THREAD - finished; i++) {
      printf("   ");
    }

    printf("\t\tIO>");
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
      if (states[i] == pIO) {
        printf("T%d ", i);
      }
    }
    printf("\n");
  } else if (TypeOfPrint == 2) {
    printf("T0\tT1\tT2\tT3\tT4\tT5\tT6\n"); // Thread ID Header
  }
}

// Check IO bursts
void checkIO(int wait) {
  if (wait > pBREAK_TIME) {
    wait = pBREAK_TIME;
  }

  // Check IO
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    int temp_state = threads[i].state;

    // If thread is finished
    if (temp_state != pIO) {
      continue;
    }

    // Temp variables
    int TempAllBurst = threads[i].AllBurst;
    int TempCpu1 = BurstOfCpu[i][0];
    int TempCpu2 = BurstOfCpu[i][1];

    int TempIo1 = BurstOfIo[i][0];
    int TempIo2 = BurstOfIo[i][1];

    int CurrentIo1 = threads[i].BurstOfIo[0];
    int CurrentIo2 = threads[i].BurstOfIo[1];

    int FirstTemp = TempCpu1 + TempIo1;
    int SecondTemp = TempCpu2 + TempIo2;

    int WaitTemp = 0;
    int CheckTemp = 0;
    int TempFrom = 0;

    // First IO state 
    if (TempAllBurst < FirstTemp) {
        int remainingIo = threads[i].BurstOfIo[0];
        if (remainingIo > wait) {
            threads[i].BurstOfIo[0] -= wait;
            WaitTemp = wait;
            CheckTemp = 1;
        } 
        else {
            threads[i].BurstOfIo[0] -= CurrentIo1;
            WaitTemp = CurrentIo1;
            CheckTemp = 2;
        }
        TempFrom = 1;
    }

    // Second IO state
    else if (TempAllBurst < FirstTemp + SecondTemp) {
        int remainingIo = threads[i].BurstOfIo[1];
        if (remainingIo > wait) {
            threads[i].BurstOfIo[1] -= wait;
            WaitTemp = wait;
            CheckTemp = 1;
        } 
        else {
            threads[i].BurstOfIo[1] -= CurrentIo2;
            WaitTemp = CurrentIo2;
            CheckTemp = 2;
        }
        TempFrom = 2;
    }

    if (CheckTemp != 0) {
        threads[i].AllBurst += WaitTemp;
        threads[i].NumberOfTickets -= WaitTemp;

        // If additional rest is required
        if (CheckTemp == 1) {
            threads[i].state = pIO;
            break;
        } 
        /// The input/output operation is finished.
        else if (CheckTemp == 2) {
            // Final input/output burst
            if (TempFrom == 2) {
                /// Exit the thread since all input/output bursts have been completed.
                exitThread(i);
                return;
            }

            threads[i].state = pREARDY;
            if (wait == 0) {
                break;
            }
            wait -= WaitTemp;
        }
    }
  }
}

// Determine number of tickets initially for lottery scheduling
void determineRemainingBursts() {
  TotalBurst = 0;
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    TotalBurst += threads[i].BurstOfCpu[0] + threads[i].BurstOfCpu[1] +
                    threads[i].BurstOfIo[0] + threads[i].BurstOfIo[1]; // Total burst times
  }

  printf("Total Tickets: %d\n", TotalBurst);
  printf("\n");

  for (int i = 0; i < pMAX_NUM_TICKET; i++) {
    lottery_tickets[i] = -1;
  }

  TicketIndex = 0;
  TotoalNumberOfTickets = 0;

  // Determine number of tickets for each thread
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    int ticket = (BurstOfCpu[i][0] + BurstOfCpu[i][1] +
                  BurstOfIo[i][0] + BurstOfIo[i][1]); //Total burst times
    threads[i].NumberOfTickets = ticket;

    for (int j = TicketIndex; j < TicketIndex + ticket; j++) {
      lottery_tickets[j] = i;
    }
    TicketIndex += ticket;
    TotoalNumberOfTickets += ticket;
  }
}



// Output the current step of the process and then wait for one second.
void printStep(int wait, int selected_thread, int check) {
  for (int val = wait; val > check; val--) {
    for (int j = 0; j < selected_thread; j++) {
      printf("\t");
    }
    printf("%d\n", val - 1);
    sleep(1);
  }
}

// Lottery scheduling
void SRTFScheduler() {
  // Check if all threads are finished
  all_finished = 0;
  int all_io = 0;
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    if (threads[i].state == pFINISHED) {
      all_finished++;
    }

    if (threads[i].state == pIO) {
      all_io++;
    }
  }

// Select a random number from the ticket index
  int random_number = rand() % TicketIndex;
  int selected_thread = lottery_tickets[random_number];
  int state = threads[selected_thread].state;

  // If only input/output threads remain
  if (all_io + all_finished == pMAX_NUM_THREAD) {
    checkIO(pBREAK_TIME);
    return;
  }

  else if (all_finished == pMAX_NUM_THREAD - 1 && all_io == 1) {
    // If in the final step
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
        // Last thread is in IO
      if (threads[i].state == pREARDY) {
        selected_thread = i;
        state = threads[i].state;
        return;
      }

      if (threads[i].state == pIO) {
        exitThread(i);
        return;
      }
    }
  } else {
    // Whther the Thread is ready
    while (state != pREARDY) {
      selected_thread += 1;
      selected_thread %= pMAX_NUM_THREAD;
      state = threads[selected_thread].state;
    }
  }

  if (threads[selected_thread].state == pREARDY) {
    threads[selected_thread].state = pRUNNING;
    TotoalNumberOfTickets--;
  }


  printStatus(1);

  // Parameters
  int AllBurst = threads[selected_thread].AllBurst;

  int cpu1 = BurstOfCpu[selected_thread][0];
  int cpu2 = BurstOfCpu[selected_thread][1];

  int io1 = BurstOfIo[selected_thread][0];
  int io2 = BurstOfIo[selected_thread][1];

  int first = BurstOfCpu[selected_thread][0] + BurstOfIo[selected_thread][0];
  int second = BurstOfCpu[selected_thread][1] + BurstOfIo[selected_thread][1];
  int wait = 0;

  // If we are in the first CPU burst
  if (AllBurst < cpu1) {
    wait = cpu1 - AllBurst;
    if (wait > pBREAK_TIME) {
      threads[selected_thread].BurstOfCpu[0] -= pBREAK_TIME;
    } else {
      threads[selected_thread].BurstOfCpu[0] -= wait;
    }

  }

  // If we are in the second CPU burst
  else if (AllBurst < first + cpu2) {
    wait = first + cpu2 - AllBurst;
    if (wait > pBREAK_TIME) {
      threads[selected_thread].BurstOfCpu[1] -= pBREAK_TIME;
    } else {
      threads[selected_thread].BurstOfCpu[1] -= wait;
    }
  }

  // Check IO
  checkIO(wait);

  int check = 0;

  if (wait > pBREAK_TIME) {
    threads[selected_thread].AllBurst += pBREAK_TIME;
    threads[selected_thread].NumberOfTickets -= pBREAK_TIME;
    threads[selected_thread].state = pREARDY;
    check = wait - pBREAK_TIME;
  } else if (wait <= pBREAK_TIME && threads[selected_thread].state == pRUNNING) {
    threads[selected_thread].AllBurst += wait;
    threads[selected_thread].NumberOfTickets -= wait;
    threads[selected_thread].state = pIO;
    check = 0;
  }

  printStep(wait, selected_thread, check);
}

// Selection
int selectThread() {
  int id = -2;
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    if (threads[i].state != pFINISHED) {
      id = i;
      return id;
    }
  }
}

// Initilization
void initializeThread() {
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    threads[i].state = pEMPTY;

    threads[i].BurstOfCpu[0] = BurstOfCpu[i][0];
    threads[i].BurstOfCpu[1] = BurstOfCpu[i][1];

    threads[i].BurstOfIo[0] = BurstOfIo[i][0];
    threads[i].BurstOfIo[1] = BurstOfIo[i][1];

    threads[i].AllBurst = 0;
  }
}

// Creation
int createThread() {
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    if (threads[i].state == pEMPTY) {
      // Get context
      ucontext_t *uc = &threads[i].context;

      getcontext(uc);

      // Set context values
      uc->uc_stack.ss_sp = malloc(pSTACK_SIZE);
      uc->uc_stack.ss_size = pSTACK_SIZE;

       // Set context link to main context
      uc->uc_link = &main_uc;

      if (uc->uc_stack.ss_sp == NULL) {
        perror("malloc: Could not allocate stack"); //system unable to create new thread
        return (-1);
      }

      // Make context and set function to run
      makecontext(uc, (void (*)(void))SRTFScheduler, 1);

      // Set state to Ready
      threads[i].state = pREARDY;
      return i;
    }
  }
  return -1;
}

// Run
void runThread(int signal) {
  int id = selectThread();
  ucontext_t *uc = &threads[id].context;
  getcontext(uc);
  makecontext(uc, (void (*)(void))SRTFScheduler, 1);
  swapcontext(&main_uc, &threads[id].context); // P&WF_scheduler
}
// Exit 
void exitThread(int id) {
  // If we reached the last thread
  if (all_finished == pMAX_NUM_THREAD - 1) {
    // Increase all bursts
    threads[id].AllBurst += threads[id].BurstOfIo[1];

    threads[id].NumberOfTickets -= threads[id].BurstOfIo[1];
    threads[id].BurstOfIo[1] -= threads[id].BurstOfIo[1];

    // Finalize the last one
    threads[id].state = pFINISHED;

    // Increase all finished
    all_finished++;

    // printStatus(0);
    printStatus(1);

    //If we are not currently processing the last thread
  } else {
    // Update the state to "FINISHED".
    threads[id].state = pFINISHED;
  }

  // Free stack
  free(threads[id].context.uc_stack.ss_sp);
}

// Print inputs
void printInputData() {
  printf("Input: \n");
  // Print CPU and IO bursts for each thread
  printf("TID\tCPU1\tCPU2\tIO1\tIO2\n"); //Creating table
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    printf("T%d\t", i);
    for (int j = 0; j < 2; j++) { 
      printf("%d\t", BurstOfCpu[i][j]);
    }
    for (int j = 0; j < 2; j++) {
      printf("%d\t", BurstOfIo[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

// Read input data from txt file
// Details and structure of input.txt file is in the InputFormat.txt file
void readInputFromTxt() {
  FILE *fp;
  char filename[] = "input.txt";
  int cpu1, cpu2, io1, io2; 
  fp = fopen(filename, "r");
  // If the File Does Not Exists
  if (fp == NULL) {
    perror("The File Does Not Exists.\n");
    exit(EXIT_FAILURE);
  }

  int thread_number = 0;
  while (fscanf(fp, "%d %d %d %d", &cpu1, &cpu2, &io1, &io2) != EOF) { 
    BurstOfCpu[thread_number][0] = cpu1;
    BurstOfCpu[thread_number][1] = cpu2;
    BurstOfIo[thread_number][0] = io1;
    BurstOfIo[thread_number][1] = io2;
    thread_number++;
  }

  fclose(fp);
}

int main(int argc) {
   // Initialize the RNG
  unsigned int seed = time(NULL);
  srand(seed);

  // Read input data from txt file
  readInputFromTxt();

  // Show the input
  printInputData();

  // Get main context
  getcontext(&main_uc);

  // Thread Initilization
  initializeThread();

  //Signal handler
  signal(SIGALRM, runThread);

  // Total Number of Tickets
  determineRemainingBursts();

  // Thread Creation
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    createThread();
  }

  // Show all values
  printStatus(0);
  printf("\n");
  printStatus(2);

  all_finished = 0;

  // Set alarm till all threads are finished
  while (all_finished != pMAX_NUM_THREAD) {
    raise(SIGALRM); //Interrupt Creation
  }

  // Calculate processor utilization and turnaround times
  int TotalBurst = 0;
  int total_turnaround = 0;
    for (int i = 0; i < pMAX_NUM_THREAD; i++) {
        TotalBurst += threads[i].AllBurst;
        total_turnaround += threads[i].AllBurst + BurstOfIo[i][0] + BurstOfIo[i][1]; 
        
        // Calculate turnaround time for each thread
        int turnaround_time = threads[i].AllBurst + BurstOfIo[i][0] + BurstOfIo[i][1] - threads[i].arrival_time;
        printf("Turnaround time for Thread %d: %d\n", i, turnaround_time); // turnaround time of each of the processes
    }
  float utilization = (float)TotalBurst / total_turnaround * 100;

  // Print processor utilization and turnaround times
  printf("\nProcessor Utilization: %.2f%%\n", utilization); // processor utilization
  printf("Test4");
  printf("Average Turnaround Time: %.2f\n", (float)total_turnaround / pMAX_NUM_THREAD);
  printf("Test3");
  // Free main context
  free(main_uc.uc_stack.ss_sp);
  free(main_uc.uc_link);
  printf("Test1");
  // Free threads
  for (int i = 0; i < pMAX_NUM_THREAD; i++) {
    free(threads[i].context.uc_stack.ss_sp);
  }
  printf("Test2");
  return 0;
  
  // Referneces:
  // *Saadet, T. (n.d.). EE442_OS_homeworks. GitHub. https://github.com/ttsaadet/EE442_OS_homeworks
  // *ramesh0201. (n.d.). User_level_Scheduler. GitHub. https://github.com/ramesh0201/User_level_Scheduler
  // *cadagong. (n.d.). User-Level-Thread-Scheduler. GitHub. https://github.com/cadagong/User-Level-Thread-Scheduler
  // *Buzurkanov, T. (n.d.). Operating-Systems-HW2. GitHub. https://github.com/Talgat-Buzurkanov/Operating-Systems-HW2
  // *Karakay, D. (n.d.). EE442. GitHub. https://github.com/dkarakay/EE442
  // *UseProxy305. (n.d.). EE442-Operating-Systems. GitHub. https://github.com/UseProxy305/EE442-Operating-Systems
}

