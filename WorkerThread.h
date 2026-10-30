#pragma once
#include<pthread.h>
#include"EventLoop.h"
typedef struct WorkerThread
{
	pthread_t threadID;
	char name[24];
	EventLoop* evloop;// 反应堆模型
	pthread_mutex_t mutex; // 互斥锁
	pthread_cond_t cond; // 条件变量
}WorkerThread;

// 初始化工作线程
int initWorkerThread(WorkerThread* thread, int index);
// 启动线程
void workerThreadRun(WorkerThread* thread);