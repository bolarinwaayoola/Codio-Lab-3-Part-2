#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_LOOPS 25
#define BANK_ACCOUNT_IDX 0
#define TURN_IDX 1

// Child process (Poor Student)
void PoorStudentProcess(int *ShmPTR) {
    int account;
    int balance;

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < NUM_LOOPS; i++) {
        // Sleep between 0 and 5 seconds
        sleep(rand() % 6);

        // Strict alternation wait: loop while Turn != 1
        while (ShmPTR[TURN_IDX] != 1) {
            // Do no-op
        }

        // Copy value in BankAccount to local account variable
        account = ShmPTR[BANK_ACCOUNT_IDX];

        // Randomly generate balance needed between 0 and 50
        balance = rand() % 51;
        printf("Poor Student needs $%d\n", balance);
        fflush(stdout);

        // Check if balance needed is <= account
        if (balance <= account) {
            account -= balance;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, account);
            fflush(stdout);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
            fflush(stdout);
        }

        // Copy value back to BankAccount and set Turn = 0
        ShmPTR[BANK_ACCOUNT_IDX] = account;
        ShmPTR[TURN_IDX] = 0;
    }

    exit(0);
}

// Parent process (Dear Old Dad)
void DearOldDadProcess(int *ShmPTR) {
    int account;
    int balance;

    srand(time(NULL) ^ getpid());

    for (int i = 0; i < NUM_LOOPS; i++) {
        // Sleep between 0 and 5 seconds
        sleep(rand() % 6);

        // Strict alternation wait: loop while Turn != 0
        while (ShmPTR[TURN_IDX] != 0) {
            // Do no-op
        }

        // Copy value in BankAccount to local account variable
        account = ShmPTR[BANK_ACCOUNT_IDX];

        // If account <= 100, try to deposit money
        if (account <= 100) {
            balance = rand() % 101;

            if (balance % 2 == 0) {
                account += balance;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", balance, account);
                fflush(stdout);
            } else {
                printf("Dear old Dad: Doesn't have any money to give\n");
                fflush(stdout);
            }
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
            fflush(stdout);
        }

        // Copy value back to BankAccount and set Turn = 1
        ShmPTR[BANK_ACCOUNT_IDX] = account;
        ShmPTR[TURN_IDX] = 1;
    }
}

int main(void) {
    int ShmID;
    int *ShmPTR;
    pid_t pid;

    // Allocate shared memory for 2 integers: BankAccount and Turn
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    ShmPTR = (int *)shmat(ShmID, NULL, 0);
    if ((long)ShmPTR == -1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize BankAccount and Turn to 0
    ShmPTR[BANK_ACCOUNT_IDX] = 0;
    ShmPTR[TURN_IDX] = 0;

    // Fork child process
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        PoorStudentProcess(ShmPTR);
    } else {
        DearOldDadProcess(ShmPTR);

        // Wait for child process to terminate
        wait(NULL);

        // Detach and remove shared memory
        shmdt((void *)ShmPTR);
        shmctl(ShmID, IPC_RMID, NULL);
    }

    return 0;
}