#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <httplib.h>
#include "Scanner.h"

void print_help(const char* prog_name){
    std::cout<<"Usage: "<<prog_name<<" [-p <port>]\n";
    std::cout<<"  -p, --port    port\n";
    std::cout<<"  -d, --delay   delay (ms)\n";
    std::cout<<"  -pwc, --ping-websocket   delay\n";
}

std::string get_public_ip(){
    httplib::Client cli("http://api.ipify.org");

    if(auto res = cli.Get("/")){
        if(res->status == 200){
            return res->body;
        }
    }

    return "<NOT FOUND>";
}

int main(int argc, char* argv[]){
    std::string ip = get_public_ip();
    int port = 12345;
    int delay = 100;
    int ping_interval = 30;

    // parse flags
    for(int i=1; i<argc; ++i){
        std::string arg = argv[i];

        if((arg == "-p" || arg == "--port") && i+1 < argc){
            port = std::stoi(argv[++i]);
        }
        else if((arg == "-d" || arg == "--delay") && i+1 < argc){
            delay = std::stoi(argv[++i]);
        }
        else if((arg == "-pwc" || arg == "--ping-websocket") && i+1 < argc){
            ping_interval = std::stoi(argv[++i]);
        }
        else if(arg == "-h" || arg == "--help"){
            print_help(argv[0]);
            return 0;
        }
    }

    // start
    std::cout<<"Start monitoring\n";
    std::cout<<"Delay: "<<delay<<" ms\n";

    httplib::Server svr;
    Scanner scanner;

    svr.set_websocket_ping_interval(ping_interval);

    svr.WebSocket("/get_monitoring_resources", [&scanner, delay](const httplib::Request &, httplib::ws::WebSocket &ws){
        while(ws.is_open()){
            scanner.monitoring();
            std::string msg = scanner.get_results().dump();

            ws.send(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    });

    std::cout<<"Server started at http://"<<ip<<":"<<port<<'\n';
    if(!svr.listen("0.0.0.0", port)){
        std::cerr<<"Failed to start HTTP server on port "<<port<<'\n';
        return 1;
    }

    return 0;
}