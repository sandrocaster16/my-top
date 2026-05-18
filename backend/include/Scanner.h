#ifndef MONITORING
#define MONITORING

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>
#include <shared_mutex>

using ull = unsigned long long;

// общие характеристики cpu
struct CpuState{
    /*
    user: тики выполнения процессов пользователя
    nice: тики выполнения процессов с положительным значением nice
    system: работа ядра
    idle: бездействие процессора
    iowait: ожидание i/o
    irq: аппаратные прерывания
    softirq: программные прерывания
    steal: "украденные" тики
    guest и guest_nice: Время работы виртуальных процессоров
    */
    ull user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;

    ull get_total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    ull get_idle() const {
        return idle + iowait;
    }
};

struct ProcessCpuState{
    ull utime;
    ull stime;
    std::chrono::steady_clock::time_point timestamp;
};

class Scanner{
private:
    nlohmann::json results;
    std::shared_mutex mutex;

    CpuState last_cpu_state;
    std::map<int, ProcessCpuState> last_proc_cpu_states;

    void update_system_metrics();
    void update_process_metrics();

    CpuState read_current_cpu_state();
    std::string get_username(int uid);

public:
    Scanner();
    ~Scanner();

    void monitoring();

    nlohmann::json get_results();
};

#endif // MONITORING
