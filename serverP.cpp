#include <stdio.h>
#include <stdlib.h>
#include "reference.h"
#include <sstream>


int main(){
    int sc,num_bytes;
    struct addrinfo *servinfo, *p;
    char buff[1000];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    std::string command;

    // Beej's guide
    servinfo = get_udp_addrinfo("127.0.0.1", UDP_portP); 

    for(p = servinfo; p != NULL; p = p->ai_next){
        if ((sc = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
        if (bind(sc, p->ai_addr, p->ai_addrlen) == -1){
            close(sc);
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    printf("[Server P] Booting up using UDP on port %s.\n", UDP_portP);
    // Loop to receive and send messages to server M.

    while (1)
    {
        addr_len = sizeof(their_addr);
        num_bytes = recvfrom(sc, buff, sizeof(buff), 0, (struct sockaddr *)&their_addr, &addr_len);
        buff[num_bytes] = '\0';
        command = buff;
        std::string name = command.substr(0, command.find(" "));
        command = command.substr(command.find(" ")+1);
        std::string sendbuff;
        int price = 0;
        /*  Computer price of reservations  */
        if (get_command_type(command) != "refund"){ 
            std::string line;
            int num_H=0, num_U=0, min_H=13, min_U=13, peak_H=0,peak_U=0;
            
            std::istringstream iss(command); 

            std::cout<<"Server P received a pricing request from the main server.\n";
            while (std::getline(iss, line))// Get every line.
            {
                // For each line, count the number of slots/peak hours and chech minimum slot.
                if (line.empty())continue; 
                std::istringstream iss2(line.substr(line.find(" ")+1));
                int x;
                if (line[0] == 'U'){
                    while (iss2>>x)
                    {
                        num_U++;
                        if (x < min_U) min_U = x;
                        if (x == 5 || x == 9) peak_U++;
                    }
                    
                }
                else{
                    while (iss2>>x)
                    {
                        num_H++;
                        if (x<min_H) min_H = x;
                        if (x == 5 || x == 9) peak_H++;
                    }
                    
                }
            }
            /* By optimizing, get this formula*/
            if (num_U != 0)price += 14*num_U + 7*peak_U + 6;
            if (min_U == 5 || min_U == 9)price += 3;
            if (num_H != 0)price += 20*num_H + 10*peak_H + 10;
            if (min_H == 5 || min_H == 9)price += 5;
            // 2-decimal precision
            sendbuff = std::to_string(price) +".00";
            std::cout<<"Calculated total price of $"<<sendbuff<<" for "<<name<<".\n";            
            std::cout<<"Server P finished sending the price to the main server.\n";
        }
        else{ // compute refund
            std::cout<<"Server P received a refund request from the main server.\n";
            command = command.substr(command.find(" ")+1);
            size_t pos = command.find(" ");
            std::string space_code = command.substr(0, pos);
            std::string idx = command.substr(pos+1);
            std::istringstream iss(idx);

            int x,n=0;// Check every time slot in corresponding code.
            while (iss>>x)
            { // count the number of refunding.
                n++;
            }
            if (space_code[0] == 'U')price = n*14;
            else price = n*20;
            sendbuff = std::to_string(price) + ".00";
            std::cout<<"Calculated refund of $"<<sendbuff<<" for "<<name<<".\n";
            std::cout<<"Server P finished sending the refund amount to the main server.\n";
        }        
        num_bytes = sendto(sc, sendbuff.c_str(), sendbuff.size(), 0, (struct sockaddr *) &their_addr, addr_len);
    }    
    close(sc);
    return 0;
}