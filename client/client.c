#include "client.h"
#define WORKERTHREADS 1
pthread_t threadspool[WORKERTHREADS];       //存放线程

void* handle_task(void* data)
{
    int sfd;
    struct sockaddr_in serveraddr;
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    //serveraddr.sin_port = htons(3333);
    //inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr);
    serveraddr.sin_port = htons(3333);
    inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr);
    int err = connect(sfd, (SA*)&serveraddr, sizeof(serveraddr));
    printf("connect err %d\n",err);
    char buf[512] = {"GET /?test=troneacheng HTTP/1.1\r\nUser-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.19.7 NSS/3.13.1.0 zlib/1.2.8 libidn/1.18 libssh2/1.2.2\r\nHost: 127.0.0.1:3333\r\nAccept: */*\r\n\r\n"};
    err = write(sfd, buf, sizeof(buf));
    printf("finish send data on fd %d\n",sfd);
    char output[512] = {0};
    err = read(sfd, output, sizeof(output));
    if(err >= 0)
    {
        printf("finish read data on fd %d\n",sfd);
        output[err] = '\n';
        write(1,  output, sizeof(output));
    }
    else
    {
        printf("read data on fd %d error\n",sfd);
    }
}

int spawn_multi_workerthread(){
    for(int i = 0; i < WORKERTHREADS; i++)
    {
        int err = pthread_create(&threadspool[i], NULL, &handle_task, NULL);
         printf("create thread %d\n", err);
    }
    return 0;
}

int main(int argc, char * argv[]) {
    spawn_multi_workerthread();
  
    for(int i = 0; i < WORKERTHREADS; i++)
    {
        pthread_join(threadspool[i], NULL);
    }
    //handle_task(NULL);
}
