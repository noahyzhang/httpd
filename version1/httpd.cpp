#include "httpd.h"
#include "thread.h"

int main()
{
    httpd http;
    http.CreateService();
    http.DealConnect();
}