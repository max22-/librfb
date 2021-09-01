#ifndef LBRFB_H
#define LIBRFB_H

/* https://datatracker.ietf.org/doc/html/rfc6143 */

typedef enum {LIBRFB_OK, LIBRFB_ERROR} librfb_result_t;
typedef ssize_t (*send_callback_t)(const void*, size_t);
typedef void (*keyboard_callback_t)(uint32_t);
typedef void (*mouse_callback_t)(uint16_t x, uint16_t y, uint8_t button_mask);

void librfb_reset();
void librfb_init(const char* server_name, int w, int h, send_callback_t callback);
void librfb_set_keyboard_callback(keyboard_callback_t);
void librfb_set_mouse_callback(mouse_callback_t);

librfb_result_t librfb_handle_data(const void* data, size_t len);



#endif