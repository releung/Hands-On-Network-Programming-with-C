/**
 * @file Name    : executionTimeOfFunctions.cpp
 * @brief        : 计算代码块或者函数运行时间, 判断程序执行性能
 * @author       : Zen Leung
 * @mail         : re2leung@gmail.com
 * @created Time : Mon 05 Jul 2021 10:20:34 AM CST
 */

// C++ program to find out execution time of
// of functions
#include <algorithm>
#include <chrono>
#include <iostream>
#include<vector>
#include <unistd.h>
using namespace std;
using namespace std::chrono;

// For demonstration purpose, we will fill up
// a vector with random integers and then sort
// them using sort function. We fill record
// and print the time required by sort function
int main()
{

	vector<int> values(10000);

	// Generate Random values
	auto f = []() -> int { return rand() % 10000; };

	// Fill up the vector
	generate(values.begin(), values.end(), f);

	// Get starting timepoint
	auto start = high_resolution_clock::now();

	// Call the function, here sort()
	sort(values.begin(), values.end());
    sleep(1); // 添加一个 sleep 延迟时间, 和 time 命令的结果对比看

	// Get ending timepoint
	auto stop = high_resolution_clock::now();

	// Get duration. Substart timepoints to
	// get durarion. To cast it to proper unit
	// use duration cast method
	auto duration = duration_cast<microseconds>(stop - start);

	cout << "Time taken by function: "
		<< duration.count() << " microseconds" << endl;

	return 0;
}

/** 
 * 运行结果:
 *   $ time ./executionTimeOfFunctions_cpp
 *     Time taken by function: 1002797 microseconds
 *     ./executionTimeOfFunctions_cpp  0.00s user 0.00s system 0% cpu 1.005 total
 * */
