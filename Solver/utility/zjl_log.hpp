/******************************
*@email:zjluestc@outlook.com
*@created date:2019/12/25
******************************/
#pragma once
#ifndef ZJL_LOG_H
#define ZJL_LOG_H

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>


//#define CLOSE_ALL_LOGS

namespace zjl_log {

class LogSwitch {
	friend class Log;
private:
    std::string name_;  // logSwitch name.
    bool off_;        	// close this log.
public:
    LogSwitch(std::string name, bool off = false) : name_(name), off_(off) {}
};

class Log {
public:
    Log(std::string dir_path = "");
    ~Log();
	
	// This class do not support copy.
	Log(const Log &other) = delete;
	Log& operator=(const Log &other) = delete;

    template<typename T>
    Log& operator<<(const T &msg) {
        #ifndef CLOSE_ALL_LOGS
        oss_ << msg;
        #endif
        return *this;
    }
private:
    std::ofstream ofs_;		// output file stream.
    std::ostringstream oss_; // log buffer.
};

template<>
inline Log& Log::operator<<<LogSwitch>(const LogSwitch &sw) {
	#ifndef CLOSE_ALL_LOGS
	if (!sw.off_) {
		if (!sw.name_.empty()) {
			if (ofs_.is_open()) {
				ofs_ << "[" << sw.name_ << "]";
			} else {
				std::cout << "[" << sw.name_ << "]";
			}
		}
		if (ofs_.is_open()) {
			ofs_ << oss_.str() << std::endl;
		} else {
			std::cout << oss_.str() << std::endl;	
		}
	}
	oss_.str("");	// clear oss.
	#endif
	return *this;
}

#pragma region External_variable
extern Log log;

extern LogSwitch debug;
extern LogSwitch info;
extern LogSwitch warn;
extern LogSwitch error;
extern LogSwitch critical;
#pragma endregion

}

#endif // !ZJL_UTILITY_H
