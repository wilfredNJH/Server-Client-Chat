/*!*************************************************************************
****
\file MinChatSvr.cpp
\author Wilfred Ng Jun Hwee
\par DP email: junhweewilfred.ng@digipen.edu
\par Course: csd2160
\par Assignment #2
\date 02-25-2021

\brief
This program is the simulation of a chat server

****************************************************************************
***/
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>

#define DEFAULT_BUFFER_LENGTH 512     
#define MAX_CLIENTS 5

std::string szServerIPAddr;                          // Put here the IP address of the server
std::string nServerPort = "5050";                    // The server port that will be used by

//running the thread for reading data 
struct CLIENT_INFO
{
    int clientId;//stores the client id 
    SOCKET hClientSocket;
    std::string clientMessage;
    bool isConnected = false;
};

std::vector<std::string> connectedClients; //stores the name of all the connected users 
int totalNumClients = 0;//tracks the total number of clients connected

bool InitWinSock2_0();
int ClientThread(CLIENT_INFO& clientData, std::vector<CLIENT_INFO>& clientStorage, std::thread& thread);

//entry point 
int main()
{
    struct addrinfo* serverAddr = NULL;
    //setting the threads and the number of clients 
    std::vector<CLIENT_INFO> client(MAX_CLIENTS);
    std::thread clientThreads[MAX_CLIENTS];

    //getting the sever ip address & port number 
    std::cout << "Enter the server IP Address: ";
    std::getline(std::cin, szServerIPAddr);

    //Initialize Winsock
    if (!InitWinSock2_0())
    {
        std::cout << "Unable to Initialize Windows Socket environment" << WSAGetLastError() << std::endl;
        return -1;
    }

    //setting up the protocol
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    //getting the server information
    getaddrinfo(static_cast<PCSTR>(szServerIPAddr.c_str()), static_cast<PCSTR>(nServerPort.c_str()), &hints, &serverAddr);

    //creating the socket 
    SOCKET hServerSocket;
    hServerSocket = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol);

    //setting up socket options 
    const char optionVal = 1;
    setsockopt(hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(int)); //Make it possible to re-bind to a port that was used within the last 2 minutes
    setsockopt(hServerSocket, IPPROTO_TCP, TCP_NODELAY, &optionVal, sizeof(int)); //Used for interactive programs

    // Bind the Server socket to the address & port
    if (bind(hServerSocket, serverAddr->ai_addr, (int)serverAddr->ai_addrlen) == SOCKET_ERROR){
        std::cout << "Unable to bind to " << szServerIPAddr << " port " << nServerPort << std::endl;
        // Free the socket and cleanup the environment initialized by WSAStartup()
        closesocket(hServerSocket);
        WSACleanup();
        return -1;
    }

    // Put the Server socket in listen state so that it can wait for client connections
    if (listen(hServerSocket, SOMAXCONN) == SOCKET_ERROR){
        std::cout << "Unable to put server in listen state" << std::endl;
        // Free the socket and cleanup the environment initialized by WSAStartup()
        closesocket(hServerSocket);
        WSACleanup();
        return -1;
    }

    //setting the clients 
    for (int i = 0; i < MAX_CLIENTS; i++){
        client[i] = { -1, INVALID_SOCKET };
    }

    std::string msg;
    int clientIndex = -1;
    //starting the infinite loop
    while (true){
        // As the socket is in listen mode there is a connection request pending.
        // Calling accept( ) will succeed and return the socket for the request.
        SOCKET clientSocket;
        struct sockaddr_in clientAddr;

        clientSocket = accept(hServerSocket, (struct sockaddr*)&clientAddr, NULL);
        if (clientSocket == INVALID_SOCKET){
            std::cout << "accept( ) failed" << std::endl;
        }

        totalNumClients = -1;
        clientIndex = -1;//setting the index 

        //loop through the clients 
        for (int i = 0; i < MAX_CLIENTS; i++){
            //check the socket 
            if (client[i].hClientSocket == INVALID_SOCKET && clientIndex == -1){
                client[i].hClientSocket = clientSocket;
                client[i].clientId = i;
                clientIndex = i;
            }
            //if socket is valid 
            if (client[i].hClientSocket != INVALID_SOCKET) {
                totalNumClients++;//increment the number of clients connected
            }
        }

        //if the socket is valid 
        if (clientIndex != -1){
            //Send the id to that client
            std::cout << "Client Connected!" << std::endl;
            msg = std::to_string(client[clientIndex].clientId);
            send(client[clientIndex].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);

            //setting the threads
            clientThreads[clientIndex] = std::thread(ClientThread, std::ref(client[clientIndex]), std::ref(client), std::ref(clientThreads[clientIndex]));
        }else{
            msg = "Server full";//telling the client that the server is full
            send(clientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);
            std::cout << msg << std::endl;
        }
    }


    closesocket(hServerSocket);

    //close all the client sockets
    for (int i = 0; i < MAX_CLIENTS; i++){
        clientThreads[i].detach();
        closesocket(client[i].hClientSocket);
    }
    WSACleanup();
    return 0;
}

/*********************
\fn bool InitWinSock2_0()
\brief initializing the winsock2
\return true when succeed, else false
*********************/
bool InitWinSock2_0()
{
    WSADATA wsaData;
    WORD wVersion = MAKEWORD(2, 0);

    if (!WSAStartup(wVersion, &wsaData))
        return true;

    return false;
}


/*********************
\fn int ClientThread(CLIENT_INFO& clientData, std::vector<CLIENT_INFO>& client_array, std::thread& thread)
\brief client thread process 
\param clientData - the client information 
\param clientStorage - the vector that stores all the client data
\parma thread - client threaed pointer
\return 0 when thread ended 
*********************/
int ClientThread(CLIENT_INFO& clientData, std::vector<CLIENT_INFO>& clientStorage, std::thread& thread) {
    std::string msg;
    char msgBuffer[DEFAULT_BUFFER_LENGTH] = "";

    //sending to the client to request for username 
    msg = "Enter Username:";
    send(clientStorage[clientData.clientId].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);//sending the message to prompt for username 

    while (true) {
        //checking if able to receive the message 
        if (recv(clientData.hClientSocket, msgBuffer, DEFAULT_BUFFER_LENGTH, 0) > 0) {
            clientData.clientMessage = msgBuffer;
            //user name not found in the vector
            if (std::find(connectedClients.begin(), connectedClients.end(), clientData.clientMessage) == connectedClients.end()) {
                connectedClients.emplace_back(clientData.clientMessage);
                clientData.isConnected = true; //user is connected
                msg = "[Welcome " + clientData.clientMessage + "!]";//welcome message 
                send(clientStorage[clientData.clientId].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);
                //broadcast User has joined
                msg = "[" + clientData.clientMessage + " joined]";
                //send to all the clients 
                for (int i{ 0 }; i < MAX_CLIENTS; ++i){
                    if (clientStorage[i].hClientSocket != INVALID_SOCKET && clientStorage[i].isConnected == true){
                        if (clientData.clientId != i) {
                            send(clientStorage[i].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);
                        }
                    }
                }
                break;
            }else{
                std::cin.clear();
                ZeroMemory(msgBuffer, DEFAULT_BUFFER_LENGTH);//clearing the message buffer
                msg = "[Username has already been used. Please enter another name.]";
                send(clientStorage[clientData.clientId].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);
            }
        }
    }

    //loop to check the messages 
    while (true) {
        ZeroMemory(msgBuffer, DEFAULT_BUFFER_LENGTH);//clearing the message buffer
        if (clientData.hClientSocket != 0){
            //checking if received message 
            if (recv(clientData.hClientSocket, msgBuffer, DEFAULT_BUFFER_LENGTH, 0) != SOCKET_ERROR){

                //check if client quits
                if (strcmp("@quit", msgBuffer) == 0){
                    clientData.isConnected = false;

                    msg = "["+clientData.clientMessage + " exited]";//telling the server client has exited 
                    std::cout << msg << std::endl;
                    closesocket(clientData.hClientSocket);
                    closesocket(clientStorage[clientData.clientId].hClientSocket);
                    clientStorage[clientData.clientId].hClientSocket = INVALID_SOCKET;

                    //Broadcast the disconnection message to the other clients
                    for (int i = 0; i < MAX_CLIENTS; i++){
                        if (clientStorage[i].hClientSocket != INVALID_SOCKET) {
                            if (clientData.clientId != i) {
                                send(clientStorage[i].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);//setting the broadcast  
                            }
                        }
                    }
                    connectedClients.erase(std::find(connectedClients.begin(), connectedClients.end(), clientData.clientMessage));//remove from the vector 
                    break;

                }else if (strcmp("@names", msgBuffer) == 0){
                    //show the connected users 
                    std::string tmpStr = "[Connected users: ";

                    //loop through all the connected users and add it 
                    for (auto it = connectedClients.begin(); it != connectedClients.end(); ++it){
                        if (it + 1 != connectedClients.end()) {
                            tmpStr += *it + ", ";
                        }else {
                            tmpStr += *it;
                        }
                    }
                    tmpStr += "]";

                    std::cout << tmpStr << std::endl;//printing out the connected users 
                    send(clientData.hClientSocket, tmpStr.c_str(), static_cast<int>(strlen(tmpStr.c_str())), 0); //sending th connected users 

                }else{
                    //printing message to the server console
                    if (strcmp("", msgBuffer)) {
                        msg = "[" + clientData.clientMessage + ":]" + msgBuffer;
                    }

                    std::cout << msg.c_str() << std::endl; // printing out message to the server console

                    //Broadcast that message to the other clients
                    for (int i = 0; i < MAX_CLIENTS; i++){
                        if (clientStorage[i].hClientSocket != INVALID_SOCKET && clientStorage[i].isConnected == true) {
                            //if same id 
                            if (clientData.clientId != i) {
                                send(clientStorage[i].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);
                            }
                        }
                    }
                }
            }else{
                //client disconnected 
                clientData.isConnected = false;
                msg = "Client Disconnected";

                std::cout << msg << std::endl;
                //closing the socket 
                closesocket(clientData.hClientSocket);
                closesocket(clientStorage[clientData.clientId].hClientSocket);
                clientStorage[clientData.clientId].hClientSocket = INVALID_SOCKET;

                //tell the other clients 
                for (int i = 0; i < MAX_CLIENTS; i++){
                    if (clientStorage[i].hClientSocket != INVALID_SOCKET) {
                       send(clientStorage[i].hClientSocket, msg.c_str(), static_cast<int>(strlen(msg.c_str())), 0);//sending the message 
                    }
                }
                connectedClients.erase(std::find(connectedClients.begin(), connectedClients.end(), clientData.clientMessage));//remove from the vector 
                break;
            }
        }
    }

    thread.detach();//detaching the thread 
    return 0;
}