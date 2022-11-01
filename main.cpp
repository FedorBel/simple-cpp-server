#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

int main(int argc, char const *argv[])
{
    char buffer[1024] = {0};
    char *hello = "Hello from server\n";

    int server_fd;
    int opt = 1;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /* setup the host_addr structure for use in bind call */
    struct sockaddr_in server_address;
    // automatically be filled with current host's IP address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    // Here, we set the maximum size for the backlog queue to 5.
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("server listen on: 127.0.0.1:%d\n", PORT);

    for (;;)
    {
        // The accept() call actually accepts an incoming connection
        struct sockaddr_in client_address;
        int client_addrlen = sizeof(client_address);
        int new_socket;

        // This accept() function will write the connecting client's address info
        // into the the address structure and the size of that structure is clilen.
        // The accept() returns a new socket file descriptor for the accepted connection.
        // So, the original socket file descriptor can continue to be used
        // for accepting new connections while the new socker file descriptor is used for
        // communicating with the connected client.
        // If no pending connections are present on the queue, and the socket is not marked as nonblocking,
        // accept() blocks the caller until a connection is present
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_address,
                                 (socklen_t *)&client_addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = (int)ntohs(client_address.sin_port);

        printf("connection opened with client: %s:%d\n", client_ip, client_port);
        for (;;)
        {
            // reset buffer
            memset(buffer, 0, 1024);
            // read is blocking call
            int valread = read(new_socket, buffer, 1024);
            if (valread <= 0)
            {
                break;
            }
            printf("%s\n", buffer);
            send(new_socket, hello, strlen(hello), 0);
            printf("Hello message sent\n");
        }
        // closing the connected socket
        close(new_socket);
        printf("connection closed with client: %s:%d\n", client_ip, client_port);
    }
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
