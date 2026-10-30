#include"EventLoop.h"
#include"sys/select.h"
#include<stdlib.h>
#include<stdio.h>
#define MAXFD 1024

typedef struct SelectData
{
	fd_set readfds;
	fd_set writefds;
}SelectData;

//初始化分发器 -->得到分发器的数据块dispatcherData
static void* selectInit();
//添加事件节点
static int selectAdd(Channel* channel, EventLoop* eventloop);
//修改事件节点
static int selectModify(Channel* channel, EventLoop* eventloop);
//删除事件节点
static int selectRemove(Channel* channel, EventLoop* eventloop);
//分配任务
static int selectDispatch(EventLoop* eventloop, int timeout);//单位s
//清除fd,堆内存
static int selectClear(EventLoop* eventloop);

Dispatcher selectDispatcher = {
	selectInit,
	selectAdd,
	selectModify,
	selectRemove,
	selectDispatch,
	selectClear
};

static void* selectInit()
{
	SelectData* selectData = (SelectData*)malloc(sizeof(SelectData));
	if (selectData == NULL)
	{
		perror("malloc selectData");
		return NULL;
	}
	FD_ZERO(&selectData->readfds);
	FD_ZERO(&selectData->writefds);
	return selectData;
}

static int selectAdd(Channel* channel, EventLoop* eventloop)
{
	SelectData* selectData = (SelectData*)eventloop->dispatcherData;
	if (channel->events == READ_EVENT)
	{
		FD_SET(channel->fd, &selectData->readfds);
	}
	if (channel->events == WRITE_EVENT)
	{
		FD_SET(channel->fd, &selectData->writefds);
	}
	return 0;
}

static int selectRemove(Channel* channel, EventLoop* eventloop)
{
	SelectData* selectData = (SelectData*)eventloop->dispatcherData;
	if (channel->events == READ_EVENT)
	{
		FD_CLR(channel->fd, &selectData->readfds);
	}
	if (channel->events == WRITE_EVENT)
	{
		FD_CLR(channel->fd, &selectData->writefds);
	}
	// 通过channel释放tcpConnection资源
	channel->destroyCallback(channel->arg);
	return 0;
}

static int selectModify(Channel* channel, EventLoop* eventloop)
{
	selectRemove(channel, eventloop);
	selectAdd(channel, eventloop);
	return 0;
}

static int selectDispatch(EventLoop* eventloop, int timeout)
{
	SelectData* selectData = (SelectData*)eventloop->dispatcherData;
	fd_set* readtmp = &selectData->readfds;
	fd_set* writetmp = &selectData->writefds;
	struct timeval time = { timeout,0 };
	int ret = select(MAXFD, readtmp,writetmp, NULL, &time);
	if (ret == -1)
	{
		perror("select");
		exit(0);
	}
	for (int i = 0; i < MAXFD; ++i)
	{
		if (FD_ISSET(i, readtmp))
		{
			//执行读事件
			eventActivate(eventloop, i, READ_EVENT);
		}
		if (FD_ISSET(i, writetmp))
		{
			//执行写事件
			eventActivate(eventloop, i, WRITE_EVENT);
		}
	}
	return 0;
}

static int selectClear(EventLoop* eventloop)
{
	SelectData* selectData = (SelectData*)eventloop->dispatcherData;
	if (selectData != NULL)
	{
		free(selectData);
		selectData = NULL;
	}
	return 0;
}