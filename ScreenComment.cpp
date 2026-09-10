#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <vector>
#include <string>

struct Comment {
    std::wstring text;
    int x;
    int y;
    int width;
};

HWND g_hWnd = nullptr;
HWND g_edit = nullptr;
HWND g_button = nullptr;

HBITMAP g_background = nullptr;
std::vector<Comment> g_comments;

WNDPROC g_oldEditProc = nullptr;

const int INPUT_HEIGHT = 55;
const int COMMENT_SPEED = 2;

void CaptureScreen(HWND hwnd)
{
    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, screenW, screenH);

    HBITMAP oldBitmap =
        (HBITMAP)SelectObject(memDC, bitmap);

    BitBlt(
        memDC,
        0,
        0,
        screenW,
        screenH,
        screenDC,
        0,
        0,
        SRCCOPY
    );

    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    if (g_background)
        DeleteObject(g_background);

    g_background = bitmap;
}

void AddComment(HWND parent)
{
    if (!g_edit)
        return;

    int length = GetWindowTextLengthW(g_edit);

    if (length <= 0)
        return;

    std::vector<wchar_t> buffer((size_t)length + 1);

    GetWindowTextW(
        g_edit,
        buffer.data(),
        length + 1
    );

    std::wstring text(buffer.data());

    if (text.empty())
        return;

    HDC dc = GetDC(parent);

    SIZE size{};
    GetTextExtentPoint32W(
        dc,
        text.c_str(),
        (int)text.length(),
        &size
    );

    ReleaseDC(parent, dc);

    RECT rc{};
    GetClientRect(parent, &rc);

    Comment comment;

    comment.text = text;

    // 댓글이 화면 오른쪽에서 시작하도록 함
    comment.x = rc.right + 20;

    // 입력창 바로 위에서 시작
    comment.y = rc.bottom - INPUT_HEIGHT - 45;

    comment.width = size.cx + 30;

    g_comments.push_back(comment);

    SetWindowTextW(g_edit, L"");

    InvalidateRect(parent, nullptr, FALSE);
}

void DrawBackground(HDC dc, RECT& rc)
{
    if (!g_background)
        return;

    HDC memDC = CreateCompatibleDC(dc);

    HBITMAP oldBitmap =
        (HBITMAP)SelectObject(memDC, g_background);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    BitBlt(
        dc,
        0,
        0,
        rc.right,
        rc.bottom,
        memDC,
        0,
        0,
        SRCCOPY
    );

    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);
}

void DrawComments(HDC dc, RECT& rc)
{
    SetBkMode(dc, TRANSPARENT);

    HFONT font = CreateFontW(
        30,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"맑은 고딕"
    );

    HFONT oldFont =
        (HFONT)SelectObject(dc, font);

    // 댓글 외곽선 효과
    for (const auto& comment : g_comments)
    {
        int x = comment.x;
        int y = comment.y;

        SetTextColor(dc, RGB(0, 0, 0));

        for (int dx = -2; dx <= 2; dx++)
        {
            for (int dy = -2; dy <= 2; dy++)
            {
                TextOutW(
                    dc,
                    x + dx,
                    y + dy,
                    comment.text.c_str(),
                    (int)comment.text.length()
                );
            }
        }

        SetTextColor(dc, RGB(255, 255, 255));

        TextOutW(
            dc,
            x,
            y,
            comment.text.c_str(),
            (int)comment.text.length()
        );
    }

    SelectObject(dc, oldFont);
    DeleteObject(font);
}

void UpdateComments(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    for (auto& comment : g_comments)
    {
        comment.x -= COMMENT_SPEED;
    }

    std::vector<Comment> alive;

    for (auto& comment : g_comments)
    {
        if (comment.x + comment.width > 0)
        {
            alive.push_back(comment);
        }
    }

    g_comments.swap(alive);

    InvalidateRect(hwnd, nullptr, FALSE);
}

LRESULT CALLBACK EditProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    if (message == WM_KEYDOWN)
    {
        // Enter = 댓글 등록
        if (wParam == VK_RETURN)
        {
            HWND parent = GetParent(hwnd);

            if (parent)
                AddComment(parent);

            return 0;
        }

        // ESC = 프로그램 종료
        if (wParam == VK_ESCAPE)
        {
            HWND parent = GetParent(hwnd);

            if (parent)
                DestroyWindow(parent);

            return 0;
        }
    }

    return CallWindowProcW(
        g_oldEditProc,
        hwnd,
        message,
        wParam,
        lParam
    );
}

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // 프로그램이 뜨기 전에 현재 화면을 캡처
        CaptureScreen(hwnd);

        RECT rc{};
        GetClientRect(hwnd, &rc);

        int width = rc.right;
        int height = rc.bottom;

        g_edit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD |
            WS_VISIBLE |
            ES_AUTOHSCROLL,
            10,
            height - INPUT_HEIGHT + 8,
            width - 110,
            INPUT_HEIGHT - 16,
            hwnd,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr
        );

        g_button = CreateWindowW(
            L"BUTTON",
            L"보내기",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON,
            width - 90,
            height - INPUT_HEIGHT + 8,
            80,
            INPUT_HEIGHT - 16,
            hwnd,
            (HMENU)1001,
            GetModuleHandleW(nullptr),
            nullptr
        );

        g_oldEditProc =
            (WNDPROC)SetWindowLongPtrW(
                g_edit,
                GWLP_WNDPROC,
                (LONG_PTR)EditProc
            );

        SetTimer(hwnd, 1, 16, nullptr);

        return 0;
    }

    case WM_SIZE:
    {
        if (g_edit && g_button)
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            MoveWindow(
                g_edit,
                10,
                height - INPUT_HEIGHT + 8,
                width - 110,
                INPUT_HEIGHT - 16,
                TRUE
            );

            MoveWindow(
                g_button,
                width - 90,
                height - INPUT_HEIGHT + 8,
                80,
                INPUT_HEIGHT - 16,
                TRUE
            );
        }

        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 1001)
        {
            AddComment(hwnd);
            return 0;
        }

        break;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
            return 0;
        }

        break;
    }

    case WM_TIMER:
    {
        if (wParam == 1)
        {
            UpdateComments(hwnd);
            return 0;
        }

        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};

        HDC dc = BeginPaint(hwnd, &ps);

        RECT rc{};
        GetClientRect(hwnd, &rc);

        // 캡처한 화면 표시
        DrawBackground(dc, rc);

        // 댓글 표시
        DrawComments(dc, rc);

        // 입력창 부분을 살짝 어둡게
        HBRUSH brush =
            CreateSolidBrush(RGB(0, 0, 0));

        RECT inputArea{
            0,
            rc.bottom - INPUT_HEIGHT,
            rc.right,
            rc.bottom
        };

        FillRect(dc, &inputArea, brush);

        DeleteObject(brush);

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd, 1);

        if (g_background)
        {
            DeleteObject(g_background);
            g_background = nullptr;
        }

        PostQuitMessage(0);

        return 0;
    }
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int
)
{
    const wchar_t CLASS_NAME[] =
        L"ScreenCommentWindow";

    WNDCLASSW wc{};

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(
        nullptr,
        IDC_ARROW
    );

    RegisterClassW(&wc);

    int screenW =
        GetSystemMetrics(SM_CXSCREEN);

    int screenH =
        GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        CLASS_NAME,
        L"Screen Comment",
        WS_POPUP,
        0,
        0,
        screenW,
        screenH,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hWnd)
        return 0;

    ShowWindow(
        g_hWnd,
        SW_SHOW
    );

    UpdateWindow(g_hWnd);

    SetForegroundWindow(g_hWnd);

    MSG msg{};

    while (GetMessageW(
        &msg,
        nullptr,
        0,
        0
    ))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
