#pragma once
#include<stdbool.h>

enum EventType
{
	TIME_OUT = 0x01,//超时事件
	READ_EVENT = 0x02,//读事件
	WRITE_EVENT = 0x04,//写事件
};

typedef int(*handleFunc)(void* arg);
typedef struct Channel
{
	//文件描述符
	int fd;
	//事件类型
	int events;
	//读写事件回调函数
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	//读写事件回调函数参数
	void* arg;
}Channel;

//初始化Channel
Channel* initChannel(int fd, int events, handleFunc readCallback, handleFunc writeCallback, handleFunc destroyCallback, void* arg);

//修改写事件（检测写事件or不检测）
bool writeEventsEnable(Channel* channel, bool enable);

//判断Channel是否检测写事件
bool isWriteEventsEnable(Channel* channel);