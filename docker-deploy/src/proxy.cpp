#include "proxy.hpp"
using namespace std;
//TODO: addReqRspLog -- the host should be real server name
//TODO: Tunnel close shoud be written after all 200-OK
//TODO: 502 and 400 error code
//TODO: check RAII(OO design, RAII, exceptions, and other C++-concepts)
//TODO: max-stale
//TODO: add requirements to gitlab
void processConnect(Request req, int id, int web_fd, int client_fd){
    //1 send 200 ok
    send(web_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    //2 write to log
    string logMsg = "Responding \"HTTP/1.1 200 OK\"";
    addLog(id,logMsg);
    // cout<<"send 200 ok in connect"<<endl;
    //3 get raw request
    string rawReq = req.getRawRequest();
    const char * rawReq_cstr=rawReq.c_str();
    //4 connect
    vector<int> fd_v{client_fd,web_fd};
    while(true){
            fd_set read_fds;
            int max_fd = * max_element(fd_v.begin(),fd_v.end());
            FD_ZERO(&read_fds);
            for(int i=0;i<fd_v.size();i++){
                FD_SET(fd_v[i],&read_fds);
            }
            select(max_fd+1,&read_fds,NULL,NULL,NULL);
            for(int i = 0; i < fd_v.size(); i++){
                if(FD_ISSET(fd_v[i],&read_fds)){
                    vector<char> buffer_rsp(65535);
                    ssize_t recvByte = recv(fd_v[i], &buffer_rsp.data()[0],65535, 0);
                    string buffer_rsp_str;
                    buffer_rsp_str.insert(buffer_rsp_str.begin(), buffer_rsp.begin(), buffer_rsp.end());
                    if (recvByte>0){
                        if (send(fd_v[1 - i],buffer_rsp_str.c_str(),recvByte, 0)<=0){  
                            return ;
                        }
                    }else if (recvByte==0){
                        return ;
                    }else{
                        return;
                    }
                }
            }
        }
}

string processGet(Request req,Cache &cache,int id,int web_fd, int client_fd){
    //1.get request info
    cout<<id<<": try get response "<<endl;
    string host = req.getHost();
    string uri = req.getURI();
    string rawReq = req.getRawRequest();
    const char * rawReq_cstr=rawReq.c_str();
    //2. get response
    string rsp_cache = cache.get(uri);
    if (rsp_cache != ""){
        //2.1 find 
        cout<<"rsp_cache is not empty"<<endl;
        Response rsp(rsp_cache);
        if (rsp.ifRevalid(id)){
            //2.1.a need revalid
            // cout<<"need revalid"<<endl;
            rsp_cache = cache.reValid(req,rsp,client_fd,id);
        }else{
            //2.2.b in cache and valid
            addLog(id, "in cache, valid");
            cout<<"id is: "<<id<<endl;
            // cache.printCache();
        }
        vector<char> response_vec(rsp_cache.begin(),rsp_cache.end());
        cout<<"--------RSP_CACHE----------"<<endl;
        cout<<rsp_cache<<endl;
        cout<<"-------------------------"<<endl;
        // my_send(web_fd,response_vec);
        send(web_fd,rsp_cache.data(),rsp_cache.size()+1,0);
        //

    }else{
        //2.2 not find 
        addLog(id, "not in cache");
        cout<<"--------RAW REQ----------"<<endl;
        cout<<rawReq<<endl;
        cout<<"--------END RAW REQ-----------------"<<endl;
        vector<char> rawReq_vec(rawReq.begin(),rawReq.end());
        my_send(client_fd,rawReq_vec);
        // send(client_fd,rawReq.data(),rawReq.size()+1,0); //*
        addReqRspLog(id,"Req",host,req.getRequest());
        // 2.2.2 receive response
        vector<char> buffer_rsp(65535);
        ssize_t recvByte = my_recv(client_fd,buffer_rsp);
        // ssize_t recvByte = recv(client_fd, &buffer_rsp.data()[0],65535, MSG_WAITALL);
        if (recvByte<0){
            //error check
            string msgLog = "WARNING: when get req from real server request receive failed in "+host;
            addLog(id, msgLog);
            return NULL;
        }
        string buffer_rsp_str;
        buffer_rsp_str.insert(buffer_rsp_str.begin(), buffer_rsp.begin(), buffer_rsp.end());
        cout<<"=========test buffer_rsp_str========="<<endl;
        cout<<buffer_rsp_str<<endl;
        cout<<"=========end test buffer_rsp_str========="<<endl;
        size_t findrnrn = buffer_rsp_str.find("/r/n/r/n");
        // if (findrnrn!=string::npos){

        // }
        // TODO: check if chunk
        // TODO: store in cache
        
        // cout<<"recvByte: "<<recvByte<<endl;
        // cout<<buffer_rsp_str<<endl;
        Response rsp(buffer_rsp_str);
        //502check
        size_t f_valid=buffer_rsp_str.find("HTTP/1.1");
        if (f_valid==string::npos){
            string Str502="HTTP/1.1 502 Bad Gateway\r\n\r\n";
            send(web_fd,&Str502,sizeof(Str502),0); 
        }
        // cout<<"-------------------------------"<<endl;
        // rsp.printInfo();
        // cout << "+++" << endl;
        if (rsp.getCode()=="304"){
            return "";
        }
        // cout<<"-------------------------------"<<endl;
        size_t f=rsp.getResponse().find("\n");
        string rsp_firstline = rsp.getResponse().substr(0,f-1);
        // cout <<"here is the first line"<<endl;
        // cout << rsp_firstline <<endl;
        addReqRspLog(id,"Rsp",host,rsp_firstline);
        // 2.2.3 send to user
        rsp_cache = buffer_rsp_str;
        // cout << "start cache store"<<endl;
        cache.storeRes(uri,rsp,id);
        // cout<<"end cache store" << endl;

        //3. write to log
        string msgLog = "Responding "+rsp_firstline;
        addLog(id,msgLog);
        //4. send it to user
        send(web_fd,rsp_cache.data(),rsp_cache.size()+1,0);
        // cout<<id<<": already send response"<<endl;
    }
    return rsp_cache;
}

void processPost(Request request, int thread_id, int browser_fd, int server_fd){
    //get request's raw content
    string rawRequest = request.getRawRequest();
    size_t pos = rawRequest.find("Content-Length: ");
    vector<char> rawReq_vec(rawRequest.begin(), rawRequest.end());
    int content_len;
    if(pos == std::string::npos){
        content_len = -1;
    }else{
        size_t header = rawRequest.find("\r\n\r\n");
        int rest_len = rawRequest.size() - int(header) - 8;
        size_t end = rawRequest.find("\r\n", pos);
        content_len = stoi(rawRequest.substr(pos + 16, end - pos - 16));
        content_len = content_len - rest_len - 4;
    }
    if(content_len != -1){
        my_send(server_fd,rawReq_vec);
        // send(server_fd,rawRequest.data(),rawRequest.size()+1,0);
        vector<char> response(65536,0);
        // ssize_t recvByte = recv(server_fd, &response.data()[0],65535, 0);
        ssize_t recvByte = my_recv(server_fd, response);
        response[recvByte]='\0';
        string buffer_rsp_str;
        buffer_rsp_str.insert(buffer_rsp_str.begin(), response.begin(), response.end());
        // cout<<"recvByte"<<recvByte<<endl;
        if(recvByte != 0){
            Response res(response);
            size_t f=res.getResponse().find("\n");
            string responsefirstline = res.getResponse().substr(0,f-1);
            string msg = "Received \""+responsefirstline +"\" from "+request.getHost();
            // cout<<"msg:"<<msg<<endl;
            addLog(thread_id, msg);
            my_send(browser_fd,response);
            // ssize_t recvByte = my_recv(server_fd, response);/* */
            // send(browser_fd,buffer_rsp_str.c_str(),recvByte, 0);
            // cout<<"POST already send"<<endl;
        }else{
            cout<<"fail to post"<<endl;
        }
    }
}

// main function to processRequest get from web
void * processRequest(void * INFO){
    // cout<<"Processing..."<<endl;
    WebInfo * web = (WebInfo *)INFO;
    Cache *cache = web->cache;
    //1 receive information, store in the req and return len
    vector<char> buffer(65535,0); /* */
    //int recvByte = my_recv(web->fd,buffer);
    // char buffer[65536]={0};
    int recvByte = recv(web->fd, &buffer.data()[0], 65535, 0);
    // buffer[recvByte] = 0;
    //1.1 if receive fail
    if(recvByte <= 0){
        string msg = "WARNING: request receive failed from "+web->host+" @ "+printTime();
        addLog(web->id, msg);
        return NULL;
    }
    //2 if receive success, make vector req to string req
    string REQUEST;
    REQUEST.insert(REQUEST.begin(), buffer.begin(), buffer.end());
    // string REQUEST = string(buffer, recvByte);
    //Request instance 
    Request req(REQUEST);
    //judge if req's method contains GET, POST, CONNECT
    //2.1 if req is not valid
    if(!req.isValid()){
        // cout<<"-----------------invalid-------------------"<<endl;
        // cout<<"method is: "<<req.getMethod()<<endl;
        string msg = string("Responding \"HTTP/1.1 400 Bad Request\"");
        send(web->fd, &msg, sizeof(msg)+1, 0);
        addLog(web->id, msg);
        return NULL;/**/
    }
    //2.2 if req is valid
    else{
        // cout<<"-----------------valid-------------------"<<endl;
        // cout<<"method is: "<<req.getMethod()<<endl;
        string msg = req.getRequest()+" from "+web->host+" @ "+printTime();
        addLog(web->id,msg);
    }

    //3.1 get request info
    string host = req.getHost();
    string port = req.getPort();
    
    //3.2 set up client
    // cout<<"--------------------"<<endl;
    // cout << "(" << host.c_str() << "," << port.c_str() << ")" << endl;
    // cout<<req.getRawRequest()<<endl;
    Client client = Client(host.c_str(),port.c_str());
    int client_fd = client.socketfd;
    if(client_fd == -1){
        cerr<<"Error: build client failed"<<endl;
        return NULL;
    }
    //3.3. send the request
    // for test
    // req.printInfo();
    //4.1 connect
    if (req.getMethod() == "CONNECT"){
        processConnect(req, web->id,web->fd,client_fd);
        addLog(web->id,"Tunnel closed");
    }
    //4.2 get
    else if(req.getMethod() == "GET"){
        //  processGet(req,*cache,web->id,web->fd,client_fd);
        //newprocessGet(req,*cache,web->id,web->fd,client_fd, recvByte);
        processGetnew(req,*cache,web->id,web->fd,client_fd);
    } 
    //4.3 post
    else if(req.getMethod() == "POST"){
        cout<<"--------RAW REQ----------"<<endl;
        cout<<req.getRawRequest()<<endl;
        cout<<"--------END RAW REQ-----------------"<<endl;
        processPost(req, web->id,web->fd,client_fd);
    } 
    return NULL;
}
string processGetnew(Request req,Cache &cache,int id,int web_fd, int client_fd){
    //1.get request info
    cout<<id<<": try get response "<<endl;
    string host = req.getHost();
    string uri = req.getURI();
    string rawReq = req.getRawRequest();
    const char * rawReq_cstr=rawReq.c_str();
    //2. get response
    string rsp_cache = cache.get(uri);
    if (rsp_cache != ""){
        //2.1 find 
        cout<<"rsp_cache is not empty"<<endl;
        Response rsp(rsp_cache);
        //2.2.b in cache and valid
        if (rsp.ifRevalid(id)){
            //2.1.a need revalid
            cout<<"need revalid"<<endl;
            rsp_cache = cache.reValid(req,rsp,client_fd,id);
        }else{
            //2.2.b in cache and valid
            addLog(id, "in cache, valid");
            cout<<"id is: "<<id<<endl;
            // cache.printCache();
        }


        // cache.printCache();
        vector<char> response_vec(rsp_cache.begin(),rsp_cache.end());
        // cout<<"--------RSP_CACHE----------"<<endl;
        // cout<<rsp_cache<<endl;
        // cout<<"-------------------------"<<endl;
        my_send(web_fd,response_vec);
        cout<<"in cache, send success"<<endl;
        //send(web_fd,rsp_cache.data(),rsp_cache.size()+1,0);
        //

    }else{
        //2.2 not find 
        addLog(id, "not in cache");
        vector<char> rawReq_vec(rawReq.begin(),rawReq.end());
        my_send(client_fd,rawReq_vec);
        // send(client_fd,rawReq.data(),rawReq.size()+1,0); //*
        addReqRspLog(id,"Req",host,req.getRequest());
        // 2.2.2 receive response
        vector<char> buffer_rsp(65535);

        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!recv wrong!!!!!!!!!!!!!!!!!!!!!!!!!
        
        ssize_t recvByte = my_recv(client_fd,buffer_rsp);
        // ssize_t recvByte = recv(client_fd, &buffer_rsp.data()[0],65535, MSG_WAITALL);
        if (recvByte<0){
            //error check
            string msgLog = "WARNING: when get req from real server request receive failed in "+host;
            addLog(id, msgLog);
            return NULL;
        }
        string buffer_rsp_str;
        buffer_rsp_str.insert(buffer_rsp_str.begin(), buffer_rsp.begin(), buffer_rsp.end());

        Response rsp(buffer_rsp_str);
        //502check
        size_t f_valid=buffer_rsp_str.find("HTTP/1.1");
        if (f_valid==string::npos){
            string Str502="HTTP/1.1 502 Bad Gateway\r\n\r\n";
            send(web_fd,&Str502,sizeof(Str502),0); 
        }

        if (rsp.getCode()=="304"){
            return "";
        }

        size_t f=rsp.getResponse().find("\n");
        string rsp_firstline = rsp.getResponse().substr(0,f-1);

        addReqRspLog(id,"Rsp",host,rsp_firstline);
        // 2.2.3 send to user
        //buffer_rsp_str is the string hold total response
        if(!rsp.isChunk()){
            int contentlength = rsp.getContentLength();
            int totalLen = 0;
            int len = 0;
            if (contentlength >= 0){
                while(totalLen < contentlength){
                    char nextMsg[1024];
                    len = recv(client_fd, nextMsg, 1000, 0);
                    if(len<=0){
                        if(totalLen >= contentlength){
                            break;
                        }
                        else{
                            string msgLog = "WARNING: when get req from real server request receive failed in "+host;
                            addLog(id, msgLog);
                            return NULL;
                        }
                    }
                    string temp(nextMsg, len);
                    buffer_rsp_str += temp;
                    totalLen += len;
                    
                }
            }}
            //ischunked
            // else{
            //     string resp;
            //     resp = rsp.getResponse();
            //     int len = 0;
            //     while(resp.find("0\r\n\r\n") == string::npos){
            //         char nextMsg[1024];
            //         len = recv(client_fd, nextMsg, 1000, 0);
            //         if(len <= 0){
            //             break;
            //         }
            //         else{
            //             string msgLog = "WARNING: when get req from real server request receive failed in "+host;
            //             addLog(id, msgLog);
            //             return NULL;
            //         }
            //         string temp(nextMsg, len);
            //         resp += temp;
            //     }
            //     buffer_rsp_str = resp;
            // }
        // cout<<"=========test buffer_rsp_str========="<<endl;
        // cout<<buffer_rsp_str<<endl;
        // cout<<"=========end test buffer_rsp_str========="<<endl;
        rsp_cache = buffer_rsp_str;
        Response newrsp(buffer_rsp_str);
        //cout << "start cache store"<<endl;
        cache.storeRes(uri,newrsp,id);
        // cout<<newrsp.getResponse()<<endl;
        //cout<<"end cache store" << endl;
        vector<char>mess(buffer_rsp_str.begin(), buffer_rsp_str.end());
        //3. write to log
        string msgLog = "Responding "+rsp_firstline;
        addLog(id,msgLog);
        //4. send it to user
        my_send(web_fd,mess);
        cout<<id<<": already send response"<<endl;
    }
    return rsp_cache;
}

int getLength(char * server_msg, int mes_len) {
  std::string msg(server_msg, mes_len);
  size_t pos;
  if ((pos = msg.find("Content-Length: ")) != std::string::npos) {
    size_t head_end = msg.find("\r\n\r\n");

    int part_body_len = mes_len - static_cast<int>(head_end) - 8;
    size_t end = msg.find("\r\n", pos);
    std::string content_len = msg.substr(pos + 16, end - pos - 16);
    int num = 0;
    for (size_t i = 0; i < content_len.length(); i++) {
      num = num * 10 + (content_len[i] - '0');
    }
    return num - part_body_len - 4;
  }
  return -1;
}

void printnote(Response & parse_res, int id) {
  if (parse_res.MaxAge() != "") {
    addLog(id, "NOTE Cache-Control: max-age=" + parse_res.MaxAge());
  }
  if (parse_res.getExpires() != "") {
    addLog(id, "NOTE Expires: " + parse_res.getExpires());
  }
  if (parse_res.isNoCache() == true) {
    addLog(id, "NOTE Cache-Control: no-cache");
  }
  if (parse_res.getETag() != "") {
    addLog(id, "NOTE ETag");
  }
  if (parse_res.getLastModify() != "") {
    addLog(id, "NOTE LastModify");
  }
}

string sendContentLen(int send_fd,
                                  char * server_msg,
                                  int mes_len,
                                  int content_len) {
  int total_len = 0;
  int len = 0;
  std::string msg(server_msg, mes_len);

  while (total_len < content_len) {
    char new_server_msg[65536] = {0};
    if ((len = recv(send_fd, new_server_msg, sizeof(new_server_msg), 0)) <= 0) {
      break;
    }
    std::string temp(new_server_msg, len);
    msg += temp;
    total_len += len;
  }
  return msg;
}

bool revalidation(Response & rep, string req, int server_fd, int id){
    if(rep.getETag() == "" && rep.getLastModify() == ""){
        return true;
    }
    string changed_input = req;
    if(rep.getETag()!=""){
        string add_etag = "If-None-Match: " + rep.getETag().append("\r\n");
        changed_input = changed_input.insert(changed_input.length() - 2, add_etag);
    }
    if(rep.getLastModify() != ""){
        string add_modified = "If-Modified-Since: "+rep.getLastModify().append("\r\n");
        changed_input = changed_input.insert(changed_input.length() - 2, add_modified);
    }
    string req_msg = changed_input;
    char req_msg_new[req_msg.size() + 1];
    int send_len;
    if((send_len = send(server_fd, req_msg_new,req_msg.size()+1,0))>0){
        cout<<"Verify: send success"<<endl;
    }
    char new_resp[65536] = {0};
    int new_len = recv(server_fd, &new_resp, sizeof(new_resp), 0);
    if(new_len <= 0){
        cout<<"Recv failed in checktime"<<endl;
    }
    string checknew(new_resp, new_len);
    if(checknew.find("HTTP/1.1 200 OK")!=string::npos){
        addLog(id,"in cache, requires validation");
        return false;
    }
    return true;
}