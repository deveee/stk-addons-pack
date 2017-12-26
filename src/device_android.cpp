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

#if defined(ANDROID)

#include <cassert>
#include <cstring>

#include "device_android.hpp"

bool DeviceAndroid::m_is_paused = true;
bool DeviceAndroid::m_is_focused = false;
bool DeviceAndroid::m_is_started = false;

// Execution of android_main() function is a kind of "onCreate" event, so this
// function should be used there to make sure that global window state variables
// have their default values on startup.
void DeviceAndroid::onCreate()
{
    m_is_paused = true;
    m_is_focused = false;
    m_is_started = false;
}

DeviceAndroid::DeviceAndroid()
{
    m_android = NULL;
    m_sensor_manager = NULL;
    m_sensor_event_queue = NULL;
    m_close = false;
    m_is_mouse_pressed = false;
    m_cursor_x = 0;
    m_cursor_y = 0;
    m_window_width = 0;
    m_window_height = 0;
}

DeviceAndroid::~DeviceAndroid()
{
    delete m_egl_context;
    
    closeSensors();
    
    if (m_android != NULL)
    {
        m_android->userData = NULL;
        m_android->onAppCmd = NULL;
        m_android->onInputEvent = NULL;
    }
}

bool DeviceAndroid::initDevice(const CreationParams& creation_params)
{
    m_creation_params = creation_params;
    
    m_android = (android_app*)(creation_params.private_data);
    
    if (m_android == NULL)
        return false;

    m_android->userData = this;
    m_android->onAppCmd = onAppCommand;
    m_android->onInputEvent = onEvent;
    
    ANativeActivity_setWindowFlags(m_android->activity,
                                   AWINDOW_FLAG_KEEP_SCREEN_ON |
                                   AWINDOW_FLAG_FULLSCREEN, 0);
    
    while (!m_is_started || !m_is_focused || m_is_paused)
    {
        int events = 0;
        android_poll_source* source = NULL;

        int id = ALooper_pollAll(-1, NULL, &events, (void**)&source);

        if (id == LOOPER_ID_MAIN && source != NULL)
        {
            source->process(m_android, source);
        }
    }
    
    if (m_android->window == NULL)
        return false;

    initSensors();
    createKeyMap();
    createVideoModeList();

    bool success = createEGLContext(0, m_android->window);
    
    return success;
}

void DeviceAndroid::closeDevice() 
{
    ANativeActivity_finish(m_android->activity);
}

void DeviceAndroid::createVideoModeList()
{
    if (m_video_modes.size() > 0)
        return;
        
    m_window_width = ANativeWindow_getWidth(m_android->window);
    m_window_height = ANativeWindow_getHeight(m_android->window);
    
    VideoMode video_mode(m_window_width, m_window_height, 32);
    m_video_modes.push_back(video_mode);
    m_video_desktop = video_mode;
}

bool DeviceAndroid::processEvents()
{
    while (!m_close)
    {
        int events = 0;
        android_poll_source* source = NULL;
        bool should_run = (m_is_started && m_is_focused && !m_is_paused);
        int id = ALooper_pollAll(should_run ? 0 : -1, NULL, &events,
                                 (void**)&source);
                                 
        if (id < 0)
            break;

        if (id == LOOPER_ID_MAIN || id == LOOPER_ID_INPUT)
        {
            if (source != NULL)
            {
                source->process(m_android, source);
            }
        }
        else if (id == LOOPER_ID_USER)
        {
            pollSensors(); 
        }
    }

    return !m_close;
}

void DeviceAndroid::onAppCommand(android_app* app, int32_t cmd)
{
    DeviceAndroid* device = (DeviceAndroid*)app->userData;
    assert(device != NULL);
    
    std::string event_str;
    
    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
        if (device->getEGLContext() != NULL)
        {
            device->getEGLContext()->reloadEGLSurface(app->window);
        }
        m_is_started = true;
        break;
    case APP_CMD_TERM_WINDOW:
        m_is_started = false;
        break;
    case APP_CMD_GAINED_FOCUS:
        m_is_focused = true;
        break;
    case APP_CMD_LOST_FOCUS:
        m_is_focused = false;
        break;
    case APP_CMD_DESTROY:
        device->m_close = true;
        break;
    case APP_CMD_PAUSE:
        m_is_paused = true;
        break;
    case APP_CMD_RESUME:
        m_is_paused = false;
        break;
    case APP_CMD_SAVE_STATE:
    case APP_CMD_START:
    case APP_CMD_STOP:
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONFIG_CHANGED:
    case APP_CMD_LOW_MEMORY:
    default:
        break;
    }
    
    Event event;
    event.type = ET_SYSTEM_EVENT;
    event.system.cmd = cmd;
    device->sendEvent(event);
}

int DeviceAndroid::onEvent(android_app* app, AInputEvent* input_event)
{
    DeviceAndroid* device = (DeviceAndroid*)app->userData;
    assert(device != NULL);
    
    int status = 0;
    
    switch (AInputEvent_getType(input_event))
    {
    case AINPUT_EVENT_TYPE_MOTION:
    {
        Event event;
        event.type = ET_TOUCH_EVENT;
        event.touch.type = TE_COUNT;
        MouseEventType mouse_event = ME_COUNT;
        
        int32_t event_action = AMotionEvent_getAction(input_event);

        switch (event_action & AMOTION_EVENT_ACTION_MASK)
        {
        case AMOTION_EVENT_ACTION_MOVE:
            event.touch.type = TE_MOVED;
            break;
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            event.touch.type = TE_PRESSED;
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            event.touch.type = TE_RELEASED;
            break;
        default:
            break;
        }
        
        if (event.touch.type == TE_MOVED)
        {
            int count = AMotionEvent_getPointerCount(input_event);

            for (int i = 0; i < count; i++)
            {
                event.touch.id = AMotionEvent_getPointerId(input_event, i);
                event.touch.x = AMotionEvent_getX(input_event, i);
                event.touch.y = AMotionEvent_getY(input_event, i);
                
                if (event.touch.id == 0)
                {
                    device->m_cursor_x = event.touch.x;
                    device->m_cursor_y = event.touch.y;
                    mouse_event = ME_MOUSE_MOVED;
                }

                device->sendEvent(event);
            }
        }
        else if (event.touch.type == TE_PRESSED || 
                 event.touch.type == TE_RELEASED)
        {
            int id = (event_action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> 
                                     AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

            event.touch.id = AMotionEvent_getPointerId(input_event, id);
            event.touch.x = AMotionEvent_getX(input_event, id);
            event.touch.y = AMotionEvent_getY(input_event, id);
            
            if (event.touch.id == 0)
            {
                bool pressed = (event.touch.type == TE_PRESSED);
                device->m_cursor_x = event.touch.x;
                device->m_cursor_y = event.touch.y;
                device->m_is_mouse_pressed = pressed;
                mouse_event = pressed ? ME_LEFT_PRESSED : ME_LEFT_RELEASED;
            }
            
            device->sendEvent(event);
        }
        
        // Simulate mouse event for first finger on multitouch device.
        if (mouse_event != ME_COUNT)
        {
            event.type = ET_MOUSE_EVENT;
            event.mouse.type = mouse_event;
            event.mouse.x = device->m_cursor_x;
            event.mouse.y = device->m_cursor_y;
            event.mouse.control = false;
            event.mouse.shift = false;
            event.mouse.button_state_left = device->m_is_mouse_pressed;
            event.mouse.button_state_middle = false;
            event.mouse.button_state_right = false;

            device->sendEvent(event);
            
            event.mouse.type = device->checkMouseClick(event.mouse);
            
            if (event.mouse.type != ME_COUNT)
            {
                device->sendEvent(event);
            }
        }

        status = 1;
        break;
    }
    case AINPUT_EVENT_TYPE_KEY:
    {
        bool ignore_event = false;

        int32_t key_code = AKeyEvent_getKeyCode(input_event);
        int32_t key_action = AKeyEvent_getAction(input_event);
        int32_t key_meta_state = AKeyEvent_getMetaState(input_event);
        int32_t key_repeat = AKeyEvent_getRepeatCount(input_event);

        Event event;
        event.type = ET_KEY_EVENT;
        event.key.text[0] = 0;
        event.key.id = device->m_key_map[key_code];
        event.key.pressed = (key_action == AKEY_EVENT_ACTION_DOWN);

        event.key.shift = (key_meta_state & AMETA_SHIFT_ON ||
                           key_meta_state & AMETA_SHIFT_LEFT_ON ||
                           key_meta_state & AMETA_SHIFT_RIGHT_ON);

        event.key.control = (key_meta_state & AMETA_CTRL_ON ||
                             key_meta_state & AMETA_CTRL_LEFT_ON ||
                             key_meta_state & AMETA_CTRL_RIGHT_ON);

        if (event.key.id > 0)
        {
            device->getKeyChar(event, key_code);
        }

        // Handle an event when back button in pressed just like an escape key
        // and also avoid repeating the event to avoid some strange behaviour
        if (key_code == AKEYCODE_BACK)
        {
            if (event.key.pressed == false || key_repeat > 0)
            {
                ignore_event = true;
            }
            
            status = 1;
        }

        // Mark escape key event as handled by application to avoid receiving
        // AKEYCODE_BACK key event
        if (key_code == AKEYCODE_ESCAPE)
        {
            status = 1;
        }

        if (!ignore_event)
        {
            device->sendEvent(event);
        }

        break;
    }
    default:
        break;
    }

    return status;
}

void DeviceAndroid::getKeyChar(Event& event, unsigned int system_key_code)
{
    memset(event.key.text, 0, 4);

    // A-Z
    if (system_key_code > 28 && system_key_code < 55)
    {
        event.key.text[0] = event.key.shift ? system_key_code + 36
                                            : system_key_code + 68;
    }

    // 0-9
    else if (system_key_code > 6 && system_key_code < 17)
    {
        event.key.text[0] = system_key_code + 41;
    }

    else if (system_key_code == AKEYCODE_BACK)
    {
        event.key.text[0] = 8;
    }
    else if (system_key_code == AKEYCODE_DEL)
    {
        event.key.text[0] = 8;
    }
    else if (system_key_code == AKEYCODE_TAB)
    {
        event.key.text[0] = 9;
    }
    else if (system_key_code == AKEYCODE_ENTER)
    {
        event.key.text[0] = 13;
    }
    else if (system_key_code == AKEYCODE_SPACE)
    {
        event.key.text[0] = 32;
    }
    else if (system_key_code == AKEYCODE_COMMA)
    {
        event.key.text[0] = 44;
    }
    else if (system_key_code == AKEYCODE_PERIOD)
    {
        event.key.text[0] = 46;
    }
}

void DeviceAndroid::createKeyMap()
{
    m_key_map[AKEYCODE_UNKNOWN] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_SOFT_LEFT] = KC_KEY_LBUTTON; 
    m_key_map[AKEYCODE_SOFT_RIGHT] = KC_KEY_RBUTTON; 
    m_key_map[AKEYCODE_HOME] = KC_KEY_HOME; 
    m_key_map[AKEYCODE_BACK] = KC_KEY_ESCAPE; 
    m_key_map[AKEYCODE_CALL] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_ENDCALL] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_0] = KC_KEY_0; 
    m_key_map[AKEYCODE_1] = KC_KEY_1; 
    m_key_map[AKEYCODE_2] = KC_KEY_2; 
    m_key_map[AKEYCODE_3] = KC_KEY_3; 
    m_key_map[AKEYCODE_4] = KC_KEY_4; 
    m_key_map[AKEYCODE_5] = KC_KEY_5; 
    m_key_map[AKEYCODE_6] = KC_KEY_6; 
    m_key_map[AKEYCODE_7] = KC_KEY_7; 
    m_key_map[AKEYCODE_8] = KC_KEY_8; 
    m_key_map[AKEYCODE_9] = KC_KEY_9; 
    m_key_map[AKEYCODE_STAR] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_POUND] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_DPAD_UP] = KC_KEY_UP; 
    m_key_map[AKEYCODE_DPAD_DOWN] = KC_KEY_DOWN; 
    m_key_map[AKEYCODE_DPAD_LEFT] = KC_KEY_LEFT; 
    m_key_map[AKEYCODE_DPAD_RIGHT] = KC_KEY_RIGHT; 
    m_key_map[AKEYCODE_DPAD_CENTER] = KC_KEY_SELECT; 
    m_key_map[AKEYCODE_VOLUME_UP] = KC_KEY_VOLUME_DOWN; 
    m_key_map[AKEYCODE_VOLUME_DOWN] = KC_KEY_VOLUME_UP; 
    m_key_map[AKEYCODE_POWER] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_CAMERA] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_CLEAR] = KC_KEY_CLEAR; 
    m_key_map[AKEYCODE_A] = KC_KEY_A; 
    m_key_map[AKEYCODE_B] = KC_KEY_B; 
    m_key_map[AKEYCODE_C] = KC_KEY_C; 
    m_key_map[AKEYCODE_D] = KC_KEY_D; 
    m_key_map[AKEYCODE_E] = KC_KEY_E; 
    m_key_map[AKEYCODE_F] = KC_KEY_F; 
    m_key_map[AKEYCODE_G] = KC_KEY_G; 
    m_key_map[AKEYCODE_H] = KC_KEY_H; 
    m_key_map[AKEYCODE_I] = KC_KEY_I; 
    m_key_map[AKEYCODE_J] = KC_KEY_J; 
    m_key_map[AKEYCODE_K] = KC_KEY_K; 
    m_key_map[AKEYCODE_L] = KC_KEY_L; 
    m_key_map[AKEYCODE_M] = KC_KEY_M; 
    m_key_map[AKEYCODE_N] = KC_KEY_N; 
    m_key_map[AKEYCODE_O] = KC_KEY_O; 
    m_key_map[AKEYCODE_P] = KC_KEY_P; 
    m_key_map[AKEYCODE_Q] = KC_KEY_Q; 
    m_key_map[AKEYCODE_R] = KC_KEY_R; 
    m_key_map[AKEYCODE_S] = KC_KEY_S; 
    m_key_map[AKEYCODE_T] = KC_KEY_T; 
    m_key_map[AKEYCODE_U] = KC_KEY_U; 
    m_key_map[AKEYCODE_V] = KC_KEY_V; 
    m_key_map[AKEYCODE_W] = KC_KEY_W; 
    m_key_map[AKEYCODE_X] = KC_KEY_X; 
    m_key_map[AKEYCODE_Y] = KC_KEY_Y; 
    m_key_map[AKEYCODE_Z] = KC_KEY_Z; 
    m_key_map[AKEYCODE_COMMA] = KC_KEY_COMMA; 
    m_key_map[AKEYCODE_PERIOD] = KC_KEY_PERIOD; 
    m_key_map[AKEYCODE_ALT_LEFT] = KC_KEY_MENU; 
    m_key_map[AKEYCODE_ALT_RIGHT] = KC_KEY_MENU; 
    m_key_map[AKEYCODE_SHIFT_LEFT] = KC_KEY_LSHIFT; 
    m_key_map[AKEYCODE_SHIFT_RIGHT] = KC_KEY_RSHIFT; 
    m_key_map[AKEYCODE_TAB] = KC_KEY_TAB; 
    m_key_map[AKEYCODE_SPACE] = KC_KEY_SPACE; 
    m_key_map[AKEYCODE_SYM] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_EXPLORER] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_ENVELOPE] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_ENTER] = KC_KEY_RETURN; 
    m_key_map[AKEYCODE_DEL] = KC_KEY_BACK; 
    m_key_map[AKEYCODE_GRAVE] = KC_KEY_OEM_3; 
    m_key_map[AKEYCODE_MINUS] = KC_KEY_MINUS; 
    m_key_map[AKEYCODE_EQUALS] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_LEFT_BRACKET] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_RIGHT_BRACKET] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BACKSLASH] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_SEMICOLON] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_APOSTROPHE] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_SLASH] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_AT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_NUM] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_HEADSETHOOK] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_FOCUS] = KC_KEY_UNKNOWN;
    m_key_map[AKEYCODE_PLUS] = KC_KEY_PLUS; 
    m_key_map[AKEYCODE_MENU] = KC_KEY_MENU; 
    m_key_map[AKEYCODE_NOTIFICATION] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_SEARCH] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_MEDIA_PLAY_PAUSE] = KC_KEY_MEDIA_PLAY_PAUSE; 
    m_key_map[AKEYCODE_MEDIA_STOP] = KC_KEY_MEDIA_STOP; 
    m_key_map[AKEYCODE_MEDIA_NEXT] = KC_KEY_MEDIA_NEXT_TRACK; 
    m_key_map[AKEYCODE_MEDIA_PREVIOUS] = KC_KEY_MEDIA_PREV_TRACK; 
    m_key_map[AKEYCODE_MEDIA_REWIND] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_MEDIA_FAST_FORWARD] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_MUTE] = KC_KEY_VOLUME_MUTE; 
    m_key_map[AKEYCODE_PAGE_UP] = KC_KEY_PRIOR; 
    m_key_map[AKEYCODE_PAGE_DOWN] = KC_KEY_NEXT; 
    m_key_map[AKEYCODE_PICTSYMBOLS] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_SWITCH_CHARSET] = KC_KEY_UNKNOWN; 

    m_key_map[AKEYCODE_BUTTON_A] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_B] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_C] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_X] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_Y] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_Z] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_L1] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_R1] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_L2] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_R2] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_THUMBL] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_THUMBR] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_START] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_SELECT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_MODE] = KC_KEY_UNKNOWN; 

    m_key_map[AKEYCODE_ESCAPE] = KC_KEY_ESCAPE; 
    m_key_map[AKEYCODE_FORWARD_DEL] = KC_KEY_DELETE; 
    m_key_map[AKEYCODE_CTRL_LEFT] = KC_KEY_CONTROL; 
    m_key_map[AKEYCODE_CTRL_RIGHT] = KC_KEY_CONTROL; 
    m_key_map[AKEYCODE_CAPS_LOCK] = KC_KEY_CAPITAL; 
    m_key_map[AKEYCODE_SCROLL_LOCK] = KC_KEY_SCROLL; 
    m_key_map[AKEYCODE_META_LEFT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_META_RIGHT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_FUNCTION] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_SYSRQ] = KC_KEY_SNAPSHOT; 
    m_key_map[AKEYCODE_BREAK] = KC_KEY_PAUSE; 
    m_key_map[AKEYCODE_MOVE_HOME] = KC_KEY_HOME; 
    m_key_map[AKEYCODE_MOVE_END] = KC_KEY_END; 
    m_key_map[AKEYCODE_INSERT] = KC_KEY_INSERT; 
    m_key_map[AKEYCODE_FORWARD] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_MEDIA_PLAY] = KC_KEY_PLAY; 
    m_key_map[AKEYCODE_MEDIA_PAUSE] = KC_KEY_MEDIA_PLAY_PAUSE; 
    m_key_map[AKEYCODE_MEDIA_CLOSE] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_MEDIA_EJECT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_MEDIA_RECORD] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_F1] = KC_KEY_F1; 
    m_key_map[AKEYCODE_F2] = KC_KEY_F2; 
    m_key_map[AKEYCODE_F3] = KC_KEY_F3; 
    m_key_map[AKEYCODE_F4] = KC_KEY_F4; 
    m_key_map[AKEYCODE_F5] = KC_KEY_F5; 
    m_key_map[AKEYCODE_F6] = KC_KEY_F6; 
    m_key_map[AKEYCODE_F7] = KC_KEY_F7; 
    m_key_map[AKEYCODE_F8] = KC_KEY_F8; 
    m_key_map[AKEYCODE_F9] = KC_KEY_F9; 
    m_key_map[AKEYCODE_F10] = KC_KEY_F10; 
    m_key_map[AKEYCODE_F11] = KC_KEY_F11; 
    m_key_map[AKEYCODE_F12] = KC_KEY_F12; 
    m_key_map[AKEYCODE_NUM_LOCK] = KC_KEY_NUMLOCK; 
    m_key_map[AKEYCODE_NUMPAD_0] = KC_KEY_NUMPAD0; 
    m_key_map[AKEYCODE_NUMPAD_1] = KC_KEY_NUMPAD1; 
    m_key_map[AKEYCODE_NUMPAD_2] = KC_KEY_NUMPAD2; 
    m_key_map[AKEYCODE_NUMPAD_3] = KC_KEY_NUMPAD3; 
    m_key_map[AKEYCODE_NUMPAD_4] = KC_KEY_NUMPAD4; 
    m_key_map[AKEYCODE_NUMPAD_5] = KC_KEY_NUMPAD5; 
    m_key_map[AKEYCODE_NUMPAD_6] = KC_KEY_NUMPAD6; 
    m_key_map[AKEYCODE_NUMPAD_7] = KC_KEY_NUMPAD7; 
    m_key_map[AKEYCODE_NUMPAD_8] = KC_KEY_NUMPAD8; 
    m_key_map[AKEYCODE_NUMPAD_9] = KC_KEY_NUMPAD9; 
    m_key_map[AKEYCODE_NUMPAD_DIVIDE] = KC_KEY_DIVIDE; 
    m_key_map[AKEYCODE_NUMPAD_MULTIPLY] = KC_KEY_MULTIPLY; 
    m_key_map[AKEYCODE_NUMPAD_SUBTRACT] = KC_KEY_SUBTRACT; 
    m_key_map[AKEYCODE_NUMPAD_ADD] = KC_KEY_ADD; 
    m_key_map[AKEYCODE_NUMPAD_DOT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_NUMPAD_COMMA] = KC_KEY_COMMA; 
    m_key_map[AKEYCODE_NUMPAD_ENTER] = KC_KEY_RETURN; 
    m_key_map[AKEYCODE_NUMPAD_EQUALS] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_NUMPAD_LEFT_PAREN] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_NUMPAD_RIGHT_PAREN] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_VOLUME_MUTE] = KC_KEY_VOLUME_MUTE; 
    m_key_map[AKEYCODE_INFO] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_CHANNEL_UP] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_CHANNEL_DOWN] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_ZOOM_IN] = KC_KEY_ZOOM; 
    m_key_map[AKEYCODE_ZOOM_OUT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_TV] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_WINDOW] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_GUIDE] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_DVR] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BOOKMARK] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_CAPTIONS] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_SETTINGS] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_TV_POWER] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_TV_INPUT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_STB_POWER] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_STB_INPUT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_AVR_POWER] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_AVR_INPUT] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_PROG_RED] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_PROG_GREEN] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_PROG_YELLOW] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_PROG_BLUE] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_APP_SWITCH] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_1] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_2] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_3] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_4] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_5] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_6] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_7] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_8] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_9] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_10] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_11] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_12] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_13] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_14] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_15] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_BUTTON_16] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_LANGUAGE_SWITCH] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_MANNER_MODE] = KC_KEY_UNKNOWN; 
    m_key_map[AKEYCODE_3D_MODE] = KC_KEY_UNKNOWN; 
}

void DeviceAndroid::initSensors()
{
    m_sensor_manager = ASensorManager_getInstance();
    
    m_accelerometer.active = false;
    m_accelerometer.sensor = ASensorManager_getDefaultSensor(m_sensor_manager,
                                                    ASENSOR_TYPE_ACCELEROMETER);
    m_gyroscope.active = false;
    m_gyroscope.sensor = ASensorManager_getDefaultSensor(m_sensor_manager,
                                                        ASENSOR_TYPE_GYROSCOPE);
    m_sensor_event_queue = ASensorManager_createEventQueue(m_sensor_manager,
                                m_android->looper, LOOPER_ID_USER, NULL, NULL);
}

void DeviceAndroid::closeSensors()
{
    if (m_sensor_event_queue != NULL)
    {
        ASensorManager_destroyEventQueue(m_sensor_manager, 
                                         m_sensor_event_queue);
    }
}

void DeviceAndroid::pollSensors()
{
    ASensorEvent sensor;
    while (ASensorEventQueue_getEvents(m_sensor_event_queue, &sensor, 1) > 0)
    {
        if (sensor.type == ASENSOR_TYPE_ACCELEROMETER)
        {
            Event event;
            event.type = ET_ACCELEROMETER_EVENT;
            event.accelerometer.x = sensor.acceleration.x;
            event.accelerometer.y = sensor.acceleration.y;
            event.accelerometer.z = sensor.acceleration.z;

            sendEvent(event);
        }
        else if (sensor.type == ASENSOR_TYPE_GYROSCOPE)
        {
            Event event;
            event.type = ET_GYROSCOPE_EVENT;
            event.gyroscope.x = sensor.vector.x;
            event.gyroscope.y = sensor.vector.y;
            event.gyroscope.z = sensor.vector.z;

            sendEvent(event);
        }
    }
}

bool DeviceAndroid::activateSensor(SensorInfo* sensor_info, float interval)
{
    if (m_sensor_event_queue == NULL)
        return false;
        
    if (sensor_info->sensor == NULL)
        return false;

    int success = ASensorEventQueue_enableSensor(m_sensor_event_queue, 
                                                 sensor_info->sensor);
    
    if (success > -1)
    {
        sensor_info->active = true;
        ASensorEventQueue_setEventRate(m_sensor_event_queue, sensor_info->sensor,
                                       (int32_t)(interval * 1000000.0f));
    }
    
    return sensor_info->active;
}

bool DeviceAndroid::deactivateSensor(SensorInfo* sensor_info)
{
    if (!sensor_info->active)
        return true;
    
    int success = ASensorEventQueue_disableSensor(m_sensor_event_queue, 
                                                  sensor_info->sensor);

    if (success > -1)
    {
        sensor_info->active = false;
    }

    return !sensor_info->active;
}

bool DeviceAndroid::activateAccelerometer(float interval)
{
    return activateSensor(&m_accelerometer, interval);
}

bool DeviceAndroid::deactivateAccelerometer()
{
    return deactivateSensor(&m_accelerometer);
}

bool DeviceAndroid::activateGyroscope(float interval)
{
    return activateSensor(&m_gyroscope, interval);
}

bool DeviceAndroid::deactivateGyroscope()
{
    return deactivateSensor(&m_gyroscope);
}

#endif
