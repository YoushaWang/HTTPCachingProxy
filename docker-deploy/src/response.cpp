#include "response.hpp"

using namespace std;
void Response::parseLine(){
    
    size_t start = response.find(" ");
    size_t end = response.find(" ", start + 1);
    code = response.substr(start+1, end - start - 1);
    size_t startDate = response.find("Date");
    size_t endDate = (startDate == string::npos) ? string::npos : response.find("\r\n", startDate + 1);
    date = (startDate == string::npos) ? "" : response.substr(startDate + 2 + 4, endDate - startDate - 2 - 4);
    size_t startCtrl = response.find("CacheControl");
    size_t endCtrl = (startCtrl == string::npos) ? string::npos : response.find("\r\n", startCtrl + 1);
    CacheControl = (startCtrl == string::npos) ? "" : response.substr(startCtrl + 2 + 13, endCtrl - startCtrl - 2 - 13);
    size_t startLM = response.find("Last-Modified");
    size_t endLM = (startLM == string::npos) ? string::npos : response.find("\r\n", startLM + 1);
    LastModify = (startLM == string::npos) ? "" : response.substr(startLM + 2 + 13, endLM - startLM - 2 - 13);
    size_t startEx = response.find("Expires");
    size_t endEx = (startEx == string::npos) ? string::npos : response.find("\r\n", startEx + 1);
    Expires = (startEx == string::npos) ? "" : response.substr(startEx + 2 + 7, endEx - startEx - 2 - 7);
    size_t startET = response.find("ETag");
    size_t endET = (startET == string::npos) ? string::npos : response.find("\r\n", startET + 1);
    ETag = (startET == string::npos) ? "" : response.substr(startET + 2 + 4, endET - startET - 2 - 4);
    size_t startCL = response.find("Content-Length");
    size_t endCL = (startCL == string::npos) ? string::npos : response.find("\r\n", startCL + 1);
    ContentLength = (startCL == string::npos) ? -1 : stoi(response.substr(startCL + 2 + 14, endET - startCL - 2 - 14));
    size_t startMS = response.find("max-stale");
    size_t endMS = (startMS == string::npos) ? string::npos : response.find("=", startMS + 1);
    int maxStale = (startMS == string::npos) ? -1 : stoi(response.substr(endMS + 1, response.find("\r\n", endMS + 1) - endMS - 1));
    size_t hasmust = response.find("must-revalidate");
    bool must_revalidate = (hasmust == string::npos) ? false : true;
}
void Response::printInfo(){
    cout <<"response:---------------------"<<endl;
    cout <<response<<endl;
    // cout << "code: " << code <<endl;
    // cout << "date: " << date <<endl;
    // cout << "CacheControl: " << CacheControl <<endl;
    // cout << "LastModify: " << LastModify <<endl;
    // cout << "Expires: " << Expires <<endl;
    // cout << "ETag: " << ETag <<endl;
    // cout << "ContentLength: " << ContentLength <<endl;
    cout << "---------------------------"<<endl;

}
int Response::getContentLength(){
    return ContentLength;
}
bool Response::isPrivate(){
    string cachecrtl = CacheControl;
    cout<<cachecrtl<<endl;
    if (cachecrtl == ""){
        return false;
    }else{
        size_t pos = cachecrtl.find("private");
        if(pos == string::npos){
            return false;
        }else{
            return true;
        }
    }
}

bool Response::isNoStore(){
    string cachecrtl = CacheControl;
    cout<<cachecrtl<<endl;
    if (cachecrtl == ""){
        return false;
    }else{
        size_t pos = cachecrtl.find("no-store");
        if(pos == string::npos){
            return false;
        }else{
            return true;
        }
    }
}

bool Response::isNoCache(){
    string cachecrtl = CacheControl;
    cout<<cachecrtl<<endl;
    if (cachecrtl == ""){
        return false;
    }else{
        size_t pos = cachecrtl.find("no-cache");
        if(pos == string::npos){
            return false;
        }else{
            return true;
        }
    }
}

string Response::MaxAge(){
    string cachecrtl = CacheControl;
    if(cachecrtl == ""){
        return "";
    }else{
        size_t pos1 = cachecrtl.find("max-age");
        if(pos1 != string::npos){
            size_t pos2 = cachecrtl.find(" ", pos1+1); 
            if (pos2 == string::npos){
                pos2 = cachecrtl.find("\r\n", pos1+1);
            }
            return cachecrtl.substr(pos1+8, pos2-pos1-8);
            }
        }
        return "";/**/
}

time_t Response::UTCtime(string time){
    tm curtime;
    size_t pos = time.find("GMT");
    if(pos != string::npos){
        time.erase(pos-1,4);
    }
    strptime(time.c_str(),"%a, %d %b %Y %H:%M:%S", &curtime);
    time_t tm = mktime(&curtime);
    return tm;
}

double Response::Age(){
    time_t startdate = UTCtime(date);
    cout << "start time: " <<startdate<<endl;
    time_t currdate = time(NULL) - 28800;
    cout << "current time: "<<currdate<<endl;
    return difftime(currdate, startdate);
}
/*function isFresh: output bool type, this function is judging whether the response satisfy fresh requirement
    the check priority is MaxAge, Expires, LastModify
*/
bool Response::isFresh(int id){
    string maxAge = MaxAge();
    //1. compare max age
    if(maxAge != ""){
        // cout << "Fresh test: max age"<<endl;
        if(stol(maxAge) <= Age()){
            addLog(id,"in cache, requires validation(MaxAge)");
            return false;
        }
        return true;
    }//2. compare expires time
    else if(Expires != ""){
        // cout << "Fresh test: expire time"<<endl;
        time_t nowtime = time(NULL)-28800;
        if(nowtime >= UTCtime(Expires)){      
            if(must_revalidate){
                if(maxStale>=nowtime-UTCtime(Expires)){
                    addLog(id,"in cache, valid");
                    return true;
                }
            }      
            addLog(id,"in cache, but expired at"+Expires);
            return false;
        }
        return true;
    }//3. compare lat modify time
    else if(LastModify != ""){
        // cout << "Fresh test: lastmodify" <<endl;
        double curAge = Age();
        double freshTime = difftime(UTCtime(date),UTCtime(LastModify))/10.0;
        if(curAge>=freshTime){
            addLog(id,"in cache, requires validation(last-modify)");
            return false;      
        }
        return true;
    }//4.
    cout<<"Fresh test: no apply,so not consider it"<<endl;
    return true;
}
bool Response::ifRevalid(int thread_id){
    if (isNoCache()){
        addLog(thread_id,"in cache, requires validation");
        return true;
    }
    return !isFresh(thread_id);
    // if(isFresh(thread_id)){
    //     if(must_revalidate){
    //         return true;
    //     }else{

    //     }
    // }else{
    //     return true;
    // }
}
Response::Response(vector<char> REP){
    response.insert(response.begin(), REP.begin(), REP.end());
    parseLine();
}
string Response::getResponse(){
    return response;
}
bool Response::isChunk(){
    size_t findTrans = response.find("Transfer-Encoding");
    if (findTrans == string::npos){
        return false;
    }
    size_t findRN = response.find("\r\n",findTrans+17);
    string res = response.substr(findTrans+17+2,findRN-findTrans-17-2);
    size_t findChunk = res.find("chunked");

    if (findChunk!=string::npos){
        return true;
    }else{
        return false;
    }
}

