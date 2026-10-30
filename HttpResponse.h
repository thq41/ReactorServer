#pragma once
#include"Buffer.h"
// 定义状态码枚举
typedef enum HttpStatusCode
{
	Unknow,
	OK = 200,
	MovedPermanently = 301,
	MoveTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
}HttpStatusCode;

// 响应头
typedef struct HttpResponseHeader
{
	char key[32];
	char value[128];
}HttpResponseHeader;

// 回复数据块 -->发送一个数据块需要知道一个文件名，还有cfd,缓冲区用来临时存放且保证数据完整性发送的数据要先存再发给客户端
typedef void (*responseBody)(const char* fileName,Buffer* sendBuf,int socket);

// 响应结构体
typedef struct HttpResponse
{
	// 状态行 ： 状态码，状态描述
	HttpStatusCode statusCode;
	char statusMsg[128];
	// 响应头
	HttpResponseHeader* headers;
	int headerNum;
	// 回复数据块 -->由于不知道回复的是目录还是文件，所以定义函数指针来描述回复，也就是回调函数
	responseBody sendDataFunc;
	// 访问文件的名字
	char fileName[128];
}HttpResponse;

// 初始化响应结构体
HttpResponse* httpResponseInit();
// 销毁函数
void httpResponseDestroy(HttpResponse* response);
// 添加响应头
void responseHeaderAdd(HttpResponse* response, const char* key, const char* value);
// 组织http响应数据
void httpResponsePreMsg(HttpResponse* response, Buffer* sendBuf, int socket);
