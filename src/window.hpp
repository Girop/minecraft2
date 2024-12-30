#pragma once
#include <string>
#include <windows.h>


class Window {
public:
    Window(std::string_view name);

    Window(Window const&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window const&) = delete;
    Window& operator=(Window&&) = delete;

    void run();

    HWND get_window() const {
        return window_handle_;
    }

    HINSTANCE get_instance() const {
        return instance_handle_;
    }

private:
    BOOL init_app();
    BOOL init_instance();

    std::string window_name_;
    HINSTANCE instance_handle_{};
    HWND window_handle_{};
};
