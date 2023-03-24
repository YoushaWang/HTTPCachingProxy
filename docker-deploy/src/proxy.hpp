#ifndef __PROXY_HPP__
#define __PROXY_HPP__

#include "socket.hpp"
#include "request.hpp"
#include "response.hpp"
#include "cache.hpp"
#include "print.hpp"


#define PORT "12345"
using namespace std;
class WebInfo{
public:
    int id;
    int fd;
    std::string host;
    Cache *cache;
    WebInfo(int _id, int _fd, std::string _h, Cache *_c):
                id(_id),fd(_fd),host(_h),cache(_c){}
};
//get response
string processGet(Request req,Cache &cache,int id,int web_fd, int client_fd);
void newprocessGet(Request req,Cache &cache,int id,int web_fd, int client_fd, int len);

//stay connect
void processConnect(Request req, int id,int web_fd, int client_fd);
//stay connect
void processPost(Request req, int id,int web_fd, int client_fd);
//process request
void * processRequest(void * INFO);
//check 502
int getLength(char * server_msg, int mes_len);
void printnote(Response & parse_res, int id);
string sendContentLen(int send_fd,
                                  char * server_msg,
                                  int mes_len,
                                  int content_len);
bool revalidation(Response & rep, string req, int server_fd, int id);
string processGetnew(Request req,Cache &cache,int id,int web_fd, int client_fd);
#endif
