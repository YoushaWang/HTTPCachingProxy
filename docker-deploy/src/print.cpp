#include "print.hpp"
//TODO: revise to connect to var/log/erss

std::mutex lock;
std::ofstream logFile;

//104: Requesting "GET www.bbc.co.uk/ HTTP/1.1" from www.bbc.co.uk
// info: the first line of the request, host is the server name
std::string addReqRspLog(int id, std::string method, std::string host, std::string info){
    std::stringstream s;
    s<<id<<": ";
    if (method=="Req"){
        s<<"Requesting ";
    }else if (method=="Rsp"){
        s<<"Received ";
    }
    s<<info<<" from "<<host<<"\n";
    writeLog(s.str());
    return s.str();
}

std::string printTime(){
    time_t now = time(0);
    tm* utc_time = localtime(&now);
    char time_char[100];
    strftime(time_char,sizeof(time_char), "%a %b %d %T %Y", utc_time);
    // std::cout<<time_char<<std::endl;
    std::string time_str(time_char);
    return time_str;
}
std::string addLog(int id, std::string msg){
    std::stringstream s;
    s<<id<<": "<<msg<<"\n";
    writeLog(s.str());
    return s.str();
}
void writeLog(std::string text){
    // write to file
    std::lock_guard<std::mutex> lck(lock);
    logFile<<text;
    logFile.flush();
}

void openFile(std::string path){    
    try{
        logFile.open(path, std::ostream::out);
    }catch (std::exception &e){
        std::cout << e.what() << std::endl;
        exit(EXIT_FAILURE);   
    }
}
// ID: "REQUEST" from IPFROM @ TIME
