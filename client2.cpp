#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

#define PORT 8080
using namespace std;

void SendMessage(int client_fd) {
    cout << "Welcome to the chat system!\n";
    cout << "Enter your chat name: ";

    string name;
    getline(cin >> ws, name);

    cout << "Commands:\n";
    cout << "  /create 'group_name' - Create a new group and join it\n";
    cout << "  /join 'group_name' - Join an existing group\n";
    cout << "  /public - Switch to sharing with all clients\n";
    cout << "  /dm 'username' 'message' - Send a direct message to a user\n";
    cout << "Enter your choice: \n";
    cout << "Type 'hey' to check and then type your messages directly to send them.\n";

    while (true) {
        string message;
        getline(cin >> ws, message);

        // If the user types "quit", exit the program
        if (message == "quit") {
            cout << "Exiting the chat...\n";
            break;
        }

        // Prefix the name to the message
        string msg_to_send = name + ": " + message;

        // Send the message to the server
        int bytes_sent = send(client_fd, msg_to_send.c_str(), msg_to_send.length(), 0);
        if (bytes_sent < 0) {
            cerr << "Error sending message.\n";
            break;
        }
    }

    close(client_fd);
}

void ReceiveMessage(int client_fd) {
    char buffer[5012];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);

        // If the server disconnects or an error occurs
        if (bytes_received <= 0) {
            cerr << "Disconnected from server.\n";
            break;
        }

        // Display the message received from the server
        cout << "\n" << buffer << endl;
    }

    close(client_fd);
}

int main() {
    // Step 1: Create a socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        cerr << "Socket creation failed!\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported.\n";
        exit(EXIT_FAILURE);
    }

    // Step 2: Connect to the server
    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection failed.\n";
        exit(EXIT_FAILURE);
    }

    cout << "Connected to the server successfully.\n";

    // Step 3: Create threads for sending and receiving messages
    thread send_thread(SendMessage, client_fd);
    thread receive_thread(ReceiveMessage, client_fd);

    // Wait for both threads to finish
    send_thread.join();
    receive_thread.join();

    return 0;
}
