/*
	Live Server on port 8888
*/
#include<io.h>
#include<stdio.h>
#include<winsock2.h>
#include <windows.h>
#define BUFFER_SIZE 1024
#pragma comment(lib,"ws2_32.lib") //Winsock Library

void handleFunc(SOCKET socket);

int main(int argc , char *argv[])
{
    WSADATA wsa;
    SOCKET s , new_socket;
    struct sockaddr_in server , client;

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }

    printf("Socket created.\n");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    //Bind
    if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    puts("Bind done");

    //Listen to incoming connections
    listen(s , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");

    int c = sizeof(struct sockaddr_in);

    while( (new_socket = accept(s , (struct sockaddr *)&client, &c)) != INVALID_SOCKET )
    {
        puts("Connection accepted");
        //pass the function to the relevant socket
        handleFunc(new_socket);
    }

    if (new_socket == INVALID_SOCKET)
    {
        printf("accept failed with error code : %d" , WSAGetLastError());
        return 1;
    }

    closesocket(s);
    WSACleanup();

    return 0;
}

void handleFunc(SOCKET socket){
    char buffer[BUFFER_SIZE];
    char request[200] = "";
    char path[100] = "./webroot";
    char filename[50] = "";
    int searchChar = '/';
    char * filenameStart;

    // parse the request, receive up until the carriage return, so we get the full request GET /filename HTTP 1.1 etc
    while(strchr(request,13) == NULL) {
        recv(socket, buffer, sizeof(request), 0);
        strcat(request ,buffer);
    }
    //Goes to the first "/" in the response since the response is GET /filename
    filenameStart = strchr(request, searchChar);
    // saves until the next space in the filename variable since nothing but the file name/path will be in there.
    // the format is GET /filename HTTP 1.1, so it will take from after the '/' up until the next space
    strcpy(filename, strtok(filenameStart, " "));

    printf("Filename: %s\n", filename);
    printf("request: %s\n", request);

    // generates the path to the file by concatenating the filename onto our known path to the webroot folder
    strcat(path,filename);
    printf("path: %s\n", path);

    printf("sending data...\n");

    // open the file we want to read data from
    FILE * fp;
    fp = fopen(path, "r");

    // this header must be sent first if we want to communicate with the webserver
    char good_header[] = "HTTP/1.1 200 OK\r\ncontent-type: text/html; charset=UTF-8\r\ncontent-disposition: inline\r\n\r\n\0";
    send(socket, good_header, sizeof (good_header), 0);
    // new buffer called message will send 1 character at a time since the test message is so small
    char message[1];
    // while we have not reached the end of that file
    while(!feof(fp)){
        // read data into the buffer
        fread(message, sizeof(message), 1, fp);
        // send the data from the file to the user's browser to be displayed
        send(socket, message, sizeof(message), 0);
    }
    fclose(fp);

    printf("data sent!\n");
    // if the socket closes too fast, only part of the text file is shown which is not ideal
    Sleep(3000);
    closesocket(socket);
}