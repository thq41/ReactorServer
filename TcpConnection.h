#pragma once
#include"Channel.h"
#include"Buffer.h"
#include"EventLoop.h"
#include"HttpRequest.h"
#include"HttpResponse.h"

//#define MSG_SEND_AUTO

/*
*	发送数据的两种方式通过宏（MSG_SEND_AUTO）来控制选哪一种
*	第一种定义宏的方式：
*		把要发送的内容完全的写到sendbuf再去发送（弊端：大文件发送不了）
*	第二种未定义宏的方式
*		写一部分发一部分，能解决发送大文件的问题
*/

typedef struct TcpConnection
{
	Buffer* readBuffer; // 读缓冲区
	Buffer* writeBuffer; // 写缓冲区
	EventLoop* evLoop; // 反应堆模型
	char name[32]; // 连接名称
	Channel* channel; // 连接的通道
	// http
	HttpRequest* request; // 请求
	HttpResponse* response; // 响应
} TcpConnection;

TcpConnection* tcpConnectionInit(int fd,EventLoop* evLoop);
int tcpConnDestroy(void* conn);