#include "simona.h"
#include "sm_event.h"

pthread_t threadspool[WORKERTHREADS];       //存放线程
pthread_cond_t condspool[WORKERTHREADS];    //存放每个工作线程的cond
Event** eventpool = NULL;
void** mempool = NULL;
Event* activeeventpool[MAXCONNECTION] = {NULL}; //存放活跃的事件数量, [i*MAXCONNECTION/WORKERTHREADS,(i+1)*MAXCONNECTION/WORKERTHREADS]被第i个工作线程做拥有
WorkerRecordData* workerRecords[WORKERTHREADS];

int create_and_bind(const char * port) {
    struct addrinfo hints;
    struct addrinfo * result, *rp;
    int s, sfd;

    memset( & hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM;
    /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;
    /* All interfaces */
    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return - 1;
    }

    for (rp =result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {
            /* We managed to bind successfully! */
            break;
        }

        close(sfd);
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not bind\n");
        return - 1;
    }

    freeaddrinfo(result);

    return sfd;
}
 
int make_socket_non_blocking(int sfd) {
    int flags,
    s;

    //得到文件状态标志  
    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return - 1;
    }

    //设置文件状态标志  
    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl");
        return - 1;
    }

    return 0;
}

int add_read_event(int efd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; //读入,边缘触发方式  
    int s = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    return s;
}

int add_write_event(int efd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events =  EPOLLOUT | EPOLLET; //读入,边缘触发方式  
    int s = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    return s;
}

int init_event(Event*** eventpool, void*** mempool)
{
    *eventpool = (Event** ) calloc(MAXCONNECTION, sizeof(Event*));
    if(*eventpool == NULL)
    {
        return -1;
    }
    for(int i = 0; i < MAXCONNECTION; i++)
    {
        (*eventpool)[i] = (Event*) calloc(1, sizeof(Event));
        if((*eventpool)[i] == NULL)
        {
            return -1;
        }
        else
        {
            memset((*eventpool)[i], 0, sizeof(Event));
        }
    }

    *mempool = (void**) calloc(MAXCONNECTION, sizeof(void*));
    if(*mempool == NULL)
    {
        return -1;
    }
    for(int i = 0; i < MAXCONNECTION; i++)
    {
        (*mempool)[i] = (void*) calloc(MEMPOOLSIZE, 1);
        if((*mempool)[i] == NULL)
        {
            return -1;
        }
        else
        {
            memset((*mempool)[i], 0, MEMPOOLSIZE);
        }
    }

    for(int i = 0; i < WORKERTHREADS; i++)
    {
        workerRecords[i] = (WorkerRecordData*) calloc(1, sizeof(WorkerRecordData));
        if(workerRecords[i] == NULL)
        {
            return -1;
        }
        else
        {
            memset(workerRecords[i], 0, sizeof(WorkerRecordData));
        }
    }
    return 0;
}

int event_handle(Event* event)
{
    char buf[512] = {0};
    read(*event->fd, buf, sizeof(buf));
    write(*event->fd, buf, sizeof(buf));
    close(*event->fd);
}

void* handle_task(void* data){
    WorkerRecordData* record = (WorkerRecordData*)data;
    pthread_cond_t* wakeUpCond = record->cond;
    int index = record->index;

    int err = pthread_cond_init(wakeUpCond, NULL);
    pthread_mutex_t mutex;
    err = pthread_mutex_init(&mutex, NULL);
    for ( ; ; )
    {
        pthread_cond_wait(wakeUpCond, &mutex);
        for( ; ; )
        {
            printf("work %d start handle task\n", index);
            int start = MAXCONNECTION/WORKERTHREADS * index + record->busypos;
            Event* event = activeeventpool[start];
            event_handle(event);
            record->eventsnum --; //todo: lock later
            record->freepos --; //todo: lock later
            if(record->eventsnum == 0)
            {
                break;
            }
        }
    }
}

int spawn_multi_workerthread(){
    for(int i = 0; i < WORKERTHREADS; i++)
    {
        WorkerRecordData* data = workerRecords[i];
        memset(data, 0, sizeof(WorkerRecordData));
        data->cond = &condspool[i];
        data->index = i;
        data->freepos = 0;
        data->busypos = 0;
        pthread_create(&threadspool[i], NULL, &handle_task, data);
    }
    return 0;
}

int delete_event(int fd){
    memset(eventpool[fd], 0, sizeof(Event));
}

int generate_fd_event(int fd){
    if(eventpool[fd]->index != 0)
        return -1;

    eventpool[fd]->index = fd;
    eventpool[fd]->fd = &fd;
    eventpool[fd]->event_state = EventState_INACTIVE;
}

long GetTickUSCount()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (tv.tv_sec*1000000 + tv.tv_usec);    
}

int load_balance(){
    return GetTickUSCount() % WORKERTHREADS;
}

int set_event_to_worker(Event* event, int workerid)
{
    WorkerRecordData* record = workerRecords[workerid];
    if(record->eventsnum < MAXCONNECTION/WORKERTHREADS)
    {
        int start = MAXCONNECTION/WORKERTHREADS * workerid + record->freepos;
        activeeventpool[start] = event;
        
        //char buf[128]={0};
        //read (*(event->fd), buf, sizeof buf);

        record->eventsnum ++; //todo: lock later
        record->freepos ++;
        if(record->eventsnum == 1)
        {
            pthread_cond_signal(&condspool[workerid]);
        }
    }
    else
    {
        return -1;
    }
    return 0;
}

int main(int argc, char * argv[]) {
    int sfd, s;
    int efd;
    struct epoll_event event;
    struct epoll_event * events;
    init_event(&eventpool, &mempool);
    spawn_multi_workerthread();
    /*  if (argc != 2)  
    {  
      fprintf (stderr, "Usage: %s [port]\n", argv[0]);  
      exit (EXIT_FAILURE);  
    } */

    sfd = create_and_bind("3333");
    if (sfd == -1) abort();

    s = make_socket_non_blocking(sfd);
    if (s == -1) abort();

    s = listen(sfd, SOMAXCONN);
    if (s == -1) {
        perror("listen");
        abort();
    }

    //除了参数size被忽略外,此函数和epoll_create完全相同  
    efd = epoll_create1(0);
    if (efd == -1) {
        perror("epoll_create");
        abort();
    }

    s = add_read_event(efd, sfd);
    if (s == -1) {
        perror("epoll_ctl");
        abort();
    }

    /* Buffer where events are returned */
    events = (epoll_event * ) calloc(MAXEVENTS, sizeof(event));

    /* The event loop */
    while (1) {
        int n,
        i;

        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN || events[i].events & EPOLLOUT))) {
                /* An error has occured on this fd, or the socket is not 
                 ready for reading (why were we notified then?) */
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                delete_event(events[i].data.fd);
                continue;
            }

            else if (sfd == events[i].data.fd) {
                /* We have a notification on the listening socket, which 
                 means one or more incoming connections. */
                while (1) {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST],
                    sbuf[NI_MAXSERV];

                    in_len = sizeof(in_addr);
                    infd = accept(sfd, &in_addr, &in_len);
                    if (infd == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            /* We have processed all incoming 
                             connections. */
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    //将地址转化为主机名或者服务名  
                    s = getnameinfo( & in_addr, in_len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV); //flag参数:以数字名返回  
                    //主机地址和服务地址  
                    if (s == 0) {
                        printf("Accepted connection on descriptor %d ""(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }

                    /* Make the incoming socket non-blocking and add it to the 
                     list of fds to monitor. */
                    s = make_socket_non_blocking(infd);
                    if (s == -1) abort();
                    
                    s = add_read_event(efd, infd);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }

                    generate_fd_event(infd);
                }
                continue;
            } else if (events[i].events & EPOLLIN) {
                Event* fdEvent = eventpool[events[i].data.fd];
                fdEvent->fd = &(events[i].data.fd);
                if(fdEvent->event_state == EventState_ACTIVE)
                {
                    //todo:should add lock, because work thread may change the value
                    fdEvent->event_type = fdEvent->event_type | EventType_CAN_READ;                    
                }
                else
                {
                    fdEvent->event_type = fdEvent->event_type | EventType_CAN_READ; 
                    fdEvent->event_state = EventState_ACTIVE;
                }
                int workerid = load_balance();
                set_event_to_worker(fdEvent, workerid);
            } 

        }
    }

    free(events);

    close(sfd);

    return EXIT_SUCCESS;
}
