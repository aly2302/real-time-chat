#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

SOCKET client_socket;

// Thread function to receive messages
unsigned __stdcall receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("\nServer disconnected\n");
            exit(1);
        }
        buffer[bytes_received] = '\0';
        printf("\n%s", buffer);
        printf("Enter message: ");
    }
    return 0;
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    HANDLE receive_thread;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Get username from user
    printf("Enter your username: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

    // Send username to server
    send(client_socket, buffer, strlen(buffer), 0);

    printf("Connected to server as %s\n", buffer);

    // Start receive thread
    receive_thread = (HANDLE)_beginthreadex(NULL, 0, receive_messages, NULL, 0, NULL);
    if (receive_thread == NULL) {
        printf("Thread creation failed\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Send messages
    while (1) {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

        // Send message to server
        if (send(client_socket, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
            printf("Send failed\n");
            break;
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}