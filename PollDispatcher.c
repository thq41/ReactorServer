#include"EventLoop.h"
#include"Channel.h"
#include<poll.h>
#include<stdlib.h>
#include<stdio.h>
#define MAXFD 1024

typedef struct PollData
{
	int maxfd;//记录有效元素后一位
	struct pollfd* pollfds;//pollfd[]
}PollData;

//初始化分发器 -->得到分发器的数据块dispatcherData
static void* pollInit();
//添加事件节点
static int pollAdd(Channel* channel, EventLoop* eventloop);
//修改事件节点
static int pollModify(Channel* channel, EventLoop* eventloop);
//删除事件节点
static int pollRemove(Channel* channel, EventLoop* eventloop);
//分配任务
static int pollDispatch(EventLoop* eventloop, int timeout);//单位s
//清除fd,堆内存
static int pollClear(EventLoop* eventloop);

Dispatcher pollDispatcher = {
	pollInit,
	pollAdd,
	pollModify,
	pollRemove,
	pollDispatch,
	pollClear
};

static void* pollInit()
{
	PollData* pollData = (PollData*)malloc(sizeof(PollData));
	if (pollData == NULL)
	{
		perror("malloc pollData");
		return NULL;
	}
	pollData->maxfd = 0;
	pollData->pollfds = (struct pollfd*)calloc(MAXFD, sizeof(struct pollfd));
	//初始化pollfds
	for (int i = 0; i < MAXFD; ++i)
	{
		pollData->pollfds[i].fd = -1;
		pollData->pollfds[i].events = 0;
		pollData->pollfds[i].revents = 0;
	}
	if (pollData->pollfds == NULL)
	{
		perror("calloc pollfds");
		free(pollData);
		return NULL;
	}
	return pollData;
}

static int pollAdd(Channel* channel, EventLoop* eventloop)
{
	PollData* pollData = (PollData*)eventloop->dispatcherData;
	int events = 0;
	//缺少异常处理
	if (channel->events == READ_EVENT)
	{
		events |= POLLIN;
	}
	if (channel->events == WRITE_EVENT)
	{
		events |= POLLOUT;
	}
	int i = 0;
	for (; i < MAXFD; ++i)
	{
		if (pollData->pollfds[i].fd == -1)
		{
			pollData->pollfds[i].fd = channel->fd;
			pollData->pollfds[i].events = events;
			pollData->maxfd = (i > pollData->maxfd) ? i : pollData->maxfd;
			break;
		}
	}
	if (i >= MAXFD)
	{
		return -1;
	}
	return 0;
}

static int pollModify(Channel* channel, EventLoop* eventloop)
{
	PollData* pollData = (PollData*)eventloop->dispatcherData;
	int events = 0;
	//缺少异常处理
	if (channel->events == READ_EVENT)
	{
		events |= POLLIN;
	}
	if (channel->events == WRITE_EVENT)
	{
		events |= POLLOUT;
	}
	for (int i = 0; i < MAXFD; ++i)
	{
		if (pollData->pollfds[i].fd == channel->fd)
		{
			pollData->pollfds[i].events = events;
			return 0;
		}
	}
	return -1;
}

static int pollRemove(Channel* channel, EventLoop* eventloop)
{
	PollData* pollData = (PollData*)eventloop->dispatcherData;
	for (int i = 0; i < MAXFD; ++i)
	{
		if (pollData->pollfds[i].fd == channel->fd)
		{
			pollData->pollfds[i].fd = -1;
			pollData->pollfds[i].events = 0;
			pollData->pollfds[i].revents = 0;
			// 通过channel释放tcpConnection资源
			channel->destroyCallback(channel->arg);
			return 0;
		}
	}
	return -1;
}

static int pollDispatch(EventLoop* eventloop, int timeout)
{
	PollData* pollData = (PollData*)eventloop->dispatcherData;
	int ret = poll(pollData->pollfds, pollData->maxfd + 1, 1000 * timeout);
	if (ret == -1)
	{
		perror("poll");
		return -1;
	}  
	if (ret == 0)
	{
		return 0;
	}
	for (int i = 0; i < pollData->maxfd; ++i)
	{
		if (pollData->pollfds[i].fd == -1)
		{
			continue;
		}
		if (pollData->pollfds[i].revents & POLLIN)
		{
			//处理读事件
			eventActivate(eventloop,pollData->pollfds[i].fd, READ_EVENT);
		}
		if (pollData->pollfds[i].revents & POLLOUT)
		{
			//处理写事件
			eventActivate(eventloop, pollData->pollfds[i].fd, WRITE_EVENT);
		}
	}
	return 0;
}

static int pollClear(EventLoop* eventloop)
{
	PollData* pollData = (PollData*)eventloop->dispatcherData;
	if (pollData != NULL)
	{
		if (pollData->pollfds != NULL)
		{
			free(pollData->pollfds);
			pollData->pollfds = NULL;
			pollData->maxfd = 0;
		}
		free(pollData);
	}
	return 0;
}