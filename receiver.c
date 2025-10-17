#include "receiver.h"
#define RED     "\033[31m"
#define BLUE    "\033[36m"
#define WHITE   "\033[37m"

#include <sys/mman.h>
void receive(message_t* message_ptr, mailbox_t* mailbox_ptr){
    /*  TODO: 
        1. Use flag to determine the communication method
        2. According to the communication method, receive the message
    */
    static sem_t *receiver_sem;
    static sem_t *sender_sem;
    if (receiver_sem == NULL){
        receiver_sem = sem_open("/receiver_sem", O_CREAT, 0666, 0); //the name of semaphore 一定要是/ slash 開頭      
    }
    if (!sender_sem){
        sender_sem = sem_open("/sender_sem", O_CREAT, 0666,0);
    }
    
    // (1) 判斷模式
    if (mailbox_ptr->flag == MSG_PASSING){
        static int msqid = -1; // sys V msg queue id 
        if (msqid ==-1){
            printf(BLUE "Message Passing\n");
            key_t key = ftok("sender.c", 1); //to get the same queue identifier as sender.c 
            if (key == -1){
                perror("ftok error");
                exit(EXIT_FAILURE);
            }
            msqid = msgget(key, 0666|IPC_CREAT); //return the existing 
            if (msqid ==-1){
                perror("msgget error");
                exit(EXIT_FAILURE);
            }
        }
        message_ptr->mType = 1; 
        if (msgrcv(msqid, message_ptr, sizeof(message_ptr->msgText),message_ptr->mType, 0) == -1){
            perror("msgrcv error");
            exit(EXIT_FAILURE);     
        }
        //output the received  msg
        if (strcmp(message_ptr->msgText, "exit")){
            printf(BLUE "Receiving Message: ");
            printf(WHITE "%s", message_ptr->msgText);
        }

    }else{
        static void *shm_address_ptr = NULL;
        int shm_file_descriptor;
        if (!shm_address_ptr){
            printf(BLUE "Shared Memory\n");
            const char * shm_name = "shm"; //same setting as sender.c
            const int SIZE = 4096;
            shm_file_descriptor = shm_open(shm_name, O_CREAT|O_RDWR, 0666);
            if (shm_file_descriptor == -1){
                perror("shm_open error");
                exit(EXIT_FAILURE);
            }

            ftruncate(shm_file_descriptor, SIZE);
            shm_address_ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_file_descriptor, 0);
            if (shm_address_ptr == MAP_FAILED){
                perror("mmap");
                exit(EXIT_FAILURE);
            }
        }

        strcpy(message_ptr->msgText, (char *)shm_address_ptr);
        if (strcmp(message_ptr->msgText, "exit")){
            printf(BLUE "Sending Message: ");
            printf(WHITE "%s", message_ptr->msgText);
        }
    }
    sem_post(sender_sem);
    sem_wait(receiver_sem);
}

int main(int argc, char* argv[]){
    /*  TODO: 
        1) Call receive(&message, &mailbox) according to the flow in slide 4
        2) Measure the total receiving time
        3) Get the mechanism from command line arguments
            • e.g. ./receiver 1
        4) Print information on the console according to the output format
        5) If the exit message is received, print the total receiving time and terminate the receiver.c
    */
    mailbox_t *mailbox = malloc(sizeof(mailbox_t));
    message_t *msg = malloc(sizeof(message_t));
    mailbox->flag = atoi(argv[1]);

    //calculate the time for receiving 
    struct timespec start, end;
    double time_taken = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);


    while(1){
        receive(msg, mailbox);
        // sem_post(sender_sem);
        if (!strcmp(msg->msgText, "exit")){
            break;
        }
        // sem_wait(receiver_sem);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken += (end.tv_sec-start.tv_sec) +(end.tv_nsec-start.tv_nsec)*1e-9;


    printf(RED "\nSender exit!\n");
    printf("Total time taken in receiving msg: %f s", time_taken);
    free(msg);
    free(mailbox);
    sem_unlink("/receiver_sem");
    sem_unlink("/sender_sem");

}
