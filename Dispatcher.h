#pragma once
#include"EventLoop.h" //-->和EventLoop.h循环include
#include"Channel.h"
//三种IO复用模型的分发器(让上层EventLoop想调用哪种模型就调哪种)
typedef struct EventLoop EventLoop;
typedef struct Dispatcher
{
	//初始化分发器 -->得到分发器的数据块dispatcherData
	void* (*init)();
	//添加事件节点
	int (*add)(Channel*,EventLoop*);
	//修改事件节点
	int (*modify)(Channel*,EventLoop*);
	//删除事件节点
	int (*remove)(Channel*,EventLoop*);
	//分配任务
	int (*dispatch)(EventLoop*,int timeout);//单位s
	//清除fd,堆内存
	int (*clear)(EventLoop*);
}Dispatcher;