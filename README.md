# Report for socket programming project

#### Student information

Full name: Hongrun Wang 

First name: Hongrun

Last name: Wang

Student ID: 3500038621

****

## What I have done:

Source code files:

+ client.cpp, serverM.cpp, serverA.cpp, serverR.cpp, serverP.cpp, reference.h, serverA.h, serverR.h

README.md



## Code files

+ client.cpp: For invalid commands, ask for new command until it gets valid command. For help and quit commands, execute the commands directly and print the messages. For other valid commands, it sends message to the main server and receives response.
+ serverM.cpp: It receives messages from client and send to different servers depending on command. For search and lookup commands, it sends request to server R to get information and delivers them to client. For reservation and cancellation commands, it sends request to server R to judge whether this command is available and valid, then pass the response to the client or server P to compute the price. In the end, it sends price to client.
+ serverA.cpp: Loading the member.txt file when booting up. Receive username and password from the main server, compare them with data in txt file to send authentication depending on the result.
+ serverR.cpp: Loading spaces spaces.txt file when booting up. Receive message from server M, provide information to main server, search data set to judge if the command is available.
+ serverP.cpp: Receive messages of reservation and cancellation to compute price and sends information of pricing to the main server.
+ reference.h:  Define some constants, `enum` and functions that are needed by all files.
+ serverA.h: Define `struct` to store data of members, and functions for loading data and encrypting passwords.
+ serverR.h: Define`struct` to store data of parking lot spaces, and functions for traversing and modifying the data.

## The format of messages exchanged

All needed information are concatenated and delimited by a space.

**Log in:** 

+ The client sends ```login username password``` to main server so that client knows it is delivered to server A. 
+ The main server sends ```username password``` to server A.
+ Server A sends ```id username``` to main server or  ```     undefined ``` for incorrect information
+ The main server sends response from A to client directly.

**Search:**

+ The client sends command to main server directly.
+ The main server sends command to server R directly.
+ The server R sends ```NA``` for no available spaces, ```WA``` for invalid name or information that could be printed directly.

**Lookup:**

+ The client sends ```lookup``` to main server.
+ The main server sends ```lookup id``` to server R.
+ The server R sends ```NA``` for no reservations or information of this id.

**Reserve:**

+ The client sends ```reserve space_code indices_of_slots``` to main server.
+ The main server sends ```Recv_buff id``` to the server R, where Recv_buff is message that main server receives from client.
+ The server R sends ```NA``` if all slots are available, or ```FAIL``` if no available slots, or ```non-available slots``` if partial reservations to the main server.
+ The main server sends the information received from server R except all available slots.
+ The client sends ```Y``` or ```N``` to main server if partial reservation
+ The main server sends Y/N to server R
+ For pricing request, main server execute a lookup commands and deliver the result to server P.
+ Server P sends the price of reservation to main server
+ The main server sends ```NA price``` if all slots are available or ```price reserved_slots``` if partial reservation to client.

**Cancel**:

+ For valid commands, the client sends this commands to main server.
+ The main server sends ```command id username``` to server R.
+ Server R sends ```FAIL ``` if no reservations, or ```space code slot_index``` to main server.
+ If successful cancellation, main server sends ```username refund space code slot_index ``` to server P.
+ The server P sends  ```refund_amount``` to main server
+ The main server sends ```price``` to client.

## Any idiosyncrasy

For  ```reserve``` command

+ Invalid length of space code are regarded as no space code specified e.g. ```reserve U12 1 2``` or ```reserve U 1 2```
+ Space code is case-sensitive, e.g. command```reserve u101 1 2 3``` will fail to reserve
+ Invalid space code or time slots will be regarded as unavailable slots e.g. ```reserve U101 13, reserve X101 1```; For command ```reserve U101 12 13```, it will request for confirmation to reserve slot 12.
  + If time slot is a float, the float will be round, e.g. ```reserve U101 2.2``` is treated as ```reserve U101 2```
  + For multiple floats, only the first float will be processed, e.g. ```reserve U101 2.2 3.3``` is executed as ```reserve U101 2```
  + If time slot is not a number: it shows reservation successful and price but no slot is reserved.
    + For example, in previous reservation, the total price is \$76. If input is ```reserve U101 A```, it shows reserve successful but the price is still \$76.
  + **Segmentation fault:** If input is negative (e.g. ```reserve U101 -1```), it will cause Segmentation fault since the index of vector is negative.

For ```cancel```  command

+ Similar to ```reserve``` command
  + Space code is case-sensitive
  + Invalid length of space code means invalid command
  + Invalid space code and time slots mean unavailable slots
+ Float is round down (e.g. ```cancel U101 1.1``` = ```cancel U101 1```)
+ Not a number: time slot is not a number, it shows cancellation successful but refund is $0.

Any other invalid command:

+ It shows "Wrong command, please enter correct command."

## Reused Code:

+ Beej's guide

+ Functions for loading and traverse data are generated by AI.

## Ubuntu version

```
Distributor ID: Ubuntu
Description:    Ubuntu 20.04.6 LTS
Release:        20.04
Codename:       focal
```