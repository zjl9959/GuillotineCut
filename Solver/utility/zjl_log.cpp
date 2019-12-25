/******************************
*@email:zjluestc@outlook.com
*@created date:2019/12/25
******************************/

#include "zjl_log.hpp"

#include <iomanip>
#include <random>
#include <algorithm>
#include <ctime>
#include <cstdlib>


using namespace std;

namespace zjl_log {

string human_date_str();
string short_date_str();
string get_code_version();

string human_date_str() {
    ostringstream os;
    time_t now = std::time(0);
    os << put_time(localtime(&now), "%y-%m-%e-%H_%M_%S");
    return os.str();
}

string short_date_str() {
	ostringstream os;
    time_t now = std::time(0);
    os << put_time(localtime(&now), "%y-%m-%e");
    return os.str();
}

/*
* Function: get this code's lasted commit from system command.
* Output: "commit_index data commit_information",
* if no depository found, return empty string.
*/
string get_code_version() {
	string res;
	srand(time(0));
	string temp_file = "temp" + to_string(rand()) + "~";
	string query_cmd = "git log -1 --pretty=format:\"%H %s\" > " + temp_file;
	system(query_cmd.c_str());
	ifstream ifs(temp_file);
	getline(ifs, res);
    ifs.close();
	// delete temp file.
	#ifdef __unix__
	string rm_cmd = "rm " + temp_file;
	system(rm_cmd.c_str());
	#else
	string del_cmd = "del " + temp_file;
	system(del_cmd.c_str());
	#endif
	return res;
}

Log::Log(std::string dir_path) {
    #ifndef CLOSE_ALL_LOGS
	if(dir_path.empty())return;
	string code_version = get_code_version();
	if(code_version.empty()) {
		ofs_.open(dir_path + human_date_str() + ".log", ios::app);
	} else {
		size_t divide = code_version.find_first_of(" ");
		string version = code_version.substr(0, divide);
		string short_version = version.substr(0, min(static_cast<size_t>(7), version.size()));
		string description = code_version.substr(divide + 1, code_version.size() - divide - 1);
		ofs_.open(dir_path + short_date_str() + "_" + short_version + ".log", ios::app);
		if(ofs_.is_open()) {
			ofs_ << "commit: " << version << " " << description << endl;
		}
    }
    ofs_ << "---------------------------------------" << endl;
    #endif // !CLOSE_ALL_LOGS
}

Log::~Log() {
    #ifndef CLOSE_ALL_LOGS
    if (ofs_.is_open()) {
        ofs_.close();
    }
    #endif // !CLOSE_ALL_LOGS
}

// Init static variable and golball variable.
Log log("logs/");    // output logs into file.

LogSwitch debug("DEBUG");
LogSwitch info("INFO");
LogSwitch warn("WARN");
LogSwitch error("REEOR");
LogSwitch critical("CRITICAL");

}
