#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "librfb.h"

static const uint16_t w = 320, h = 240;
static const uint16_t port = 5900;
static int sock;
static uint16_t mouse_x = 0, mouse_y = 0;

static void send_frame();

static void error(const char *msg)
{
    perror(msg);
    exit(1);
}

static ssize_t send_callback(const void *data, size_t count)
{
    return write(sock, data, count);
}

static void mouse_callback(uint16_t x, uint16_t y, uint8_t button_mask)
{
    mouse_x = x;
    mouse_y = y;
    if(button_mask)
        printf("click at %d, %d\n", x, y);
    send_frame();
}

static void frame_request_callback(uint16_t xpos, uint16_t ypos, uint16_t w, uint16_t h)
{
    send_frame();
}

static void send_frame()
{
    uint32_t buffer[w];
    const uint32_t 
        green = LIBRFB_RGBA(0, 255, 0, 0), 
        black = LIBRFB_RGBA(0, 0, 0, 0);
    librfb_send_frame_header(0, 0, w, h);
    for(uint16_t y = 0; y < h; y++) {
        for(uint16_t x = 0; x < w; x++) {
            if(y < mouse_y)
                buffer[x] = green;
            else
                buffer[x] = black;
        }
        send_callback(buffer, sizeof(buffer));
    }
}

int main(int argc, char *argv[])
{
    char buffer[1024];
    printf("librfb example\n");
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
    librfb_init("librfb", w, h, send_callback);
    librfb_set_frame_request_callback(frame_request_callback);
    librfb_set_mouse_callback(mouse_callback);
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
