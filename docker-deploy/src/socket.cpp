#include "socket.hpp"
#include <arpa/inet.h>
#define MESS_SIZE 65535
// for Socket class
void Socket::printInfo(){
//  Test:
    std::cout<<"status:  "<<status<<std::endl;
    std::cout<<"socketfd:"<<socketfd<<std::endl;
    std::cout<<"host:    "<<host<<std::endl;
    std::cout<<"port:    "<<port<<std::endl;
}

// for Server
void Server::serverSetUp(){
    // make sure the struct is empty
    memset(&hostInfo, 0, sizeof(hostInfo));
    // don't care IPv4 or IPv6
    hostInfo.ai_family = AF_UNSPEC;  
    // TCP stream sockets
    hostInfo.ai_socktype = SOCK_STREAM;
    // fill in my IP for me
    hostInfo.ai_flags = AI_PASSIVE;  
    // get status and error check
    status = getaddrinfo(host, port, &hostInfo, &hostInfo_list);
    if (status != 0) {
        // cerr << "no-id: ERROR cannot get address info for host" << endl;
        // cerr << "  (" << host << "," << port << ")" << endl;
        // exit(EXIT_FAILURE);
        throw(ErrMsg("no-id: ERROR getaddrinfo of server\n"));
    }
    // get the File Descriptor and error check
    socketfd = socket(hostInfo_list->ai_family,
                            hostInfo_list->ai_socktype,
                            hostInfo_list->ai_protocol);

    if (socketfd == -1) {
        throw(ErrMsg("no-id: ERROR in socket of server\n"));
    }
    // allow to reuse the port
    int yes=1;
    status = setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    if (status == -1){
        throw(ErrMsg("no-id: ERROR in setsockopt of server\n"));
    }
    // bind: decide on which port
    status = bind(socketfd, hostInfo_list->ai_addr, hostInfo_list->ai_addrlen);
    if (status == -1){
        throw(ErrMsg("no-id: ERROR in bind of server\n"));
    }    
    // listen: seconf argument is backlog which default limit is 20
    status = listen(socketfd, 10);
    if (status == -1){
        throw(ErrMsg("no-id: ERROR in listen of server\n"));
    }
}
int Server::serverAccept(std::string *ip){
    struct sockaddr_storage socket_addr;
    socklen_t addr_size;
    int new_fd;
    // now accept an incoming connection:
    addr_size = sizeof(socket_addr);
    new_fd = accept(socketfd, (struct sockaddr *)&socket_addr, &addr_size);
    if (new_fd == -1){
        throw(ErrMsg("no-id: ERROR in accept of server\n"));
    }
    // ready to communicate on socket descriptor new_fd!
    //convert binary ip to decimal ip
    struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
    *ip = inet_ntoa(addr->sin_addr);
    return new_fd;
}

// for Client
void Client::clientSetUp(){
    // make sure the struct is empty
    memset(&hostInfo, 0, sizeof(hostInfo));
    // don't care IPv4 or IPv6
    hostInfo.ai_family = AF_UNSPEC;  
    // TCP stream sockets
    hostInfo.ai_socktype = SOCK_STREAM;
    // get ready to connect
    status = getaddrinfo(host, port, &hostInfo, &hostInfo_list);
    //cout the client info about host and port
    // cout << "  (" << host << "," << port << ")" << endl;
    if (status != 0) {
        // throw(ErrMsg("no-id: ERROR in getaddrinfo of client\n"));
        cerr << "no-id: ERROR cannot get address info for Client host" << endl;
        cerr << "  (" << host << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
    }
    socketfd = socket(hostInfo_list->ai_family,
                    hostInfo_list->ai_socktype,
                    hostInfo_list->ai_protocol);
    if (socketfd == -1) {
        throw(ErrMsg("no-id: ERROR in socket of client\n"));
    }
    status = connect(socketfd,hostInfo_list->ai_addr,hostInfo_list->ai_addrlen);
    if (status == -1) {
        cerr << "no-id: ERROR in connect of client\n" << endl;
        cerr << "  (" << host << "," << port << ")" << endl;
        exit(EXIT_FAILURE);
        // throw(ErrMsg("no-id: ERROR in connect of client\n"));
    }
}

void fdset_traversial(std::vector<int> fd_v){
    // use select to listen all ports
    fd_set read_fds;
    int max_fd = * max_element(fd_v.begin(),fd_v.end());

    FD_ZERO(&read_fds);
    for(int i=0;i<fd_v.size();i++){
        FD_SET(fd_v[i],&read_fds);
    }
    select(max_fd+1,&read_fds,NULL,NULL,NULL);
    for(int i = 0; i < fd_v.size(); i++){
        if(FD_ISSET(fd_v[i],&read_fds)){
            vector<char> buffer_rsp(65536);
            ssize_t recvByte = recv(fd_v[i], &buffer_rsp.data()[0],65535, 0);
            cout<<"recvByte: "<<recvByte<<endl;
            string buffer_rsp_str;
            buffer_rsp_str.insert(buffer_rsp_str.begin(), buffer_rsp.begin(), buffer_rsp.end());
            cout<<buffer_rsp_str<<endl;
            if (recvByte>=0){
                break;
            }else{
                // error
                return;
            }
        }
    }
}

int my_send(int fd, vector<char> &mess){
    int lens = mess.size();
    int buff_lens, status;
    char buff[MESS_SIZE];
    int round = lens/MESS_SIZE;
    int remain = lens % MESS_SIZE;
    int index = 0;
    for (int i=0; i<round;i++){
        for (int i=0;i<MESS_SIZE;i++){
            buff[i] = mess[i+index];
        }
        buff_lens = MESS_SIZE;
        status = my_sendHelper(fd,buff,buff_lens);
        if (status==-1){
            cerr<<"send error"<<endl;
            return status;
        }
        index += MESS_SIZE;
    }
    for(int i =0;i<remain;i++){
        buff[i] = mess[i+index];
    }
    buff_lens=remain;
    status = my_sendHelper(fd,buff,buff_lens);
    if (status == -1){
        cerr<<"send error"<<endl;
        return status;
    }
    return status;
}
int my_sendHelper(int fd, char * buff, int lens){
    int total = 0;
    int byteleft = lens;
    int n = 0;
    while(total < lens){
        n = send(fd,buff+total, byteleft,0);
        if (n == -1){
            break;
        }
        total += n;
        byteleft -= n;
    }
    lens = total;
    if (n==-1){
        return -1;
    }
    return 0;
}
int my_recv(int sockfd, vector<char> & mess){
    mess.clear();
    char buffer[65536];
    int status = 1;
    status = recv(sockfd, buffer, 65536, 0);
    if(status == -1){
        cerr<<"cannot receive"<<endl;
        return -1;
    }
    for(int i = 0;i<status;i++){
        mess.push_back(buffer[i]);
    }
    return status;
}


/*
//send
  const char *message = "127.0.0.1";
  send(socket_fd, message, strlen(message), 0);
//receive
  char buffer[512];
  recv(client_connection_fd, buffer, 9, 0);
  buffer[9] = 0;

  cout << "Server received: " << buffer << endl;
*/