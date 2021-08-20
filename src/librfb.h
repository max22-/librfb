#ifndef LBRFB_H
#define LIBRFB_H

/* https://datatracker.ietf.org/doc/html/rfc6143 */

typedef enum {
    INIT, DATA
} event_type_t;

typedef struct {
    event_type_t type;
    void* data;
    int len;
} event_t;

// sme_ prefix means "state machine event"

event_t sme_init();
event_t sme_data(void* data, int len);

void state_machine_init();
int state_machine(int sock, event_t event);

#endif