#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "reference.h"


int main(int argc, char *argv[]){

    //client TCP socket initialization (Beej's guide)
    int s1,numberbytes;
    struct addrinfo *servinfo;
    std::string username, pswd;
    char buff[256],recv_buff[1000];
    std::string msg;
    cmd command;
    int num_port;
    bool Guest; // True for guest, false for member.

    // user's information,
    printf("Client is up and running.\n");
    if(argc == 3){username = argv[1];pswd = argv[2];}
    else{
        std::cout<<"Please enter username and password:\n";
        std::cin>>username>>pswd;
    }

    // Intial socket and connect to server. Beej's guide
    servinfo = get_addrinfo("127.0.0.1", TCP_portM, SOCK_STREAM);

    s1 = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    connect(s1, servinfo->ai_addr, servinfo->ai_addrlen);
    freeaddrinfo(servinfo);

    if (s1 == -1){perror("client: initial error");}
    
    // Get the port number of client.
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    getsockname(s1, (struct sockaddr*)&addr, &addrlen);
    num_port = addr.sin_port;

    // Send username and password to the server
    msg = "login " + username + " " + pswd;
    std::cout<<username<<" sent an authentication request to the main server.\n";
    numberbytes = send(s1, msg.c_str(), msg.size(),0);
    if (numberbytes == -1)perror("send failed");
    // Receive message to know guest or member
    numberbytes = recv(s1, buff, 255, 0);
    if (numberbytes == -1)perror("recv failed");
    buff[numberbytes] = '\0';
    // Output the result on the screen.
    if (strcmp(buff, "0 guest") == 0){
        std::cout<<"You have been granted guest access.\n";
        username = "guest";
        Guest = true;
    }
    else if (strcmp(buff, "undefined") == 0){
        std::cout<<"Authentication failed: username or password is incorrect.\n";
        close(s1);
        return 0;
    }
    else{
        std::cout<<username<<" received the authentication result.\nAuthentication successful.\n";
        Guest = false;
    }

    //Loop to receive and send messages.
    while(1){
        // get the input and analyse which command is.
        std::getline(std::cin, msg);
        msg = remove_spaces(msg); // erase all spaces in the beginning and ending.
        command = get_command_type_enum(msg);
        memset(recv_buff, 0, sizeof(recv_buff));
        // If quit, break the while loop
        if (command == quit)break;
        /* Help and seach commands are available from both member and guest.
           Wrong commands - ask for new command
           other commands for guest, "continue" to next loop to get new commands */
        switch (command) 
        {
            // Help command, show message depending on guest or member.
            case help:
                if (Guest)std::cout<<"Please enter the command: <search <parking lot>>, <quit>\n";
                else std::cout<<"Please enter the command: <search <parking lot>>, <reserve <space> <timeslots>>, <lookup>, <cancel <space> <timeslots>>, <quit>\n";
                break;  

            case search: // Search command, same for guest and member.
                send(s1, msg.c_str(), msg.size(), 0); // Send search command to server M and receive
                                                      // the response, show information on screen.
                std::cout<<username<<" sent an availability request to the main server.\n";
                numberbytes = recv(s1, recv_buff, sizeof(recv_buff), 0);
                recv_buff[numberbytes] = '\0';
                // Receive message from server M
                std::cout<<"The client received the response from the main server using TCP over port "<<num_port<<".\n";
                if(strcmp(recv_buff,"NA") == 0){ // NA - no available space
                    std::cout<<"No available spaces in "<<msg.substr(msg.find(" ")+1)<<".\n";
                }
                else if(strcmp(recv_buff, "WA") == 0){ // Wrong name
                    std::cout<<"Invalid parking lot name.\n";
                }
                else{ // show information on screen.
                    std::cout<<recv_buff;}
                std::cout<<"---Start a new request---\n";
                break;
            case others: // Wrong command, need to re-enter.
                std::cout<<"Wrong command, please enter correct commmand.\n";
                continue;

            default: // Get new command if guest. Guest cann't access other commands.
                if (Guest){
                    std::cout<<"Guests can only check availability. Please log in as a member for full access.\n";
                    std::cout<<"---Start a new request---\n";
                    continue;
                }
                break;
        }
        /* All other commands are only available for members 
           lookup - search all slots with id
           reserve/cancel - change data in server R  */
        switch (command){
            case lookup: // Get all slots reserved by this id.
                send(s1, msg.c_str(), msg.size(),0);
                std::cout<<username<<" sent a lookup request to the main server.\n";
                numberbytes = recv(s1, recv_buff, sizeof(recv_buff), 0);
                recv_buff[numberbytes] = '\0';
                std::cout<<"The client received the response from the main server using TCP over port "<<num_port<<".\n";
                if (strcmp(recv_buff, "NA") == 0){
                    std::cout<<"You have no current reservations.\n";
                }
                else{
                    std::cout<<recv_buff;
                }
                std::cout<<"---Start a new request---\n";
                break;
            case reserve:{
                // If the reserve command is invalid.
                size_t first = msg.find(" "),second = msg.find(" ", first+1);
                if(msg == "reserve"|| first == std::string::npos || second == std::string::npos || msg.substr(first+1,second-first-1).size()!=4){
                    std::cout<<"Error: Space code and timeslot(s) are required. Please specify a space code and at least one timeslot.\n";
                }
                else{ // Valid command
                    // Sent request to the server M.
                    std::cout<<username<<" sent a reservation request to the main server.\n";
                    send(s1, msg.c_str(), msg.size(), 0);

                    // Receive response.
                    numberbytes = recv(s1, recv_buff, sizeof(recv_buff), 0);
                    recv_buff[numberbytes] = '\0';
                    if (strcmp(recv_buff, "FAIL") == 0){ // No availabel slots, reservation failed.
                        std::cout<<"The client received the response from the main server using TCP over port "<<num_port<<".\n";
                        std::cout<<"Reservation failed. No slots were reserved.\n";
                        std::cout<<"---Start a new request---\n";
                        
                    }
                    else{ // partial/full available
                        std::string comd = recv_buff;
                        
                        std::string space_name = msg.substr(msg.find(" ")+1), slots = space_name.substr(space_name.find(" ")+1);
                        space_name = space_name.substr(0, space_name.find(" "));

                        if (get_command_type(comd) == "na"){ // all available slots, reservation successful.
                            // Get the price.
                            std::string price = comd.substr(comd.find(" ")+1);
                            
                            std::cout<<"The client received the response from the main server using TCP over port "<<num_port<<".\n";
                            std::cout<<"Reservation successful for "<<space_name<<" at time slot(s) "<<slots<<".\n";
                            std::cout<<"Total cost: $"<<price<<".\n";
                            std::cout<<"---Start a new request---\n";
                        }
                        else{ // Partial available slots, need to confirm.
                            // get the input and analyse which command is.
                            std::cout<<"Time slot(s) "<<recv_buff<<" not available. Do you want to reserve the remaining slots? (Y/N):\n";
                            while (1)
                            { // Valid input.
                                std::getline(std::cin, msg);
                                msg = remove_spaces(msg); // erase all spaces in the beginning and ending.
                                if (msg == "Y"|| msg == "y" || msg == "N" || msg == "n")break;
                            }
                            // Send the confirmation to server M.
                            send(s1, msg.c_str(), msg.size(), 0);
                            // Get the response from server M.
                            numberbytes = recv(s1, recv_buff, sizeof(recv_buff), 0);
                            recv_buff[numberbytes] = '\0';
                            if (strcmp(recv_buff, "FAIL")==0){ // Input "N", reservations fail.
                                std::cout<<"The client received the response from the main server using TCP over port "<<num_port<<".\n";
                                std::cout<<"Reservation failed. No slots were reserved.\n";
                                std::cout<<"---Start a new request---\n";                                
                            }
                            else{ // Input "Y", receive price and available slots (recv_buff = A + B)
                                msg = recv_buff;
                                std::string price = msg.substr(0, msg.find(" "));
                                slots = msg.substr(msg.find(" ")+1);
                                std::cout<<"The client received the response from the main server using TCP over port "<<num_port<<".\n";
                                std::cout<<"Reservation successful for "<<space_name<<" at time slot(s) "<<slots<<".\n";
                                std::cout<<"Total cost: $"<<price<<".\n";
                                std::cout<<"---Start a new request---\n"; 
                            }                        
                        }                        
                    }
                }
                break;
            }
            case cancel:{
                // If the reserve command is invalid.
                size_t first = msg.find(" "),second = msg.find(" ", first+1);
                if(msg == "cancel"|| first == std::string::npos || second == std::string::npos || msg.substr(first+1,second-first-1).size()!=4){
                    std::cout<<"Error: Space code and timeslot(s) are required. Please specify what to cancel.\n";
                }
                else{ // Send the command to main server
                    std::cout<<username<<" sent a cancellation request to the main server.\n";
                    send(s1, msg.c_str(),msg.size(), 0);
                    numberbytes = recv(s1, recv_buff, sizeof(recv_buff), 0);
                    recv_buff[numberbytes] = '\0';
                    std::cout<<"The client received the response from the main server using TCP over port "<<num_port<<".\n";
                    if (strcmp(recv_buff, "FAIL")==0){ // FAIL - no reservation
                        std::cout<<"Cancellation failed: You do not have reservations for the specified slots.\n";
                        
                    }
                    else{ // Valid reservateion to cancel, output the refund.
                        msg = msg.substr(msg.find(" ")+1);
                        std::string space = msg.substr(0,msg.find(" "));
                        std::string slots = msg.substr(msg.find(" ")+1);
                        std::cout<<"Cancellation successful for "<<space<<" at time slot(s) "<<slots<<".\n";
                        std::cout<<"Refund amount: $"<<recv_buff<<"\n";
                    }
                    std::cout<<"---Start a new request---\n";
                }
                break;
            }
            default:
                break;
            
        }
    }

    close(s1);

    return 0;
}