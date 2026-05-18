#include "Scanner.h"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <set>

using ull = unsigned long long;

Scanner::Scanner(){
    last_cpu_state = read_current_cpu_state();
}

Scanner::~Scanner(){}

void Scanner::monitoring(){
    std::unique_lock lock(mutex);

    results.clear();
    update_system_metrics();
    update_process_metrics();
}

nlohmann::json Scanner::get_results(){
    std::shared_lock lock(mutex);
    return results;
}

CpuState Scanner::read_current_cpu_state(){
    CpuState state = {0};
    std::ifstream file("/proc/stat");
    std::string line;

    if(std::getline(file, line)){
        std::stringstream ss(line);
        std::string cpu;

        ss>>cpu>>state.user>>state.nice>>state.system>>state.idle
            >>state.iowait>>state.irq>>state.softirq>>state.steal
            >>state.guest>>state.guest_nice;
    }

    return state;
}

void Scanner::update_system_metrics(){
    // CPU
    CpuState current_cpu_state = read_current_cpu_state();
    ull total_diff = current_cpu_state.get_total() - last_cpu_state.get_total();
    ull idle_diff = current_cpu_state.get_idle() - last_cpu_state.get_idle();

    double cpu_usage = 0.0;
    if(total_diff > 0){
        cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
    }

    results["cpu"] = {
        {"usage_percent", cpu_usage},
        {"user", current_cpu_state.user},
        {"system", current_cpu_state.system},
        {"idle", current_cpu_state.idle}
    };

    last_cpu_state = current_cpu_state;


    // MEM
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long total_mem = 0, free_mem = 0, available_mem = 0, buffers = 0, cached = 0;

    while(std::getline(meminfo, line)){
        std::stringstream ss(line);
        std::string key;
        long value;

        ss>>key>>value;

        if(key == "MemTotal:") total_mem = value;
        else if(key == "MemFree:") free_mem = value;
        else if(key == "MemAvailable:") available_mem = value;
        else if(key == "Buffers:") buffers = value;
        else if(key == "Cached:") cached = value;
    }

    results["mem"] = {
        {"total", total_mem},
        {"free", free_mem},
        {"available", available_mem},
        {"used", total_mem - available_mem},
        {"buffers", buffers},
        {"cached", cached}
    };
}

std::string Scanner::get_username(int uid){
    struct passwd *pw = getpwuid(uid);

    if(pw)
        return std::string(pw->pw_name);

    return std::to_string(uid);
}

void Scanner::update_process_metrics(){
    DIR* dir = opendir("/proc");

    if(!dir) return;

    struct dirent* entry;
    int total_tasks = 0, running = 0, sleeping = 0, stopped = 0, zombie = 0;
    std::vector<nlohmann::json> process_list;

    long clock_ticks = sysconf(_SC_CLK_TCK);
    ull page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
    auto now = std::chrono::steady_clock::now();
    std::set<int> active_pids;

    while((entry = readdir(dir)) != nullptr){
        if(!isdigit(entry->d_name[0])) continue;

        int pid = std::atoi(entry->d_name);
        std::string path = "/proc/" + std::string(entry->d_name);

        // stat
        std::ifstream stat_file(path + "/stat");
        if(!stat_file.is_open()) continue;

        std::string s_tmp, comm, state;
        long long ppid, pgrp, session, tty_nr, tpgid, priority, nice;
        ull flags, minflt, cminflt, majflt, cmajflt, utime, stime, cutime, cstime;

        stat_file>>s_tmp; // pid

        // чистим команду от мусора: '(' & ')'
        std::getline(stat_file, comm, ')');
        if(!comm.empty() && comm[0] == ' ')
            comm = comm.substr(2);

        stat_file>>state>>ppid>>pgrp>>session>>tty_nr>>tpgid
                >>flags>>minflt>>cminflt>>majflt>>cmajflt
                >>utime>>stime>>cutime>>cstime>>priority>>nice;

        // state
        ++total_tasks;
        if(state == "R") ++running;
        else if(state == "S" || state == "D") ++sleeping;
        else if(state == "T") ++stopped;
        else if(state == "Z") ++zombie;

        // statm
        std::ifstream statm_file(path + "/statm");
        if(!statm_file.is_open()) continue;

        ull size, resident, share, text, lib, data, dt;
        statm_file>>size>>resident>>share>>text>>lib>>data>>dt;

        // status
        std::ifstream status_file(path + "/status");
        if(!status_file.is_open()) continue;

        int uid = 0;
        std::string line;

        while(std::getline(status_file, line)){
            if(line.substr(0, 4) == "Uid:"){
                std::stringstream ss(line.substr(4));
                ss>>uid;
                break;
            }
        }

        // %CPU
        double cpu_percent = 0.0;
        ull total_time = utime + stime;
        active_pids.insert(pid);

        if(last_proc_cpu_states.count(pid)){
            ull time_diff = total_time - last_proc_cpu_states[pid].utime - last_proc_cpu_states[pid].stime;
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_proc_cpu_states[pid].timestamp).count();

            if(duration > 0){
                cpu_percent = (100.0 * time_diff / clock_ticks) / (duration / 1000.0);
            }
        }

        last_proc_cpu_states[pid] = {utime, stime, now};

        // TIME+
        ull total_seconds = total_time / clock_ticks;
        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%llu:%02llu.%02llu",
            total_seconds / 60,
            total_seconds % 60,
            (total_time % (ull)clock_ticks) * 100 / clock_ticks);

        nlohmann::json proc;
        proc["pid"] = pid;
        proc["user"] = get_username(uid);
        proc["pr"] = priority;
        proc["ni"] = nice;
        proc["virt"] = size * page_size_kb;
        proc["res"] = resident * page_size_kb;
        proc["shr"] = share * page_size_kb;
        proc["s"] = state;
        proc["cpu_percent"] = cpu_percent;
        proc["mem_percent"] = (results["mem"]["total"] > 0)
            ? (100.0 * resident * page_size_kb / (long)results["mem"]["total"])
            : 0.0;
        proc["time"] = time_buf;
        proc["command"] = comm;

        process_list.push_back(proc);
    }
    closedir(dir);

    // O(n) cleanup мёртвых процессов
    for(auto it = last_proc_cpu_states.begin(); it != last_proc_cpu_states.end(); ){
        if(!active_pids.count(it->first))
            it = last_proc_cpu_states.erase(it);
        else
            ++it;
    }

    results["tasks"] = {
        {"total", total_tasks},
        {"running", running},
        {"sleeping", sleeping},
        {"stopped", stopped},
        {"zombie", zombie}
    };

    results["processes"] = process_list;
}
