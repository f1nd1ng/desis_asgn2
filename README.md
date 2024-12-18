# desis_assgn2

###### Realtime Chat System

### Overview

This project is a multithreaded chat server written in C++ using the socket programming interface and mutithreading. 
It supports public broadcasts, private messaging, and group messaging. The server is designed to handle multiple clients simultaneously and logs all chat activity.

### Features

    -Public Messaging: Messages sent in public mode are broadcasted to all connected clients.
    -Direct Messaging (DM): Clients can send private messages to specific users.
    -Group Messaging:
      ->Create new groups.
      ->Join existing groups.
      ->Send messages to all members of a group.
    -Thread-Safe Logging: All chat activity is logged to a file chat_history.log.

### How it Works

    -Client Connection:
    When a client connects, they must provide a unique username as their first message.
    If the username is already in use, the client is disconnected.
    -Commands Used for
      -Public Mode:
        ->Default mode.
        ->Messages are broadcast to all connected clients.
      -Direct Messaging (DM):
        ->Syntax: /dm <username> <message>
        ->Sends a private message to the specified user.
      -Group Commands:
        ->/create <group_name>: Creates a new group and joins it.
        ->/join <group_name>: Joins an existing group.
        ->Messages in group mode are only sent to members of the group.
      -Switching to Public Mode:
        ->/public: Switches back to public mode.


### Technologies Used

- **Programming Language:** C++
- **Data Structures:** Unordered maps and vectors for efficient management.

#### Installation

1.Build the server:
    
       g++ -std=c++11 -pthread server.cpp -o server

2.Run the server:

       ./server

3. Build the client1:

       g++ -std=c++11 -pthread client1.cpp -o client2
   
4. Run the client1:
   
       ./client1

5. Build the client2:

       g++ -std=c++11 -pthread client1.cpp -o client2
   
6. Run the client2:
   
       ./client1

7. Build the client3:

       g++ -std=c++11 -pthread client1.cpp -o client2
   
8. Run the client3:
   
       ./client1

9. Build the client4:

       g++ -std=c++11 -pthread client1.cpp -o client2
   
10. Run the client4:
   
        ./client1

11. Build the client5:

        g++ -std=c++11 -pthread client1.cpp -o client2
   
12. Run the client5:
   
        ./client1

