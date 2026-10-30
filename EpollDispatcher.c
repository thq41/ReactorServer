#include"EventLoop.h"
#include"Dispatcher.h"
#include<stdlib.h>
#include<sys/epoll.h>
#include<stdio.h>
#include<unistd.h>

#define MAX_EVENTS 520//最大事件数 -->events数组的大小

typedef struct EpollData
{
	int epfd;
	struct epoll_event* events;//用来存epoll_wait传出参数
}EpollData;



//初始化分发器 -->得到分发器的数据块dispatcherData
static void* epollInit();
//添加事件节点
static int epollAdd(Channel* channel, EventLoop* eventloop);
//修改事件节点
static int epollModify(Channel* channel, EventLoop* eventloop);
//删除事件节点
static int epollRemove(Channel* channel, EventLoop* eventloop);
//分配任务
static int epollDispatch(EventLoop* eventloop, int timeout);//单位s
//清除fd,堆内存
static int epollClear(EventLoop* eventloop);

static int epollCtl(Channel* channel, EventLoop* eventloop, int op);



Dispatcher epollDispatcher = {
	epollInit,
	epollAdd,
	epollModify,
	epollRemove,
	epollDispatch,
	epollClear
};

static int epollCtl(Channel* channel, EventLoop* eventloop, int op)
{
	EpollData* epollData = (EpollData*)eventloop->dispatcherData;
	int events = 0;
	if (channel->events & READ_EVENT)
	{
		events |= EPOLLIN;
	}
	if (channel->events & WRITE_EVENT)
	{
		events |= EPOLLOUT;
	}
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = channel->fd;
	int ret = epoll_ctl(epollData->epfd, op, channel->fd, &ev);//ret 用来判断epoll_ctl是否调用成功
	if (ret == -1)
	{
		perror("epoll_ctl");
		return ret;
	}
	return ret;
}


static void* epollInit()
{
	EpollData* epollData = (EpollData*)malloc(sizeof(EpollData));
	if (epollData == NULL)  
	{
		perror("malloc EpollData");
		return NULL;  // malloc失败返回NULL
	}
	epollData->epfd = epoll_create(MAX_EVENTS);
	if(epollData->epfd ==-1)									//这部分代码可能有问题
	{
		perror("epoll_create");
		free(epollData);
		return NULL;
	}
	epollData->events = (struct epoll_event*)calloc(MAX_EVENTS, sizeof(struct epoll_event));
	if (epollData->events == NULL)
	{
		perror("calloc epollData->events");
		//close(epollData->epfd);
		free(epollData);
		return NULL;
	}
	return epollData;
}

static int epollAdd(Channel* channel, EventLoop* eventloop)
{
	int ret = epollCtl(channel, eventloop, EPOLL_CTL_ADD);
	if (ret == -1)
	{
		perror("epoll_ctl add");
		exit(0);
	}
	return ret;
}

static int epollModify(Channel* channel, EventLoop* eventloop)
{
	int ret = epollCtl(channel, eventloop, EPOLL_CTL_MOD);
	if (ret == -1)
	{
		perror("epoll_ctl modify");
		exit(0);
	}
	return ret;
}

static int epollRemove(Channel* channel, EventLoop* eventloop)
{
	int ret = epollCtl(channel, eventloop, EPOLL_CTL_DEL);
	if (ret == -1)
	{
		perror("epoll_ctl remove");
		exit(0);
	}
	// 通过channel释放资源
	channel->destroyCallback(channel->arg);
	return ret;
}

static int epollDispatch(EventLoop* eventloop, int timeout)
{
	EpollData* epollData = (EpollData*)eventloop->dispatcherData;
	int count = epoll_wait(epollData->epfd, epollData->events, MAX_EVENTS, timeout * 1000);
	for (int i = 0; i < count; ++i)
	{
		int fd = epollData->events[i].data.fd;
		int events = epollData->events[i].events;
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			//对方断开连接
			//epollRemove(channel, eventloop);
			continue;
		}
		if (events & EPOLLIN)
		{
			//处理读事件
			eventActivate(eventloop, fd, READ_EVENT);
		}
		if (events & EPOLLOUT)
		{
			//处理写事件
			eventActivate(eventloop, fd, WRITE_EVENT);
		}
	}
	return 0;
}

static int epollClear(EventLoop* eventloop)
{
	//释放资源，关文件描述符
	EpollData* epollData = (EpollData*)eventloop->dispatcherData;
	if (epollData != NULL)
	{
		if (epollData->events)
		{
			free(epollData->events);
			epollData->events = NULL;
		}
		close(epollData->epfd);
		free(epollData);
		epollData = NULL;
		return 0;
	}
	return -1;
}