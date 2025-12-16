CXX = g++
CXXFLAG = -g -Wall

TARGETS = serverM serverA serverR serverP client

SRCS = serverM.cpp serverA.cpp serverR.cpp serverP.cpp client.cpp

all:$(TARGETS)

#$(TARGETS): $(TARGETS).cpp
#	$(CXX) -o $(TARGETS) $(TARGETS).cpp

serverM: serverM.cpp
	$(CXX) -o serverM serverM.cpp
	
serverA: serverA.cpp
	$(CXX) -o serverA serverA.cpp

serverR: serverR.cpp
	$(CXX) -o serverR serverR.cpp

serverP: serverP.cpp
	$(CXX) -o serverP serverP.cpp
	
client: client.cpp
	$(CXX) -o client client.cpp
	
clean: 
	rm -f $(TARGETS)
