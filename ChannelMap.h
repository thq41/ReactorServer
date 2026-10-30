#pragma once
#include<stdbool.h>
#include"Channel.h"
#include<stdlib.h>
#include<string.h>

typedef struct ChannelMap
{
	int size;//记录当前map中channel的数量
	//struct Channel* list[];
	Channel** list;
}ChannelMap;

//初始化ChannelMap
ChannelMap* initChannelMap(int size);
//清空ChannelMap
void clearChannelMap(ChannelMap* channelMap);
//重新分配ChannelMap的内存空间（扩容）
bool reallocChannelMap(ChannelMap* channelMap, int newSize,int unitSize);