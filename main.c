#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/wait.h>


#define HIT_RATE            0.5
#define WR_RATE             0.5

//system properties
#define N 5
#define USED_SLOTS_TH 3

//TIMES IN ns
#define SIM_TIME                    1
#define TIME_BETWEEN_SNAPSHOTS      100000000
#define MEM_WR_T                    1000
#define HD_ACCESS_T                 100000
#define INTER_MEM_ACCS_T          10000
/**--------------------------------------------------------------------------   */
///                           declarations
void fetch_to_RAM(int page);
int MMU_request();
bool is_memory_empty();
bool is_memory_full();
void* evicter_threading(void* arg);
void* printer(void* arg);
/**--------------------------------------------------------------------------   */
///                           semaphores & mutexes
int terminate = 0;
sem_t ACK;

pthread_mutex_t mem_lock;
pthread_mutex_t HD_lock;
pthread_cond_t printer_cond;
pthread_cond_t mem_not_full;
pthread_cond_t mem_not_empty;
pthread_cond_t evict_cond;
pthread_cond_t HD_request;
void* main_thread(void* arg);
/**--------------------------------------------------------------------------   */
///                           semaphores & mutexes
//pthread_t process1;
//pthread_t process2;

pthread_t MMU_main_thread;
pthread_t evicter_thread;
pthread_t printer_thread;

pthread_t HD_thread;

enum requester {
    MMU,
    EVICTER
};


/**--------------------------------------------------------------------------   */
/**                                Process                                      */
/** --------------------------------------------------------------------------  */
enum access_mode {
    READ,
    WRITE
};

_Noreturn int process() {
    int i=0;
    srand(time(NULL)); //CHECK RANDOM TIME
    while (1) {
        // wait for INTER_MEM_ACCS_T
        struct timespec req = {0, INTER_MEM_ACCS_T };
        nanosleep(&req, NULL);

        // determine if access is read or write
        enum access_mode mode;
        double r = (double)rand() / RAND_MAX;
        if (r < WR_RATE) {
            mode = WRITE;
        } else {
            mode = READ;
        }

        // send request to MMU
        // TODO comlete this


        MMU_request();
        printf("Sending %s request to MMU\n", mode == READ ? "read" : "write");


        // wait for ack from MMU
        // TODO comlete this




        // create threads and condition threads
        pthread_mutex_init(&mem_lock, NULL);
        pthread_cond_init(&evict_cond, NULL);
        pthread_cond_init(&printer_cond,NULL);
        pthread_create(&MMU_main_thread, NULL, main_thread, NULL);

        // ...
        pthread_create(&evicter_thread, NULL, evicter_threading, NULL);
        pthread_create(&printer_thread,NULL, printer,NULL);



    }
}

/**--------------------------------------------------------------------------   */
/**                                                                             */
/**                                MMU                                          */
/**                                                                             */
/** --------------------------------------------------------------------------- */

/**This code simulates a memory with N pages, each represented by a Boolean value
 * indicating whether the page is valid (true) or invalid (false). The memory
 * has two pointers, one to indicate the next page to be loaded and another to
 * indicate the next page to be evicted. The memory starts empty and as pages are
 * loaded, the next_page_to_load pointer is incremented, and once the memory is full,
 * the next_page_to_evict pointer is incremented to evict the next page. The function
 * fetch_to_RAM() simulates a write operation that takes a certain amount of time (MEM_WR_T).
 * The read operation is not simulated in this code as it was stated that it is immediate.
*/
bool memory[N];
bool dirty[N];


int next_page_to_load = 0;
int next_page_to_evict = 0;


bool is_memory_empty() {
    for (int i = 0; i < N; i++) {
        if (memory[i]) {
            return false;
        }
    }
    return true;
}

bool is_memory_full() {
    for (int i = 0; i < N; i++) {
        if (!memory[i]) {
            return false;
        }
    }
    return true;
}

void fetch_to_RAM(int page) {
    struct timespec req = {0, MEM_WR_T};
    nanosleep(&req, NULL);
    memory[page] = true;
}

void evict_from__RAM() {
    memory[next_page_to_evict] = false;
    dirty[next_page_to_evict] = false;
    next_page_to_evict =(next_page_to_evict +1) % N;
}
int count_used_slots() {
    int count = 0;
    for (int i = 0; i < N; i++) {
        if (memory[i]) {
            count++;
        }
    }
    return count;
}

int MMU_request() {

    // check if memory is full
    if (is_memory_full()) {
        // evict page at next_page_to_evict
        memory[next_page_to_evict] = false;
        next_page_to_evict = (next_page_to_evict + 1) % N;
    }

    // load page at next_page_to_load
    fetch_to_RAM(next_page_to_load);
    next_page_to_load = (next_page_to_load + 1) % N;

    // do other things
    printf("GOT request to MMU\n");

    return 0;
}


void* main_thread(void* arg) {




    sem_init(&ACK, 0, 0);
    int read = 0;

    while (! terminate) {

        // receive request from process
        // TODO: sleep until request
        // TODO: update "read" according to request!
        read = 0;

        pthread_mutex_lock(&mem_lock);

        if (is_memory_empty()) {
            // miss (page fault)

            // send request to HD
            // TODO: implement HD request

            // write page to memory
            fetch_to_RAM(next_page_to_load);
            next_page_to_load = (next_page_to_load + 1) % N;

            // acknowledge requesting process
            sem_post(&ACK);
        }
        // determine if request is a hit or miss
        double is_hit = (double)rand() / RAND_MAX;
        if (is_hit < HIT_RATE) {
            // hit

            ///if WRITE, page becomes dirty
            if(write){
                int dirty_page = rand() % N;

                // look for a page we ACTUALLY have
                while (!memory[dirty_page]) {
                    dirty_page = rand() % N;
                    if (memory[dirty_page])
                        dirty[dirty_page] = true;
                }
            }else
                //TODO: if read



            // acknowledge requesting process
            sem_post(&ACK);
        } else {
            ///________________________
            /// miss (page fault)
            ///------------------------

            //wake up evicter
            if (is_memory_full()) {

                pthread_mutex_unlock(&mem_lock);
                pthread_cond_signal(&evict_cond);
                pthread_cond_wait(&mem_not_full, &mem_lock);
                pthread_mutex_lock(&mem_lock);

            }


            // send request to HD
            // ...

            // write page to memory
            memory[next_page_to_load] = true;
            next_page_to_load = (next_page_to_load + 1) % N;

            // acknowledge requesting process
            sem_post(&ACK);
        }


        //wake up printer
        pthread_mutex_unlock(&mem_lock);
        pthread_cond_signal(&printer_cond);


    }


    return NULL;
}


/**--------------------------------------------------------------------------   */
/**                                Evicter                                      */
/** --------------------------------------------------------------------------- */
#define USED_SLOTS_TH 3

int next_page_to_evict;

void* evicter_threading(void* arg) {

    //TODO increment to 5 only
    int num_used_slots = 5;

    while (! terminate) {
        pthread_mutex_lock(&mem_lock);

        while (num_used_slots <= USED_SLOTS_TH) {
            pthread_mutex_unlock(&mem_lock);
            pthread_cond_wait(&mem_not_full, &mem_lock);
        }

        // choose page to evict using the clock scheme
        while (1) {
            if (num_used_slots <= USED_SLOTS_TH) {
                break;
            }

            if (memory[next_page_to_evict]) {

                if(dirty[next_page_to_evict]){
                    //TODO: update HD
                }

                // evict page
                evict_from__RAM();
                num_used_slots--;
            }
        }
        /// free mutex, wake up main
        pthread_cond_signal(&mem_not_full);
        pthread_mutex_unlock(&mem_lock);

    }

    return NULL;
}
/* the evicter thread uses a global variable next_page_to_evict to keep track of the current position
 * in the memory array.
 * It also uses an array dirty that keeps track of whether a page has been accessed or not.
 *
The evicter thread waits on the mem_not_full condition variable until the main thread signals
 it, indicating that the memory is full. When the evicter thread wakes up, it starts evicting
 pages using the clock scheme: it chooses the page at the current position of the clock hand
 and checks if it has been accessed or not. If it has been accessed, the page is marked as not
 accessed (the reference bit is cleared), and the clock hand is moved to the next page.
 If the page has not been accessed, it is evicted.
When the evicter thread evicts a page, it decrements the num_used_slots counter and checks if
 the number of used slots in the memory is now equal to N-1 (namely, the memory was full
 before evicting), if this the case it wakes up the main thread using the pthread_cond_signal()
 function, to let him load a page if needed.
The evicter thread continues evicting pages until the number of used slots in the memory is below
 USED_SLOTS_TH. Then, the evicter stops evicting, and waits for the main thread to wake it up again.
*/




/**--------------------------------------------------------------------------   */
/**                             Hard Disc                                       */
/** --------------------------------------------------------------------------- */

void* HD_tread(void* arg) {

    int num_used_slots = 5;

    while (! terminate) {

        pthread_cond_wait(&HD_request,&HD_lock);
        ///HD request called

        //delay
        struct timespec req = {0, HD_ACCESS_T};
        nanosleep(&req, NULL);

        /// free mutex, wake up caller
        //TODO: figure out WHO exactly requested HD, and wake him up

    }

    return NULL;
}

/**--------------------------------------------------------------------------   */
/**                             Printer                                       */
/** --------------------------------------------------------------------------- */


void* printer(void* arg){
    while(1) {
        pthread_mutex_lock(&mem_lock);

        struct timespec req2;
        clock_gettime(CLOCK_REALTIME, &req2);
        req2.tv_sec += TIME_BETWEEN_SNAPSHOTS / 1000000000;
        req2.tv_nsec += TIME_BETWEEN_SNAPSHOTS % 1000000000;
        pthread_cond_timedwait(&printer_cond, &mem_lock, &req2);
        for(int i = 0; i < N; i++) {
           if(memory[i]&&dirty[i])
               printf("%d|1\n",i);
           else if(memory[i]&&!dirty[i])
               printf("%d|0\n",i);
           else
               printf("%d|-\n",i);

        }
        pthread_mutex_unlock(&mem_lock);
    }

}


/**--------------------------------------------------------------------------   */
/**                                                                             */
/**                                Main                                         */
/**                                                                             */
/** --------------------------------------------------------------------------- */
int main() {


    pid_t pid1 = fork();
    if (pid1 == 0) {
        process();
    } else {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            process();
        } else {
            wait(NULL);
            wait(NULL);
        }
    }
    pthread_join(MMU_main_thread, NULL);
    pthread_join(evicter_thread, NULL);
    pthread_join(printer_thread, NULL);

    /* Destroy the memory mutex and condition variable */
    pthread_mutex_destroy(&mem_lock);
    pthread_cond_destroy(&printer_cond);
    pthread_cond_destroy(&evict_cond);
}