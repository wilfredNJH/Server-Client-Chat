/*!*************************************************************************
****
\file MinChatClient.cpp
\author Wilfred Ng Jun Hwee
\par DP email: junhweewilfred.ng@digipen.edu
\par Course: csd2160
\par Assignment #2
\date 02-25-2021

\brief
This program is the simulation of a chat client 

****************************************************************************
***/
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

#define DEFAULT_BUFFER_LENGTH 512            

std::string szServerIPAddr;                 // IP address of the server
std::string nServerPort;                    // The server port that will be used by            

//stores the client information 
struct CLIENT_INFO
{
    SOCKET hClientSocket;//stores the client socket 
    int clientId;//stores the client id 
    char msgBuffer[DEFAULT_BUFFER_LENGTH];//stores the message buffer
};

int client_thread(CLIENT_INFO& clientData);
int main();
bool InitWinSock2_0();//for initializing win sock 

/*********************
\fn int client_thread(CLIENT_INFO& clientData)
\brief running the thread for reading data 
\param clientData - pointer to the client data 
\return 0 when thread has ended 
*********************/
int client_thread(CLIENT_INFO& clientData){
    //loop through 
    while (true){
        ZeroMemory(clientData.msgBuffer, DEFAULT_BUFFER_LENGTH);//clearing the buffer 

        //checking whether the client socket is available 
        if (clientData.hClientSocket != 0){
            //receives the message from the 
            if (recv(clientData.hClientSocket, clientData.msgBuffer, DEFAULT_BUFFER_LENGTH, 0) != SOCKET_ERROR){
                std::cout << clientData.msgBuffer << std::endl;
            }else{
                std::cout << "Error reading data " << std::endl;
                break;
            }
        }
    }

    //if the server disconnected 
    if (WSAGetLastError() == WSAECONNRESET) {
        std::cout << "Server has disconnected" << std::endl;
    }

    return 0;
}

//main entry point 
int main()
{
    struct addrinfo* result = NULL;
    struct addrinfo* serverAddr = NULL;
    CLIENT_INFO client = { INVALID_SOCKET, -1, "" };

    //getting the sever ip address & port number 
    std::cout << "Enter the server IP Address: ";
    std::getline(std::cin, szServerIPAddr);
    std::cout << "Enter the server port number: ";
    std::getline(std::cin, nServerPort);

    //init winsock2
    if (!InitWinSock2_0())
    {
        std::cout << "Unable to Initialize Windows Socket environment" << WSAGetLastError() << std::endl;
        return -1;
    }

    //setting the socket protocols
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;              // The address family. AF_INET specifies TCP/IP
    hints.ai_socktype = SOCK_STREAM;        // Protocol type. SOCK_STREM specified TCP
    hints.ai_protocol = 0;                  // Protoco Name. Should be 0 for AF_INET address family

    //getting the address sever and the port number 
    if (getaddrinfo(static_cast<PCSTR>(szServerIPAddr.c_str()), static_cast<PCSTR>(nServerPort.c_str()), &hints, &result)) {
        std::cout << "failed to get address info of the sever: " << std::endl;
        WSACleanup();
        return -1;
    }

    //attempting to connect to sever 
    serverAddr = result;
    //creating the socket 
    client.hClientSocket = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol);

    //checking if the socket is valid 
    if (client.hClientSocket == INVALID_SOCKET) {
        std::cout << "Unable to create Server socket" << std::endl;
        // Cleanup the environment initialized by WSAStartup()
        WSACleanup();
        std::system("pause");
        return -1;
    }

    // Connect to server
    if (connect(client.hClientSocket, serverAddr->ai_addr, (int)serverAddr->ai_addrlen) < 0) {
        std::cout << "Unable to connect to " << szServerIPAddr << " on port " << nServerPort << std::endl;
        closesocket(client.hClientSocket);
        WSACleanup();
        return -1;
    }

    freeaddrinfo(result);//freeing the addr info 

    //check if the client socket is correct 
    if (client.hClientSocket == INVALID_SOCKET) {
        std::cout << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return -1;
    }

    //getting the message from the client 
    recv(client.hClientSocket, client.msgBuffer, DEFAULT_BUFFER_LENGTH, 0);
    std::string serverResultMessage = client.msgBuffer;

    //checking if the server is full 
    if (serverResultMessage != "Server full"){
        //getting the client id 
        client.clientId = atoi(client.msgBuffer);

        //creating the thread 
        std::thread my_thread(client_thread, std::ref(client));

        std::string clientMessage;//the client message 

        //client loop
        while (true){

            //getting the client's message 
            getline(std::cin, clientMessage);

            if (send(client.hClientSocket, clientMessage.c_str(), strlen(clientMessage.c_str()), 0) <= 0){
                std::cout << "Error sending the data to server" << std::endl;
                break;
            }
            //Quit typed, break out of the loop
            if (strcmp(clientMessage.c_str(), "@quit") == 0){
                break;
            }

            std::cin.clear();
        }

        //detaching the thread 
        my_thread.detach();
    }else {
        std::cout << client.msgBuffer << std::endl;
    }

    std::cout << "Closing connection!" << std::endl;
    //shutting down the socket 
    if (shutdown(client.hClientSocket, SD_BOTH) < 0) {
        std::cout << "failed to shutdown the socket error : " << WSAGetLastError() << std::endl;
        closesocket(client.hClientSocket);
        WSACleanup();
        return -1;
    }

    closesocket(client.hClientSocket);
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