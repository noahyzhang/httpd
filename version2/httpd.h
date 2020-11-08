#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include "thread.h"

class httpd
{
private:
    int server_sock;
    u_short port;
    int client_sock;
    thread thread_obj;
public:
    httpd(int port);
    ~httpd();
public:
    int CreateService();
    int DealConnect();
};

httpd::httpd(int port = 0)
{
    this->port = port;
    this->server_sock = -1;
    this->client_sock = -1;
}

httpd::~httpd()
{
    if (this->server_sock > 0)
        close(this->server_sock);
    if(this->client_sock > 0)
        close(this->client_sock);
}

int httpd::CreateService()
{
    int httpd = 0;
    struct sockaddr_in name;
    httpd = socket(PF_INET,SOCK_STREAM,0);
    if(httpd == -1)
    {
        perror("socket");
        return -1;
    }
    memset(&name,0,sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(this->port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(httpd,(struct sockaddr*)&name,sizeof(name)) < 0)
    {
        perror("bind");
        return -2;
    }
    if(this->port == 0)
    {
        socklen_t namelen = sizeof(name);
        if(getsockname(httpd,(struct sockaddr*)&name,&namelen) == -1)
        {
            perror("getsockname");
            return -3;
        }
        this->port = ntohs(name.sin_port);
    }
    if(listen(httpd,5) < 0)
    {
        perror("listen");
        return -4;
    }
    this->server_sock = httpd;
    return httpd;
}

int httpd::DealConnect()
{
    if(this->server_sock <= 0)
    {
        std::cerr << "server_sock less than or equal to 0" << std::endl;
        return -1;
    }

    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof(client_name);
    while (true)
    {
        this->client_sock = accept(server_sock,(struct sockaddr*)&client_name,&client_name_len);
        if(this->client_sock == -1)
        {
            perror("accept");
            return -2;
        }
        if(thread_obj.ThreadCreate(this->client_sock) <= 0)
        {
            std::cerr << "ThreadCreate failed" << std::endl;
            return -3;
        }
    }
}