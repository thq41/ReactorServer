#pragma once
#include"ThreadPool.h"
#include"EventLoop.h"
typedef struct Listener
{
	int lfd;
	unsigned short port;
}Listener;

typedef struct TcpServer
{
	Listener* listener;
	int threadCount;
	ThreadPool* threadPool;
	EventLoop* mainLoop;
}TcpServer;

// 初始化TcpServer
TcpServer* tcpServerInit(unsigned short port, int threadCount);
// 初始化Listener
Listener* listenerInit(unsigned short port);
// 启动TcpServer
void tcpServerRun(TcpServer* server);