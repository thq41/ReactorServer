#include "EventLoop.h"
#include<assert.h>
#include<sys/socket.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include"Log.h"
EventLoop* eventLoopInit()
{
	return eventLoopInitEx(NULL);
}

// 读主线程传的信息
int readLocalMsg(void* arg)
{
	EventLoop* evLoop = (EventLoop*)arg;
	char buf[256];
	read(evLoop->socketPair[1], buf, strlen(buf));
	return 0;
}

// 写数据唤醒子线程
void taskWakeup(EventLoop* evLoop)
{
	const char* msg = "唤醒子线程";
	write(evLoop->socketPair[0], msg, sizeof(msg));
}

EventLoop* eventLoopInitEx(const char* threadName)
{
	EventLoop* evLoop = (EventLoop*)malloc(sizeof(EventLoop));
	evLoop->isQuit = false;//默认开启
	evLoop->threadID = pthread_self();
	pthread_mutex_init(&evLoop->mutex, NULL);
	strcpy(evLoop->threadName, (threadName == NULL ? "MainThread" : threadName));
	evLoop->dispatcher = &epollDispatcher;
	evLoop->dispatcherData = evLoop->dispatcher->init();
	//任务链表初始化
	evLoop->head = evLoop->tail = NULL;
	//map
	evLoop->channelMap = initChannelMap(128);
	// 初始化一对socketPair[2]
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
	if (ret == -1)
	{
		perror("socketPair");
		exit(0);
	}
	// 指定规则：socketPair[0]发数据，socketPair[1]接收数据
	Channel* channel = initChannel(evLoop->socketPair[1], READ_EVENT, readLocalMsg, NULL, NULL, evLoop);
	// 添加到任务队列
	eventLoopAddTask(evLoop, ADD, channel);
	return evLoop;
}

int eventLoopRun(EventLoop* evLoop)
{
	assert(evLoop != NULL);
	//取出事件分发和检测模型
	Dispatcher* dispatcher = evLoop->dispatcher;
	//比较线程ID是否正常
	if (evLoop->threadID != pthread_self())
	{
		return -1;
	}
	//循环处理事件
	while (!evLoop->isQuit)
	{
		dispatcher->dispatch(evLoop, 2);//超时时长为2s
		eventLoopProcessTask(evLoop);
	}
	return 0;
}

int eventActivate(EventLoop* evLoop, int fd, int event)
{
	if (fd < 0 || evLoop == NULL)
	{
		return -1;
	}
	//取出channel
	Channel* channel = evLoop->channelMap->list[fd];
	assert(channel->fd == fd);
	//调用读回调
	if (event & READ_EVENT && channel->readCallback != NULL)
	{
		channel->readCallback(channel->arg);
	}
	//调用写回调
	if (event & WRITE_EVENT && channel->writeCallback != NULL)
	{
		channel->writeCallback(channel->arg);
	}
	return 0;
}

int eventLoopAddTask(EventLoop* evLoop, int type, Channel* channel)
{
	ChannelElement* channelElement = (ChannelElement*)malloc(sizeof(ChannelElement));
	channelElement->channel = channel;
	channelElement->type = type;
	channelElement->next = NULL;
	pthread_mutex_lock(&evLoop->mutex);
	// 如果任务队列为空
	if (!evLoop->head && !evLoop->tail)
	{
		evLoop->head = evLoop->tail = channelElement;
	}
	// 不为空就用尾插法
	else
	{
		evLoop->tail->next = channelElement;
		evLoop->tail = channelElement;
	}
	pthread_mutex_unlock(&evLoop->mutex);
	/*
	*	版本是单Reactor时
	*	处理节点 
	*	添加任务的节点可能是当前线程也可能是其他线程（主线程）
	*		1）fd修改，当前子线程发起，当前子线程处理
	*		2）添加fd，主线程发起
	*	不能让主线程处理任务队列，需要子线程处理，所以任务的添加都由子线程去做，这样就不需要用锁了
	*	
	*/
	if (evLoop->threadID == pthread_self())
	{
		// 子线程调用
		eventLoopProcessTask(evLoop);
	}
	else
	{
		// 主线程调用
		// 任务：告诉子线程处理任务队列里的任务，包括添加
		// 当前子线程的状态：1. 子线程在工作（那不用管） 2. 子线程无任务阻塞时（唤醒）
		// 唤醒方法是通过给epoll_wait()中添加一个触发任务的fd
		// 完成这个就是通过主线程的一个fd，和子线程中的一个fd,主线程发送，子线程就可以被唤醒
		taskWakeup(evLoop);
	}
	return 0;
}

int eventLoopProcessTask(EventLoop* evLoop)
{
	// 取出任务队列头节点 ->不断的处理任务
	pthread_mutex_lock(&evLoop->mutex);
	ChannelElement* head = evLoop->head;
	while (head != NULL)
	{
		if (head->type == ADD)
		{
			//添加
			eventLoopAdd(evLoop, head->channel);
		}
		else if (head->type == DELETE)
		{
			//删除
			eventLoopRemove(evLoop, head->channel);
			//删除后需要释放channel的内存
		}
		else if (head->type == MODIFY)
		{
			//修改
			eventLoopModify(evLoop, head->channel);
		}
		//一个任务处理后删除一个任务
		ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
		tmp = NULL;
	}
	evLoop->head = NULL;
	evLoop->tail = NULL;
	pthread_mutex_unlock(&evLoop->mutex);
	return 0;
}

int eventLoopAdd(EventLoop* evLoop, Channel* channel)
{
	// 1.拿到fd --> 2.拿到channelMap中的channel --> 3.判断fd是否大于channelMap的容量 --> 4.如果大于就扩容 
	// --> 5.添加channel到channelMap中 --> 6.调用dispatcher的add方法
	int fd = channel->fd;
	ChannelMap* channelMap = evLoop->channelMap;
	if(fd >= channelMap->size)
	{
		if (!reallocChannelMap(channelMap, fd, sizeof(ChannelMap*)))
		{
			return -1;
		}
	}
	if(channelMap->list[fd] == NULL)
	{
		channelMap->list[fd] = channel;
		evLoop->dispatcher->add(channel, evLoop);
	}
	return 0;
}

int eventLoopModify(EventLoop* evLoop, Channel* channel)
{
	// 1.拿到fd --> 2.拿到channelMap中的channel --> 3.判断fd是否有效
	int fd = channel->fd;
	ChannelMap* channelMap = evLoop->channelMap;
	if (channelMap->list[fd] == NULL)
	{
		return -1;
	}
	int ret  = evLoop->dispatcher->modify(channel, evLoop);
	return ret;
}

int eventLoopRemove(EventLoop* evLoop, Channel* channel)
{
	int fd = channel->fd;
	ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size || channelMap->list[fd] == NULL)
	{
		return -1;
	}
	int ret = evLoop->dispatcher->remove(channel, evLoop);
	return ret;
}

int destroyChannel(EventLoop* evLoop, Channel* channel)
{
	//1.删除channelMap中的channel --> 2.关闭fd -> 3.释放channel内存
	ChannelMap*channelMap = evLoop->channelMap;
	int fd = channel->fd;
	channelMap->list[fd] = NULL;
	close(fd);
	free(channel);
	return 0;
}
