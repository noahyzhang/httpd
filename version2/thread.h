#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>

#include <iostream>
#include <string>

class thread
{
private:
    int client_sock;
public:
    thread();
    ~thread();
public:
    int ThreadCreate(int client_sock);
private:
    void cannot_execute();
    void bad_request();
    void cat(FILE* resource);
    void headers(const char* filename);
    void server_file(const char* filename);
    void execute_cgi(const char* path,const char* method,const char* query_string);
    void not_found(int client);
    void unimplemented(int client);
    int get_line(int sock,char* buf,int size);
    static void* accept_request(void* arg);
};

thread::thread()
{
    this->client_sock = -1;
}

thread::~thread()
{
    if(this->client_sock > 0)
        close(this->client_sock);
}

int thread::get_line(int sock,char* buf,int size)
{
    int i = 0;
    char c = '\0';
    ssize_t n;
    while ((i < size-1) && (c != '\n'))
    {
        n = recv(sock,&c,1,0);
        if(n > 0)
        {
            if(c == '\r')
            {
                n = recv(sock,&c,1,MSG_PEEK);
                if((n > 0) && (c == '\n'))
                    recv(sock,&c,1,0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';
    return i;
}

void thread::unimplemented(int client)
{
    std::string buf;
    buf = "HTTP/1.0 501 Method Not Implemented\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "Server: jdbhttpd/0.1.0\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "Content-Type: text/html\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "<HTML><HEAD><TITLE>Method Not Implemented\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "</TITLE></HEAD>\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "<BODY><P>HTTP request method not supported.\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "</BODY></HTML>\r\n";
    send(client,buf.c_str(),buf.size(),0);
}

void thread::not_found(int client)
{
    std::string buf;
    buf = "HTTP/1.0 404 NOT FOUND\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "Server: jdbhttpd/0.1.0\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "Content-Type: text/html\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "<HTML><TITLE>Not Found</TITLE>\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "<BODY><P>The server could not fulfill\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "your request because the resource specified\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "is unavailable or nonexistent.\r\n";
    send(client,buf.c_str(),buf.size(),0);
    buf = "</BODY></HTML>\r\n";
    send(client,buf.c_str(),buf.size(),0);
}

void thread::headers(const char* filename)
{
    std::string buf;
    buf = "HTTP/1.0 200 OK\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "Server: jdbhttpd/0.1.0\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "Content-Type: text/html\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
}

void thread::cat(FILE* resource)
{
    char buf[1024];
    fgets(buf,sizeof(buf),resource);
    while(!feof(resource))
    {
        send(this->client_sock,buf,strlen(buf),0);
        fgets(buf,sizeof(buf),resource);
    }
}

void thread::server_file(const char* filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];
    buf[0] = 'A'; buf[1] = '\0';
    while((numchars > 0) && strcmp("\n",buf))
        numchars = this->get_line(this->client_sock,buf,sizeof(buf));
    resource = fopen(filename,"r");
    if(resource == NULL)
        this->not_found(this->client_sock);
    else
    {
        headers(filename);
        cat(resource);
    }
    fclose(resource);
}

void thread::bad_request()
{
    std::string buf;
    buf = "HTTP/1.0 400 BAD REQUEST\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "Content-type: text/html\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "<P>Your browser sent a bad request, ";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "such as a POST without a Content-Length.\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
}

void thread::cannot_execute()
{
    std::string buf;
    buf = "HTTP/1.0 500 Internal Server Error\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "Content-type: text/html\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
    buf = "<P>Error prohibited CGI execution.\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
}

void thread::execute_cgi(const char* path,const char* method,const char* query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if(strcasecmp(method,"GET") == 0)
        while((numchars > 0) && strcmp("\n",buf))
            numchars = this->get_line(this->client_sock,buf,sizeof(buf));
    else
    {
        numchars = this->get_line(this->client_sock,buf,sizeof(buf));
        while ((numchars > 0) && strcmp("\n",buf))
        {
            buf[15] = '\0';
            if(strcasecmp(buf, "Contenct-Length:") == 0)
                content_length = atoi(&buf[16]);
            numchars = this->get_line(this->client_sock,buf,sizeof(buf));
        }   
        if(content_length == -1)
        {
            this->bad_request();
            return;
        }
    }
    sprintf(buf,"HTTP/1.0 200 OK\r\n");
    send(this->client_sock,buf,strlen(buf),0);
    if (pipe(cgi_output) < 0)
    {
        cannot_execute();
        return;
    }
    if (pipe(cgi_input) < 0) 
    {
        cannot_execute();
        return;
    }
    if ((pid = fork()) < 0)
    {
        cannot_execute();
        return;
    }
    if(pid == 0)
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1],1);
        dup2(cgi_input[0],0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if(strcasecmp(method,"GET") == 0)
        {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else
        {
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path,path,NULL);
        exit(0);
    }
    else
    {
        close(cgi_output[1]);
        close(cgi_input[0]);
        if(strcasecmp(method,"POST") == 0)
        {
            for(i = 0;i < content_length;i++)
            {
                recv(this->client_sock,&c,1,0);
                write(cgi_input[1],&c,1);
            }
        }
        while(read(cgi_output[0],&c,1) > 0)
            send(this->client_sock,&c,1,0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid,&status,0);
    }   
}

void* thread::accept_request(void* arg)
{
    thread* thread_obj = (thread*)arg;
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i,j;
    struct stat st;
    int cgi = 0;

    char* query_string = NULL;

    numchars = thread_obj->get_line(thread_obj->client_sock,buf,sizeof(buf));
    i = 0;j = 0;
    while (!isspace(buf[j]) && (i < sizeof(method)-1))
    {
        method[i] = buf[j];
        i++;j++;
    }
    method[i] = '\0';
    if(strcasecmp(method,"GET") && strcasecmp(method,"POST"))
    {
        thread_obj->unimplemented(thread_obj->client_sock);
        return NULL;
    }
    if(strcasecmp(method,"POST") == 0)
        cgi = 1;
    i = 0;
    while (isspace(buf[j]) && j < sizeof(buf))
        j++;
    while(!isspace(buf[j]) && (i < sizeof(url)-1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];
        i++;j++;
    }
    url[i] = '\0';

    if(strcasecmp(method,"GET") == 0)
    {
        query_string = url;
        while((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if(*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }
    sprintf(path, "htdocs%s", url);
    if(path[strlen(path)-1] == '/')
        strcat(path,"index.html");
    if(stat(path,&st) == -1)
    {
        while((numchars > 0) && strcmp("\n",buf))
            numchars = thread_obj->get_line(thread_obj->client_sock,buf,sizeof(buf));
        thread_obj->not_found(thread_obj->client_sock);
    }
    else
    {
        if((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path,"/index.html");
        if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            cgi = 1;
        if(!cgi)
            thread_obj->server_file(path);
        else
            thread_obj->execute_cgi(path,method,query_string);
    }
    close(thread_obj->client_sock);
    return NULL;
}

int thread::ThreadCreate(int client_sock)
{
    if(client_sock <= 0)
    {
        std::cerr << "client_sock less than or equal to 0" << std::endl;
        return -1;
    }
    this->client_sock = client_sock;
    pthread_t newthread;
    int res = pthread_create(&newthread,NULL,accept_request,(void*)this);
    if(res != 0)
    {
        std::cerr << "pthread_create failed, errno:" << res << std::endl;
        return -2;
    }
    return 1;
}

