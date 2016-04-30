#ifndef _SIMONA_HTTP_INCLUDED_
#define _SIMONA_HTTP_INCLUDED_
#include "sm_base.h"

#define MAX_HTTP_HEADER_GET_PARAMS_LENGTH 128  //http get请求中参数的长度
typedef struct _HttpConnection
{
    int		queryfd;		//和客户端建立连接的fd
    int		usfd;			//和后端建立连接的fd
    char*	qMethod;		//http请求的方法：目前仅支持get
    char*	qHeader;		//http请求的头
    char* 	qBody; 			//http请求的body
    char*	qParams;		//http请求的参数

    char*	rHeader;
    char*	rCode;
    char*	rContent;

}HttpConnection;


int parse_http_connection(char* httpContent, HttpConnection* hc);

#endif
