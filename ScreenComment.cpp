#define UNICODE
#define _UNICODE
#include <windows.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

struct Comment { std::wstring text; float x,y,speed,alpha; };
HBITMAP bg=nullptr; HDC bgdc=nullptr; HWND editBox=nullptr, sendBtn=nullptr;
HFONT commentFont=nullptr; std::vector<Comment> comments; std::mt19937 rng((unsigned)GetTickCount());

void Capture(){
    int w=GetSystemMetrics(SM_CXSCREEN), h=GetSystemMetrics(SM_CYSCREEN);
    HDC s=GetDC(nullptr);
    bgdc=CreateCompatibleDC(s); bg=CreateCompatibleBitmap(s,w,h);
    HGDIOBJ old=SelectObject(bgdc,bg);
    BitBlt(bgdc,0,0,w,h,s,0,0,SRCCOPY);
    SelectObject(bgdc,old); ReleaseDC(nullptr,s);
}
void Add(HWND hwnd){
    int n=GetWindowTextLengthW(editBox); if(!n) return;
    std::wstring t(n,L'\0'); GetWindowTextW(editBox,t.data(),n+1);
    RECT r; GetClientRect(hwnd,&r);
    std::uniform_real_distribution<float> x(20.f,(float)max(21,r.right-400));
    std::uniform_real_distribution<float> sp(1.3f,2.7f);
    comments.push_back({t,x(rng),(float)r.bottom-105,sp(rng),1.f});
    SetWindowTextW(editBox,L""); SetFocus(editBox); InvalidateRect(hwnd,nullptr,FALSE);
}
LRESULT CALLBACK W(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
    case WM_CREATE:{
        Capture();
        commentFont=CreateFontW(30,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Malgun Gothic");
        editBox=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,20,0,500,45,h,(HMENU)1,nullptr,nullptr);
        sendBtn=CreateWindowW(L"BUTTON",L"전송",WS_CHILD|WS_VISIBLE,530,0,80,45,h,(HMENU)2,nullptr,nullptr);
        SetTimer(h,1,16,nullptr); SetFocus(editBox); return 0;}
    case WM_SIZE:{
        int W=LOWORD(l),H=HIWORD(l);
        MoveWindow(editBox,20,H-60,max(100,W-130),45,TRUE);
        MoveWindow(sendBtn,W-100,H-60,80,45,TRUE); return 0;}
    case WM_COMMAND:
        if(LOWORD(w)==2 && HIWORD(w)==BN_CLICKED) Add(h);
        return 0;
    case WM_TIMER:
        for(auto &c:comments){c.y-=c.speed;c.alpha-=.0015f;}
        comments.erase(remove_if(comments.begin(),comments.end(),[](auto&c){return c.y<-70||c.alpha<=0;}),comments.end());
        InvalidateRect(h,nullptr,FALSE); return 0;
    case WM_PAINT:{
        PAINTSTRUCT p; HDC d=BeginPaint(h,&p); RECT r; GetClientRect(h,&r);
        BitBlt(d,0,0,r.right,r.bottom,bgdc,0,0,SRCCOPY);
        SetBkMode(d,TRANSPARENT); HGDIOBJ old=SelectObject(d,commentFont);
        for(auto &c:comments){
            RECT a={(int)c.x+3,(int)c.y+3,r.right,(int)c.y+60};
            SetTextColor(d,RGB(0,0,0)); DrawTextW(d,c.text.c_str(),-1,&a,DT_LEFT|DT_SINGLELINE);
            RECT b={(int)c.x,(int)c.y,r.right,(int)c.y+60};
            SetTextColor(d,RGB(255,255,255)); DrawTextW(d,c.text.c_str(),-1,&b,DT_LEFT|DT_SINGLELINE);
        }
        SelectObject(d,old);
        HBRUSH br=CreateSolidBrush(RGB(20,20,20)); RECT bar={0,r.bottom-70,r.right,r.bottom}; FillRect(d,&bar,br); DeleteObject(br);
        EndPaint(h,&p); return 0;}
    case WM_KEYDOWN: if(w==VK_ESCAPE){DestroyWindow(h);return 0;} return 0;
    case WM_DESTROY:
        KillTimer(h,1); if(commentFont)DeleteObject(commentFont);
        if(bgdc)DeleteDC(bgdc); if(bg)DeleteObject(bg); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE, PWSTR,int){
    WNDCLASSW c{}; c.lpfnWndProc=W; c.hInstance=hi; c.lpszClassName=L"ScreenComment"; c.hCursor=LoadCursor(nullptr,IDC_ARROW); RegisterClassW(&c);
    HWND h=CreateWindowExW(WS_EX_TOPMOST,L"ScreenComment",L"ScreenComment",WS_POPUP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),nullptr,nullptr,hi,nullptr);
    ShowWindow(h,SW_SHOW); UpdateWindow(h); MSG m; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);} return 0;
}
