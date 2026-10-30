#include "WorkerThread.h"
#include<stdio.h>

int initWorkerThread(WorkerThread* thread, int index)
{
	thread->threadID = 0;
	sprintf(thread->name, "SubThread-%d", index);
	pthread_mutex_init(&thread->mutex, NULL);
	pthread_cond_init(&thread->cond, NULL);
	thread->evloop = NULL;
	return 0;
}

// 子线程回调函数
void* subThreadRunning(void* arg)
{
	WorkerThread* thread = (WorkerThread*)arg;
	// 初始化子线程的反应堆模型
	pthread_mutex_lock(&thread->mutex);
	thread->evloop = eventLoopInitEx(thread->name);
	pthread_mutex_unlock(&thread->mutex);
	pthread_cond_signal(&thread->cond);
	// 启动反应堆模型
	eventLoopRun(thread->evloop);
	return NULL;
}

void workerThreadRun(WorkerThread* thread)
{
	pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
	pthread_mutex_lock(&thread->mutex);
	while (thread->evloop == NULL)
	{
		// 等待子线程初始化完成
		pthread_cond_wait(&thread->cond, &thread->mutex);
	}
	pthread_mutex_unlock(&thread->mutex);
}
