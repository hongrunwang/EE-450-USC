#include <stdio.h>
#include <stdlib.h>
#include "reference.h"
#include "serverR.h"


int main(){ 
    int sc,num_bytes;
    struct addrinfo *servinfo, *p;
    char buff[256];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    std::string commd,ret;
    cmd type;

    // Inital the UDP connection. Beej's guide
    servinfo = get_udp_addrinfo("127.0.0.1", UDP_portR);

    for(p = servinfo; p != NULL; p = p->ai_next){
        if ((sc = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
        if (bind(sc, p->ai_addr, p->ai_addrlen) == -1){
            close(sc);
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    // Load the spaces.txt file to store data.
    std::vector<Space> spaces = load_Spaces_file("spaces.txt");

    printf("[Server R] Booting up using UDP on port %s.\n", UDP_portR);
    // Loop to receive and send messages to server M.
    while (1)
    {
        // Receive request from server M and classify the type.
        addr_len = sizeof(their_addr);
        num_bytes = recvfrom(sc, buff, 255, 0, (struct sockaddr *)&their_addr, &addr_len);
        buff[num_bytes] = '\0';
        commd = buff;
        type = get_command_type_enum(commd);
        switch (type)
        {
        case search:
            std::cout<<"Server R received an availability request from the main server.\n";
            ret = search_parking(spaces, commd);
            std::cout<<"Server R finished sending the response to the main server.\n";
            num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
            break;
        case lookup:{
            std::cout<<"Server R received a lookup request from the main server.\n";
            std::string id = commd.substr(commd.find(" ")+1);
            ret = lookup_parking(spaces, id);
            std::cout<<"Server R finished sending the reservation information to the main server.\n";
            num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
            break;
        }
        case reserve:{
            std::cout<<"Server R received a reservation request from the main server.\n";
            std::string id = commd.substr(commd.find_last_of(" ")+1);
            std::string code = commd.substr(0, commd.find_last_of(" "));// original command
            std::string available = "";
            code = code.substr(code.find(" ")+1); // command removed "reserve" e.g. U101 1 2 3 4
            ret = reserve_parking(spaces, code, available); // NA - all available and reserverd/ All not available slot
            std::string slots = code.substr(code.find(" ")+1); // slots the user want to reserve, e.g. 1 2 3 4
            std::string name = code.substr(0, code.find(" ")); // parking space name e.g. U101, U222 
            ret = remove_spaces(ret);
            available = remove_spaces(available);
            if (ret == slots){ // All slots are already reserved. No available slots.
                std::cout<<"Time slot(s) "<<ret<<" not available for "<<name<<".\n";
                ret = "FAIL";
                num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
            }
            else if (ret == "NA"){ // All slots are available.
                std::cout<<"All requested time slots are available.\n";
                spaces = confirm_reserve(spaces, code, id);
                num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
                num_bytes = recvfrom(sc, buff, 255, 0, (struct sockaddr *)&their_addr, &addr_len);
                buff[num_bytes] = '\0';
                std::string username = buff;
                std::cout<<"Successfully reserved "<<name<<" at time slot(s) "<<slots<<" for "<<username<<".\n";
            }
            else{ // Partial availability - ask for confirmation. ret - all not available slots.
                // Get the confirmation to reserve.
                std::cout<<"Time slot(s) "<<ret<<" not available for "<<name<<".\nRequesting to reserve rest available slots (Y/N).\n";
                num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
                num_bytes = recvfrom(sc, buff, 255, 0, (struct sockaddr *)&their_addr, &addr_len);
                buff[num_bytes] = '\0';
                commd = buff;
                // Confirmation with username.
                std::string username = commd.substr(commd.find(" ")+1);
                commd = commd.substr(0, commd.find(" "));
                if (commd == "N" || commd == "n"){ // N - nothing reserved, just sent back,
                    std::cout<<"Reservation cancelled.\n";
                    ret = "FAIL";
                    num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
                }
                else if (commd == "Y" || commd == "y"){ // Y - successfully reserved, sent reserved slots back.
                    std::cout<<"User confirmed partial reservation.\n";
                    spaces = confirm_reserve(spaces, code, id);
                    std::cout<<"Successfully reserved "<<name<<" at time slot(s) "<<available<<" for "<<username<<".\n";
                    num_bytes = sendto(sc, available.c_str(), available.size(), 0, (struct sockaddr *) &their_addr, addr_len);
                }
            }
            break;
        }
        case cancel:{
            std::cout<<"Server R received a cancellation request from the main server.\n";

            std::string username = commd.substr(commd.find_last_of(" ")+1); // Get username
            commd = commd.substr(0, commd.find_last_of(" "));
            std::string id = commd.substr(commd.find_last_of(" ")+1); // Get id
            commd = commd.substr(0, commd.find_last_of(" ")); // original command, e.g. cancel H666 7
            commd = commd.substr(commd.find(" ")+1); // command removed cancel, e.g. H666 7
            std::string code = commd.substr(0,commd.find(" "));
            std::string slots = commd.substr(commd.find(" ")+1);

            if(check_parking(spaces, commd, id)){ // True
                spaces = cancel_reserve(spaces, commd, id);
                ret = commd;
                std::cout<<"Successfully cancelled reservation for "<<code<<" at time slot(s) "<<slots<<" for "<<username<<".\n";
            }
            else{
                ret = "FAIL";
                std::cout<<"No reservation found for "<<username<< " at "<<code<<" time slot(s) "<<slots<<".\n";
            }
            num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
            break;
        }
        default: // Lookup to compute price
            std::string id = commd.substr(commd.find(" ")+1);
            ret = lookup_parking_price(spaces, id);
            num_bytes = sendto(sc, ret.c_str(), ret.size(), 0, (struct sockaddr *) &their_addr, addr_len);
            break;
        }
    }    
    close(sc);

    return 0;

}