#include "simona.h"

pthread_t threadspool[WORKERTHREADS];       //存放线程
pthread_cond_t condspool[WORKERTHREADS];    //存放每个工作线程的cond
Event* eventpool = NULL;
void** mempool = NULL;

int create_and_bind(const char * port) {
    struct addrinfo hints;
    struct addrinfo * result,
    *rp;
    int s,
    sfd;

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

    for (rp = result; rp != NULL; rp = rp - >ai_next) {
        sfd = socket(rp - >ai_family, rp - >ai_socktype, rp - >ai_protocol);
        if (sfd == -1) continue;

        s = bind(sfd, rp - >ai_addr, rp - >ai_addrlen);
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

int add_read_event(int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; //读入,边缘触发方式  
    s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    return s;
}

int add_write_event(int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events =  EPOLLOUT | EPOLLET; //读入,边缘触发方式  
    s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    return s;
}

int init_event(Event** eventpool, void*** mempool){
    *eventpool = (Event* ) calloc(MAXCONNECTION, sizeof Event);
    if(*eventpool == NULL)
    {
        return -1;
    }
    memset(*eventpool, 0, MAXCONNECTION * sizeof(Event));

    *mempool = (void* *) calloc(MAXCONNECTION, sizeof void*);
    if(*mempool == NULL)
    {
        return -1;
    }
    for(int i = 0; i < MAXCONNECTION; i++)
    {
        *mempool[i] = (void*) calloc(MEMPOOLSIZE, 1);
        if(*mempool[i] == NULL)
        {
            return -1;
        }
        else
        {
            memset(*mempool[i], 0, MEMPOOLSIZE);
        }
    }
    return 0;
}

int spawn_multi_workthread(){
    for(int i = 0; i < WORKERTHREADS; i++)
    {
        WorkInitData data;
        memset(&data, 0, sizeof WorkInitData);
        data.cond = &condspoos[i];
        data.index = i;
        int err = pthread_create(threadspool[i], NULL, &handle_task, &data);
    }
}

int handle_task(void* data){
    pthread_cond_t* wakeUpCond = (WorkInitData*)data->cond;
    int index = (WorkInitData*)data->index;

    pthread_cond_init(wakeUpCond, NULL);
    for ( ; ; )
    {
        pthread_cond_wait(wakeUpCond, NULL);
        // Event eve = get_event();
    }
}

int main(int argc, char * argv[]) {
    int sfd, s;
    int efd;
    struct epoll_event event;
    struct epoll_event * events;
    init_event(&eventpool, &mempool);
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

    s = add_read_event(sfd);
    if (s == -1) {
        perror("epoll_ctl");
        abort();
    }

    /* Buffer where events are returned */
    events = (epoll_event * ) calloc(MAXEVENTS, sizeof event);

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

                    in_len = sizeof in_addr;
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
                    s = getnameinfo( & in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV); //flag参数:以数字名返回  
                    //主机地址和服务地址  
                    if (s == 0) {
                        printf("Accepted connection on descriptor %d ""(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }

                    /* Make the incoming socket non-blocking and add it to the 
                     list of fds to monitor. */
                    s = make_socket_non_blocking(infd);
                    if (s == -1) abort();

                    s = add_read_event(infd);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;
            } else if (events[i].events & EPOLLIN) {
                /* We have data on the fd waiting to be read. Read and 
                 display it. We must read whatever data is available 
                 completely, as we are running in edge-triggered mode 
                 and won't get a notification again for the same 
                 data. */
                int done = 0;

                while (1) {
                    ssize_t count;
                    char buf[512];

                    count = read(events[i].data.fd, buf, sizeof(buf));
                    if (count == -1) {
                        /* If errno == EAGAIN, that means we have read all 
                         data. So go back to the main loop. */
                        if (errno != EAGAIN) {
                            perror("read");
                            done = 1;
                        }
                        break;
                    } else if (count == 0) {
                        /* End of file. The remote has closed the 
                         connection. */
                        done = 1;
                        break;
                    }

                    /* Write the buffer to standard output */
                    s = write(events[i].data.fd, buf, count);
                    if (s == -1) {
                        perror("write");
                        abort();
                    }
                }

                if (done) {
                    printf("Closed connection on descriptor %d\n", events[i].data.fd);

                    /* Closing the descriptor will make epoll remove it 
                     from the set of descriptors which are monitored. */
                    close(events[i].data.fd);
                }
            } 
            // else if (events[i].events & EPOLLOUT) {
            //     printf("fd %d triger write event\n", events[i].data.fd);
            //     int done = 0;
            //     ssize_t count;
            //     char buf[8192000];
            //     for (int i = 0; i < 8192000; i++) {
            //         buf[i] = 'a';
            //     }
            //     s = write(events[i].data.fd, buf, sizeof buf);
            //     if (s < 0) {
            //         perror("write error");
            //         //abort (); 
            //         int state = epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &event);
            //         close(events[i].data.fd);
            //     }
            // }
        }
    }

    free(events);

    close(sfd);

    return EXIT_SUCCESS;
}