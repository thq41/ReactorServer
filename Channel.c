#include "Channel.h"
#include<stdlib.h>
Channel* initChannel(int fd, int events, handleFunc readCallback, handleFunc writeCallback, handleFunc destroyCallback, void* arg)
{
	Channel* channel = (Channel*)malloc(sizeof(Channel));
	channel->fd = fd;
	channel->events = events;
	channel->readCallback = readCallback;
	channel->writeCallback = writeCallback;
	channel->destroyCallback = destroyCallback;
	channel->arg = arg;
	return channel;
}

bool writeEventsEnable(Channel* channel, bool enable)
{
	if (enable)
	{
		channel->events |= WRITE_EVENT;
		return true;
	}
	else
	{
		channel->events &= ~WRITE_EVENT;
		return false;
	}
}

bool isWriteEventsEnable(Channel* channel)
{
	return channel->events & WRITE_EVENT;
}
