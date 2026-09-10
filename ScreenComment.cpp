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

// 댓글 이동 속도
const int COMMENT_SPEED = 2;

// 댓글 사이의 세로 간격
const int COMMENT_VERTICAL_GAP = 55;

// 댓글이 시작할 때 사용할 위쪽 여백
const int COMMENT_TOP_MARGIN = 20;


// ------------------------------------------------------------
// 현재 화면 캡처
// ------------------------------------------------------------
void CaptureScreen(HWND hwnd)
{
    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP bitmap =
        CreateCompatibleBitmap(
            screenDC,
            screenW,
            screenH
        );

    HBITMAP oldBitmap =
        (HBITMAP)SelectObject(
            memDC,
            bitmap
        );

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

    SelectObject(
        memDC,
        oldBitmap
    );

    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    if (g_background)
        DeleteObject(g_background);

    g_background = bitmap;
}


// ------------------------------------------------------------
// 댓글 추가
// ------------------------------------------------------------
void AddComment(HWND parent)
{
    if (!g_edit)
        return;

    int length =
        GetWindowTextLengthW(g_edit);

    if (length <= 0)
        return;

    std::vector<wchar_t> buffer(
        (size_t)length + 1
    );

    GetWindowTextW(
        g_edit,
        buffer.data(),
        length + 1
    );

    std::wstring text(
        buffer.data()
    );

    if (text.empty())
        return;


    // --------------------------------------------------------
    // 글자 크기 계산
    // --------------------------------------------------------
    HDC dc = GetDC(parent);

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
        (HFONT)SelectObject(
            dc,
            font
        );

    SIZE size{};

    GetTextExtentPoint32W(
        dc,
        text.c_str(),
        (int)text.length(),
        &size
    );

    SelectObject(
        dc,
        oldFont
    );

    DeleteObject(font);

    ReleaseDC(
        parent,
        dc
    );


    RECT rc{};

    GetClientRect(
        parent,
        &rc
    );


    Comment comment;

    comment.text = text;

    // --------------------------------------------------------
    // 오른쪽 화면 밖에서 시작
    // --------------------------------------------------------
    comment.x =
        rc.right + 20;


    // --------------------------------------------------------
    // 위쪽부터 순서대로 배치
    //
    // 기존 댓글 개수를 기준으로 세로 위치 결정
    // --------------------------------------------------------
    int index =
        (int)g_comments.size();

    int usableHeight =
        rc.bottom - INPUT_HEIGHT;

    int maxRows =
        (usableHeight - COMMENT_TOP_MARGIN)
        / COMMENT_VERTICAL_GAP;

    if (maxRows < 1)
        maxRows = 1;

    index %= maxRows;

    comment.y =
        COMMENT_TOP_MARGIN +
        index * COMMENT_VERTICAL_GAP;


    comment.width =
        size.cx + 30;


    g_comments.push_back(
        comment
    );


    SetWindowTextW(
        g_edit,
        L""
    );

    InvalidateRect(
        parent,
        nullptr,
        FALSE
    );
}


// ------------------------------------------------------------
// 배경 그리기
// ------------------------------------------------------------
void DrawBackground(
    HDC dc,
    RECT& rc
)
{
    if (!g_background)
        return;

    HDC memDC =
        CreateCompatibleDC(dc);

    HBITMAP oldBitmap =
        (HBITMAP)SelectObject(
            memDC,
            g_background
        );

    int width =
        GetSystemMetrics(SM_CXSCREEN);

    int height =
        GetSystemMetrics(SM_CYSCREEN);


    // --------------------------------------------------------
    // 화면을 확대/축소하지 않고
    // 캡처한 화면을 1:1로 그대로 출력
    // --------------------------------------------------------
    BitBlt(
        dc,
        0,
        0,
        min(rc.right, width),
        min(rc.bottom, height),
        memDC,
        0,
        0,
        SRCCOPY
    );


    SelectObject(
        memDC,
        oldBitmap
    );

    DeleteDC(memDC);
}


// ------------------------------------------------------------
// 댓글 그리기
// ------------------------------------------------------------
void DrawComments(
    HDC dc,
    RECT& rc
)
{
    SetBkMode(
        dc,
        TRANSPARENT
    );


    HFONT font =
        CreateFontW(
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
        (HFONT)SelectObject(
            dc,
            font
        );


    for (const auto& comment : g_comments)
    {
        int x = comment.x;
        int y = comment.y;


        // ----------------------------------------------------
        // 검은색 외곽선
        // ----------------------------------------------------
        SetTextColor(
            dc,
            RGB(0, 0, 0)
        );


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


        // ----------------------------------------------------
        // 본문은 빨간색
        // ----------------------------------------------------
        SetTextColor(
            dc,
            RGB(255, 0, 0)
        );


        TextOutW(
            dc,
            x,
            y,
            comment.text.c_str(),
            (int)comment.text.length()
        );
    }


    SelectObject(
        dc,
        oldFont
    );

    DeleteObject(font);
}


// ------------------------------------------------------------
// 댓글 이동
// ------------------------------------------------------------
void UpdateComments(
    HWND hwnd
)
{
    RECT rc{};

    GetClientRect(
        hwnd,
        &rc
    );


    int usableHeight =
        rc.bottom - INPUT_HEIGHT;


    int maxRows =
        (usableHeight - COMMENT_TOP_MARGIN)
        / COMMENT_VERTICAL_GAP;

    if (maxRows < 1)
        maxRows = 1;


    for (auto& comment : g_comments)
    {
        // -----------------------------------------------
        // 오른쪽 방향으로 이동
        // -----------------------------------------------
        comment.x += COMMENT_SPEED;
    }


    // ----------------------------------------------------
    // 화면 오른쪽으로 완전히 사라진 댓글 처리
    //
    // 사라진 댓글은 아래쪽 줄에서 다시 등장
    // ----------------------------------------------------
    for (auto& comment : g_comments)
    {
        if (comment.x > rc.right + comment.width)
        {
            comment.x =
                rc.right + 20;


            // 현재 y 위치를 기준으로
            // 다음 줄로 이동
            int currentRow =
                (comment.y - COMMENT_TOP_MARGIN)
                / COMMENT_VERTICAL_GAP;

            currentRow++;


            // 맨 아래까지 갔으면 다시 맨 위
            if (currentRow >= maxRows)
            {
                currentRow = 0;
            }


            comment.y =
                COMMENT_TOP_MARGIN +
                currentRow * COMMENT_VERTICAL_GAP;
        }
    }


    InvalidateRect(
        hwnd,
        nullptr,
        FALSE
    );
}


// ------------------------------------------------------------
// EDIT 키 처리
// ------------------------------------------------------------
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
            HWND parent =
                GetParent(hwnd);

            if (parent)
                AddComment(parent);

            return 0;
        }


        // ESC = 종료
        if (wParam == VK_ESCAPE)
        {
            HWND parent =
                GetParent(hwnd);

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


// ------------------------------------------------------------
// 윈도우 프로시저
// ------------------------------------------------------------
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
        // ----------------------------------------------------
        // 프로그램 창이 화면에 표시되기 전에 캡처
        // ----------------------------------------------------
        CaptureScreen(hwnd);


        RECT rc{};

        GetClientRect(
            hwnd,
            &rc
        );


        int width =
            rc.right;

        int height =
            rc.bottom;


        // ----------------------------------------------------
        // 입력창
        // ----------------------------------------------------
        g_edit =
            CreateWindowExW(
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


        // ----------------------------------------------------
        // 보내기 버튼
        // ----------------------------------------------------
        g_button =
            CreateWindowW(
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


        // ----------------------------------------------------
        // 16ms 타이머
        // ----------------------------------------------------
        SetTimer(
            hwnd,
            1,
            16,
            nullptr
        );


        return 0;
    }


    // --------------------------------------------------------
    // 창 크기가 변경될 때
    // --------------------------------------------------------
    case WM_SIZE:
    {
        if (g_edit && g_button)
        {
            int width =
                LOWORD(lParam);

            int height =
                HIWORD(lParam);


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


    // --------------------------------------------------------
    // 버튼
    // --------------------------------------------------------
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 1001)
        {
            AddComment(hwnd);
            return 0;
        }

        break;
    }


    // --------------------------------------------------------
    // ESC
    // --------------------------------------------------------
    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
            return 0;
        }

        break;
    }


    // --------------------------------------------------------
    // 댓글 이동
    // --------------------------------------------------------
    case WM_TIMER:
    {
        if (wParam == 1)
        {
            UpdateComments(hwnd);
            return 0;
        }

        break;
    }


    // --------------------------------------------------------
    // 화면 그리기
    // --------------------------------------------------------
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};

        HDC dc =
            BeginPaint(
                hwnd,
                &ps
            );


        RECT rc{};

        GetClientRect(
            hwnd,
            &rc
        );


        // 배경
        DrawBackground(
            dc,
            rc
        );


        // 댓글
        DrawComments(
            dc,
            rc
        );


        // ----------------------------------------------------
        // 입력창 영역
        // ----------------------------------------------------
        HBRUSH brush =
            CreateSolidBrush(
                RGB(0, 0, 0)
            );


        RECT inputArea{
            0,
            rc.bottom - INPUT_HEIGHT,
            rc.right,
            rc.bottom
        };


        FillRect(
            dc,
            &inputArea,
            brush
        );


        DeleteObject(
            brush
        );


        EndPaint(
            hwnd,
            &ps
        );


        return 0;
    }


    // --------------------------------------------------------
    // 종료
    // --------------------------------------------------------
    case WM_DESTROY:
    {
        KillTimer(
            hwnd,
            1
        );


        if (g_background)
        {
            DeleteObject(
                g_background
            );

            g_background =
                nullptr;
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


// ------------------------------------------------------------
// 프로그램 시작
// ------------------------------------------------------------
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

    wc.lpfnWndProc =
        WndProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        CLASS_NAME;

    wc.hCursor =
        LoadCursorW(
            nullptr,
            IDC_ARROW
        );


    RegisterClassW(
        &wc
    );


    // --------------------------------------------------------
    // 현재 모니터의 실제 해상도
    // --------------------------------------------------------
    int screenW =
        GetSystemMetrics(
            SM_CXSCREEN
        );

    int screenH =
        GetSystemMetrics(
            SM_CYSCREEN
        );


    // --------------------------------------------------------
    // 화면 전체를 덮지만
    // 창 자체의 크기는 현재 해상도로 고정
    // --------------------------------------------------------
    g_hWnd =
        CreateWindowExW(
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


    // --------------------------------------------------------
    // 최대화/확대 없이 그대로 표시
    // --------------------------------------------------------
    ShowWindow(
        g_hWnd,
        SW_SHOW
    );


    UpdateWindow(
        g_hWnd
    );


    SetForegroundWindow(
        g_hWnd
    );


    MSG msg{};


    while (
        GetMessageW(
            &msg,
            nullptr,
            0,
            0
        )
    )
    {
        TranslateMessage(
            &msg
        );

        DispatchMessageW(
            &msg
        );
    }


    return (int)msg.wParam;
}
