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
    thread(/* args */);
    ~thread();
public:
    int ThreadCreate(int client_sock);
private:
    void cannot_execute();
    void bad_request();
    void cat(FILE* resource);
    void headers(std::string& filename);
    void server_file(std::string& filename);
    void execute_cgi(std::string& path,std::string& method,int found);
    void not_found(int client);
    void unimplemented(int client);
    int get_line(int sock,std::string buf);
    static void* accept_request(void* arg);
};

thread::thread(/* args */)
{
    this->client_sock = -1;
}

thread::~thread()
{
    if(this->client_sock > 0)
        close(this->client_sock);
}

int thread::get_line(int sock,std::string buf)
{
    int i = 0;
    char c = '\0';
    ssize_t n;
    while (c != '\n')
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

void thread::headers(std::string& filename)
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

void thread::server_file(std::string& filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    std::string buf;
    buf[0] = 'A'; buf[1] = '\0';
    while((numchars > 0) && buf != "\n")
        numchars = this->get_line(this->client_sock,buf);
    resource = fopen(filename.c_str(),"r");
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

void thread::execute_cgi(std::string& path,std::string& method,int found)
{
    std::string buf;
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if(method == "GET")
        while((numchars > 0) && buf != "\n")
            numchars = this->get_line(this->client_sock,buf);
    else
    {
        numchars = this->get_line(this->client_sock,buf);
        while ((numchars > 0) && buf != "\n")
        {
            buf[15] = '\0';
            if(buf == "Contenct-Length:")
                content_length = atoi(&buf[16]);
            numchars = this->get_line(this->client_sock,buf);
        }   
        if(content_length == -1)
        {
            this->bad_request();
            return;
        }
    }
    buf =  "HTTP/1.0 200 OK\r\n";
    send(this->client_sock,buf.c_str(),buf.size(),0);
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
        std::string meth_env;
        std::string query_env;
        std::string length_env;

        dup2(cgi_output[1],1);
        dup2(cgi_input[0],0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        meth_env = "REQUEST_METHOD=" + method;
        putenv((char*)meth_env.c_str());
        if(method == "GET")
        {
            query_env = "QUERY_STRING=" + method.substr(found);
            putenv((char*)query_env.c_str());
        }
        else
        {
            length_env = "CONTENT_LENGTH=" + content_length;
            putenv((char*)length_env.c_str());
        }
        execl(path.c_str(),path.c_str(),NULL);
        exit(0);
    }
    else
    {
        close(cgi_output[1]);
        close(cgi_input[0]);
        if(method == "POST")
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
    std::string buf;
    int numchars;
    std::string method;
    std::string url;
    std::string path;
    size_t i,j;
    struct stat st;
    int cgi = 0;

    numchars = thread_obj->get_line(thread_obj->client_sock,buf);
    i = 0;j = 0;
    while (!isspace(buf[j]))
    {
        method[i] = buf[j];
        i++;j++;
    }
    if(method != "GET" && method != "POST")
    {
        thread_obj->unimplemented(thread_obj->client_sock);
        return NULL;
    }
    if(method == "POST")
        cgi = 1;
    i = 0;
    while (isspace(buf[j]))
        j++;
    while(!isspace(buf[j]))
    {
        url[i] = buf[j];
        i++;j++;
    }
    std::size_t found = 0;
    if(method == "GET" && (found = method.find("?") != std::string::npos))
        cgi = 1;
    path = "htdocs" + url;
    if(path[path.size()-1] == '/')
        path += "index.html";
    if(stat(path.c_str(),&st) == -1)
    {
        while((numchars > 0) && buf != "\n")
            numchars = thread_obj->get_line(thread_obj->client_sock,buf);
        thread_obj->not_found(thread_obj->client_sock);
    }
    else
    {
        if((st.st_mode & S_IFMT) == S_IFDIR)
            path += "index.html";
        if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            cgi = 1;
        if(!cgi)
            thread_obj->server_file(path);
        else
            thread_obj->execute_cgi(path,method,found+1);
    }
    close(thread_obj->client_sock);
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
    if(pthread_create(&newthread,NULL,accept_request,(void*)this) != 0)
    {
        perror("pthread_create");
        return -2;
    }
}

