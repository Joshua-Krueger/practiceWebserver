/*
	Live Server on port 8888
*/
#include<io.h>
#include<stdio.h>
#include<winsock2.h>
#include<pthread.h>
#include <windows.h>
#define BUFFER_SIZE 1024
#pragma comment(lib,"ws2_32.lib") //Winsock Library

typedef struct cacheRecord{
    char *filename;
    char *data;
    int dataLen;
    time_t expiration;
}cacheRecord;

// the cache was stated in the project description to only hold a maximum of five items
cacheRecord cache[5];

void * handleFunc(void * response_sock);
struct cacheRecord createNewRecord(char* filename, char *data, int datalen);
cacheRecord * getFile(char* filename);

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
        //pass the function to the relevant socket in a thread
        pthread_t thread;
        SOCKET * pass_socket = (SOCKET *) malloc(sizeof(SOCKET));
        *pass_socket = new_socket;
        pthread_create(&thread, NULL, handleFunc, (void *)pass_socket);
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

void * handleFunc(void * response_sock){
    // ensure that the socket makes it through and is dereferenced
    Sleep(1000);
    SOCKET socket = *(SOCKET*) response_sock;
    free(response_sock);

    printf("connection received in thread\n");

    char buffer[BUFFER_SIZE];
    char request[200] = "";
    char filename[50] = "";
    int searchChar = '/';
    char * filenameStart;


    // parse the request, receive up until the carriage return, so we get the full request GET /filename HTTP 1.1 etc
    while(strchr(request,13) == NULL) {
        recv(socket, buffer, sizeof(request), 0);
        strcat(request ,buffer);
    }
    // Goes to the first "/" in the response since the response is GET /filename
    filenameStart = strchr(request, searchChar);
    // saves until the next space in the filename variable since nothing but the file name/path will be in there.
    // the format is GET /filename HTTP 1.1, so it will take from after the '/' up until the next space
    strcpy(filename, strtok(filenameStart, " "));

    printf("Filename: %s\n", filename);
    printf("request: %s\n", request);

    // these headers are required for sending HTTP 1.1 responses
    char bad_header[] = "HTTP/1.1 404 Not Found\r\n\0";
    char good_header[] = "HTTP/1.1 200 OK\r\ncontent-type: text/html; charset=UTF-8\r\ncontent-disposition: inline\r\n\r\n\0";
    if (getFile(filename) != NULL) {
        send(socket, good_header, strlen(good_header), 0);
        send(socket, getFile(filename)->data, getFile(filename)->dataLen, 0);
    } else {
        // send this only if the file isn't on the disk
        send(socket, bad_header, strlen(bad_header), 0);
    }
    // if the socket closes too fast, only part of the text file is shown which is not ideal
    Sleep(3000);
    closesocket(socket);
}

cacheRecord* getFile(char* filename){
    char path[50] = "./webroot/";
    int indexToReplace;
    boolean isExpired = FALSE;
    cacheRecord* file = NULL;

    // loop through each element in the cache
    for(int i = 0; i < 5; i++){
        // if there is a file there and that file matches my file
        if(cache[i].filename != NULL && strcmp(cache[i].filename,filename) == 0){
            printf("file found!\n");
            printf("file expiration time: %lld\n", cache[i].expiration);
            printf("current time: %lld\n", time(NULL));
            // check the expiration for the file to see if it should be replaced
            if (cache[i].expiration < time(NULL)){
                // if it is expired, clean out that file so it gets replaced below
                cache[i].filename = NULL;
                cache[i].data = NULL;
                // the file we are looking for is expired
                isExpired = TRUE;
                // this is where we need to replace it, save for later
                indexToReplace = i;
                break;
            }else {
                // if the file isn't expired
                printf("file is not expired\n");
                // take the location of that item and save it
                file = &cache[i];
                break;
            }
        }
    }
    // if there was no file found, or it was expired
    if (file == NULL){
        printf("file is expired or was not found in cache\n");
        // find and open the desired file for reading
        strcat(path,filename);
        FILE* fp = fopen(path, "r");
        // if we find the file
        if (fp != NULL){
            printf("found file\n");
            // go to the end of the file
            fseek(fp, 0, SEEK_END);
            // count each byte in the file to know how big it is
            int size = ftell(fp);
            // go back to the start of the file
            rewind(fp);
            // read the content of the file into data
            char* data = (char*)malloc(sizeof(char)*size);
            fread(data, sizeof(char), size, fp);
            fclose(fp);

            printf("adding file to cache\n");
            // create a new cacheRecord based on the information gained above
            cacheRecord newFile = createNewRecord(filename, data, size);

            // if expired, replace the old file
            if (isExpired){
                printf("replacing index %d\n", indexToReplace);
                cache[indexToReplace] = newFile;
                file = &cache[indexToReplace];
            }else{
                // if not expired, look for a space that has a NULL filename
                for (int i = 0; i < 5; i++) {
                    printf("looping...\n");
                    if (cache[i].filename == NULL){
                        indexToReplace = i;
                        i = 5;
                    }
                }
                // replace the empty slot
                printf("replacing index %d\n", indexToReplace);
                cache[indexToReplace] = newFile;
                file = &cache[indexToReplace];
            }
        }
    }
    return file;
}

// basic object build function
struct cacheRecord createNewRecord(char*filename, char *data, int dataLen){
    struct cacheRecord newFile = *(cacheRecord*)malloc(sizeof(cacheRecord));
    newFile.filename = strdup(filename);
    newFile.data = strdup(data);
    newFile.dataLen = dataLen;
    // set expiration for 5 seconds after creation
    newFile.expiration = time(NULL)+5;
    return newFile;
}