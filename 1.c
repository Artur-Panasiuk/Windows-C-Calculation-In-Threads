#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <math.h>

double globalResult = 1;
int counter = 0;
double exitVal[100];
HANDLE lock;

struct args {
    long unsigned int start;
    long unsigned int end;
};

double wallisInRange(long unsigned int start, long unsigned int end) {
    double result = 1;
    if (start < 1) return result;

    for (long unsigned int n = start; n <= end; n++) {
        result = result + (pow(-1, n) / (2 * n + 1));
    }

    return result;
}

DWORD WINAPI test(LPVOID input) {
    long unsigned int a = ((struct args*)input)->start;
    long unsigned int b = ((struct args*)input)->end;

    double wallis = wallisInRange(a, b);

    lock = CreateMutex(NULL, FALSE, NULL);
    WaitForSingleObject(lock, INFINITE);
    globalResult *= wallisInRange(a, b);
    ReleaseMutex(lock);

    exitVal[counter] = wallis;
    counter++;
}

void createT(HANDLE* tids, DWORD* thirds, struct args** argArr, int w, long unsigned jump) {
    long unsigned int siz;
    long unsigned int first = 1;
    long unsigned int last = jump;

    for (int i = 0; i < w; i++) {
        argArr[i] = (struct args*)malloc(sizeof(struct args));
        argArr[i]->start = first;
        argArr[i]->end = last;

        siz = last - first + 1;

        first += jump;
        last += jump;

        tids[i] = CreateThread(NULL, 0, test, (void*)argArr[i], 0, &thirds[i]);
        printf("Thread #%lu size=%lu first=%lu\n", tids[i], siz, argArr[i]->start - 1);
    }
}

void joinThreads(HANDLE* tids, DWORD* exits, int w) {
    for (int j = 0; j < w; j++) {
        WaitForSingleObject(tids[j], INFINITE);
        CloseHandle(tids[j]);
        printf("Thread #%lu prod=%f\n", tids[j], exitVal[j]);
    }
}

int main(int argc, char* argv[]) {

    if (argc < 3 || argc > 5) return 1;

    int n = atoi(argv[1]);
    int w = atoi(argv[2]);

    if (n <= 1 || n >= 1000000000) return 1;
    if (w <= 1 || w >= 100) return 1;

    struct args** argArr = malloc(w * sizeof(struct args*));
    HANDLE* tids = (HANDLE*)malloc(sizeof(HANDLE)*w);
    DWORD* thirds = (DWORD*)malloc(sizeof(DWORD)*w);
    DWORD* exits = (DWORD*)malloc(sizeof(DWORD)*w);

    clock_t wt1, wt2, wot1, wot2;
    double time1, time2;

    wt1 = clock();

    if (n % w == 0) {
        createT(tids, thirds, argArr, w, n / w);
        joinThreads(tids, exits, w);
    }
    else {
        long unsigned int siz, diff = n % w;
        long unsigned int jump = (n - diff) / w;

        createT(tids, thirds, argArr, w - 1, jump);

        argArr[w - 1] = (struct args*)malloc(sizeof(struct args));
        argArr[w - 1]->start = n - (jump + diff);
        argArr[w - 1]->end = n;
        siz = argArr[w - 1]->end - argArr[w - 1]->start + 1;
        tids[w-1] = CreateThread(NULL, 0, test, (void*)argArr[w-1], 0,&thirds[w-1]);
        printf("Thread #%lu size=%lu first=%lu\n", tids[w - 1], siz, argArr[w - 1]->start);

        joinThreads(tids, exits, w);
    }

    wt2 = clock();

    free(argArr);
    free(tids);
    free(exits);
    free(thirds);

    wot1 = clock();

    double woThreadsWallis = wallisInRange(1, n);

    wot2 = clock();

    time1 = ((double)(wt2 - wt1)) / CLOCKS_PER_SEC;
    time2 = ((double)(wot2 - wot1)) / CLOCKS_PER_SEC;

    printf("w/Threads: PI=%Lf time=%fs\n", (globalResult * 4), time1);
    printf("wo/Threads: PI=%Lf time=%fs\n", (woThreadsWallis * 4), time2);

    return 0;
}