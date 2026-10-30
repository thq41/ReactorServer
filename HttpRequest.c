#include "HttpRequest.h"
#include "TcpConnection.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<assert.h>
#include<sys/stat.h>
#include <stddef.h>
#include <dirent.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h> // 【修改】读取文件遇到 EINTR 时重试
#define URL_DECODE_OK          0
#define URL_DECODE_INVALID    -1
#define URL_DECODE_TOO_SMALL  -2

#define MAX_REQUEST_HEADER_NUM 12

/* 把一个十六进制字符转换成数字 */
static int hex_to_value(char c);
static int extensionEquals(const char* a, const char* b);
const char* getFileType(const char* name);

HttpRequest* httpRequestInit()
{
	HttpRequest* httpRequest = (HttpRequest*)malloc(sizeof(HttpRequest));
	httpRequestReset(httpRequest);
	// 初始化请求头数组
	httpRequest->reqReaders = (HttpHeader*)malloc(sizeof(HttpHeader) * MAX_REQUEST_HEADER_NUM); // 假设最多有12个请求头
	return httpRequest;
}

void httpRequestReset(HttpRequest* httpRequest)
{
	httpRequest->method = NULL;
	httpRequest->url = NULL;
	httpRequest->version = NULL;
	httpRequest->curState = REQUEST_LINE;
	httpRequest->requestHeaderNum = 0;
}

void httpRequestResetEx(HttpRequest* httpRequest)
{
	free(httpRequest->method);
	free(httpRequest->url);
	free(httpRequest->version);
	if (httpRequest->reqReaders != NULL)
	{
		for (int i = 0; i < httpRequest->requestHeaderNum; i++)
		{
			free(httpRequest->reqReaders[i].key);
			free(httpRequest->reqReaders[i].value);
		}
		free(httpRequest->reqReaders);
		httpRequest->reqReaders = NULL;
	}
	httpRequestReset(httpRequest);
}

void httpRequestDestroy(HttpRequest* httpRequest)
{
	if (httpRequest != NULL)
	{
		httpRequestResetEx(httpRequest);
		free(httpRequest);
		httpRequest = NULL;
	}
}

HttpRequestState httpRequestGetCurState(HttpRequest* httpRequest)
{
	return httpRequest->curState;
}

void httpRequestAddHeader(HttpRequest* httpRequest, const char* key, const char* value)
{
	httpRequest->reqReaders[httpRequest->requestHeaderNum].key = (char*)key;
	httpRequest->reqReaders[httpRequest->requestHeaderNum].value = (char*)value;
	httpRequest->requestHeaderNum++;
}

char* httpRequestGetHeader(HttpRequest* httpRequest, const char* key)
{
	if (httpRequest != NULL)
	{
		HttpHeader* reqReaders = httpRequest->reqReaders;
		for (int i = 0; i < httpRequest->requestHeaderNum; i++)
		{
			if (strncasecmp(reqReaders[i].key, key, strlen(key)) == 0)
			{
				return reqReaders[i].value;
			}
		}
	}
	return NULL;
}

char* splitRequestLine(char* start,char* end,char* substr,char** ptr)
{
	char* space = end;
	if (substr != NULL)
	{
		space = memmem(start, end - start, substr, strlen(substr));
		assert(space != NULL);		
	}
	char* tmp = (char*)malloc(space - start + 1);
	strncpy(tmp, start, space - start);
	tmp[space - start] = '\0';
	*ptr = tmp;
	return space+1;
}

bool parseHttpRequestLine(HttpRequest* httpRequest, Buffer* readBuf)
{
	// 请求行样例 Get /xxx/xxx http/1.1
	// 读出请求行，保存的结束地址
	char* end = bufferFindCRLF(readBuf);
	// 保存字符串的起始地址
	char* start = readBuf->data + readBuf->readPos;
	int lineLength = end - start; // 这里不包括\r，所以不需要+1
	if (lineLength)
	{
		// 优化后代码
		start = splitRequestLine(start, end, " ", &httpRequest->method);
		start = splitRequestLine(start, end, " ", &httpRequest->url);
		splitRequestLine(start, end, NULL, &httpRequest->version);

#if 0
		// 拿出请求方法
		char* space = memmem(start, lineLength, " ", 1); 
		assert(space != NULL);
		httpRequest->method = (char*)malloc(space - start + 1);
		strncpy(httpRequest->method, start, space - start);
		httpRequest->method[space - start] = '\0';
		// 更新start
		start = space + 1;
		// 拿出请求的url
		space = memmem(start, end - start, " ", 1);
		assert(space != NULL);
		httpRequest->url = (char*)malloc(space - start + 1);
		strncpy(httpRequest->url, start, space - start);
		httpRequest->url[space - start] = '\0';
		// 更新start
		start = space + 1;
		// 拿出请求的http版本
		httpRequest->version = (char*)malloc(end - start + 1);
		strncpy(httpRequest->version, start, end - start);
		httpRequest->version[end - start] = '\0';
#endif
		// 为解析请求头做准备
		readBuf->readPos += lineLength;
		readBuf->readPos += 2;
		// 修改解析状态
		httpRequest->curState = REQUEST_HEADER;
		return true;
	}
	return false;
}

bool parseHttpRequestHeader(HttpRequest* httpRequest, Buffer* readBuf)
{
	// 1.拿出一行请求头 -->2. 把他放入httpRequest里面的请求头数组中 -->3. 返回true,如果拿不到key和value返回false
	// 请求头格式 key: value
	char* end = bufferFindCRLF(readBuf);
	if (end != NULL)
	{
		char* start = readBuf->data + readBuf->readPos;
		int headerLength = end - start;
		char* middle = memmem(start, headerLength, ": ", 2);
		if (middle != NULL)
		{
			// 保存key
			char* key = (char*)malloc(middle - start + 1);
			key[middle - start] = '\0';
			
			// 保存value
			char* value = (char*)malloc(end - middle - 2 + 1);
			value[end - middle - 2] = '\0';
			
			// 添加到httpRequestHeaders中
			httpRequestAddHeader(httpRequest, key, value);

			// 更新readPos
			readBuf->readPos += headerLength;
			readBuf->readPos += 2;
			// return true;// 这里可能是要修改的！！！！！！！！！！！！！！！！！！！！！！！！！
		}
		else
		{
			// 找不到的情况是遇到了空行\r\n
			readBuf->readPos += 2;// 跳过\r\n
			// 当前仅处理get请求,未处理post
			// 修改解析解析状态
			httpRequest->curState = REQUEST_DONE;
		}
		return true;  //-- 教程这里return true
	}
	return false;
}

bool parseHttpRequest(HttpRequest* httpRequest, Buffer* readBuf, Buffer* sendBuf, HttpResponse* response, int socket)
{
	// 该函数包含了解析请求行，并对解析完的数据进行处理，再组织要发送的数据到sendBuf
	// 通过循环去解析下面的内容循环变量参考解析状态
	// 1.解析请求行
	// 2.解析请求头
	//  解析成功后处理
	// 3.复原解析状态，因为客户端会发来下一次请求
	bool flag = true; // 标记解析是否成功
	while (httpRequest->curState != REQUEST_DONE)
	{
		switch (httpRequest->curState)
		{
		case REQUEST_LINE:
			flag = parseHttpRequestLine(httpRequest, readBuf);
			break;
		case REQUEST_HEADER:
			flag = parseHttpRequestHeader(httpRequest, readBuf);
			break;
		case REQUEST_BODY:
			// 当前只处理get请求所以这块还未开发！！！！！！！
			break;
		default:
			// 解析完成
			flag = true;
			break;
		}
		// 如果解析失败了直接返回
		if (flag == false)
		{
			return flag;
		}
		// 解析成功对请求做出处理
		if (httpRequest->curState == REQUEST_DONE)
		{
			// 1.根据解析出来的原始数据，对客户端的请求做出处理
			processHttpRequest(httpRequest, response);
			// 2.组织响应数据块回复
			httpResponsePreMsg(response, sendBuf, socket);
		}
	}
	// 复原解析状态，保证还能处理后续请求
	httpRequest->curState = REQUEST_LINE;
	return flag;
}

bool processHttpRequest(HttpRequest* httpRequest,HttpResponse* response)
{
	//当前版本只处理get
	if (strcasecmp(httpRequest->method, "get") != 0)
	{
		return false;
	}
	http_url_decode(httpRequest->url, httpRequest->url, strlen(httpRequest->url)+1, 1);
	//处理客户端请求的静态资源（目录或文件）
	char* file = NULL;
	if (strcmp(httpRequest->url, "/") == 0)
	{
		file = "./";
	}
	else
	{
		file = httpRequest->url + 1;
	}
	//获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		//文件不存在 -- 回复404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType("404.html"), -1);
		//sendFile("404.html", cfd);
		// 响应行
		strcpy(response->fileName, "404.html");
		response->statusCode = NotFound;
		strcpy(response->statusMsg, "Not Found");
		// 响应头
		responseHeaderAdd(response, "Content-type", getFileType(".html"));
		// 回复数据
		response->sendDataFunc = sendFile;
		return false;
	}
	else
	{
		// 响应行
		strcpy(response->fileName, file);
		response->statusCode = OK;
		strcpy(response->statusMsg, "OK");
		//判断是文件还是目录
		if (S_ISDIR(st.st_mode))
		{
			//给客户端返回目录
			//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
			//sendDir(cfd, file);
			// 响应头
			responseHeaderAdd(response, "Content-type", getFileType(".html"));
			// 回复数据
			response->sendDataFunc = sendDir;
		}
		else
		{
			//把文件发给客户端 ---http响应
			//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
			//sendFile(file, cfd);
			//printf("send file %s --- tid = %ld\n", file, pthread_self());
			// 响应头
			responseHeaderAdd(response, "Content-type", getFileType(file));
			// 【修改：MP4下载】存在的普通 MP4 文件按附件下载，扩展名不区分大小写
			const char* extension = strrchr(file, '.');
			if (S_ISREG(st.st_mode) && extension != NULL &&
				strcasecmp(extension, ".mp4") == 0)
			{
				// 不拼接 filename，浏览器根据请求 URL 建议保存名称
				responseHeaderAdd(response, "Content-Disposition", "attachment");
			}
			char tmp[128] = { 0 };
			sprintf(tmp, "%ld", st.st_size);
			responseHeaderAdd(response, "Content-length", tmp);
			// 回复数据
			response->sendDataFunc = sendFile;
		}
	}
	return true;
}

static int hex_to_value(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';

	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;

	return -1;
}

int http_url_decode(const char* src,
	char* dst,
	size_t dst_size,
	int plus_as_space)
{
	size_t i = 0;
	size_t out = 0;

	if (src == NULL || dst == NULL || dst_size == 0)
		return URL_DECODE_INVALID;

	while (src[i] != '\0') {
		unsigned char value;

		if (src[i] == '%') {
			int high;
			int low;

			/* '%' 后面必须有两个十六进制字符 */
			if (src[i + 1] == '\0' || src[i + 2] == '\0')
				return URL_DECODE_INVALID;

			high = hex_to_value(src[i + 1]);
			low = hex_to_value(src[i + 2]);

			if (high < 0 || low < 0)
				return URL_DECODE_INVALID;

			value = (unsigned char)((high << 4) | low);

			/*
			 * %00 会产生字符串结束符，
			 * C 字符串无法安全保存它，所以拒绝。
			 */
			if (value == '\0')
				return URL_DECODE_INVALID;

			i += 3;
		}
		else {
			value = (unsigned char)src[i];

			/*
			 * 查询参数中通常把 '+' 当成空格。
			 * 但是 URL 路径中 '+' 通常应该保留。
			 */
			if (plus_as_space && value == '+')
				value = ' ';

			i++;
		}

		/* 多留一个字节给 '\0' */
		if (out + 1 >= dst_size)
			return URL_DECODE_TOO_SMALL;

		dst[out++] = (char)value;
	}

	dst[out] = '\0';

	return URL_DECODE_OK;
}

typedef struct {
	const char* extension;
	const char* content_type;
} MimeType;

const char* getFileType(const char* name)
{
	static const MimeType mimeTypes[] = {
		{ ".html", "text/html; charset=utf-8" },
		{ ".htm",  "text/html; charset=utf-8" },
		{ ".css",   "text/css; charset=utf-8" },
		{ ".js",    "text/javascript; charset=utf-8" },
		{ ".json",  "application/json; charset=utf-8" },
		{ ".xml",   "application/xml; charset=utf-8" },
		{ ".txt",   "text/plain; charset=utf-8" },

		{ ".jpg",   "image/jpeg" },
		{ ".jpeg",  "image/jpeg" },
		{ ".png",   "image/png" },
		{ ".gif",   "image/gif" },
		{ ".bmp",   "image/bmp" },
		{ ".webp",  "image/webp" },
		{ ".svg",   "image/svg+xml" },
		{ ".ico",   "image/x-icon" },

		{ ".mp3",   "audio/mpeg" },
		{ ".wav",   "audio/wav" },
		{ ".ogg",   "audio/ogg" },

		{ ".mp4",   "video/mp4" },
		{ ".webm",  "video/webm" },
		{ ".avi",   "video/x-msvideo" },

		{ ".pdf",   "application/pdf" },
		{ ".zip",   "application/zip" },
		{ ".gz",    "application/gzip" },
		{ ".wasm",  "application/wasm" }
	};

	const char* dot;
	const char* slash;
	size_t i;

	if (name == NULL || *name == '\0')
		return "application/octet-stream";

	/* 找到最后一个扩展名点号。 */
	dot = strrchr(name, '.');

	/* 防止把目录名中的点号误认为文件扩展名。 */
	slash = strrchr(name, '/');
	if (slash == NULL)
		slash = strrchr(name, '\\');

	if (dot == NULL || (slash != NULL && dot < slash))
		return "text/plain; charset=utf-8";

	for (i = 0; i < sizeof(mimeTypes) / sizeof(mimeTypes[0]); ++i) {
		if (extensionEquals(dot, mimeTypes[i].extension))
			return mimeTypes[i].content_type;
	}

	/* 未知二进制文件的通用类型。 */
	return "application/octet-stream";
}

void sendDir(const char* fileName, Buffer* sendBuf, int cfd)
{
	char buf[4096] = { 0 };//拼接html
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", fileName);
	struct dirent** nameList;
	int num = scandir(fileName, &nameList, NULL, alphasort);
	for (int i = 0; i < num; ++i)
	{
		//取出文件名
		const char* name = nameList[i]->d_name;
		//获取文件属性
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", fileName, name);
		struct stat st;
		stat(subPath, &st);
		//判断是文件还是目录
		if (S_ISDIR(st.st_mode))
		{
			//拼接目录
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			//拼接文件
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		//发送这部分数据
		// send(cfd, buf, strlen(buf), 0);
		// 存入buffer
		bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
		bufferSendData(cfd, sendBuf);
#endif 
		memset(buf, 0, sizeof buf);
		//释放内存
		free(nameList[i]);

	}
	sprintf(buf, "</table></body></html>");
	// send(cfd, buf, strlen(buf), 0);
	// 存入buffer
	bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
	bufferSendData(cfd, sendBuf);
#endif 
	free(nameList);
}

void sendFile(const char* fileName, Buffer* sendBuf, int cfd)
{
	//1.打开文件
	int fd = open(fileName, O_RDONLY);
	if (fd <= 0)
	{
		fd = open("404.html", O_RDONLY);
	}
	assert(fd > 0);
	// 【修改】开始：64 KB 分块读取；发送失败后停止本次文件响应
	char buf[64 * 1024];
	while (1)
	{
		ssize_t len = read(fd, buf, sizeof(buf));
		if (len > 0)
		{
			if (bufferAppendData(sendBuf, buf, (size_t)len) < 0)
			{
				break;
			}
#ifndef MSG_SEND_AUTO
			if (bufferSendData(cfd, sendBuf) < 0)
			{
				// 客户端取消下载，不再继续读取和发送图片
				break;
			}
#endif 
		}
		else if (len == 0)
		{
			// 断开连接
			break;
		}
		else if (errno != EINTR)
		{
			perror("read file");
			break;
		}
	}
	// 【修改】结束：循环结束后由函数末尾统一关闭文件 fd

#if 0
	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	off_t offset = 0;
	const size_t CHUNK_SIZE = 1024 * 1024 * 4; // 每次发送的块大小为4MB
	while (offset < size)
	{
		size_t send_len = (size - offset) < CHUNK_SIZE ? (size - offset) : CHUNK_SIZE;
		int ret = sendfile(cfd, fd, &offset, send_len);
		if (ret < 0)
		{
			perror("sendfile");
			break;
		}
	}
#endif
	//关闭文件描述符
	close(fd);
}


/* 仅用于比较 ASCII 文件扩展名，不依赖平台的 strcasecmp。 */
static int extensionEquals(const char* a, const char* b)
{
	while (*a != '\0' && *b != '\0') {
		unsigned char ca = (unsigned char)*a++;
		unsigned char cb = (unsigned char)*b++;

		if (ca >= 'A' && ca <= 'Z')
			ca = (unsigned char)(ca + ('a' - 'A'));

		if (cb >= 'A' && cb <= 'Z')
			cb = (unsigned char)(cb + ('a' - 'A'));

		if (ca != cb)
			return 0;
	}

	return *a == '\0' && *b == '\0';
}
