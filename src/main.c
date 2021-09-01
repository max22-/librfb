#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "librfb.h"

const uint16_t port = 5900;
int sock;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

ssize_t send_callback(const void *data, size_t count)
{
    return write(sock, data, count);
}

int main(int argc, char *argv[])
{
    char buffer[1024];
    printf("remote-framebuffer\n");
    int lsock = socket(AF_INET, SOCK_STREAM, 0);
    if(lsock < 0)
        error("Error creating socket");

    int optval = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if(bind(lsock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        error("Bind error");

    if(listen(lsock, 1) < 0) 
        error("Listen error");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    librfb_init("librfb", 320, 240, send_callback);
    while(1) {
        sock = accept(lsock, (struct sockaddr*)&client_addr, &client_len);
        if(sock < 0)
            error("Accept error");
        printf("Client connected\n");
        librfb_reset();
        while(1) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = read(sock, buffer, sizeof(buffer));
            if(bytes <= 0)
                break;
            librfb_handle_data(buffer, bytes);
        }
        close(sock);
        printf("socket closed\n");
    }
    
    return 0;
}
