#include <sys/types.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define PASS_LEN 6
#define NUM_THREADS 4
#define PBSTR "############################################################"
#define PBWIDTH 60

struct thread_info {
    pthread_t id;
    struct args *args;
};

struct args {
    int thread_num;
    long ini;
    long end;
    int *found;
    int *finalizar;
    char **md5;
    pthread_mutex_t *mutex;
    long *cnt;
    int num_passwd;
};

long ipow(long base, int exp)
{
    long res = 1;
    for (;;)
    {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return res;
}

long pass_to_long(char *str) {
    long res = 0;

    for(int i=0; i < PASS_LEN; i++)
        res = res * 26 + str[i]-'a';

    return res;
};

void long_to_pass(long n, unsigned char *str) {  // str should have size PASS_SIZE+1
    for(int i=PASS_LEN-1; i >= 0; i--) {
        str[i] = n % 26 + 'a';
        n /= 26;
    }
    str[PASS_LEN] = '\0';
}

void to_hex(unsigned char *res, char *hex_res) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(&hex_res[i*2], 3, "%.2hhx", res[i]);
    }
    hex_res[MD5_DIGEST_LENGTH * 2] = '\0';
}

void *break_pass(void *ptr) {
    struct args *args = ptr;
    unsigned char res[MD5_DIGEST_LENGTH];
    char hex_res[MD5_DIGEST_LENGTH * 2 + 1];
    unsigned char *pass = malloc((PASS_LEN + 1) * sizeof(char));

    int cnt = 0;
    for(int j = 0; j < args->num_passwd; j++) {
        if(*args->finalizar)  break;

        for (long i = args->ini; i < args->end; i++) {
            if (args->found[j]) break;

            pthread_mutex_lock(args->mutex);
            *args->cnt += 1;
            pthread_mutex_unlock(args->mutex);

            long_to_pass(i, pass);

            MD5(pass, PASS_LEN, res);

            to_hex(res, hex_res);
            if (!strcmp(hex_res, args->md5[j])) {
                pthread_mutex_lock(args->mutex);
                args->found[j] = 1;
                pthread_mutex_unlock(args->mutex);
                printf("\n%s: %s\n", args->md5[j], pass);
                break;
            }
        }

        if(args->found[j])  cnt++;
        if(cnt == args->num_passwd) {
            pthread_mutex_lock(args->mutex);
            *args->finalizar = 1;
            pthread_mutex_unlock(args->mutex);
            break; // Found it!
        }
    }
    free(pass);
    return NULL;
}

void printProgress(float percentage) {
    float val = percentage * 100;
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3.2f%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

void *progress(void *ptr) {
    struct args *args = ptr;
    long bound = ipow(26, PASS_LEN);
    float percentage = 0;
    while(1) {
        percentage = ((float) *args->cnt / (float) bound);
        printProgress(percentage);
        if (*args->finalizar)   break;
    }
    return NULL;
}

struct thread_info *start_threads(char **md5, int argc)
{
    int i;
    struct thread_info *threads;
    long bound = ipow(26, PASS_LEN);

    threads = malloc(sizeof(struct thread_info) * NUM_THREADS);
    printf("creating %d threads\n", NUM_THREADS+1);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));

    int *encontrado = malloc(sizeof(int)*(argc-1));
    for(i = 0; i < argc-1; i++) {
        encontrado[i] = 0;
    }

    long *cnt = malloc(sizeof(long));
    *cnt = 0;

    int *finalizar = malloc(sizeof(int));
    *finalizar = 0;

    pthread_mutex_init(mutex, NULL);

    // Create num_thread threads running swap()
    for (i = 0; i < NUM_THREADS+1; i++) {
        threads[i].args = malloc(sizeof(struct args));

        threads[i].args -> thread_num = i;
        threads[i].args -> md5        = md5;
        threads[i].args -> found      = encontrado;
        threads[i].args -> ini        = threads[i].args->thread_num * bound/NUM_THREADS;
        threads[i].args -> end        = (threads[i].args->thread_num+1) * bound/NUM_THREADS;
        threads[i].args -> mutex      = mutex;
        threads[i].args -> cnt        = cnt;
        threads[i].args -> num_passwd = argc-1;
        threads[i].args -> finalizar  = finalizar;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if (0 != pthread_create(&threads[i].id, NULL, break_pass, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    if (0 != pthread_create(&threads[i].id, NULL, progress, threads[i].args)) {
        printf("Could not create thread #%d", i);
        exit(1);
    }


    return threads;
}

void wait(struct thread_info *thrs) {
    int i;
    for(i = 0; i < NUM_THREADS+1; i++)  pthread_join(thrs[i].id, NULL);

    if (0 != pthread_mutex_destroy(thrs->args->mutex)) {
        printf("Mutex destroy failed");
        exit(1);
    }

    free(thrs->args->mutex);
    free(thrs->args->found);
    free(thrs->args->cnt);
    free(thrs->args->finalizar);
    
    for(i = 0; i < NUM_THREADS+1; i++)  free(thrs[i].args);

    free(thrs);
}

int main(int argc, char *argv[]) {
    struct thread_info *thrs;
    if(argc < 2) {
        printf("Use: %s string\n", argv[0]);
        exit(0);
    }

    thrs = start_threads(argv+1, argc);
    wait(thrs);
    return 0;
}
