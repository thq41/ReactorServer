#include "Buffer.h"
#include<string.h>
#include<sys/uio.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h> // 【修改】识别发送过程中被信号中断的情况
#include<sys/socket.h> // 【修改】send() 与 MSG_NOSIGNAL
Buffer* bufferInit(size_t capacity)
{
	Buffer* buffer = (Buffer*)malloc(sizeof(Buffer));
	if (buffer == NULL)
	{
		return NULL;
	}
	buffer->data = (char*)malloc(capacity);
	if (buffer->data == NULL)
	{
		free(buffer);
		return NULL;
	}
	buffer->capacity = capacity;
	buffer->readPos = 0;
	buffer->writePos = 0;
	return buffer;
}

void bufferDestroy(Buffer* buffer)
{
	if (buffer != NULL)
	{
		if (buffer->data)
		{
			free(buffer->data);
			buffer->data = NULL;
		}
		free(buffer);
		buffer = NULL;
	}
}

void bufferExpandRoom(Buffer* buffer, size_t size)
{
	// 1.剩余可写空间足够
	if (buffer->capacity - buffer->writePos >= size)
	{
		return;
	}
	// 2.剩余可写空间不足，但是已读数据+剩余可写空间足够
	else if (bufferWritableSize(buffer) + buffer->readPos >= size)
	{
		// 把未读数据移动到缓冲区的起始位置
		memmove(buffer->data,buffer->data + buffer->readPos, bufferReadableSize(buffer));
		buffer->writePos = bufferReadableSize(buffer);
		buffer->readPos = 0;
	}
	// 3.剩余可写空间不足，已读数据+剩余可写空间也不足 --> 扩容
	else
	{ 
		size_t newCapacity = buffer->capacity + size;
		void* temp = realloc(buffer->data, newCapacity);
		if (temp == NULL)
		{
			return;
		}
		// 初始化新扩容的空间
		memset((char*)temp + buffer->capacity, 0, size);
		// 更新缓冲区的指针和容量
		buffer->data = (char*)temp;
		buffer->capacity = newCapacity;
	}
}

size_t bufferReadableSize(Buffer* buffer)
{
	return buffer->writePos - buffer->readPos;
}

size_t bufferWritableSize(Buffer* buffer)
{
	return buffer->capacity - buffer->writePos;
}

int bufferAppendData(Buffer* buffer, const char* data, size_t size)
{
	if (buffer == NULL || data == NULL || size == 0)
	{
		return -1;
	}
	// 先扩容 --> 调用的这个函数也不一定会扩容，如果需要的话，他会自己扩容，但是一定要保证有足够的空间写入数据
	bufferExpandRoom(buffer, size);
	memcpy(buffer->data + buffer->writePos, data, size);
	buffer->writePos += size;
	return 0;
}

int bufferAppendString(Buffer* buffer, const char* str)
{ 
	int size = strlen(str);
	int ret = bufferAppendData(buffer, str, size);
	return ret;
}

int bufferSocketRead(int fd, Buffer* buffer)
{
	// 调用readv来接收数据，再把数据写入缓冲区
	if (buffer == NULL || fd < 0)
	{
		return -1;
	}
	int writableSize = bufferWritableSize(buffer);
	struct iovec vec[2];
	// 初始化vec
	vec[0].iov_base = buffer->data + buffer->writePos; // 这里vec[0]读的这部分已经进入buffer,剩下vec[1]这部分需要添加到buffer
	vec[0].iov_len = writableSize;
	vec[1].iov_base = (char*)malloc(40960);
	vec[1].iov_len = 40960;
	// 调用readv来接收数据
	int ret = readv(fd, vec, 2);
	if (ret == -1)
	{
		free(vec[1].iov_base);
		return -1;
	}
	else if (ret <= writableSize)
	{
		buffer->writePos += ret;
	}
	else
	{
		buffer->writePos = buffer->capacity;
		int extraDataSize = ret - writableSize;
		bufferAppendData(buffer, (char*)vec[1].iov_base, extraDataSize);
	}
	// 释放vec[1]的内存
	free(vec[1].iov_base);
	vec[1].iov_base = NULL;
	// 表示接收的字节数
	return ret;
}

char* bufferFindCRLF(Buffer* buffer)
{
	char* ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
	return ptr;
}

int bufferSendData(int fd, Buffer* buffer)
{
	// 【修改】开始：循环发送全部待发送数据，避免部分发送后丢失尾部数据
	int total = 0;
	while (bufferReadableSize(buffer) > 0)
	{
		ssize_t count = send(fd, buffer->data + buffer->readPos,
			bufferReadableSize(buffer), MSG_NOSIGNAL);
		if (count > 0)
		{
			buffer->readPos += (size_t)count;
			total += (int)count;
			continue;
		}
		if (count < 0 && errno == EINTR)
		{
			continue;
		}
		// EPIPE、ECONNRESET 或发送无进展：上层停止本次响应
		return -1;
	}
	buffer->readPos = 0;
	buffer->writePos = 0;
	return total;
	// 【修改】结束：MSG_NOSIGNAL 避免 SIGPIPE，移除每次发送后的休眠
}
