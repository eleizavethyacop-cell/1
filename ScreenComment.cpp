#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

struct Comment
{
    std::wstring text;
    float x;
    float y;
    float speed;
};

HBITMAP bg = nullptr;
HDC bgdc = nullptr;

HWND editBox = nullptr;
HWND sendBtn = nullptr;

HFONT commentFont = nullptr;

std::vector<Comment> comments;

std::mt19937 rng((unsigned)GetTickCount());

WNDPROC oldEditProc = nullptr;


// ========================================
// 현재 화면 캡처
// ========================================
void CaptureScreen()
{
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC screenDC = GetDC(nullptr);

    bgdc = CreateCompatibleDC(screenDC);
    bg = CreateCompatibleBitmap(
        screenDC,
        width,
        height
    );

    HGDIOBJ oldBitmap = SelectObject(
        bgdc,
        bg
    );

    BitBlt(
        bgdc,
        0,
        0,
        width,
        height,
        screenDC,
        0,
        0,
        SRCCOPY
    );

    SelectObject(
        bgdc,
        oldBitmap
    );

    ReleaseDC(
        nullptr,
        screenDC
    );
}


// ========================================
// 댓글 추가
// ========================================
void AddComment(HWND hwnd)
{
    int length = GetWindowTextLengthW(editBox);

    if (length <= 0)
        return;

    // 문자열 저장 공간 확보
    std::vector<wchar_t> buffer(
        length + 1
    );

    GetWindowTextW(
        editBox,
        buffer.data(),
        length + 1
    );

    std::wstring text(
        buffer.data()
    );

    if (text.empty())
        return;

    RECT rect;

    GetClientRect(
        hwnd,
        &rect
    );

    int maxX = (std::max)(
        20,
        rect.right - 400
    );

    std::uniform_real_distribution<float> xDist(
        20.0f,
        (float)maxX
    );

    std::uniform_real_distribution<float> speedDist(
        1.3f,
        2.8f
    );

    Comment comment;

    comment.text = text;

    comment.x = xDist(rng);

    comment.y =
        (float)rect.bottom - 110.0f;

    comment.speed =
        speedDist(rng);

    comments.push_back(
        comment
    );

    // 입력창 비우기
    SetWindowTextW(
        editBox,
        L""
    );

    SetFocus(
        editBox
    );

    InvalidateRect(
        hwnd,
        nullptr,
        FALSE
    );
}


// ========================================
// 입력창 Enter 키 처리
// ========================================
LRESULT CALLBACK EditProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    if (
        message == WM_KEYDOWN &&
        wParam == VK_RETURN
    )
    {
        HWND parent =
            GetParent(hwnd);

        if (parent)
        {
            AddComment(parent);
        }

        return 0;
    }

    return CallWindowProcW(
        oldEditProc,
        hwnd,
        message,
        wParam,
        lParam
    );
}


// ========================================
// 메인 윈도우
// ========================================
LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (message)
    {
    // ------------------------------------
    // 시작
    // ------------------------------------
    case WM_CREATE:
    {
        // 프로그램이 시작되는 순간
        // 현재 화면을 캡처
        CaptureScreen();


        // 댓글 글꼴
        commentFont = CreateFontW(
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
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH,
            L"Malgun Gothic"
        );


        // 입력창
        editBox = CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD |
            WS_VISIBLE |
            WS_BORDER |
            ES_AUTOHSCROLL,
            20,
            20,
            500,
            45,
            hwnd,
            (HMENU)1,
            nullptr,
            nullptr
        );


        // 전송 버튼
        sendBtn = CreateWindowW(
            L"BUTTON",
            L"전송",
            WS_CHILD |
            WS_VISIBLE,
            530,
            20,
            80,
            45,
            hwnd,
            (HMENU)2,
            nullptr,
            nullptr
        );


        // Enter 키를 전송으로 사용
        oldEditProc =
            (WNDPROC)SetWindowLongPtrW(
                editBox,
                GWLP_WNDPROC,
                (LONG_PTR)EditProc
            );


        // 약 60 FPS
        SetTimer(
            hwnd,
            1,
            16,
            nullptr
        );


        SetFocus(
            editBox
        );

        return 0;
    }


    // ------------------------------------
    // 창 크기 변경
    // ------------------------------------
    case WM_SIZE:
    {
        int width =
            LOWORD(lParam);

        int height =
            HIWORD(lParam);


        MoveWindow(
            editBox,
            20,
            height - 60,
            (std::max)(
                100,
                width - 130
            ),
            45,
            TRUE
        );


        MoveWindow(
            sendBtn,
            width - 100,
            height - 60,
            80,
            45,
            TRUE
        );

        return 0;
    }


    // ------------------------------------
    // 버튼 클릭
    // ------------------------------------
    case WM_COMMAND:
    {
        if (
            LOWORD(wParam) == 2 &&
            HIWORD(wParam) == BN_CLICKED
        )
        {
            AddComment(hwnd);
        }

        return 0;
    }


    // ------------------------------------
    // 댓글 이동
    // ------------------------------------
    case WM_TIMER:
    {
        for (
            auto& comment : comments
        )
        {
            comment.y -=
                comment.speed;
        }


        // 화면 위로 완전히 사라진 댓글 삭제
        comments.erase(
            std::remove_if(
                comments.begin(),
                comments.end(),
                [](const Comment& c)
                {
                    return c.y < -70.0f;
                }
            ),
            comments.end()
        );


        InvalidateRect(
            hwnd,
            nullptr,
            FALSE
        );

        return 0;
    }


    // ------------------------------------
    // 화면 그리기
    // ------------------------------------
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        HDC dc =
            BeginPaint(
                hwnd,
                &ps
            );


        RECT rect;

        GetClientRect(
            hwnd,
            &rect
        );


        // 캡처한 화면을 배경으로 표시
        BitBlt(
            dc,
            0,
            0,
            rect.right,
            rect.bottom,
            bgdc,
            0,
            0,
            SRCCOPY
        );


        // 댓글 글꼴
        HGDIOBJ oldFont =
            SelectObject(
                dc,
                commentFont
            );


        SetBkMode(
            dc,
            TRANSPARENT
        );


        // --------------------------------
        // 댓글 표시
        // --------------------------------
        for (
            auto& comment : comments
        )
        {
            int x =
                (int)comment.x;

            int y =
                (int)comment.y;


            // 검은색 그림자
            RECT shadowRect =
            {
                x + 3,
                y + 3,
                rect.right,
                y + 60
            };


            SetTextColor(
                dc,
                RGB(
                    0,
                    0,
                    0
                )
            );


            DrawTextW(
                dc,
                comment.text.c_str(),
                -1,
                &shadowRect,
                DT_LEFT |
                DT_SINGLELINE |
                DT_NOPREFIX
            );


            // 흰색 글자
            RECT textRect =
            {
                x,
                y,
                rect.right,
                y + 60
            };


            SetTextColor(
                dc,
                RGB(
                    255,
                    255,
                    255
                )
            );


            DrawTextW(
                dc,
                comment.text.c_str(),
                -1,
                &textRect,
                DT_LEFT |
                DT_SINGLELINE |
                DT_NOPREFIX
            );
        }


        SelectObject(
            dc,
            oldFont
        );


        // --------------------------------
        // 아래 입력창 영역
        // --------------------------------
        HBRUSH brush =
            CreateSolidBrush(
                RGB(
                    20,
                    20,
                    20
                )
            );


        RECT bottomBar =
        {
            0,
            rect.bottom - 70,
            rect.right,
            rect.bottom
        };


        FillRect(
            dc,
            &bottomBar,
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


    // ------------------------------------
    // ESC = 종료
    // ------------------------------------
    case WM_KEYDOWN:
    {
        if (
            wParam == VK_ESCAPE
        )
        {
            DestroyWindow(
                hwnd
            );

            return 0;
        }

        return 0;
    }


    // ------------------------------------
    // 종료
    // ------------------------------------
    case WM_DESTROY:
    {
        KillTimer(
            hwnd,
            1
        );


        if (commentFont)
        {
            DeleteObject(
                commentFont
            );

            commentFont = nullptr;
        }


        if (bgdc)
        {
            DeleteDC(
                bgdc
            );

            bgdc = nullptr;
        }


        if (bg)
        {
            DeleteObject(
                bg
            );

            bg = nullptr;
        }


        PostQuitMessage(
            0
        );

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


// ========================================
// 프로그램 시작
// ========================================
int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int
)
{
    WNDCLASSW wc{};

    wc.lpfnWndProc =
        WindowProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        L"ScreenComment";

    wc.hCursor =
        LoadCursor(
            nullptr,
            IDC_ARROW
        );


    RegisterClassW(
        &wc
    );


    int screenWidth =
        GetSystemMetrics(
            SM_CXSCREEN
        );

    int screenHeight =
        GetSystemMetrics(
            SM_CYSCREEN
        );


    // 전체 화면
    HWND hwnd =
        CreateWindowExW(
            WS_EX_TOPMOST,
            L"ScreenComment",
            L"ScreenComment",
            WS_POPUP,
            0,
            0,
            screenWidth,
            screenHeight,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );


    if (!hwnd)
        return 1;


    ShowWindow(
        hwnd,
        SW_SHOW
    );


    UpdateWindow(
        hwnd
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


    return 0;
}
