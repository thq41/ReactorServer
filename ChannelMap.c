#include"ChannelMap.h"

ChannelMap* initChannelMap(int size)
{
	ChannelMap* channelMap = (ChannelMap*)malloc(sizeof(ChannelMap));
	channelMap->size = size;
	channelMap->list = (Channel**)malloc(sizeof(Channel*) * size);
	memset(channelMap->list, 0, sizeof(Channel*) * size);
	return channelMap;
}

void clearChannelMap(ChannelMap* channelMap)
{
	if (channelMap != NULL)
	{
		int size = channelMap->size;
		for (int i = 0; i < size; i++)
		{
			if (channelMap->list[i] != NULL)
			{
				free(channelMap->list[i]);
				channelMap->list[i] = NULL;
			}
		}
		channelMap->size = 0;
		free(channelMap->list);
		channelMap = NULL;
	}
}

bool reallocChannelMap(ChannelMap* channelMap, int newSize, int unitSize)
{
	int curSize = channelMap->size;
	if (curSize < newSize)
	{
		while (curSize < newSize)
		{
			curSize *= 2;//扩容为原来的两倍
		}
		Channel**temp = realloc(channelMap->list, unitSize * curSize);
		if (temp == NULL)
		{
			return false;
		}
		else
		{
			channelMap->list = temp;
			memset(channelMap->list[channelMap->size],0,unitSize * (curSize - channelMap->size));
			channelMap->size = curSize;
			return true;
		}
	}
	return false;
}
