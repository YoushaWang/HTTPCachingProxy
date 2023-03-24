# erss-hwk2-kh492-yw531
<h2>Set Up</h2>

1. log is stored in '/var/log/erss'  
 
2. "cd docker-deploy"  

3. Type "chmod o+x run.sh" inside docker-deploy/scr.   
   
4. Type "sudo docker-compose up"     

Proxy: GET, POST, CONNECT

<h4>TEST for addrinfo in socket.cpp</h4>
    <!-- // TEST
    #include <arpa/inet.h>
    #include <netinet/in.h>
    struct addrinfo * p;
    char ipstr[INET6_ADDRSTRLEN];
    std::cout<<"IP address for "<<this->host<<" "<<this->port<<" is:"<<std::endl;
    for (p = this->hostInfo_list;p!=NULL;p=p->ai_next){
        void *addr;
        int ipver;
        if (p->ai_family == AF_INET){
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = 1;
        }else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = 2;
        }
        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %i: %s\n", ipver, ipstr);
    } -->