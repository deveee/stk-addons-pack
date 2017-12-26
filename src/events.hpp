//    STK Add-ons pack - Simple add-ons installer for Android
//    Copyright (C) 2017 Dawid Gan <deveee@gmail.com>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef EVENTS_HPP
#define EVENTS_HPP

enum KeyId
{
    KC_KEY_UNKNOWN,
    KC_KEY_0,
    KC_KEY_1,
    KC_KEY_2,
    KC_KEY_3,
    KC_KEY_4,
    KC_KEY_5,
    KC_KEY_6,
    KC_KEY_7,
    KC_KEY_8,
    KC_KEY_9,
    KC_KEY_A,
    KC_KEY_B,
    KC_KEY_C,
    KC_KEY_D,
    KC_KEY_E,
    KC_KEY_F,
    KC_KEY_G,
    KC_KEY_H,
    KC_KEY_I,
    KC_KEY_J,
    KC_KEY_K,
    KC_KEY_L,
    KC_KEY_M,
    KC_KEY_N,
    KC_KEY_O,
    KC_KEY_P,
    KC_KEY_Q,
    KC_KEY_R,
    KC_KEY_S,
    KC_KEY_T,
    KC_KEY_U,
    KC_KEY_V,
    KC_KEY_W,
    KC_KEY_X,
    KC_KEY_Y,
    KC_KEY_Z,
    KC_KEY_BACK,
    KC_KEY_TAB,
    KC_KEY_CLEAR,
    KC_KEY_RETURN,
    KC_KEY_SHIFT,
    KC_KEY_LSHIFT,
    KC_KEY_RSHIFT,
    KC_KEY_CONTROL,
    KC_KEY_LCONTROL,
    KC_KEY_RCONTROL,
    KC_KEY_MENU,
    KC_KEY_LMENU,
    KC_KEY_RMENU,
    KC_KEY_PAUSE,
    KC_KEY_CAPITAL,
    KC_KEY_ESCAPE,
    KC_KEY_SPACE,
    KC_KEY_PRIOR,
    KC_KEY_NEXT,
    KC_KEY_END,
    KC_KEY_HOME,
    KC_KEY_LEFT,
    KC_KEY_UP,
    KC_KEY_RIGHT,
    KC_KEY_DOWN,
    KC_KEY_PRINT,
    KC_KEY_INSERT,
    KC_KEY_DELETE,
    KC_KEY_LWIN,
    KC_KEY_RWIN,
    KC_KEY_NUMPAD0,
    KC_KEY_NUMPAD1,
    KC_KEY_NUMPAD2,
    KC_KEY_NUMPAD3,
    KC_KEY_NUMPAD4,
    KC_KEY_NUMPAD5,
    KC_KEY_NUMPAD6,
    KC_KEY_NUMPAD7,
    KC_KEY_NUMPAD8,
    KC_KEY_NUMPAD9,
    KC_KEY_MULTIPLY,
    KC_KEY_ADD,
    KC_KEY_SEPARATOR,
    KC_KEY_SUBTRACT,
    KC_KEY_DECIMAL,
    KC_KEY_DIVIDE,
    KC_KEY_F1,
    KC_KEY_F2,
    KC_KEY_F3,
    KC_KEY_F4,
    KC_KEY_F5,
    KC_KEY_F6,
    KC_KEY_F7,
    KC_KEY_F8,
    KC_KEY_F9,
    KC_KEY_F10,
    KC_KEY_F11,
    KC_KEY_F12,
    KC_KEY_NUMLOCK,
    KC_KEY_SCROLL,
    KC_KEY_PLUS,
    KC_KEY_COMMA,
    KC_KEY_MINUS,
    KC_KEY_PERIOD,
    KC_KEY_OEM_1,
    KC_KEY_OEM_2,
    KC_KEY_OEM_3,
    KC_KEY_OEM_4,
    KC_KEY_OEM_5,
    KC_KEY_OEM_6,
    KC_KEY_OEM_7,
    KC_KEY_OEM_8,
    KC_KEY_OEM_102,
    KC_KEY_LBUTTON,
    KC_KEY_RBUTTON,
    KC_KEY_SELECT,
    KC_KEY_VOLUME_DOWN,
    KC_KEY_VOLUME_UP,
    KC_KEY_MEDIA_PLAY_PAUSE,
    KC_KEY_MEDIA_STOP,
    KC_KEY_MEDIA_NEXT_TRACK,
    KC_KEY_MEDIA_PREV_TRACK,
    KC_KEY_VOLUME_MUTE,
    KC_KEY_SNAPSHOT,
    KC_KEY_PLAY,
    KC_KEY_ZOOM
};

enum EventType
{
    ET_MOUSE_EVENT,
    ET_KEY_EVENT,
    ET_TOUCH_EVENT,
    ET_ACCELEROMETER_EVENT,
    ET_GYROSCOPE_EVENT,
    ET_JOYSTICK_EVENT,
    ET_SYSTEM_EVENT
};

enum MouseEventType
{
    ME_LEFT_PRESSED,
    ME_MIDDLE_PRESSED,
    ME_RIGHT_PRESSED,
    ME_LEFT_RELEASED,
    ME_MIDDLE_RELEASED,
    ME_RIGHT_RELEASED,
    ME_MOUSE_MOVED,
    ME_MOUSE_WHEEL,
    ME_LEFT_CLICK,
    ME_MIDDLE_CLICK,
    ME_RIGHT_CLICK,
    ME_LEFT_DOUBLE_CLICK,
    ME_MIDDLE_DOUBLE_CLICK,
    ME_RIGHT_DOUBLE_CLICK,
    ME_COUNT
};

enum TouchEventType
{
    TE_PRESSED,
    TE_RELEASED,
    TE_MOVED,
    TE_COUNT
};

struct MouseEvent
{
    MouseEventType type;
    int x;
    int y;
    float wheel;
    bool shift;
    bool control;
    bool button_state_left;
    bool button_state_middle;
    bool button_state_right;
};

struct KeyEvent
{
    KeyId id;
    char text[4];
    bool pressed;
    bool shift;
    bool control;
};

struct TouchEvent
{
    TouchEventType type;
    int id;
    int x;
    int y;
};

struct AccelerometerEvent
{
    double x;
    double y;
    double z;
};

struct GyroscopeEvent
{
    double x;
    double y;
    double z;
};

struct JoystickEvent
{
    unsigned int id;
    bool button_states[32];
    int axis[32];
};

struct SystemEvent
{
    int cmd;
};

struct Event
{
    EventType type;
    union
    {
        struct MouseEvent mouse;
        struct KeyEvent key;
        struct TouchEvent touch;
        struct AccelerometerEvent accelerometer;
        struct GyroscopeEvent gyroscope;
        struct JoystickEvent joystick;
        struct SystemEvent system;
    };
};

#endif
