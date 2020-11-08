#include "httpd.h"


int main()
{
    httpd http;
    int res = http.CreateService();
    if(res < 0)
    {
        std::cout << "http create service failed, error code:" << res << std::endl;
        return 0;
    }
    res = http.DealConnect();
    if(res < 0)
    {
        std::cout << "Deal connect failed,error code:" << res << std::endl;
        return 0;
    }
    return 0;
}