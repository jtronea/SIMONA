#ifndef _SIMONA_EVENT_INCLUDED_
#define _SIMONA_EVENT_INCLUDED_

#include	"sm_base.h"

typedef enum _EventType
{
	EventType_INVALID = 0,
	EventType_CAN_READ = 0x00000001,
	EventType_CAN_WRITE= 0x00000002
}EventType;

typedef enum _EventState
{
	EventState_INVALID = 0,
	EventState_ACTIVE,
	EventState_INACTIVE
}EventState;

typedef struct _Command
{
	char	command_str[32];
}Command;

typedef struct _Event
{
	int 				index;
	int					*fd; 					//和客户端连接的文件描述符
	int					*usfd;					//用于up stream的文件描述符
	EventState 			event_state;			//用于标识事件状态
	int     			event_type;				//用于标识事件类型
	void* 				mem_space;   			//内存首地址
	int 				mem_offset;  			//内存偏移
	Command* 			commands; 				//记录一系列操作的指令指令
	void*				callback;
}Event;

typedef struct _WorkerRecordData
{
	pthread_cond_t*			cond;				//让该线程唤起的条件；
	int 					index;				//该工作线程的序号
	int 					freepos;			//该工作线程的事件池的空位置
	int 					busypos;			//该工作线程的事件池的忙位置
	int 					eventsnum;			//事件的数量
}WorkerRecordData;
#endif
