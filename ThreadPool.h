#pragma once
#include"WorkerThread.h"
#include"EventLoop.h"
#include<stdbool.h>
typedef struct ThreadPool
{
	int threadCount; // 线程池中线程的数量
	WorkerThread* workerThreads; // 线程池中的线程数组
	int index; // 当前线程池中线程的索引
	EventLoop* mainLoop; // 主线程的反应堆模型,如果线程池为0个子线程时，主线程也可以作为工作线程使用
	bool isStarted; // 线程池是否已经启动
}ThreadPool;

// 初始化线程池
ThreadPool* threadPoolInit(int threadCount, EventLoop* mainLoop);
// 启动线程池
void threadPoolRun(ThreadPool* pool);
// 获取线程池中的子线程的反应堆实例
EventLoop* takeWorkerEventLoop(ThreadPool* pool);