#include <stdio.h>
#include <stdlib.h>
#include "reference.h"
#include "serverA.h"

int main(){
    // UDP socket initialization
    int sc,num_bytes;
    struct addrinfo *servinfo, *p;
    char buff[100];
    std::string sendbuff;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    // Beej's guide
    servinfo = get_udp_addrinfo("127.0.0.1", UDP_portA);

    for(p = servinfo; p != NULL; p = p->ai_next){
        if ((sc = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
        if (bind(sc, p->ai_addr, p->ai_addrlen) == -1){
            close(sc);
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    // Read members.txt file and store it.
    std::vector<Member> members = load_Members_file("members.txt");


    printf("[Server A] Booting up using UDP on port %s.\n", UDP_portA);
    
    // Loop to receive and send messages to server M.
    while (1)
    {
        addr_len = sizeof(their_addr);
        num_bytes = recvfrom(sc, buff, 99, 0, (struct sockaddr *)&their_addr, &addr_len);
        if (num_bytes<=0)continue;
        buff[num_bytes] = '\0';

        //Split to get username and pswd.
        strtok(buff, " ");
        std::string username = get_lower(strtok(NULL, " "));
        std::string pswd = strtok(NULL, " ");
        sendbuff = "undefined";
        std::cout<<"[Server A] Received username "<<username<<" and password ******.\n";
        //Compare if it is a member or guest.
        if (username == "guest" && pswd == "123456"){
            sendbuff = "0 guest";
            std::cout<<"[Server A] Guest has been authenticated.\n";
        }
        else{
            std::string pswd_encrypted = encrypt(pswd);
            std::vector<Member>::size_type i;
            for(i=0;i<members.size();i++){
                if(username == get_lower(members[i].name) && pswd_encrypted == members[i].password){
                    sendbuff = members[i].id+" "+members[i].name;
                    std::cout<<"[Server A] Member "<<members[i].name<<" has been authenticated.\n";
                }
            }
            if(sendbuff == "undefined")std::cout<<"[Server A]The username "<<members[i].name<<" or password ****** is incorrect.\n";
        }
        num_bytes = sendto(sc, sendbuff.c_str(), sendbuff.size(), 0, (struct sockaddr *) &their_addr, addr_len);
    }    
    close(sc);
    return 0;
}