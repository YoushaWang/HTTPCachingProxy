#include "cache.hpp"

std::mutex copy_mutex;
std::mutex cache_mutex;
void Cache::printCache(){
    // cout<<"-----------------------printCache-------------"<<endl;
    // for(auto it = cachemap.begin(); it != cachemap.end(); it++){  
    //     cout << "Key: " << it->first << ", Value: " <<endl<< it->second->value << endl;
    // }
    // cout<<"-----------------------ENDprintCache-------------"<<endl;
}
string Cache::get(string Key){
    std::lock_guard<std::mutex> lck(cache_mutex);
    
    if(cachemap.count(Key)){
        LRUop("get", Key, "");
        // cout<<"-----------------------getgetget-------------"<<endl;
        // cout<<"get::key:"<<Key<<"Value"<<cachemap[Key]->value<<endl;
        // cout<<"-----------------------ENDgetget-------------"<<endl;
        return cachemap[Key]->value;   
    }
    return "";
}

void Cache::add(string Key, string Value){
    std::lock_guard<std::mutex> lck(cache_mutex);
    if(cachemap.count(Key)){
        cachemap[Key]->value = Value;
        LRUop("get",Key,"");
        
    }else{
        if(capacity > size){
            // cout<<"addtoplace"<<endl;
            LRUop("AddToPlace", Key, Value);
            
        }
        else{
            // cout<<"addtono"<<endl;
            LRUop("AddToNoPlace", Key, Value);
            
        }
    }
}

void Cache::LRUop(string op, string Key, string Value){
    if (op=="get"){
        Node * node = cachemap[Key];
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }
    else if(op=="AddToPlace"){
        // cout<<"in lru"<<endl;
        Node * node = new Node(Key, Value);
        cachemap[Key] = node;
        size++;
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }
    else if(op=="AddToNoPlace"){
        Node * node = new Node(Key, Value);
        cachemap[Key] = node;
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
        Node * tailnode = tail->prev;
        tailnode->prev->next = tail;
        tail->prev = tailnode->prev;
        cachemap.erase(tailnode->key);
        delete tailnode;
    }
}

//copy constructor
// Cache::Cache(const Cache & rhs):cachemap(),capacity(rhs.capacity),size(0){
//     head = new Node();
//     tail = new Node();
//     head->next = tail;
//     tail->prev = head;
//     Node * traverse = rhs.head->next;
//     cur = head;
//     while(traverse!=NULL && traverse->next!=NULL){
//         Node * node = new Node(traverse->key, traverse->value);
//         cachemap[traverse->key] = node;
//         size++;
//         cur->next = node;
//         cur = cur->next;
//         cur->next = tail;
//         tail.prev = cur;
//     }
// }

bool Cache::storeRes(string uri, Response res, int id){
    //get code of response
    string code = res.getCode();
    //get cache control content from response
    string cacheCtrl = res.getCacheControl();
    //if code == 200 and is not private and can be cached
    if(code == "200" && !res.isPrivate() && !res.isNoStore()){
        //we add this key and value to our cachemap
        add(uri, res.getResponse());
        // cout<<"cachemapvalue:"<<cachemap[uri]->value<<endl;
        if(res.getExpires() != ""){
            string msg = "cached, expires at" + res.getExpires();
            addLog(id, msg);

        }else{
            string msg = "cached, but requires re-validation";
            addLog(id,msg);
        }
        return true;
    }
    else{
        //figure out which failure cause cache fail
        string cause;
        if(code != "200"){
            cause = "not 200 OK";
        }else if(res.isPrivate()){
            cause = "Cache-control: private";
        }else if(res.isNoStore()){
            cause = "Cache-control: no-store";
        }
        string msg = "not cacheable because " + cause;
        addLog(id, msg);
        return false;
    }
}

string Cache::reValid(Request req, Response res, int socket, int id){
    if(res.getETag() != ""){
        cout<<"need revalid:etag"<<endl;
        return checkValid(req, res, socket, "If-None-Match: ", res.getETag(), id);
    }else if(res.getLastModify()!=""){
        cout<<"need revalid:getLastModify"<<endl;
        return checkValid(req, res, socket, "If-Modified-Since: ", res.getLastModify(), id);

    }
    else{
        cout <<"need revalid: else"<<endl;
        string origin=req.getRawRequest();
        cout<<origin<<endl;
        send(socket,origin.data(), origin.size(),0);
        cout<<"In revalid: send success"<<endl;
        vector<char> msg;
        recvFrom(socket,msg);
        for (char c : msg){
            cout<<c;
        }
        Response newResponse(msg);
        storeRes(req.getURI(),newResponse,id);
        return newResponse.getResponse();
    }
}


string Cache::checkValid(Request req, Response res, int socket, string type, string content, int id){
    size_t f=req.getRawRequest().find("\r\n\r\n");
    string origin = req.getRawRequest().substr(0,f);
    string newReq = origin + "\r\n" + type + content + "\r\n\r\n";
    send(socket, newReq.data(), newReq.size()+1,0);
    vector<char> msg;
    recvFrom(socket, msg);
    Response newres(msg);
    if(newres.getCode() == "304"){
        return res.getResponse();
    }else{
        storeRes(req.getURI(), newres, id);
        return newres.getResponse();
    }
}

void recvFrom(int fd, vector<char> &msg){
    cout<<"in RecvFrom"<<endl;
    ssize_t index = 0;
    msg.resize(65536);
    ssize_t msglen = recv(fd, &msg.data()[index], msg.size(),0);
    cout<<"in RecvFrom11"<<endl;
    if(msglen == 0){
        cerr<<"Received nothing\n"<<endl;
    }else if(msglen == -1){
        cerr<<"Receive error"<<endl;
    }
    index += msglen;
    Response res(msg);

    if(res.getCode() == "304"){
        cout<<"in RecvFrom: 304 code"<<endl;
        return;
    }
    cout<<"in RecvFrom22"<<endl;
    if(res.isChunk()){
        cout<<"is chunked"<<endl;
        string resp;
        resp.insert(resp.begin(), msg.begin(), msg.end());
        while(resp.find("0\r\n\r\n") == string::npos){
            msg.resize(index + 65536);
            msglen = recv(fd, &msg.data()[index], 65536, 0);
            if(msglen <= 0){
                break;
            }
            resp = "";
            resp.insert(resp.begin(), msg.begin() + index, msg.begin() + index + msglen);
            index += msglen;
        }
        cout<<"print msg in is chunked"<<endl;
            for(char c : msg){
                cout<<c;
            }
        return;
    }
    else{
        cout<<"in RecvFrom33"<<endl;
        string resp(msg.begin(), msg.end());
        int contentlength = res.getContentLength();
        int totalLen = 0;
        int len = 0;
        if (contentlength >= 0){
            while(totalLen < contentlength){
                char nextMsg[1024];
                len = recv(fd, nextMsg, 1000, 0);
                if(len<=0){
                    if(totalLen >= contentlength){
                        break;
                    }
                    else{
                        return;
                    }
                }
                string temp(nextMsg, len);
                resp += temp;
                totalLen += len;
                
            }
        }
        msg.clear();
        vector<char> newmsg(resp.begin(),resp.end());
        msg = newmsg;
        cout<<"print msg"<<endl;
        for(char c: msg){
            cout<<c;
        }
    
    }
}

// Copy constructor
Cache::Cache(const Cache& other) : capacity(other.capacity), size(other.size), head(NULL), tail(NULL) {
    std::lock_guard<std::mutex> lck(copy_mutex);
    for (auto it = other.cachemap.begin(); it != other.cachemap.end(); ++it) {
        Node* node = new Node(it->second->key, it->second->value);
        cachemap[it->first] = node;
        if (!head) {
            head = node;
            tail = node;
        } else {
            node->prev = tail;
            tail->next = node;
            tail = node;
        }
    }
}

// Destructor
Cache::~Cache() {
    for (auto it = cachemap.begin(); it != cachemap.end(); ++it) {
        delete it->second;
    }
}

// Assignment operator
Cache& Cache::operator=(const Cache& other) {
    std::lock_guard<std::mutex> lck(copy_mutex);
    if (this == &other) {
        return *this;
    }
    // Free memory for existing nodes
    for (auto it = cachemap.begin(); it != cachemap.end(); ++it) {
        delete it->second;
    }
    cachemap.clear();
    head = NULL;
    tail = NULL;
    size = other.size;
    capacity = other.capacity;
    // Copy nodes from other
    for (auto it = other.cachemap.begin(); it != other.cachemap.end(); ++it) {
        Node* node = new Node(it->second->key, it->second->value);
        cachemap[it->first] = node;
        if (!head) {
            head = node;
            tail = node;
        } else {
            node->prev = tail;
            tail->next = node;
            tail = node;
        }
    }
    return *this;
}

// Output stream operator
ostream& operator<<(ostream& os, const Cache& cache) {
    os << "Cache content:" << endl;
    for (auto it = cache.cachemap.begin(); it != cache.cachemap.end(); ++it) {
        os << it->first << ": " << it->second->value << endl;
    }
    return os;
}
