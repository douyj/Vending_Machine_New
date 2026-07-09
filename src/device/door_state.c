#include "device/door_state.h"


/*
    @brief 柜门状态转字符串
    @param state 柜门状态
    @return 状态字符串
*/
const char *door_state_to_string(door_state_t state)
{
    switch (state) {
    case DOOR_STATE_CLOSED:
        return "CLOSED";

    case DOOR_STATE_OPENING:
        return "OPENING";

    case DOOR_STATE_OPENED:
        return "OPENED";

    case DOOR_STATE_CLOSING:
        return "CLOSING";

    case DOOR_STATE_ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}