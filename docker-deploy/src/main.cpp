#include "proxy.hpp"
#define CACHE_SIZE 1000
#define PORT "12345"
#define PATH "/var/log/erss/proxy.log"
// TODO: check the format of log file with requirements
// TODO: cache may exist relative path or absolute path
// TODO: test
// TODO: test dockerfile
//#define PATH "proxy.log"
int main(){
    //1. Open Log File
    openFile(PATH);

    //2. create cache and thread id
    Cache cache(CACHE_SIZE);
    int id = 0;
    while(true){
        try{
            //2.1 create a server
            Server s = Server(PORT);            
            // cout<<"Waiting for user to open web..."<<endl;
            //2.2 web connected
            string web_host;
            int web_fd = s.serverAccept(&web_host);
            // cout<<"Connected: "<<"Host_fd="<<s.socketfd<<" web_host="<<web_host<<endl;
            id += 1;
            //2.3 create a thread
            pthread_t thread;
            WebInfo * t = new WebInfo(id,web_fd,web_host,&cache);
            pthread_create(&thread,NULL,processRequest,t);
            // cout<<"Thread created successfully"<<endl;
        }
        catch (std::exception e) {
            addLog(id,e.what());
            cerr<<id<<": "<<e.what()<<endl;
        }
    }
    //3. test part

    //4. exit program successfully
    return 0;
}
