#include "sm_http.h"

/*query*/
// "GET /?test0=0&test1=1 HTTP/1.1\r\nUser-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcur
// l/7.19.7 NSS/3.13.1.0 zlib/1.2.8 libidn/1.18 libssh2/1.2.2\r\nHost: 127.0.0.1:333
// 3\r\nAccept: */*\r\n\r\n"

/*res*/
// HTTP/1.1 200 OK\r\nServer: nginx/1.2.0\r\nDate: Thu, 28 Apr 2016 06:50:50 GMT\r\nContent-Type: text/htm
// l\r\nContent-Length: 154\r\nLast-Modified: Tue, 22 Mar 2016 13:02:21 GMT\r\nConnection: keep-a
// live\r\nAccept-Ranges: bytes\r\n\r\n<html>\n<head>\n<title>Welcome to nginx!asd</title>\n</head>\n<bo
// dy bgcolor=\"white\" text=\"black\">\n<center><h1>Welcome to nginx!</h1></center>\n</body>\n</html>\n<h
// tml>\r\n<head><title>400 Bad Request</title></head>\r\n<body bgcolor=\"white\">\r\n<center><h1>400 Bad Request</h1></center>\r\n<hr><center>nginx/1.2.0"

char METHOD[2][10] = {"GET","POST"};

int parse_http_connection(char* httpContent, HttpConnection* hc)
{
	if(hc == NULL)
	{
		return 1;
	}

	int charIndex = 0;
	int charLength = strlen(httpContent);
	if( memcmp(httpContent, METHOD[0], 3) == 0 )
	{
		charIndex = 3;
		hc->qMethod = (char*)malloc(sizeof(char) * 4);
		memcpy(hc->qMethod, METHOD[0], 3);
		hc->qMethod[3] = 0;
	}
	else if( memcmp(httpContent, METHOD[1], 4) == 0 )
	{
		charIndex = 4;
		hc->qMethod = (char*)malloc(sizeof(char) * 5);
		memcpy(hc->qMethod, METHOD[1], 4);
		hc->qMethod[4] = 0;
	}
	else
	{
		return 2; //http方法不能识别
	}


	int paramBegin = 0;
	int paramEnd = 0;
	bool cond0 = true; //首次出现'/?';
	bool cond1 = false; //第二次出现' ';
	int i = 0;
    for(int i = charIndex; i < charLength; i++)
	{
		if(httpContent[i] != ' ' && cond0)
		{
			if(httpContent[i] == '/' || httpContent[i] == '?')
			{
				continue;
			}
			else
			{
				paramBegin = i;
				cond0 = false;
			}	
		}
		else if(httpContent[i] == ' ')
		{
			if(!cond1)
			{
				cond1 = true;
			}
			else
			{
				paramEnd = i;
				charIndex = i;
				break;
			}
		}
	}
    if(i >= charLength)
    {
        return 3;
    }


    if(!hc->qParams)
    {
        return 4;
    }
    if(paramEnd - paramBegin >= MAX_HTTP_HEADER_GET_PARAMS_LENGTH)
    {
        return 5;
    }
    memcpy(hc->qParams, httpContent+paramBegin, paramEnd - paramBegin);
    return 0;
}
