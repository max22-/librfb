#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "state_machine.h"

const uint16_t port = 5900;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
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
    while(1) {
        int sock = accept(lsock, (struct sockaddr*)&client_addr, &client_len);
        if(sock < 0)
            error("Accept error");
        printf("Client connected\n");
        if(!state_machine(sock, sme_init())) {
            close(sock);
            continue;
        }
        char buffer[1024];
        while(1) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = read(sock, buffer, sizeof(buffer));
            if(bytes <= 0)
                break;
            state_machine(sock, sme_data(buffer, bytes));
        }
        close(sock);
        printf("socket closed\n");
    }
    
    return 0;
}
