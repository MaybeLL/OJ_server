#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/time.h>

using namespace std;

//创建一个简单的echo服务器
//epoll 服务器

int main(){

    //mysql
    

    pid_t cur_pid;
    cur_pid = getpid();
    cout << "当前进程号为:" << cur_pid << endl;
    cout << "开始创建socket" << endl;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        cout << "创建socket失败" << endl;
        return -1;
    }

    // //查看socket的状态
    // int opt;
    // socklen_t optlen = sizeof(opt);
    // getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &opt, &optlen);
    // cout << "socket的状态为:" << opt << endl;

    //设置socket为非阻塞
    int flags = fcntl(sockfd, F_GETFL, 0);        //获取文件状态标志
    if(flags&O_NONBLOCK){                       //判断文件状态标志是否为非阻塞
        cout << "socket为非阻塞" << endl;
    }else{
        cout << "socket为阻塞" << endl;
    }                   

    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);   //设置文件状态标志
    cout << "设置socket为非阻塞" << endl;

    flags = fcntl(sockfd, F_GETFL, 0);        //获取文件状态标志
    if(flags&O_NONBLOCK){                       //判断文件状态标志是否为非阻塞
        cout << "当前socket为非阻塞" << endl;
    }else{
        cout << "当前socket为阻塞" << endl;
    }

    // //查看读写权限
    // flags = fcntl(sockfd, F_GETFL, 0);        
    // if(flags&O_RDONLY){
    //     cout << "socket为只读" << endl;
    // }
    // if(flags&O_WRONLY){
    //     cout << "socket为只写" << endl;
    // }
    // if(flags&O_RDWR){
    //     cout << "socket为读写" << endl;
    // }

    cout << "设定协议、ip和port:" << endl;
    struct sockaddr_in addr;
    cout << "设置地址协议为AF_INET" << endl;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = INADDR_ANY;
    cout << "绑定地址" << endl;
    
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))){
        cout << "绑定地址失败" << endl;
        return -1;
    }
    cout << "开始监听" << endl;
    if(listen(sockfd, 5)){
        cout << "监听失败" << endl;
        return -1;
    }
    struct epoll_event ev, events[20];
    //1.创建epoll句柄，参数是需要监听的最大描述符的数量
    int epfd = epoll_create(256);
    //2.设置与要处理的事件相关的文件描述符
    ev.data.fd = sockfd;
    ev.events = EPOLLIN | EPOLLET;
    //3.将所有需要监听的socket添加到epfd中(对应内核行为:socket插入红黑树中
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    cout << "开始循环" << endl;
    for(;;){
        //4.如果有事件发生，epoll_wait会将发生的事件放到events数组中；如果
        int nfds = epoll_wait(epfd, events, 20, 500);
        //处理发生的事件
        for(int i = 0; i < nfds; ++i){
            //判断事件是哪个socket发生的
            if(events[i].data.fd == sockfd){
                struct sockaddr_in client_addr;       //用来存客户端的sockaddr
                socklen_t length = sizeof(client_addr);
                //conn是新的socket描述符，用来和客户端通信
                int conn = accept(sockfd, (struct sockaddr*)&client_addr, &length);
                cout << "connect with client: " << inet_ntoa(client_addr.sin_addr) << endl;
                ev.data.fd = conn;
                ev.events = EPOLLIN | EPOLLET;
                //设置用于读操作的文件描述符
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev);
            }
            else if(events[i].events & EPOLLIN){
                char buffer[1024];
                int len = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                if(len > 0){
                    buffer[len] = '\0';
                    cout << "recv: " << buffer << endl;
                    send(events[i].data.fd, buffer, len, 0);
                }
            }
        }
    }



    return 0;
}


