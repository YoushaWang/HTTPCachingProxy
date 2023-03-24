#include "request.hpp"
// example:
// !!!! SUPPOSE: every endline is \r\n - findRN
// std::string req = "GET http://www.example.com:8080/index.html HTTP/1.1\r\n 
//                    Host: www.example.com\r\n";
//private function
using namespace std;
Request::Request(vector<char> REQ){
    string req;
    req.insert(req.begin(), REQ.begin(), REQ.end());
    size_t findPlace1 = req.find("HTTP/1.1");
    rawRequest = req;
    request = req.substr(0,findPlace1+8);
    parseLine();
}
void Request::parseLine(){
    size_t findProtocal = rawRequest.find("HTTP/1.1");
    request = rawRequest.substr(0,findProtocal+8);
    size_t findSpace1 = rawRequest.find(' ');
    method = rawRequest.substr(0,findSpace1);
    size_t findSpace2 = rawRequest.find(' ',findSpace1+1);
    uri = rawRequest.substr(findSpace1+1,findSpace2-findSpace1-1);
    size_t findHost = rawRequest.find("Host: ");

    size_t findRN = rawRequest.find("\r\n",findHost+1);
    string hostport = rawRequest.substr(findHost+6,findRN-findHost-6);
    
    size_t findComma = hostport.find(":");
    host = hostport.substr(0,findComma);    
    
    size_t findSlash2 = uri.find("//");
    if (findSlash2!=std::string::npos){
        findComma = uri.find(":",findSlash2+1);
        if (findComma!=std::string::npos){
            size_t findSlash = uri.find("/",findComma+1);
            port = uri.substr(findComma+1,findSlash-findComma-1);
        }
    }
    if (port==""){
        if (method=="CONNECT"){
            port = "443";
        }else{
            port = "80";
        }
    }
    //if cannot find host
    // size_t findHostinUri = uri.find(host);
    // if (findHostinUri==std::string::npos){
        // string uri = "http://" + host + uri;
        // cout<<"cannot find host in uri "<<uri<<endl;
    // }
    // cout <<"find host in uri "<<endl;
}
// public function
string Request::getRequest(){
    return request;
}
string Request::getRawRequest(){
    return rawRequest;
}
string Request::getMethod(){
    return method;
}
string Request::getURI(){
    return uri;
}
string Request::getHost(){
    return host;
}
string Request::getPort(){
    return port;
}
bool Request::isValid(){
    if (method =="GET"){
        return true;
    }
    if (method =="POST"){
        return true;
    }
    if (method =="CONNECT"){
        return true;
    }
    return false;
}
void Request::printInfo(){
//  Test:
    cout<<"request: "<<request<<endl;
    cout<<"method:  "<<method<<endl;
    cout<<"uri:     "<<uri<<endl;
    cout<<"host:    "<<host<<endl;
    cout<<"port:    "<<port<<endl;
}