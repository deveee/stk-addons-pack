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

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <map>
#include <string>
#include <vector>

#include "context_egl.hpp"
#include "events.hpp"

typedef void (*EventReceiver)(Event);

enum DriverType
{
    DRIVER_OPENGL,
    DRIVER_OPENGL_ES
};

struct CreationParams
{
    int window_width;
    int window_height;
    bool fullscreen;
    bool vsync;
    bool handle_srgb;
    bool alpha_channel;
    bool force_legacy_device;
    bool joystick_support;
    void* private_data;
    DriverType driver_type;
};

struct VideoMode
{
    int width;
    int height;
    int depth;
    
    VideoMode(int w = 0, int h = 0, int d = 0): width(w), height(h), depth(d) {}
};

struct MouseMultiClicks
{
    int count;
    int last_button;
    int last_pos_x;
    int last_pos_y;
    unsigned long last_click_time;
    
    MouseMultiClicks() : count(0), last_button(0), last_pos_x(0), last_pos_y(0),
                         last_click_time(0) {}
};

struct JoystickInfo
{
    int fd;
    int axes;
    int buttons;
    int id;
    std::string name;
    JoystickEvent data;

    JoystickInfo(): fd(-1), axes(0), buttons(0), id(0) {};
};

class Device
{
protected:
    ContextManagerEGL* m_egl_context;
    CreationParams m_creation_params;
    MouseMultiClicks m_mouse_clicks;
    VideoMode m_video_desktop;
    std::vector<VideoMode> m_video_modes;
    std::vector<JoystickInfo> m_active_joysticks;
    unsigned int m_window_width;
    unsigned int m_window_height;
    EventReceiver m_event_receiver;
    
    void sendEvent(Event event);
    MouseEventType checkMouseClick(MouseEvent event);

public:
    Device();
    virtual ~Device() {};
    
    virtual bool initDevice(const CreationParams& creation_params) = 0;
    virtual void closeDevice() = 0;
    virtual bool processEvents() = 0;
    virtual void clearSystemMessages() = 0;
    
    virtual void setWindowCaption(const char* text) = 0;
    virtual void setWindowClass(const char* text) = 0;
    virtual void setWindowFullscreen(bool fullscreen) = 0;
    virtual void setWindowResizable(bool resizable) = 0;
    virtual bool isWindowActive() = 0;
    virtual bool isWindowFocused() = 0;
    virtual bool isWindowMinimized() = 0;
    virtual bool isWindowFullscreen() = 0;
    virtual bool setWindowPosition(int x, int y) = 0;
    virtual bool getWindowPosition(int* x, int* y) = 0;
    virtual void setWindowMinimized() = 0;
    virtual void setWindowMaximized() = 0;
    
    virtual std::string getClipboardContent() = 0;
    virtual void setClipboardContent(std::string text) = 0;

    virtual void setCursorVisible(bool visible) = 0;
    virtual bool isCursorVisible() = 0;
    virtual void setCursorPosition(int x, int y) = 0;
    virtual void getCursorPosition(int* x, int* y) = 0;
    
    void sleep(unsigned int time_ms);
    unsigned long getMicroTickCount();
    
    void setEventReceiver(EventReceiver event_receiver) 
                                            {m_event_receiver = event_receiver;}

    bool createEGLContext(EGLNativeDisplayType display,
                          EGLNativeWindowType window);
    ContextManagerEGL* getEGLContext() {return m_egl_context;}
    void swapBuffers();
    
    std::vector<VideoMode> getVideoModeList() {return m_video_modes;}
    VideoMode getDesktopMode() {return m_video_desktop;}
    
    void getJoystickInfo(std::vector<JoystickInfo>& joystick_info) 
                                           {joystick_info = m_active_joysticks;}
                                           
    unsigned int getWindowWidth() {return m_window_width;}
    unsigned int getWindowHeight() {return m_window_height;}
};

#endif
