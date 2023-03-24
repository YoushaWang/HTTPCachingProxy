#ifndef __CACHE_H__
#define __CACHE_H__
#include "response.hpp"
#include "request.hpp"
#include "socket.hpp"
#include <map>
#include <algorithm>
#include "print.hpp"
#include <unordered_map>
#include <mutex>
#include <string>
using namespace std;
class Node{
    public:
        string key;
        string value;
        Node* prev;
        Node* next;
        Node():key(),value(),prev(NULL),next(NULL){}
        Node(string Key,string Value):key(Key),value(Value),prev(NULL),next(NULL){}
    };
class Cache{
    private:
        map<string,Node*> cachemap;
        Node* head;
        Node* tail;
        int capacity;
        int size;
    public:
        Cache(int capa): cachemap(),head(new Node()), tail(new Node()), capacity(capa), size(0){
            head->next = tail;
            tail->prev = head;
        }
        string get(string Key);
        void add(string Key, string Value);
        void LRUop(string op, string Key, string Value);
        
        Cache(const Cache& other); // Copy constructor
        ~Cache(); // Destructor
        Cache& operator=(const Cache& other); // Assignment operator
        friend ostream& operator<<(ostream& os, const Cache& cache); 
        bool storeRes(string uri, Response res, int id);
        // string isValid(Request req, Response res, int socket, string type, string content, int id);
        string reValid(Request req, Response res, int socket, int id);
        string checkValid(Request req, Response res, int socket, string type, string content, int id);
        void printCache();


};
void recvFrom(int fd, vector<char> &msg);
#endif
