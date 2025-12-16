#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "reference.h"

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

int main(){
    // TCP socket initialization
    int s1,socket_child,num_byte;
    struct addrinfo *servinfo;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char recv_buff[1000];
    struct sigaction sa;
    
    // Beej's guide
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    servinfo = get_addrinfo("127.0.0.1", TCP_portM, SOCK_STREAM);

    s1 = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    int opt = 1;
    if (setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))<0){
        perror("setsockopt");
    }

    printf("[Server M]Booting up using UDP on port %s.\n",UDP_portM);
    
    if (bind(s1, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
        perror("TCP bind error");
        exit(1);
    }
    if (listen(s1, 20) == -1){
        perror("listen error");
        exit(1);
    }

    freeaddrinfo(servinfo);

    if (s1 == -1)perror("client: initial error");

    // UDP recv socket initialization
    int sc;
    struct addrinfo *udp_servinfo, *udp_p;
    struct sockaddr_storage udp_their_addr;
    socklen_t addr_len = sizeof(udp_their_addr);
    // struct sockaddr_storage udp_their_addr;
    char udp_buff[1000];

    
    udp_servinfo = get_addrinfo("127.0.0.1", UDP_portM,SOCK_DGRAM);
    for(udp_p = udp_servinfo; udp_p != NULL; udp_p = udp_p->ai_next){
        if((sc = socket(udp_p->ai_family, udp_p->ai_socktype, udp_p->ai_protocol)) == -1)continue;
        break;
    }
    
    if (bind(sc, udp_p->ai_addr, udp_p->ai_addrlen) == -1){
        close(sc);
        perror("UDP bind error");
    }

    // UDP to server A, P, R
 
    int numA,numR,numP;
    struct addrinfo *servinfoA,*servinfoR, *servinfoP;

    servinfoA = get_addrinfo("127.0.0.1", UDP_portA,SOCK_DGRAM);
    servinfoR = get_addrinfo("127.0.0.1", UDP_portR,SOCK_DGRAM);
    servinfoP = get_addrinfo("127.0.0.1", UDP_portP,SOCK_DGRAM);

    // Loop to receive and send messages.

    while(1){
  
        addr_size = sizeof(their_addr);

        socket_child = accept(s1,(struct sockaddr *)&their_addr, &addr_size);
        
        if (socket_child == -1){
            perror("accept");
            continue;
        }

        if (!fork()){
            close(s1);
            std::string name,commd,id;
            cmd type;

            while(1){
                memset(recv_buff, 0, sizeof(recv_buff));
                num_byte = recv(socket_child, recv_buff, sizeof(recv_buff), 0);
                if (num_byte <=0)exit(1);
                recv_buff[num_byte] = '\0';

                commd = recv_buff; // classify which type of command.
                type = get_command_type_enum(commd);

                /*  Depending on the type of command, excute different parts
                    If guest, username = guest        */
                switch(type){
                    // login to check the authentication
                    case (login):{
                        // Get username to print message on screen.
                        std::string username = commd.substr(commd.find(" ")+1, commd.find(" ", commd.find(" ")+1)-(commd.find(" ")+1));
                        std::cout<<"Server M received username "<<username<<" and password ******.\n";
                        
                        // Send request to server A.
                        std::cout<<"Server M sent the authentication request to Server A.\n";
                        numA = sendto(sc, recv_buff, num_byte, 0, servinfoA->ai_addr, servinfoA->ai_addrlen);
                        numA = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                        std::cout<<"server M received the response from server A using UDP over port "<<UDP_portM<<".\n";
                        udp_buff[numA] = '\0';
                        
                        // Send the result to client.
                        send(socket_child, udp_buff, strlen(udp_buff), 0);
                        std::cout<<"Server M sent the response to the client using TCP over port "<<TCP_portM<<".\n";
                        
                        // Receive the response and get user id and name.
                        username = udp_buff;
                        id = username.substr(0, username.find(" "));
                        name = username.substr(username.find(" ")+1);
                        break;
                    }
                    case (search):{
                        // Send request to server R. TODO: for "search" without name.
                        std::string parking_name;
                        if (commd == "search")parking_name = "UPC HSC"; // No paramet in search command.
                        else{parking_name = commd.substr(commd.find(" ")+1);}
                        std::cout<<"Server M received an availability request from "<<name<<" for "<<parking_name<<" using TCP over port "<<TCP_portM<<".\n";
                        numR = sendto(sc, recv_buff, strlen(recv_buff), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                        
                        // Receive response from server R.
                        std::cout<<"Server M sent the availability request to Server R.\n";
                        numR = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                        udp_buff[numR] = '\0';
                        std::cout<<"Server M received the response from Server R using UDP over port "<<UDP_portM<<".\n";

                        // Send message to client
                        send(socket_child, udp_buff, strlen(udp_buff), 0);
                        std::cout<<"Server M sent the availability information to the client.\n"; 
                        break;
                    }
                    case lookup:{
                        // print message on screen
                        std::cout<<"Server M received a lookup request from "<<name<<" using TCP over port "<<TCP_portM<<".\n";

                        // Sent request to R.
                        std::string msg_lookup= "lookup "+id;
                        numR = sendto(sc, msg_lookup.c_str(), msg_lookup.size(), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                        std::cout<<"Server M sent the lookup request to Server R.\n";

                        // Receive from R.
                        std::cout<<"server M received the response from server R using UDP over port "<<UDP_portM<<".\n";
                        numR = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                        udp_buff[numR] = '\0';

                        // Sent result to client.
                        send(socket_child, udp_buff, strlen(udp_buff), 0);
                        std::cout<<"Server M sent the lookup result to the client.\n";
                        break;
                    }
                    case reserve:{
                        // print message
                        std::cout<<"Server M received a reservation request from "<<name<<" using TCP over port "<<TCP_portM<<".\n";

                        // Send request and pass user id to server R.
                        commd = commd +" "+id;
                        numR = sendto(sc, commd.c_str(), commd.size(), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                        std::cout<<"Server M sent the reservation request to Server R.\n";

                        // Receive response
                        numR = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                        udp_buff[numR] = '\0';
                        std::cout<<"Server M received the response from Server R using UDP over port "<<UDP_portM<<".\n";
                        // If there are all available slots, reserve and compute price
                        if(strcmp(udp_buff, "NA") == 0){ 
                            sendto(sc, name.c_str(), name.size(), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                            commd = "others "+id; // Search all slots with this user id.
                            std::cout<<"Server M sent the pricing request to Server P.\n";
                            sendto(sc, commd.c_str(), commd.size(), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                            numR = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                            udp_buff[numR] = '\0';
                            // Send request to server P and get the price.
                            commd = name + " " + std::string(udp_buff);
                            sendto(sc, commd.c_str(), commd.size(), 0, servinfoP->ai_addr, servinfoP->ai_addrlen);
                            numP = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                            udp_buff[numP] = '\0';
                            std::cout<<"Server M received the pricing information from Server P using UDP over port "<<UDP_portM<<".\n";

                            // Send the reservations and price to client
                            commd = "NA " + std::string(udp_buff);
                            std::cout<<"Server M sent the reservation reslut to the client.\n";
                            send(socket_child, commd.c_str(), commd.size(), 0);
                        }
                        // No available slots and reservation failed.
                        else if (strcmp(udp_buff, "FAIL") == 0){ 
                            std::cout<<"Server M sent the reservation reslut to the client.\n";
                            send(socket_child, udp_buff, strlen(udp_buff), 0);
                        }
                        else{ // Partial reservation 
                            std::cout<<"Server M sent the partial reservation confirmation request to the client.\n";
                            send(socket_child, udp_buff, strlen(udp_buff), 0);// Send confirmation request to client.
                            num_byte = recv(socket_child, recv_buff, sizeof(recv_buff), 0);
                            if (num_byte <=0)exit(1); // Receive confirmation from client.
                            recv_buff[num_byte] = '\0';
                            commd = recv_buff;
                            commd = commd + " " + name;
                            // Sent confirmation to R.
                            std::cout<<"Server M sent the confirmation response to Server R.\n";
                            numR = sendto(sc, commd.c_str(), commd.size(), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                            // Get the response of confirmation and sent the pricing quest if Y.
                            numR = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                            udp_buff[numR] = '\0';
                            if (strcmp(udp_buff, "FAIL")==0){ // N - reservation fails.
                                std::cout<<"Server M sent the reservation result to the client.\n";
                                send(socket_child, udp_buff, strlen(udp_buff), 0);
                            }
                            else{ // Y - reservation successfully and compute price.
                                std::string slots = udp_buff;
                                commd = "others "+id; // search all slots with this id to server R
                                std::cout<<"Server M sent the pricing request to Server P.\n";
                                sendto(sc, commd.c_str(), commd.size(), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                                numR = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                                udp_buff[numR] = '\0';
                                // Compute the price, send to server P and receive response from P.
                                commd = name + " " + std::string(udp_buff);
                                sendto(sc, commd.c_str(), commd.size(), 0, servinfoP->ai_addr, servinfoP->ai_addrlen);
                                numP = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                                udp_buff[numP] = '\0'; // udp_buff - price (2-dec precision)
                                std::cout<<"Server M received the pricing information from Server P using UDP over port "<<UDP_portM<<".\n";

                                // Send the reservation and price to client
                                commd =  std::string(udp_buff) + " " + slots;
                                std::cout<<"Server M sent the reservation reslut to the client.\n";
                                send(socket_child, commd.c_str(), commd.size(), 0);
                            }
                        }
                        break;
                    }
                    case cancel:{ // Cancel the reservations and get refund.
                        std::cout<<"Server M received a cancellation request from "<<name<<" using TCP over port "<<TCP_portM<<".\n";
                        std::cout<<"Server M sent the cancellation request to Server R.\n";
                        commd = commd + " " + id + " " + name; // Send cancel request to server R to know if cancel successfully.
                        numR = sendto(sc, commd.c_str(), commd.size(), 0, servinfoR->ai_addr, servinfoR->ai_addrlen);
                        
                        // Receive response from server R.
                        std::cout<<"Server M received the response from Server R using UDP over port "<<UDP_portM<<".\n";
                        numR = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                        udp_buff[numR] = '\0';
                        if(strcmp(udp_buff, "FAIL")!=0){ // Cancel successfully
                            std::cout<<"Server M sent the refund request to Server P.\n";
                            commd = name + " refund " + std::string(udp_buff); // Send name and cancellations to server P
                            sendto(sc, commd.c_str(), commd.size(), 0, servinfoP->ai_addr, servinfoP->ai_addrlen);
                            numP = recvfrom(sc, udp_buff, sizeof(udp_buff), 0, (struct sockaddr *)&udp_their_addr, &addr_len);
                            udp_buff[numP] = '\0'; // udp_buff - price sent to client
                            std::cout<<"Server M received the refund information from Server P using UDP over port "<<UDP_portM<<".\n";
                        }
                        send(socket_child, udp_buff, strlen(udp_buff), 0);
                        break;
                    }
                    default:
                        std::cout<<"other commands\n";
                        break;
                }

            }
            close(socket_child);
            exit(0);
        }
        else{
            close(socket_child);
        }
        close(socket_child);
        
    }
    close(s1);
    freeaddrinfo(udp_servinfo);
    freeaddrinfo(servinfoA);
    freeaddrinfo(servinfoP);
    freeaddrinfo(servinfoR);
}