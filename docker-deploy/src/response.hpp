#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__

// #include "request.hpp"
// #include "log.hpp"
#include <iostream>
#include <string>
#include <ctime>
#include <vector>
#include "print.hpp"
using namespace std;
class Response{
    private:
        string response;
        string code;
        string date;
        string CacheControl;
        string LastModify;
        string Expires;
        string ETag;
        int maxStale;
        int ContentLength;
        bool must_revalidate;
    public:
        void printInfo();
        Response():response(){}
        Response(string res):response(res){
            parseLine();
            }
        Response(vector<char> REP);
        void setRes(string msg){
            response = msg;
        }
        bool ismustRevalidate(){
            return must_revalidate;
        }
        int getContentLength();
        string getCode(){
            return code;
        }
        int getMaxStale(){
            return maxStale;
        }
        string getDate(){
            return date;
        }
        string getCacheControl(){
            return CacheControl;
        }
        string getLastModify(){
            return LastModify;
        }
        string getExpires(){
            return Expires;
        }
        string getETag(){
            return ETag;
        }
        void parseLine();
        bool isPrivate();
        bool isNoStore();
        bool isNoCache();
        string MaxAge();
        time_t UTCtime(string time);
        double Age();
        bool isFresh(int thread_id);
        bool ifRevalid(int thread_id);
        string getResponse();
        bool isChunk();
};
#endif // RESPONSE_HPP
