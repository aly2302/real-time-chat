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

typedef struct {
    SOCKET socket;
    char username[BUFFER_SIZE];
} Client;

Client clients[MAX_CLIENTS];
HANDLE mutex;

// Function to handle communication with a client
unsigned __stdcall handle_client(void *arg) {
    int client_index = *(int *)arg;
    SOCKET client_socket = clients[client_index].socket;
    char buffer[BUFFER_SIZE];
    int username_set = 0; // Flag to track if username is set

    // Receive username as the first message
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        printf("Client disconnected before setting username\n");
        goto cleanup;
    }
    buffer[bytes_received] = '\0';

    // Store the username
    strncpy(clients[client_index].username, buffer, BUFFER_SIZE);
    username_set = 1;

    // Notify all clients about the new user
    char join_msg[BUFFER_SIZE + 50];
    snprintf(join_msg, sizeof(join_msg), "[Server] %s joined the chat!", clients[client_index].username);
    WaitForSingleObject(mutex, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && i != client_index) {
            send(clients[i].socket, join_msg, strlen(join_msg), 0);
        }
    }
    ReleaseMutex(mutex);

    printf("%s connected\n", clients[client_index].username);

    // Handle subsequent messages
    while (1) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("%s disconnected\n", clients[client_index].username);
            break;
        }
        buffer[bytes_received] = '\0';

        // Format message as "[username]: message"
        char formatted_msg[BUFFER_SIZE + 50];
        snprintf(formatted_msg, sizeof(formatted_msg), "[%s]: %s", clients[client_index].username, buffer);

        // Broadcast the formatted message
        WaitForSingleObject(mutex, INFINITE);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != 0 && i != client_index) {
                send(clients[i].socket, formatted_msg, strlen(formatted_msg), 0);
            }
        }
        ReleaseMutex(mutex);

        printf("%s\n", formatted_msg);
    }

cleanup:
    // Remove client and notify others
    WaitForSingleObject(mutex, INFINITE);
    if (username_set) {
        char leave_msg[BUFFER_SIZE + 50];
        snprintf(leave_msg, sizeof(leave_msg), "[Server] %s left the chat.", clients[client_index].username);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != 0 && i != client_index) {
                send(clients[i].socket, leave_msg, strlen(leave_msg), 0);
            }
        }
    }
    clients[client_index].socket = 0;
    clients[client_index].username[0] = '\0';
    ReleaseMutex(mutex);

    closesocket(client_socket);
    free(arg); // Free the client index pointer
    _endthreadex(0);
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr;
    int addr_len = sizeof(struct sockaddr_in);

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
        SOCKET new_socket = accept(server_socket, (struct sockaddr *)&server_addr, &addr_len);
        if (new_socket == INVALID_SOCKET) {
            printf("Accept failed\n");
            continue;
        }

        // Find an empty slot for the new client
        int *client_index = malloc(sizeof(int));
        *client_index = -1;
        WaitForSingleObject(mutex, INFINITE);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = new_socket;
                *client_index = i;
                break;
            }
        }
        ReleaseMutex(mutex);

        if (*client_index == -1) {
            printf("Server full. Connection rejected.\n");
            closesocket(new_socket);
            free(client_index);
            continue;
        }

        // Create a thread for the new client
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, handle_client, client_index, 0, NULL);
        if (thread == NULL) {
            printf("Thread creation failed\n");
            closesocket(new_socket);
            free(client_index);
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}