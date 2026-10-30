#include "../settings/Functions.h"
#include "../../SDK/Hooks/Hooks.h"

static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM param) {
    const auto MainWindow = [handle]() { return GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle) && handle != GetConsoleWindow(); };

    DWORD nPID = 0;
    GetWindowThreadProcessId(handle, &nPID);

    if (GetCurrentProcessId() != nPID || !MainWindow())
        return TRUE;

    *reinterpret_cast<HWND*>(param) = handle;
    return FALSE;
}

bool CInput::Init() {
    while (Draw->window == nullptr) {
        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&Draw->window));
        Sleep(200U);
    }

    this->old_wnd_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(Draw->window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hooks_t::WndProc)));
    if (this->old_wnd_proc == nullptr)
        return false;

    return true;
}

void CInput::Restore() {
    if (this->old_wnd_proc != nullptr) {
        SetWindowLongPtrW(Draw->window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(this->old_wnd_proc));
        this->old_wnd_proc = nullptr;
    }
}

bool CInput::OnWndProc(UINT msg, WPARAM wparam, LPARAM lparam) {
    if (Draw->opened && wparam != VK_INSERT && wparam != VK_DELETE)
        return false;

    WPARAM key = 0;
    EKeyState state = EKeyState::none;

    switch (msg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wparam < 256U) {
            key = wparam;
            state = EKeyState::down;

            if (wparam == VK_SHIFT) {
                key = (MapVirtualKey((lparam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT) ? VK_RSHIFT : VK_LSHIFT;
            } else if (wparam == VK_MENU) {
                key = (lparam & (1 << 24)) ? VK_RMENU : VK_LMENU;
            } else if (wparam == VK_CONTROL) {
                key = (lparam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
            }
        }
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wparam < 256U) {
            key = wparam;
            state = EKeyState::up;

            if (wparam == VK_SHIFT) {
                key = (MapVirtualKey((lparam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT) ? VK_RSHIFT : VK_LSHIFT;
            } else if (wparam == VK_MENU) {
                key = (lparam & (1 << 24)) ? VK_RMENU : VK_LMENU;
            } else if (wparam == VK_CONTROL) {
                key = (lparam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
            }
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        key = VK_LBUTTON;
        state = EKeyState::down;
        break;
    case WM_LBUTTONUP:
        key = VK_LBUTTON;
        state = EKeyState::up;
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        key = VK_RBUTTON;
        state = EKeyState::down;
        break;
    case WM_RBUTTONUP:
        key = VK_RBUTTON;
        state = EKeyState::up;
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
        key = VK_MBUTTON;
        state = EKeyState::down;
        break;
    case WM_MBUTTONUP:
        key = VK_MBUTTON;
        state = EKeyState::up;
        break;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    {
        uint16_t xButton = ((WORD)((((DWORD_PTR)((wparam)) >> 16) & 0xffff)));
        key = (xButton == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        state = EKeyState::down;
        break;
    }
    case WM_XBUTTONUP:
    {
        uint16_t xButton = ((WORD)((((DWORD_PTR)((wparam)) >> 16) & 0xffff)));
        key = (xButton == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        state = EKeyState::up;
        break;
    }
    default:
        return false;
    }

    if (key != 0 && state != EKeyState::none) {
        if (state == EKeyState::up && arr_key_state.at(key) == EKeyState::down)
            this->arr_key_state.at(key) = EKeyState::released;
        else
            this->arr_key_state.at(key) = state;

        return true;
    }

    return false;

}

bool CInput::IsKeyReleased(const uint32_t button_code) {
    if (button_code >= this->arr_key_state.size())
        return false;

    if (this->arr_key_state[button_code] == EKeyState::released) {
        this->arr_key_state[button_code] = EKeyState::up;
        return true;
    }

    return false;
}