#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#define PORT 8080
using namespace std;

vector<int> clients; // List of all connected clients
unordered_map<string, int> client_names; // Maps client name to socket descriptor
unordered_map<string, vector<int>> groups; // Groups mapped to their members
mutex clients_mutex; // Mutex for clients list
mutex groups_mutex; // Mutex for groups
mutex client_names_mutex; // Mutex for client_names

// Add a global log file and a mutex for thread-safe logging
ofstream log_file("chat_history.log", ios::app);
mutex log_mutex;

// Helper function to get the current timestamp as a string
string GetTimestamp() {
    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Function to log a message to the file
void LogMessage(const string &message) {
    lock_guard<mutex> lock(log_mutex);
    log_file << "[" << GetTimestamp() << "] " << message << endl;
    log_file.flush(); // Ensure the message is written to the file immediately
}

void BroadcastMessage(const string &message, int sender_socket) {
    lock_guard<mutex> lock(clients_mutex);
    for (int client : clients) {
        if (client != sender_socket) { // Avoid sending the message back to the sender
            send(client, message.c_str(), message.length(), 0);
        }
    }
}

void SendToGroup(const string &group_name, const string &message, int sender_socket) {
    lock_guard<mutex> lock(groups_mutex);
    if (groups.find(group_name) != groups.end()) {
        for (int client : groups[group_name]) {
            if (client != sender_socket) { // Avoid sending the message back to the sender
                send(client, message.c_str(), message.length(), 0);
            }
        }
    }
}

void SendDM(const string &recipient_name, const string &message, int sender_socket) {
    lock_guard<mutex> lock(clients_mutex);
    if (client_names.find(recipient_name) != client_names.end()) {
        int recipient_socket = client_names[recipient_name];
        send(recipient_socket, message.c_str(), message.length(), 0); // Send the DM
    } else {
        string error_message = "User '" + recipient_name + "' is not online.\n";
        send(sender_socket, error_message.c_str(), error_message.length(), 0);
    }
}

void HandleClient(int client_socket) {
    char buffer[5012] = {0};
    string current_group = ""; // Stores the group the client is currently in
    bool public_mode = true; // Starts in public mode by default
    string client_name = ""; // To store the client’s name

    {
        lock_guard<mutex> lock(clients_mutex);
        clients.push_back(client_socket); // Add the new client to the list
    }

    // Step 1: Receive the client's name as the first message
    memset(buffer, 0, sizeof(buffer));
    int valread = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (valread <= 0) { // Client disconnected without sending a name
        cerr << "Client disconnected before providing a name.\n";
        close(client_socket);
        return;
    }
    buffer[valread] = '\0'; // Null-terminate the received data
    string client = string(buffer);
    size_t pos = client.find(":");
    client_name = client.substr(0, pos);

    {
        lock_guard<mutex> lock(client_names_mutex);
        if (client_names.find(client_name) == client_names.end()) {
            client_names[client_name] = client_socket; // Add the client to the map
            cout << "Client '" << client_name << "' connected.\n";

            // Log the connection
            LogMessage("Client '" + client_name + "' connected.");

            string welcome_message = "Welcome, " + client_name + "!\n";
            send(client_socket, welcome_message.c_str(), welcome_message.length(), 0);
        } else {
            cerr << "Client name '" << client_name << "' is already taken.\n";
            string error_message = "Error: Name already taken. Disconnecting.\n";
            send(client_socket, error_message.c_str(), error_message.length(), 0);
            close(client_socket);
            return;
        }
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (valread <= 0) { // Client disconnected
            cerr << "Client disconnected.\n";
            break;
        }

        buffer[valread] = '\0'; // Null-terminate the received data
        string msg(buffer);
        cout << "Message received: " << msg << endl;

        // Log the received message
        LogMessage(client_name + ": " + msg);

        size_t pos = msg.find(":");
        string message = msg.substr(pos + 2); // Extract message after the name
        if (message.rfind("/create", 0) == 0) { // Create group command
            string group_name = message.substr(8); // Extract group name
            {
                lock_guard<mutex> lock(groups_mutex);
                if (groups.find(group_name) == groups.end()) {
                    groups[group_name] = vector<int>(); // Create the group if it doesn't exist
                }
                groups[group_name].push_back(client_socket);
            }

            current_group = group_name;
            public_mode = false;
            string confirmation = "Group '" + group_name + "' created and joined.\n";
            send(client_socket, confirmation.c_str(), confirmation.length(), 0);

            // Log group creation
            LogMessage(client_name + " created and joined group '" + group_name + "'.");
        } else if (message.rfind("/join", 0) == 0) { // Join group command
            string group_name = message.substr(6); // Extract group name
            {
                lock_guard<mutex> lock(groups_mutex);
                if (groups.find(group_name) != groups.end()) {
                    groups[group_name].push_back(client_socket);
                    current_group = group_name;
                    public_mode = false;
                    string confirmation = "Joined group '" + group_name + "'.\n";
                    send(client_socket, confirmation.c_str(), confirmation.length(), 0);
                    // Log group join
                    LogMessage(client_name + " joined group '" + group_name + "'.");
                } else {
                    string error_message = "Group '" + group_name + "' does not exist.\n";
                    send(client_socket, error_message.c_str(), error_message.length(), 0);
                }
            }
        } else if (message == "/public") { // Switch to public mode
            current_group = "";
            public_mode = true;
            string confirmation = "Switched to public mode.\n";
            send(client_socket, confirmation.c_str(), confirmation.length(), 0);
            // Log mode switch
            LogMessage(client_name + " switched to public mode.");
        } else if (message.rfind("/dm", 0) == 0) { // Direct message command
            string message_content = message.substr(4);
            cout << message_content << endl;
            size_t space_pos = message_content.find(" ", 4); // Find the first space after "/dm"
            if (space_pos != string::npos) {
                string recipient_name = message_content.substr(0, space_pos); // Extract recipient name
                cout << recipient_name << endl;
                string dm_message = message_content.substr(space_pos + 1); // Extract the DM message
                cout << dm_message << endl;
                SendDM(recipient_name, client_name + ": " + dm_message, client_socket);
                // Log the DM
                LogMessage(client_name + " sent a DM to " + recipient_name + ": " + dm_message);
            }
        } else { // Normal message
            if (public_mode) {
                BroadcastMessage(msg, client_socket);
                // Log the broadcast
                LogMessage(client_name + " broadcasted: " + message);
            } else if (!current_group.empty()) {
                SendToGroup(current_group, msg, client_socket);
                // Log the group message
                LogMessage(client_name + " to group '" + current_group + "': " + message);
            } else {
                string error_message = "You are not in any group. Use /public or /create or /join to set a mode.\n";
                send(client_socket, error_message.c_str(), error_message.length(), 0);
            }
        }
    }
    // Log client disconnection
    LogMessage("Client '" + client_name + "' disconnected.");

    // Remove client from global lists on disconnect
    {
        lock_guard<mutex> lock(clients_mutex);
        clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end());
        client_names.erase(client_name); // Remove client from the map
    }
    {
        lock_guard<mutex> lock(groups_mutex);
        for (auto &[group_name, group_clients] : groups) {
            group_clients.erase(remove(group_clients.begin(), group_clients.end(), client_socket), group_clients.end());
        }
    }
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server is running on port " << PORT << "...\n";

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Start a new thread for each client
        thread client_thread(HandleClient, new_socket);
        client_thread.detach();
    }

    return 0;
}
