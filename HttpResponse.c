#include"HttpResponse.h"
#include"TcpConnection.h"
#include<strings.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#define HttpResponseHeaderSize 16

HttpResponse* httpResponseInit()
{
	HttpResponse* response = (HttpResponse*)malloc(sizeof(HttpResponse));
	// 初始化响应头
	response->headerNum = 0;
	int size = sizeof(HttpResponseHeader) * HttpResponseHeaderSize;
	response->headers = (HttpResponseHeader*)malloc(size);
	bzero(response->headers, size);
	// 初始化状态行
	response->statusCode = Unknow;
	bzero(response->statusMsg, sizeof(response->statusMsg));
	// 初始化回复数据块
	response->sendDataFunc = NULL;
	// 初始化文件名
	bzero(response->fileName, sizeof(response->fileName));
	return response;
}

void httpResponseDestroy(HttpResponse* response)
{
	if (!response)
	{
		free(response->headers);
		response->headers = NULL;
		free(response);
		response = NULL;
	}
}

void responseHeaderAdd(HttpResponse* response, const char* key, const char* value)
{
	if (response == NULL || key == NULL || value == NULL)
	{
		return;
	}
	strncpy(response->headers[response->headerNum].key, key,strlen(key));
	strncpy(response->headers[response->headerNum].value, value, strlen(value));
	response->headerNum++;
}

void httpResponsePreMsg(HttpResponse* response, Buffer* sendBuf, int socket)
{
	// 状态行
	char tmp[1024] = { 0 };
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
	bufferAppendString(sendBuf, tmp);
	memset(tmp, 0, sizeof(tmp)); // 这里和教程不一样！！！！！！！！！！
	// 响应头
	for (int i = 0; i < response->headerNum; ++i)
	{
		sprintf(tmp, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
		bufferAppendString(sendBuf, tmp);
		memset(tmp, 0, sizeof(tmp)); // 这里和教程不一样！！！！！！！！！！
	}

	// 空行
	bufferAppendString(sendBuf, "\r\n");
#ifndef MSG_SEND_AUTO
	bufferSendData(socket, sendBuf);
#endif 
	// 回复数据
	response->sendDataFunc(response->fileName, sendBuf, socket);
}
