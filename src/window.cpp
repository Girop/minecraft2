#include <assert.h>
#include "window.hpp"

Window::Window(std::string_view name):
    window_name_{name} 
{
    init_app();
    init_instance();
}

BOOL Window::init_app() 
{ 
    WNDCLASSEX wcx; 
 
    wcx.cbSize = sizeof(wcx); 
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = DefWindowProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = instance_handle_;
    wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wcx.hbrBackground = (HBRUSH) GetStockObject( WHITE_BRUSH); 
    wcx.lpszMenuName =  "MainMenu"; 
    wcx.lpszClassName = "MainWClass"; 
    wcx.hIconSm = (HICON)LoadImage(instance_handle_,  
        MAKEINTRESOURCE(5),
        IMAGE_ICON, 
        GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), 
        LR_DEFAULTCOLOR); 
 
    return RegisterClassEx(&wcx);
} 

BOOL Window::init_instance() {
    auto window_style {(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU) & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX}; // Make it not resizable

     window_handle_ = CreateWindowEx(
        0,
        "MainWClass",
        window_name_.c_str(),
        window_style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (HWND)NULL,
        (HMENU)NULL,
        instance_handle_,
        NULL
    );
    assert(window_handle_ != 0);

    ShowWindow(window_handle_, SW_SHOWDEFAULT);
    UpdateWindow(window_handle_);

    return TRUE;
}


void Window::run() {
    int fGotMessage;
    MSG msg;
    while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 && fGotMessage != -1) 
    { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    } 
}
