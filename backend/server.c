#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

SOCKET client_sockets[MAX_CLIENTS]; // Array to store client sockets
HANDLE mutex; // Mutex for thread safety

// Function to handle communication with a client
unsigned __stdcall handle_client(void *arg) {
    SOCKET client_socket = *(SOCKET *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        // Receive message from the client
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // Client disconnected
            break;
        }
        buffer[bytes_received] = '\0'; // Null-terminate the message

        // Broadcast the message to all clients
        WaitForSingleObject(mutex, INFINITE);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != 0 && client_sockets[i] != client_socket) {
                send(client_sockets[i], buffer, strlen(buffer), 0);
            }
        }
        ReleaseMutex(mutex);
    }

    // Remove the client from the list
    WaitForSingleObject(mutex, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0;
            break;
        }
    }
    ReleaseMutex(mutex);

    closesocket(client_socket); // Close the client socket
    _endthreadex(0); // Exit the thread
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    HANDLE thread;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server started on port %d\n", PORT);

    // Create a mutex for thread safety
    mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex == NULL) {
        printf("Mutex creation failed\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Accept incoming connections
    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket == INVALID_SOCKET) {
            printf("Accept failed\n");
            continue;
        }

        // Add new client to the list
        WaitForSingleObject(mutex, INFINITE);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;
                break;
            }
        }
        ReleaseMutex(mutex);

        // Create a thread for the new client
        thread = (HANDLE)_beginthreadex(NULL, 0, handle_client, &new_socket, 0, NULL);
        if (thread == NULL) {
            printf("Thread creation failed\n");
            closesocket(new_socket);
        }
    }

    closesocket(server_socket); // Close the server socket
    WSACleanup(); // Cleanup Winsock
    return 0;
}