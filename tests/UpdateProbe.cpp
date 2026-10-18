#include <windows.h>
static LRESULT CALLBACK windowProcedure(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(window,message,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR arguments,int show) {
    if(wcsstr(arguments,L"--background")){Sleep(INFINITE);return 0;}
    WNDCLASSW type{};type.lpfnWndProc=windowProcedure;type.hInstance=instance;type.lpszClassName=L"VSRendererUpdateProbe";RegisterClassW(&type);
    HWND window=CreateWindowW(type.lpszClassName,L"VSRenderer update test",WS_OVERLAPPEDWINDOW,0,0,320,180,nullptr,nullptr,instance,nullptr);
    ShowWindow(window,show);MSG message{};
    while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}return 0;
}
