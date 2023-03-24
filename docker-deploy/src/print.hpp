#ifndef __PRINT_HPP__
#define __PRINT_HPP__

#include <mutex>
#include <sstream>
#include <iostream>
#include <ctime>    //can use strftime
#include <fstream>  //write file
#include <string>   //transfer char to string

std::string addReqRspLog(int id, std::string method, std::string host, std::string info);
std::string printTime();
std::string addLog(int id, std::string msg);
void writeLog(std::string text);
void openFile(std::string path);

#endif