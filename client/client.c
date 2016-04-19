#include "client.h"
#define WORKERTHREADS 50
pthread_t threadspool[WORKERTHREADS];       //存放线程

void* handle_task(void* data)
{
    int sfd;
    struct sockaddr_in serveraddr;
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(3333);
    inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr);
    int err = connect(sfd, (SA*)&serveraddr, sizeof(serveraddr));
    printf("connect err %d\n",err);
    char buf[8] = {"12345"};
    err = write(sfd, buf, sizeof(buf));
    printf("finish send data on fd %d\n",sfd);
    char output[8] = {0};
    err = read(sfd, output, sizeof(output));
    printf("finish read data on fd %d\n",sfd);
    output[7] = '\n';
    write(1,  output, sizeof(output));
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
