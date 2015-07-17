#ifndef __CTRCOMMON_INPUT_HPP__
#define __CTRCOMMON_INPUT_HPP__

#include <string>

#ifndef BIT
#define BIT(n) (1U << (n))
#define BIT_SELF_DEFINED
#endif
typedef enum {
    BUTTON_NONE = 0,
    BUTTON_A = BIT(0),
    BUTTON_B = BIT(1),
    BUTTON_SELECT = BIT(2),
    BUTTON_START = BIT(3),
    BUTTON_DRIGHT = BIT(4),
    BUTTON_DLEFT = BIT(5),
    BUTTON_DUP = BIT(6),
    BUTTON_DDOWN = BIT(7),
    BUTTON_R = BIT(8),
    BUTTON_L = BIT(9),
    BUTTON_X = BIT(10),
    BUTTON_Y = BIT(11),
    BUTTON_ZL = BIT(14),
    BUTTON_ZR = BIT(15),
    BUTTON_TOUCH = BIT(20),
    BUTTON_CSTICK_RIGHT = BIT(24),
    BUTTON_CSTICK_LEFT = BIT(25),
    BUTTON_CSTICK_UP = BIT(26),
    BUTTON_CSTICK_DOWN = BIT(27),
    BUTTON_CPAD_RIGHT = BIT(28),
    BUTTON_CPAD_LEFT = BIT(29),
    BUTTON_CPAD_UP = BIT(30),
    BUTTON_CPAD_DOWN = BIT(31),
    BUTTON_UP = BUTTON_DUP | BUTTON_CPAD_UP,
    BUTTON_DOWN = BUTTON_DDOWN | BUTTON_CPAD_DOWN,
    BUTTON_LEFT = BUTTON_DLEFT | BUTTON_CPAD_LEFT,
    BUTTON_RIGHT = BUTTON_DRIGHT | BUTTON_CPAD_RIGHT,
} Button;
#ifdef BIT_SELF_DEFINED
#undef BIT
#undef BIT_SELF_DEFINED
#endif

typedef struct {
    int x;
    int y;
} Touch;

const std::string inputGetButtonName(Button button);
void inputPoll();
bool inputIsAnyPressed();
Button inputGetAnyPressed();
bool inputIsReleased(Button button);
bool inputIsPressed(Button button);
bool inputIsHeld(Button button);
Touch inputGetTouch();

#endif
