#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <iostream>

#define PORT 8080

class ClientHandler
{
public:
    explicit ClientHandler(int server_fd, struct sockaddr_in server_address) : server_fd_(server_fd), server_address_(server_address)
    {
    }
    ~ClientHandler()
    {
    }

    void accept_thread()
    {
        static bool is_active = false;
        if (is_active == false)
        {
            std::thread(&ClientHandler::accept, this).detach();
            is_active = true;
        }
    }
    void accept()
    {
        for (;;)
        {
            // The accept() call actually accepts an incoming connection
            struct sockaddr_in client_address;
            int client_addrlen = sizeof(client_address);
            int client_socket;

            // This accept() function will write the connecting client's address info
            // into the the address structure and the size of that structure is clilen.
            // The accept() returns a new socket file descriptor for the accepted connection.
            // So, the original socket file descriptor can continue to be used
            // for accepting new connections while the new socker file descriptor is used for
            // communicating with the connected client.
            // If no pending connections are present on the queue, and the socket is not marked as nonblocking,
            // accept() blocks the caller until a connection is present
            if ((client_socket = ::accept(server_fd_, (struct sockaddr *)&client_address,
                                          (socklen_t *)&client_addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // ClientInfo client_info(client_socket, client_address);
            std::lock_guard<std::mutex> lg(mutex_);
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = (int)ntohs(client_address.sin_port);
            printf("connection opened with client: %s:%d\n", client_ip, client_port);
            database.insert(std::make_pair(client_socket, ClientInfo(client_socket, client_address)));
        }
    }

    void printDatabase()
    {
        std::cout << "Active connections: " << database.size()
                  << "\n";
        for (const auto &client : database)
        {
            const auto &adress = client.second.getAdress();
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(adress.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = (int)ntohs(adress.sin_port);
            printf("client: %s:%d\n", client_ip, client_port);
        }
    }

private:
    int server_fd_;
    struct sockaddr_in server_address_;
    std::mutex mutex_;

    class ClientInfo
    {
    public:
        explicit ClientInfo(int client_socket, struct sockaddr_in client_address) : client_socket_(client_socket), client_address_(client_address) {}
        ClientInfo(const ClientInfo &source) = delete;
        ClientInfo(ClientInfo &&source)
        {
            this->client_socket_ = source.client_socket_;
            this->client_address_ = source.client_address_;
            this->seq1_ = source.seq1_;
            this->seq2_ = source.seq2_;
            this->seq3_ = source.seq3_;

            source.client_socket_ = -1;
        }
        ~ClientInfo()
        {
            if (client_socket_ > 0)
            {
                close(client_socket_);
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_address_.sin_addr), client_ip, INET_ADDRSTRLEN);
                int client_port = (int)ntohs(client_address_.sin_port);
                printf("connection closed with client: %s:%d\n", client_ip, client_port);
            }
        }
        struct sockaddr_in getAdress() const { return client_address_; }

    private:
        int client_socket_ = -1;
        struct sockaddr_in client_address_;
        std::string seq1_;
        std::string seq2_;
        std::string seq3_;
    };

    using client_socket = int;
    std::unordered_map<client_socket, ClientInfo> database;
};

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

    // for (;;)
    // {
    //     // The accept() call actually accepts an incoming connection
    //     struct sockaddr_in client_address;
    //     int client_addrlen = sizeof(client_address);
    //     int new_socket;

    //     // This accept() function will write the connecting client's address info
    //     // into the the address structure and the size of that structure is clilen.
    //     // The accept() returns a new socket file descriptor for the accepted connection.
    //     // So, the original socket file descriptor can continue to be used
    //     // for accepting new connections while the new socker file descriptor is used for
    //     // communicating with the connected client.
    //     // If no pending connections are present on the queue, and the socket is not marked as nonblocking,
    //     // accept() blocks the caller until a connection is present
    //     if ((new_socket = accept(server_fd, (struct sockaddr *)&client_address,
    //                              (socklen_t *)&client_addrlen)) < 0)
    //     {
    //         perror("accept");
    //         exit(EXIT_FAILURE);
    //     }
    //     char client_ip[INET_ADDRSTRLEN];
    //     inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    //     int client_port = (int)ntohs(client_address.sin_port);

    //     printf("connection opened with client: %s:%d\n", client_ip, client_port);
    //     for (;;)
    //     {
    //         // reset buffer
    //         memset(buffer, 0, 1024);
    //         // read is blocking call
    //         int valread = read(new_socket, buffer, 1024);
    //         if (valread <= 0)
    //         {
    //             break;
    //         }
    //         printf("%s\n", buffer);
    //         send(new_socket, hello, strlen(hello), 0);
    //         printf("Hello message sent\n");
    //     }
    //     // closing the connected socket
    //     close(new_socket);
    //     printf("connection closed with client: %s:%d\n", client_ip, client_port);
    // }
    ClientHandler client_handler(server_fd, server_address);
    client_handler.accept_thread();
    for (;;)
    {
        std::string command;
        std::cin >> command;
        if (command == "print")
        {
            client_handler.printDatabase();
        }
    }

    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
