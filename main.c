#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include"TcpServer.h"
int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("./a.out port path");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    char* path = argv[2];
    //切换工作目录
    chdir(path);
    // 启动服务器
    TcpServer* tcpServer = tcpServerInit(port, 4);
    tcpServerRun(tcpServer);
    return 0;
}