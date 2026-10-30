#pragma once
#include"Dispatcher.h"
#include"ChannelMap.h"
#include<pthread.h>
typedef struct Dispatcher Dispatcher;

extern Dispatcher epollDispatcher;
extern Dispatcher pollDispatcher;
extern Dispatcher selectDispatcher;

enum TYPE{ADD,DELETE,MODIFY};

typedef struct ChannelElement
{
	Channel* channel;
	int type;
	struct ChannelElement* next;
}ChannelElement;

typedef struct EventLoop
{
	bool isQuit;
	Dispatcher* dispatcher;
	void* dispatcherData;//用来保存分发器的数据块
	ChannelElement* head;//head,tail用来维护任务队列
	ChannelElement* tail;
	ChannelMap* channelMap;//拿到文件描述符中的事件，和回调函数
	pthread_t threadID;
	char threadName[32];
	int socketPair[2];
	pthread_mutex_t mutex;//用来保护任务队列的互斥锁
}EventLoop;

EventLoop* eventLoopInit(); // 用于初始化主线程
EventLoop* eventLoopInitEx(const char* threadName);// 主要用于初始化子线程
int eventLoopRun(EventLoop* evLoop);// 启动反应堆
// 处理被激活的文件描述符事件
int eventActivate(EventLoop* evLoop, int fd, int event);
// 添加新任务到任务队列
int eventLoopAddTask(EventLoop* evLoop, int type, Channel* channel);
// 处理任务队列的任务
int eventLoopProcessTask(EventLoop* evLoop);
// 处理dispatcher的节点
int eventLoopAdd(EventLoop* evLoop, Channel* channel);
int eventLoopModify(EventLoop* evLoop, Channel* channel);
int eventLoopRemove(EventLoop* evLoop, Channel* channel);
// 释放channel内存
int destroyChannel(EventLoop* evLoop, Channel* channel);