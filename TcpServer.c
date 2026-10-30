#include"TcpServer.h"
#include<arpa/inet.h>
#include"Channel.h"
#include"TcpConnection.h"
#include<stdio.h>
#include"Log.h"
TcpServer* tcpServerInit(unsigned short port, int threadCount)
{
	TcpServer* tcpServer = (TcpServer*)malloc(sizeof(TcpServer));
	tcpServer->listener = listenerInit(port);
	tcpServer->threadCount = threadCount;
	tcpServer->mainLoop = eventLoopInit();
	tcpServer->threadPool = threadPoolInit(threadCount, tcpServer->mainLoop);
	return tcpServer;
}

Listener* listenerInit(unsigned short port)
{
	Listener* listener = (Listener*)malloc(sizeof(Listener));
	//1.创建套接字
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket");
		return NULL;
	}
	//2.端口复用
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1)
	{
		perror("setsockkopt");
		return NULL;
	}
	//3.绑定端口
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind");
		return NULL;
	}
	//4.监听
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen");
		return NULL;
	}
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}

int acceptConn(void* arg)
{
	TcpServer* server = (TcpServer*)arg;
	// 1.接收连接
	int cfd = accept(server->listener->lfd, NULL, NULL);
	if (cfd < 0)
	{
		perror("accept");
		return -1;
	}
	// 2.获取工作线程的反应堆模型
	EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
	// 调用TcpConnection初始化函数
	tcpConnectionInit(cfd, evLoop);
	return 0;
}

void tcpServerRun(TcpServer* server)
{
	// 启动线程池
	threadPoolRun(server->threadPool);
	// 添加检测任务
	Channel* channel = initChannel(server->listener->lfd,READ_EVENT,acceptConn,NULL, NULL, server);
	eventLoopAddTask(server->mainLoop,ADD,channel);
	// 启动主线程的事件循环
	eventLoopRun(server->mainLoop);
	Debug("服务器程序启动了...");
}
