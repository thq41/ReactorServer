#pragma once
#include"Buffer.h"
#include"HttpResponse.h"
#include<stdbool.h>

// 定义请求头
typedef struct HttpHeader
{
	char* key; // 请求头的键
	char* value; // 请求头的值
}HttpHeader;

// 定义http请求的解析状态
typedef enum HttpRequestState
{
	REQUEST_LINE, // 请求行
	REQUEST_HEADER, // 请求头
	REQUEST_BODY, // 请求体
	REQUEST_DONE // 解析完成
}HttpRequestState;

// 定义http请求的结构体
typedef struct HttpRequest
{
	char* method; // 请求方法
	char* url; // 请求url
	char* version; // http版本
	HttpHeader* reqReaders; // 请求头数组
	int requestHeaderNum; // 请求头的数量
	HttpRequestState curState; // 解析状态
}HttpRequest;

// 初始化http请求
HttpRequest* httpRequestInit();

// 重置http请求
void httpRequestReset(HttpRequest* httpRequest);
void httpRequestResetEx(HttpRequest* httpRequest);
void httpRequestDestroy(HttpRequest* httpRequest);

// 获取当前解析状态
HttpRequestState httpRequestGetCurState(HttpRequest* httpRequest);

// 添加数据到请求头数组
void httpRequestAddHeader(HttpRequest* httpRequest,const char* key,const char* value);// 需要传入一块有效的内存（堆内存），释放会在销毁httpRequest的时候
// 根据请求头的key返回value
char* httpRequestGetHeader(HttpRequest* httpRequest, const char* key);

// 解析请求行
bool parseHttpRequestLine(HttpRequest* httpRequest,Buffer* readBuf);
// 解析请求头中的一行
bool parseHttpRequestHeader(HttpRequest* httpRequest, Buffer* readBuf);
// 解析http请求协议
bool parseHttpRequest(HttpRequest* httpRequest, Buffer* readBuf,Buffer* sendBuf,HttpResponse* response,int socket);
// 处理http请求协议
bool processHttpRequest(HttpRequest* httpRequest, HttpResponse* response);

void sendFile(const char* fileName, Buffer* sendBuf, int cfd);
void sendDir(const char* fileName, Buffer* sendBuf, int cfd);


int http_url_decode(const char* src,
	char* dst,
	size_t dst_size,
	int plus_as_space);
/*
 * URL 百分号解码
 *
 * src            原始 HTTP 路径或参数
 * dst            解码后的输出缓冲区
 * dst_size       dst 的大小
 * plus_as_space  是否把 '+' 解码为空格
 *
 * 返回值：
 *  0   成功
 * -1   输入格式错误
 * -2   输出缓冲区太小
 */