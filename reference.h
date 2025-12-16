#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define TCP_portM "25621"
#define UDP_portM "24621"
#define UDP_portA "21621"
#define UDP_portR "22621"
#define UDP_portP "23621"

enum cmd{
    search,
    reserve,
    help,
    quit,
    lookup,
    cancel,
    login,
    others
};

struct addrinfo* get_udp_addrinfo(const char *ip, const char *port)
{ // Beej's guide
    struct addrinfo hints, *servinfo;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // IPv4/IPv6
    hints.ai_socktype = SOCK_DGRAM;  // UDP
    
    int rv = getaddrinfo(ip, port, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        return NULL;
    }

    return servinfo;  // 
}

struct addrinfo* get_addrinfo(const char *ip, const char *port, int socketpye)
{
    struct addrinfo hints, *servinfo;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // IPv4/IPv6
    hints.ai_socktype = socketpye;  // TCP or UDP
    
    int rv = getaddrinfo(ip, port, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        return NULL;
    }

    return servinfo;  // 
}

// Analysis the command to get its type.
std::string get_command_type(std::string input){
    size_t pos = input.find(" ");
    std::string type;
    if (pos != std::string::npos){
        type = input.substr(0, pos);
    }
    else{
        type = input;
    }
    for (size_t i = 0;i<type.size(); i++){
        type[i] = tolower(type[i]);
    }
    return type;
}

// String to lower
std::string get_lower(std::string input){
    for (size_t i = 0;i<input.size(); i++){
        input[i] = tolower(input[i]);
    }
    return input;
}

// judge the type of command.
enum::cmd get_command_type_enum(std::string input){
    size_t pos = input.find(" ");
    std::string type;
    cmd ret;
    if (pos != std::string::npos){
        type = input.substr(0, pos);
    }
    else{
        type = input;
    }
    type = get_lower(type);
    if (type == "help"){
        ret = help;
    }
    else if (type == "quit"){
        ret = quit;
    }
    else if (type == "lookup"){
        ret = lookup;
    }
    else if (type == "search")
    {
        ret = search;
    }
    else if (type == "reserve")
    {
        ret = reserve;
    }
    else if (type == "cancel")
    {
        ret = cancel;
    }
    else if (type == "login")
    {
        ret = login;
    }
    
    else {ret = others;}
    
    return ret;
}

//Remove all spaces in the begining and ending.
std::string remove_spaces(std::string msg){
    size_t first = msg.find_first_not_of(" ");
    size_t last = msg.find_last_not_of(" ");
    if (first != std::string::npos && last != std::string::npos){
        msg = msg.substr(first, last - first +1);
    }
    else {msg = "";}
    return msg;
}

void sigchld_handler(int s){
    while (waitpid(-1, NULL, WNOHANG)>0);
}
