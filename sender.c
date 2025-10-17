#include "sender.h"
#define RED     "\033[31m"
#define BLUE    "\033[36m"
#define WHITE   "\033[37m"
#include <sys/mman.h>

// #include <mqueue.h> error prone
void send(message_t message, mailbox_t* mailbox_ptr){
    /*  TODO: 
        1. Use flag to determine the communication method
        2. According to the communication method, send the message
    */

    // (1) 
    //用static 確保只會宣告一次, btw sem 跟 mutex 在運作上不太一樣，所以要用兩個semaphore設定 
    static sem_t *receiver_sem;
    static sem_t *sender_sem;
    if (receiver_sem == NULL){
        receiver_sem = sem_open("/receiver_sem", O_CREAT, 0666, 0); //the name of semaphore 一定要是/ slash 開頭      
    }
    if (!sender_sem){
        sender_sem = sem_open("/sender_sem", O_CREAT, 0666,0);
    }

    if (mailbox_ptr->flag == MSG_PASSING){
        // static mqd_t msg_queue_fd  = -1;, posix does not work
        static int msqid = -1; // sys V msg queue id 
        if (msqid == -1){ //first time 
            printf(BLUE "Message Passing\n");
            key_t key = ftok("sender.c", 1);
            if (key ==-1){
                perror("ftok");
                exit(EXIT_FAILURE);
            }
            msqid = msgget(key, 0666 | IPC_CREAT);
            if (msqid ==-1){
                perror("msg queue msgget");
                exit(EXIT_FAILURE);
            }
        }

        message.mType = 1;  //like specify the channel, should be the same as receiver
        //停在 msgsnd 不動 可能是因為 Message Queue 滿了。
        if (msgsnd(msqid, &message, sizeof(message.msgText), 0) ==-1){
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
        if (strcmp(message.msgText, "exit")){
            printf(BLUE "Sending Message: ");
            printf(WHITE "%s", message.msgText);
        }

        
    }else{
        static void *shm_address_ptr = NULL;
        int shm_file_descriptor;
        if (!shm_address_ptr){ //only initialize for first time
            printf(BLUE "Shared Memory\n");
            const char * shm_name = "shm";
            const int SIZE = 4096;
            shm_file_descriptor = shm_open(shm_name, O_CREAT|O_RDWR, 0666);
            if (shm_file_descriptor ==-1){
                perror("shm_open");
                exit(EXIT_FAILURE);
            }
            ftruncate(shm_file_descriptor, SIZE);
            shm_address_ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_file_descriptor, 0);
            if (shm_address_ptr == MAP_FAILED){
                perror("mmap");
                exit(EXIT_FAILURE);
            }
        }
        // strcpy(shm_address_ptr, message.msgText); // strcpy 比 sprintf 稍微快一點點
        sprintf(shm_address_ptr,"%s", message.msgText );
        if (strcmp(message.msgText, "exit")){
            printf(BLUE "Sending Message: ");
           printf(WHITE "%s", message.msgText);
        }



    }


    sem_post(receiver_sem);
    sem_wait(sender_sem);
}

int main(int argc, char* argv[]){
    /*  TODO: 
        1) Call send(message, &mailbox) according to the flow in slide 4
        2) Measure the total sending time
        3) Get the mechanism and the input file from command line arguments
            • e.g. ./sender 1 input.txt
                    (1 for Message Passing, 2 for Shared Memory)
        4) Get the messages to be sent from the input file
        5) Print information on the console according to the output format
        6) If the message form the input file is EOF, send an exit message to the receiver.c
        7) Print the total sending time and terminate the sender.c
    */
    //get the input arguments 
    mailbox_t *mailbox = malloc(sizeof(mailbox_t));
    message_t *msg = malloc(sizeof(message_t));
    mailbox->flag = atoi(argv[1]); //turn the '1' or '2' to integer
    FILE * file_ptr = fopen(argv[2], "r");
    if (!file_ptr){
        fprintf(stderr, "file open failed.");
        exit(EXIT_FAILURE);
    }

    //calculate the time
    struct timespec start, end;
    double time_taken = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);

    //read all message to be sent
    while(fgets(msg->msgText, sizeof(msg->msgText), file_ptr)){
        send(*msg, mailbox);
    }

    //the last exit message to receiver 
    strcpy(msg->msgText, "exit");
    send(*msg, mailbox);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    // sem_post(receiver_sem);
    // sem_wait(sender_sem);
    time_taken += (end.tv_sec-start.tv_sec) +(end.tv_nsec-start.tv_nsec)*1e-9;

    printf(RED "\nEnd of input file! exit!\n");
    printf("Total time taken in sending msg: %f s", time_taken);

    //CLEAR
    if (mailbox->flag == MSG_PASSING){
        key_t key = ftok("sender.c", 1);
        if (key != -1) {
            int msqid = msgget(key, 0666);
            if (msqid != -1) {
                msgctl(msqid, IPC_RMID, NULL); // 刪除 message queue
            }
        }
    }else{
        shm_unlink("shm");
        //about the shm_address_ptr
        //因為 shm_address_ptr 是在 send 函式內的 static 變數，main 函式無法直接存取它。一個常見的作法是在 main 函式的最後，直接呼叫 shm_unlink() 即可。munmap 則是在 process 結束時，作業系統會自動幫你處理，但自己明確地呼叫是更標準的作法。對於這個 lab 來說，在 sender 結束前呼叫 shm_unlink() 和 sem_unlink() 是最重要的。
    }

    fclose(file_ptr);
    free(msg);
    free(mailbox);
    sem_unlink("/receiver_sem");
    sem_unlink("/sender_sem");

    return 0;
    
}
