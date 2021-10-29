/**
 * @file Name    : executionTimeOfFunctions.c
 * @brief        : 计算代码块或者函数运行时间, 判断程序执行性能
 * @author       : Zen Leung
 * @mail         : re2leung@gmail.com
 * @created Time : Mon 05 Jul 2021 10:24:48 AM CST
 */

/* Program to demonstrate time taken by function fun() */
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
    return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
        ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

// A function that terminates when enter key is pressed
void fun()
{
    printf("fun() starts \n");
    printf("Press enter to stop fun \n");
    int i = 0;
    unsigned int tmp[10001] = {0};
    for (i = 0; i < 10000; ++i) {
        tmp[i] = i; 
    }
    while(1)
    {
        if (getchar())
            break;
    }
    printf("fun() ends \n");
}

/** 
 * 执行可执行程序的时候, 加上 time 命令, 可以对比一下时间
 * 例如:
 *     $ time ./executionTimeOfFunctions_c
 *       fun() starts
 *       Press enter to stop fun
 *
 *       fun() ends
 *       TEST2 fun() took 2.116700 s to execute.
 *       TEST1 fun() took 0.000147 seconds to execute. CLOCKS_PER_SEC:1000000
 *       ./executionTimeOfFunctions_c  0.00s user 0.00s system 0% cpu 2.118 total
 * */
#define TEST1 /** 这个方法不准确, 不是实际执行的时间. 这个应该是不计算让出 cpu 的时间, 只计算了暂用 cpu 的时间 */
#define TEST2 /** 这个方式是准确的, 是实际执行的时间 */

// The main program calls fun() and measures time taken by fun()
int main()
{
#ifdef TEST1
    // Calculate the time taken by fun()
    clock_t t;
    t = clock();
#endif


#ifdef TEST2
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif
    fun();

#ifdef TEST2
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t timeElapsed = timespecDiff(&end, &start);
    printf("TEST2 fun() took %f s to execute. \n", (double)timeElapsed/1000000000);
#endif


#ifdef TEST1
    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
    printf("TEST1 fun() took %f seconds to execute. CLOCKS_PER_SEC:%ld \n", time_taken, CLOCKS_PER_SEC);
#endif

    return 0;
}

