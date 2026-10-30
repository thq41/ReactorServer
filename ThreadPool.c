#include"ThreadPool.h"
#include<assert.h>
ThreadPool* threadPoolInit(int threadCount, EventLoop* mainLoop)
{
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	pool->index = 0;
	pool->threadCount = threadCount;
	pool->isStarted = false;
	pool->mainLoop = mainLoop;
	pool->workerThreads = (WorkerThread*)malloc(sizeof(WorkerThread) * threadCount); // 实例化
	return pool;
}

void threadPoolRun(ThreadPool* pool)
{
	// 线程池启动
	assert(pool != NULL && pool->isStarted == false);
	if (pool->mainLoop->threadID != pthread_self())
	{
		exit(0);
	}
	pool->isStarted = true;
	if (pool->threadCount > 0)
	{
		for(int i=0;i<pool->threadCount;i++)
		{
			// 初始化线程池中的每个线程
			initWorkerThread(&pool->workerThreads[i], i);
			workerThreadRun(&pool->workerThreads[i]);
		}
	}
}

EventLoop* takeWorkerEventLoop(ThreadPool* pool)
{
	// 1. 判断线程池是否启动 --> 2. 判断线程池中是否有子线程 
	// --> 3. 如果有就轮询获取子线程的反应堆实例 --> 4. 如果没有就返回主线程的反应堆实例
	assert(pool->isStarted);
	EventLoop* evLoop = pool->mainLoop;
	if(pool->threadCount)
	{
		evLoop = pool->workerThreads[pool->index].evloop;
		pool->index = (pool->index + 1) % pool->threadCount;
	}
	return evLoop;
}
