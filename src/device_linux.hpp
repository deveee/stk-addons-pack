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

#ifndef DEVICE_LINUX_HPP
#define DEVICE_LINUX_HPP

#if (defined(__linux__) || defined(__CYGWIN__)) && !defined(ANDROID)

#include "device.hpp"

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

class DeviceLinux : public Device
{
private:
    Display* m_display;
    Window m_window;
    XVisualInfo* m_visual;
    XSizeHints* m_std_hints;
    std::string m_clipboard;
    int m_screen_nr;

    bool m_window_has_focus;
    bool m_window_minimized;
    bool m_window_fullscreen;
    bool m_close;
    bool m_netwm_supported;
    Atom m_atom_clipboard;
    Atom m_atom_targets;
    Atom m_atom_utf8_string;
    
    RROutput m_output_id;
    RRMode m_default_mode;
    int m_crtc_x;
    int m_crtc_y;
    bool m_mode_changed;

    XIM m_input_method;
    XIC m_input_context;
    int m_numlock_mask;
    std::map<int, KeyId> m_key_map;

    Cursor m_cursor_invisible;
    bool m_cursor_is_visible;
    int m_cursor_x;
    int m_cursor_y;

    bool createWindow();
    Window getToplevelParent();
    static int printXError(Display* display, XErrorEvent* event);
    
    bool createVideoModeList();
    bool changeResolution();
    bool restoreResolution();
    
    bool createInputContext();
    void destroyInputContext();
    void findNumlockMask();
    void createKeyMap();
    KeyId getKeyCode(XEvent &event);
    
    void initCursor();
    void closeCursor();
    
    void activateJoysticks();
    void pollJoysticks();
    void closeJoysticks();
    
public:
    DeviceLinux();
    ~DeviceLinux();
    
    bool initDevice(const CreationParams& creation_params);
    void closeDevice() {m_close = true;}
    bool processEvents();
    void clearSystemMessages();
    
    void setWindowCaption(const char* text);
    void setWindowClass(const char* text);
    void setWindowFullscreen(bool fullscreen);
    void setWindowResizable(bool resizable);
    bool isWindowActive() {return m_window_has_focus && !m_window_minimized;}
    bool isWindowFocused() {return m_window_has_focus;}
    bool isWindowMinimized() {return m_window_minimized;}
    bool isWindowFullscreen() {return m_window_fullscreen;}
    bool setWindowPosition(int x, int y);
    bool getWindowPosition(int* x, int* y);
    void setWindowMinimized();
    void setWindowMaximized();
    
    std::string getClipboardContent();
    void setClipboardContent(std::string text);

    void setCursorVisible(bool visible);
    bool isCursorVisible() {return m_cursor_is_visible;}
    void setCursorPosition(int x, int y);
    void getCursorPosition(int* x, int* y);
};

#endif

#endif
