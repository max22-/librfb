#ifndef LBRFB_H
#define LIBRFB_H

/* https://datatracker.ietf.org/doc/html/rfc6143 */

typedef enum {LIBRFB_OK, LIBRFB_ERROR} librfb_result_t;
typedef ssize_t (*send_callback_t)(const void*, size_t);
typedef void (*keyboard_callback_t)(uint8_t down_flag, uint32_t key);
typedef void (*mouse_callback_t)(uint16_t x, uint16_t y, uint8_t button_mask);
typedef void (*frame_request_callback_t)(uint16_t xpos, uint16_t ypos, uint16_t w, uint16_t h);

void librfb_reset();
void librfb_init(const char* server_name, uint16_t w, uint16_t h, send_callback_t callback);
void librfb_set_keyboard_callback(keyboard_callback_t);
void librfb_set_mouse_callback(mouse_callback_t);
void librfb_set_frame_request_callback(frame_request_callback_t);

librfb_result_t librfb_handle_data(const void* data, size_t len);
void librfb_send_frame_header(uint16_t xpos, uint16_t ypos, uint16_t w, uint16_t h);

#define LIBRFB_RGBA(r, g, b, a) htonl(b << 24 | g << 16 | r << 8 | a)


#endif