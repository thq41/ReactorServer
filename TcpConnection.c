#include"TcpConnection.h"
#include<stdio.h>
#include"Log.h"
int processRead(void* arg)
{
	TcpConnection* tcpConn = (TcpConnection*)arg;
	// 1.从套接字中读取数据到读缓冲区
	int count = bufferSocketRead(tcpConn->channel->fd, tcpConn->readBuffer);

	Debug("接收到的http请求数据: %s", tcpConn->readBuffer->data + tcpConn->readBuffer->readPos);
	if (count > 0)
	{
		// 接收到了http请求，解析http请求
		int socket = tcpConn->channel->fd;
#ifdef MSG_SEND_AUTO
		writeEventsEnable(tcpConn->channel, true);
		eventLoopAddTask(tcpConn->evLoop, MODIFY, tcpConn->channel);
#endif // MSG_SEND_AUTO,当不定义这个宏时，processWrite,将不会调用，也就是说读一部分发一部分
		bool flag = parseHttpRequest(tcpConn->request, tcpConn->readBuffer, tcpConn->writeBuffer, tcpConn->response, socket);
		if (!flag)
		{
			// 解析失败，回复简单的html
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			bufferAppendString(tcpConn->writeBuffer, errMsg);
#ifndef MSG_SEND_AUTO
			bufferSendData(socket, tcpConn->writeBuffer);
#endif
		}
	} 
#ifdef MSG_SEND_AUTO  
	else
	{
		// 当接收数据为空时断开连接
		eventLoopAddTask(tcpConn->evLoop, DELETE, tcpConn->channel);
	}
#endif
#ifndef MSG_SEND_AUTO
	// 断开连接 -->采用把内容完全写入buf再发送的方式，不能断开连接，因为还没有该线程还没发送数据，一个线程负责接收从浏览器发来的数据，也负责发送数据
	eventLoopAddTask(tcpConn->evLoop, DELETE, tcpConn->channel);
#endif
	return 0;
}

int processWrite(void* arg)
{
	Debug("数据开始发送了(基于写事件发送)...");
	TcpConnection* tcpConn = (TcpConnection*)arg;
	// 发送数据
	int count = bufferSendData(tcpConn->channel->fd, tcpConn->writeBuffer);
	if (count > 0)
	{
		// 判断数据是否发完
		if (bufferReadableSize(tcpConn->writeBuffer) == 0)
		{
			// 1.不在检测写事件
			writeEventsEnable(tcpConn->channel, false);
			// 2.修改dispatcher
			eventLoopAddTask(tcpConn->evLoop, MODIFY, tcpConn->channel);
			// 3.删除节点
			eventLoopAddTask(tcpConn->evLoop, DELETE, tcpConn->channel);	
		}
	}
	else
	{
		// bufferSendData调用失败直接断开连接
		eventLoopAddTask(tcpConn->evLoop, DELETE, tcpConn->channel);
	}
	return 0;
}

TcpConnection* tcpConnectionInit(int fd, EventLoop* evLoop)
{
	TcpConnection* tcpConn = (TcpConnection*)malloc(sizeof(TcpConnection));
	
	tcpConn->readBuffer = bufferInit(10240);
	tcpConn->writeBuffer = bufferInit(10240);
	// http
	tcpConn->request = httpRequestInit();
	tcpConn->response = httpResponseInit();
	sprintf(tcpConn->name, "TcpConnection-%d", fd);
	tcpConn->evLoop = evLoop;
	tcpConn->channel = initChannel(fd, READ_EVENT, processRead, processWrite, tcpConnDestroy,tcpConn);
	// 1.添加channel到反应堆模型中
	eventLoopAddTask(evLoop, ADD,tcpConn->channel);
	Debug("和客户端建立连接，threadName %s, threadID:%ld, connName: %s",
		evLoop->threadName, evLoop->threadID, tcpConn->name);

	return tcpConn;
}

int tcpConnDestroy(void* conn)
{
	TcpConnection* tcpConn = (TcpConnection*)conn;
	Debug("连接断开，释放资源，gameover, connName: %s", tcpConn->name);
	if (tcpConn != NULL)
	{
		if (tcpConn->readBuffer && bufferReadableSize(tcpConn->readBuffer) == 0 &&
			tcpConn->writeBuffer && bufferReadableSize(tcpConn->writeBuffer) == 0)
		{
			bufferDestroy(tcpConn->writeBuffer);
			bufferDestroy(tcpConn->readBuffer);
			httpRequestDestroy(tcpConn->request);
			httpResponseDestroy(tcpConn->response);
			destroyChannel(tcpConn->evLoop, tcpConn->channel);
		}
		free(tcpConn);
	}
	return 0;
}
