#ifndef __DOOR_MANAGER_H__
#define __DOOR_MANAGER_H__

#include "device/door_state.h"

typedef enum {
    DOOR_ERR_OK = 0,
    DOOR_ERR_INVALID_PARAM = -1,
    DOOR_ERR_INVALID_STATE = -2,
    DOOR_ERR_OPEN_FAILED = -3,
    DOOR_ERR_CLOSE_FAILED = -4
} door_err_t;

int door_manager_init(void);
int door_open(const char *reason);
int door_close(const char *reason);
door_state_t door_get_state(void);
int door_is_opened(void);
int door_is_closed(void);
const char *door_error_to_string(int err);

#endif
