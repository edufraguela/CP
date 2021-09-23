#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "options.h"

#define MAX_AMOUNT 20

struct bank {
    int num_accounts;        // number of accounts
    int *accounts;           // balance array
    pthread_mutex_t *mutex;
    int no_deposits_transfer;   //Indica que los saldos no serán modificados
    pthread_cond_t *balances_increment;
};

struct args {
    int		thread_num;       // application defined thread #
    int		delay;			  // delay between operations
    int		iterations;       // number of operations
    int     net_total;        // total amount deposited by this thread
    int withdraw;
    struct bank *bank;        // pointer to the bank (shared with other threads)
};

struct thread_info {
    pthread_t    id;        // id returned by pthread_create()
    struct args *args;      // pointer to the arguments
};

void *withdraw(void *ptr)
{
    struct args* args = ptr;
    int amount, account, balance;

    amount = rand() % MAX_AMOUNT;
    account = rand() % args->bank->num_accounts;

    pthread_mutex_lock(&args->bank->mutex[account]);
    while((balance = args->bank->accounts[account]) < amount) {
        pthread_cond_wait(&args->bank->balances_increment[account], &args->bank->mutex[account]);
        if(args->bank->no_deposits_transfer) {
            pthread_mutex_unlock(&args->bank->mutex[account]);
            return NULL;
        }
    }

    printf("Withdraw thread %d withdrawing %d from account %d\n", args->thread_num, amount, account);

    balance -= amount;
    if(args->delay) usleep(args->delay);

    args->bank->accounts[account] = balance;
    if(args->delay) usleep(args->delay);

    args->withdraw = amount;
    pthread_mutex_unlock(&args->bank->mutex[account]);

    return NULL;
}

void *transfer(void *ptr)
{
    struct args *args = ptr;

    int amount, accountOrigin, accountDest, balanceOrigin, balanceDest;

    while(args->iterations--) {
        accountOrigin = rand() % args->bank->num_accounts;
        do {
            accountDest = rand() % args->bank->num_accounts;
        } while (accountOrigin == accountDest);

        if(accountOrigin < accountDest) {
            pthread_mutex_lock(&args->bank->mutex[accountOrigin]);
            pthread_mutex_lock(&args->bank->mutex[accountDest]);
        } else {
            pthread_mutex_lock(&args->bank->mutex[accountDest]);
            pthread_mutex_lock(&args->bank->mutex[accountOrigin]);
        }

        if (args->bank->accounts[accountOrigin] == 0)
            amount = 0;
        else
            amount = rand() % args->bank->accounts[accountOrigin];

        printf("Thread %d transfering %d from account %d to %d\n",
               args->thread_num, amount, accountOrigin, accountDest);

        balanceOrigin = args->bank->accounts[accountOrigin];
        if (args->delay) usleep(args->delay); // Force a context switch

        balanceOrigin -= amount;
        if (args->delay) usleep(args->delay);

        args->bank->accounts[accountOrigin] = balanceOrigin;
        if (args->delay) usleep(args->delay);

        balanceDest = args->bank->accounts[accountDest];
        if (args->delay) usleep(args->delay);

        balanceDest += amount;
        if (args->delay) usleep(args->delay);

        args->bank->accounts[accountDest] = balanceDest;

        pthread_mutex_unlock(&args->bank->mutex[accountOrigin]);
        pthread_mutex_unlock(&args->bank->mutex[accountDest]);

        pthread_cond_broadcast(&args->bank->balances_increment[accountDest]);
    }
    return NULL;
}

// Threads run on this function
void *deposit(void *ptr)
{
    struct args *args =  ptr;
    int amount, account, balance;

    while(args->iterations--) {
        amount  = rand() % MAX_AMOUNT;
        account = rand() % args->bank->num_accounts;
        pthread_mutex_lock(&args->bank->mutex[account]);

        printf("Thread %d depositing %d on account %d\n",
               args->thread_num, amount, account);

        balance = args->bank->accounts[account];
        if(args->delay) usleep(args->delay); // Force a context switch

        balance += amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[account] = balance;
        if(args->delay) usleep(args->delay);

        args->net_total += amount;
        pthread_mutex_unlock(&args->bank->mutex[account]);

        pthread_cond_broadcast(&args->bank->balances_increment[account]);
    }
    return NULL;
}

// start opt.num_threads threads running on deposit.
struct thread_info *start_threads(struct options opt, struct bank *bank)
{
    int i;
    struct thread_info *threads;

    printf("creating %d threads for deposit\n", opt.num_threads);
    printf("creating %d threads for transfer\n\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * opt.num_threads*3);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running swap()
    for (i = 0; i < (opt.num_threads*3); i++) {
        threads[i].args = malloc(sizeof(struct args));

        threads[i].args -> thread_num = i;
        threads[i].args -> net_total  = 0;
        threads[i].args -> bank       = bank;
        threads[i].args -> delay      = opt.delay;
        threads[i].args -> iterations = opt.iterations;
        threads[i].args -> withdraw = 0;

        if (0 != pthread_mutex_init(&threads[i].args->bank->mutex[i], NULL)) {
            printf("mutex_init failed\n");
            exit(1);
        }

        if(0 != pthread_cond_init(&threads[i].args->bank->balances_increment[i], NULL)) {
            printf("mutex_init failed\n");
            exit(1);
        }
    }

    for(i = 0; i < opt.num_threads; i++) {
        if (0 != pthread_create(&threads[i].id, NULL, deposit, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    for(i = opt.num_threads; i < opt.num_threads*2; i++) {
        if (0 != pthread_create(&threads[i].id, NULL, transfer, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    for(i = opt.num_threads*2; i < opt.num_threads*3; i++) {
        if(0 != pthread_create(&threads[i].id, NULL, withdraw, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    return threads;
}

// Print the final balances of accounts and threads
void print_balances(struct bank *bank, struct thread_info *thrs, int num_threads) {
    int total_deposits=0, bank_total=0, total_withdraws=0;
    printf("\nNet deposits by thread\n");

    for(int i=0; i < num_threads; i++) {
        printf("%d: %d\n", i, thrs[i].args->net_total);
        total_deposits += thrs[i].args->net_total;
    }
    printf("Total deposits: %d\n", total_deposits);

    printf("\nWithdraws by thread\n");
    for(int i = num_threads*2; i < num_threads*3; i++) {
        printf("%d: %d\n", i, thrs[i].args->withdraw);
        total_withdraws += thrs[i].args->withdraw;
    }
    printf("Total withdraws: %d\n", total_withdraws);

    printf("\nAccount balance\n");
    for(int i=0; i < bank->num_accounts; i++) {
        printf("%d: %d\n", i, bank->accounts[i]);
        bank_total += bank->accounts[i];
    }
    printf("Total: %d\n", bank_total);

}

// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct bank *bank, struct thread_info *threads) {
    // Wait for the threads to finish
    for (int i = 0; i < opt.num_threads*2; i++)
        pthread_join(threads[i].id, NULL);

    print_balances(bank, threads, opt.num_threads);

    for (int i = 0; i < opt.num_threads*2; i++)
        free(threads[i].args);

    bank->no_deposits_transfer = 1;
    for(int i = 0; i < bank->num_accounts; i++)
        pthread_cond_broadcast(&bank->balances_increment[i]);

    for(int i = opt.num_threads*2; i < opt.num_threads*3; i++)
        free(threads[i].args);

    free(threads);
    free(bank->accounts);
    free(bank->mutex);
}

// allocate memory, and set all accounts to 0
void init_accounts(struct bank *bank, int num_accounts) {
    bank->num_accounts = num_accounts;
    bank->accounts     = malloc(bank->num_accounts * sizeof(int));
    bank->mutex = malloc(bank->num_accounts * sizeof(pthread_mutex_t));
    bank->no_deposits_transfer = 0;
    bank->balances_increment = malloc(bank->num_accounts * sizeof(pthread_cond_t));

    for(int i=0; i < bank->num_accounts; i++)
        bank->accounts[i] = 0;
}

int main (int argc, char **argv)
{
    struct options      opt;
    struct bank         bank;
    struct thread_info *thrs;

    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 5;
    opt.num_accounts = 10;
    opt.iterations   = 5;
    opt.delay        = 10;

    read_options(argc, argv, &opt);

    init_accounts(&bank, opt.num_accounts);

    thrs = start_threads(opt, &bank);
    wait(opt, &bank, thrs);

    return 0;
}
