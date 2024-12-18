#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
using namespace std;

int main()
{

    // step 1: socket creation
    int client_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        cerr << "Socket creation failed!"<<endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    //coversion into binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<= 0) {
            cerr<<"Invalid address/ Address not supported \n";
            exit(EXIT_FAILURE);
    }

    // step 2: connect
    int status;
    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr)))< 0) {
        cerr<<"Connection Failed \n";
        exit(EXIT_FAILURE);
    }

    cout<<"connected to server successfully "<<endl;

    int bytessent= send(client_fd, "hello from client", strlen("hello from client"), 0);
    cout<<"Hello message sent\n";

    int valread;
    char buffer[5012] = { 0 };
    valread = read(client_fd, buffer, 5012 - 1); // subtract 1 for the null
                                                 // terminator at the end
    cout<<"message from server: "<<buffer<<endl;

    // closing the connected socket
    close(client_fd);
    return 0;

}