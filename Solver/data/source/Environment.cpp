#include "Solver/data/header/Environment.h"

#include <thread>
#include <fstream>
#include <map>

#include "Solver/algorithm/header/Solver.h"


using namespace std;

namespace szx {

void Environment::load(const Map<String, char*> &optionMap) {
    char *str;

    str = optionMap.at(Cli::InstancePathOption());
    if (str != nullptr) { instName = str; }

    str = optionMap.at(Cli::SolutionPathOption());
    if (str != nullptr) { slnPath = str; }

    str = optionMap.at(Cli::RandSeedOption());
    if (str != nullptr) { randSeed = atoi(str); }

    str = optionMap.at(Cli::TimeoutOption());
    if (str != nullptr) { msTimeout = static_cast<Duration>(atof(str) * Timer::MillisecondsPerSecond); }

    str = optionMap.at(Cli::MaxIterOption());
    if (str != nullptr) { maxIter = atoi(str); }

    str = optionMap.at(Cli::JobNumOption());
    if (str != nullptr) { jobNum = atoi(str); }

    str = optionMap.at(Cli::RunIdOption());
    if (str != nullptr) { rid = str; }

    str = optionMap.at(Cli::ConfigPathOption());
    if (str != nullptr) { cfgPath = str; }

    str = optionMap.at(Cli::LogPathOption());
    if (str != nullptr) { logPath = str; }

    calibrate();
}

void Environment::calibrate() {
    // adjust thread number.
    int threadNum = std::thread::hardware_concurrency();
    if ((jobNum <= 0) || (jobNum > threadNum)) { jobNum = threadNum; }

    // adjust timeout.
    msTimeout -= Environment::SaveSolutionTimeInMillisecond;
}

void Configuration::load(const String &path) {
    ifstream ifs(path);
    if (ifs.is_open() == false) return;
    map<string, string> param_switch;
    string one_line;
    while (getline(ifs, one_line)) {
        if (!one_line.empty() && one_line.front() != '#') {
            istringstream iss (one_line);
            string name, equal, val;
            iss >> name >> equal >> val;
            if (equal == "=") {
                param_switch.insert(make_pair(name, val));
            }
        }
    }
    map<string, string>::iterator it;
    it = param_switch.find("pick_item");
    if (it != param_switch.end()) {
        if (it->second == "true")
            this->pick_item = true;
        else if(it->second == "false")
            this->pick_item = false;
    }
    it = param_switch.find("top_mode");
    if (it != param_switch.end()) {
        if (it->second == "beam")
            top_mode = TBEAM;
        else if (it->second == "local")
            top_mode = TLOCAL;
    }
    it = param_switch.find("plate_mode");
    if (it != param_switch.end()) {
        if (it->second == "beam")
            plate_mode = PBEAM;
        else if (it->second == "0")
            plate_mode = P0;
    }
    it = param_switch.find("cut_mode");
    if (it != param_switch.end()) {
        if (it->second == "beam")
            cut_mode = CBEAM;
        else if (it->second == "dfs")
            cut_mode = CDFS;
        else if (it->second == "pfs")
            cut_mode = CPFS;
        else if (it->second == "A*")
            cut_mode = CASTAR;
    }
    it = param_switch.find("mtbn");
    if (it != param_switch.end()) {
        this->mtbn = stoull(it->second);
    }
    it = param_switch.find("mpbn");
    if (it != param_switch.end()) {
        this->mpbn = stoull(it->second);
    }
    it = param_switch.find("mppn");
    if (it != param_switch.end()) {
        this->mppn = stoull(it->second);
    }
    it = param_switch.find("mcbn");
    if (it != param_switch.end()) {
        this->mcbn = stoull(it->second);
    }
    it = param_switch.find("mcit");
    if (it != param_switch.end()) {
        this->mcit = stoull(it->second);
    }
}

String Configuration::toBriefStr() const {
    std::ostringstream os;
    if (!pick_item)os << "nopick;";
    os << "mtbn=" << mtbn;
    if(plate_mode == PBEAM)
        os << ";mpbn=" << mpbn;
    if (pick_item)os << ";mppn=" << mppn;
    if (cut_mode == CBEAM)
        os << ";mcbn=" << mcbn;
    else if (cut_mode == CPFS)
        os << ";mcit=" << mcit;
    os << ";mode=";
    // ������ԡ�
    if (top_mode == TBEAM)
        os << "beam";
    else if (top_mode == TLOCAL)
        os << "local";
    // ԭ�ϲ���ԡ�
    if (plate_mode == PBEAM)
        os << "+beam";
    else if (plate_mode == P0)
        os << "+0";
    // 1-cut����ԡ�
    if (cut_mode == CBEAM)
        os << "+beam";
    else if (cut_mode == CDFS)
        os << "+dfs";
    else if (cut_mode == CPFS)
        os << "+pfs";
    else if (cut_mode == CASTAR)
        os << "+A*";
    return os.str();
}

}
