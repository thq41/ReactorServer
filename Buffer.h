#pragma once
#include<stdlib.h>
typedef struct Buffer
{
	char* data; // 数据缓冲区
	size_t capacity; // 缓冲区容量
	size_t readPos; // 读指针位置
	size_t writePos; // 写指针位置
} Buffer;


Buffer* bufferInit(size_t capacity); // 初始化缓冲区
void bufferDestroy(Buffer* buffer); // 销毁缓冲区
// 扩容
void bufferExpandRoom(Buffer* buffer, size_t size);
// 得到缓冲区中剩余未读数据的长度
size_t bufferReadableSize(Buffer* buffer);
// 得到缓冲区中剩余可写数据的长度
size_t bufferWritableSize(Buffer* buffer);
// 1.把数据写入缓冲区 2.把接收的数据写入缓冲区
int bufferAppendData(Buffer* buffer, const char* data, size_t size);
int bufferAppendString(Buffer* buffer, const char* str);
// 把接收的数据写入缓冲区
int bufferSocketRead(int fd, Buffer* buffer);
// 根据\r\n取出一行，找到数据块的位置返回地址
char* bufferFindCRLF(Buffer* buffer);
// 发送缓冲区中的数据
int bufferSendData(int fd, Buffer* buffer);