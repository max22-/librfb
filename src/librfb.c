#warning remove stdio include later
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "librfb.h"

static uint16_t width = 0, height = 0;
static char server_name[256] = {0};
static send_callback_t send_callback = NULL;
static keyboard_callback_t keyboard_callback = NULL;
static mouse_callback_t mouse_callback = NULL;

typedef enum {
    SEND_PROTOCOL_VERSION,
    RECEIVE_PROTOCOL_VERSION,
    SEND_SECURITY_RESULT,
    SEND_SERVER_INIT,
    SEND_STUFF
} state_t;

static state_t state = SEND_PROTOCOL_VERSION;

/* messages sent to the client */

static const char protocol_version[] = "RFB 003.008\n";
static const char security_types[] = {1, 1}; // 1 security type, no authentication
static uint32_t security_result = 0;   // result : we always send OK

typedef struct {
    uint8_t bits_per_pixel;
    uint8_t depth;
    uint8_t big_endian_flag;
    uint8_t true_color_flag;
    uint16_t red_max, green_max, blue_max;
    uint8_t red_shift, green_shift, blue_shift;
    uint8_t padding[3];
} pixel_format_t;

typedef struct {
    uint16_t frame_buffer_width;
    uint16_t frame_buffer_height;
    pixel_format_t pixel_format;
} server_init_t;

typedef struct {
    uint8_t message_type;
    uint8_t padding;
    uint16_t number_of_rectangles;
} frame_buffer_update_t;

typedef struct {
    uint16_t x, y, w, h;
    int32_t encoding_type;
} rectangle_t;


/* Messages received from the client */

typedef struct {
    uint8_t message_type;
    uint8_t incremental;
    uint16_t xpos, ypos, width, height;
} framebuffer_update_request_t;

typedef struct {
    uint8_t message_type;
    uint8_t button_mask;
    uint16_t x, y;
} pointer_event_t;

typedef struct {
    uint8_t message_type;
    uint8_t down_flag;
    uint16_t padding;
    uint32_t key;
} key_event_t;

static uint16_t x_mouse=0, y_mouse=0;

static ssize_t send_bytes(const void* data, size_t count)
{
    if(send_callback != NULL)
        return send_callback(data, count);
    else
        return -1;
}

static void send_frame(uint16_t xpos, uint16_t ypos, uint16_t w, uint16_t h)
{
    static int i = 0;
    printf("%d send_frame(xpos=%d, ypos=%d, w=%d, h=%d)\n", i++, xpos, ypos, w, h);
    frame_buffer_update_t frame_buffer_update = {0, 0, htons(1)};
    rectangle_t rectangle = {htons(0), htons(0), htons(w), htons(h), htonl(0)};
    send_bytes(&frame_buffer_update, sizeof(frame_buffer_update));
    send_bytes(&rectangle, sizeof(rectangle));
    for(uint16_t y = 0; y < h; y++) {
        for(uint16_t x = 0; x < w; x++) {
            uint8_t red = 0;//255*(xpos + x <= x_mouse && ypos + y <= y_mouse);
            uint8_t green = 255 * (y < y_mouse);
            uint8_t blue = 0;
            uint8_t alpha = 0;
            uint32_t color = htonl(blue << 24 | green << 16 | red << 8 | alpha);
            send_bytes(&color, sizeof(color));
        }
    }
    printf("end send_frame()\n");
}

void librfb_reset()
{
    state = SEND_PROTOCOL_VERSION;
    librfb_handle_data(NULL, 0);
}

void librfb_init(const char* name, int w, int h, send_callback_t callback)
{
    strncpy(server_name, name, sizeof(server_name) - 1);
    server_name[sizeof(server_name) - 1] = 0;
    width = w;
    height = w;
    send_callback = callback;
}

void librfb_set_keyboard_callback(keyboard_callback_t callback)
{
    keyboard_callback = callback;
}

void librfb_set_mouse_callback(mouse_callback_t callback)
{
    mouse_callback = callback;
}

librfb_result_t librfb_handle_data(const void* data, size_t len)
{
    if(width == 0 || height == 0 || server_name[0] == 0 || send_callback == NULL)
        return LIBRFB_ERROR;
    switch(state) {
        case SEND_PROTOCOL_VERSION:
            printf("sending protocol version : \"%s\"\n", protocol_version);
            send_callback(protocol_version, strlen(protocol_version));
            state = RECEIVE_PROTOCOL_VERSION;
            break;
        case RECEIVE_PROTOCOL_VERSION:
            printf("receiving protocol version from client\n");
            printf("sending security types supported : only 1 (None)\n");
            send_callback(security_types, sizeof(security_types));
            state = SEND_SECURITY_RESULT;
            break;
        case SEND_SECURITY_RESULT:
            printf("sending security result (always ok)\n");
            send_callback(&security_result, sizeof(security_result));
            state = SEND_SERVER_INIT;
            break;
        case SEND_SERVER_INIT: {
            printf("sending server init\n");
            server_init_t server_init;
            server_init.frame_buffer_width = htons(width);
            server_init.frame_buffer_height = htons(height);
            server_init.pixel_format = (pixel_format_t){
                32,
                24,
                0,
                1,
                htons(255),htons(255),htons(255),
                16,8,0,
                {0,0,0}
            };
            int32_t len = htonl(strlen(server_name));
            send_callback(&server_init, sizeof(server_init));
            send_callback(&len, sizeof(len));
            send_callback(server_name, strlen(server_name));
            state = SEND_STUFF;
            break;
        }
        case SEND_STUFF: {
            uint8_t message_type = ((uint8_t*)data)[0];
            if(message_type == 3) {
                framebuffer_update_request_t* fbur = (framebuffer_update_request_t*)data;
                fbur->xpos = ntohs(fbur->xpos);
                fbur->ypos = ntohs(fbur->ypos);
                fbur->width = ntohs(fbur->width);
                fbur->height = ntohs(fbur->height);
                printf("Received FramebufferUpdateRequest(incremental = %d, %d, %d, %d, %d)\n", fbur->incremental, fbur->xpos, fbur->ypos, fbur->width, fbur->height);
                if(fbur->incremental == 0) {
                    send_frame(fbur->xpos, fbur->ypos, fbur->width, fbur->height);
                    printf("frame sent\n");
                }
                
            }
            else if(message_type == 5) {
                pointer_event_t *pointer_event = (pointer_event_t*)data;
                x_mouse = ntohs(pointer_event->x);
                y_mouse = ntohs(pointer_event->y);
                printf("x=%d, y=%d mask=%d\n", x_mouse, y_mouse, pointer_event->button_mask);
                printf("Sending updated frame\n");
                send_frame(0, 0, width, height);
                printf("frame sent\n");
            }
            else if(message_type == 4) {
                key_event_t* key_event = (key_event_t*)data;
                key_event->key = ntohl(key_event->key);
                printf("key \"%c\" %s\n" , key_event->key, key_event->down_flag ? "pressed" : "released");
            }
            else {
                printf("Received %ld bytes of unknown message type\n", len);
            }

            break;
        }


    }
    return LIBRFB_OK;
}