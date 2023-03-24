#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__
#include <iostream>
#include <string>
#include <vector>

using namespace std;
// 1. CONNECT method:
//    CONNECT server.example.com:80 HTTP/1.1 //request
//    Host: server.example.com:80            //host
//    Proxy-Authorization: basic aGV...(optional)
// 2. POST method:
//    POST /submit HTTP/1.1
//    Host: www.example.com
//    exception: 206 (Partial Content), 304 (Not Modified), and 416 (Range Not Satisfiable)
//    if success: 201(created)
// 3. GET method
//    GET /index.html HTTP/1.1
//    Host: www.example.com
//    if success,200 (ok);201(created);204 (No Content)

// the second line host of CONNECT is called as sender
class Request{
private:
    string rawRequest;
    string request;
    string method;
    string uri;
    string host;
    string port;
    void parseLine();
public:
    Request(vector<char> REQ);
    Request(string _r): rawRequest(_r){
            parseLine();
        }
    string getRequest();
    string getRawRequest();
    string getMethod();
    string getURI();
    string getHost();
    string getPort();
    bool isValid();
    void printInfo();
};
#endif
