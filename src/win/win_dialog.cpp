/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "win_dialog.h"
#include "win_taskbar.h"

namespace win {

// =============================================================================

Dialog::Dialog() :
  m_bModal(true), m_iSnapGap(0)
{
  m_SizeLast.cx = 0; m_SizeLast.cy = 0;
  m_SizeMax.cx  = 0; m_SizeMax.cy  = 0;
  m_SizeMin.cx  = 0; m_SizeMin.cy  = 0;
  m_PosLast.x   = 0; m_PosLast.y   = 0;
}

Dialog::~Dialog() {
  EndDialog(0);
}

INT_PTR Dialog::Create(UINT uResourceID, HWND hParent, bool bModal) {
  m_bModal = bModal;
  if (bModal) {
    INT_PTR nResult = ::DialogBoxParam(instance_, MAKEINTRESOURCE(uResourceID), 
      hParent, DialogProcStatic, reinterpret_cast<LPARAM>(this));
    window_ = NULL;
    return nResult;
  } else {
    window_ = ::CreateDialogParam(instance_, MAKEINTRESOURCE(uResourceID), 
      hParent, DialogProcStatic, reinterpret_cast<LPARAM>(this));
    return reinterpret_cast<INT_PTR>(window_);
  }
}

void Dialog::EndDialog(INT_PTR nResult) {
  // Remember last position and size
  RECT rect; GetWindowRect(&rect);
  m_PosLast.x = rect.left;
  m_PosLast.y = rect.top;
  m_SizeLast.cx = rect.right - rect.left;
  m_SizeLast.cy = rect.bottom - rect.top;

  // Destroy window
  if (::IsWindow(window_)) {
    if (m_bModal) {
      ::EndDialog(window_, nResult);
    } else {
      Destroy();
    }
  }
  window_ = NULL;
}

void Dialog::SetSizeMax(LONG cx, LONG cy) {
  m_SizeMax.cx = cx; m_SizeMax.cy = cy;
}

void Dialog::SetSizeMin(LONG cx, LONG cy) {
  m_SizeMin.cx = cx; m_SizeMin.cy = cy;
}

void Dialog::SetSnapGap(int iSnapGap) {
  m_iSnapGap = iSnapGap;
}

// =============================================================================

void Dialog::SetMinMaxInfo(LPMINMAXINFO lpMMI) {
  if (m_SizeMax.cx > 0) lpMMI->ptMaxTrackSize.x = m_SizeMax.cx;
  if (m_SizeMax.cy > 0) lpMMI->ptMaxTrackSize.y = m_SizeMax.cy;
  if (m_SizeMin.cx > 0) lpMMI->ptMinTrackSize.x = m_SizeMin.cx;
  if (m_SizeMin.cy > 0) lpMMI->ptMinTrackSize.y = m_SizeMin.cy;
}

void Dialog::SnapToEdges(LPWINDOWPOS lpWndPos) {
  if (m_iSnapGap == 0) return;

  HMONITOR hMonitor;
  MONITORINFO mi;
  RECT mon;

  // Get monitor info
  SystemParametersInfo(SPI_GETWORKAREA, NULL, &mon, NULL);
  if (GetSystemMetrics(SM_CMONITORS) > 1) {
    hMonitor = MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST);
    if (hMonitor) {
      mi.cbSize = sizeof(mi);
      GetMonitorInfo(hMonitor, &mi);
      mon = mi.rcWork;
    }
  }
  // Snap X axis
  if (abs(lpWndPos->x - mon.left) <= m_iSnapGap) {
    lpWndPos->x = mon.left;
  } else if (abs(lpWndPos->x + lpWndPos->cx - mon.right) <= m_iSnapGap) {
    lpWndPos->x = mon.right - lpWndPos->cx;
  }
  // Snap Y axis
  if (abs(lpWndPos->y - mon.top) <= m_iSnapGap) {
    lpWndPos->y = mon.top;
  } else if (abs(lpWndPos->y + lpWndPos->cy - mon.bottom) <= m_iSnapGap) {
    lpWndPos->y = mon.bottom - lpWndPos->cy;
  }
}

// =============================================================================

void Dialog::OnCancel() {
  EndDialog(IDCANCEL);
}

BOOL Dialog::OnClose() {
  return FALSE;
}

BOOL Dialog::OnInitDialog() {
  return TRUE;
}

void Dialog::OnOK() {
  EndDialog(IDOK);
}

// Superclassing
void Dialog::RegisterDlgClass(LPCWSTR lpszClassName) {
  WNDCLASS wc;
  ::GetClassInfo(instance_, WC_DIALOG, &wc); // WC_DIALOG is #32770
  wc.lpszClassName = lpszClassName;
  ::RegisterClass(&wc);
}

// =============================================================================

BOOL Dialog::AddComboString(int nIDDlgItem, LPCWSTR lpString) {
  return ::SendDlgItemMessage(window_, nIDDlgItem, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lpString));
}

BOOL Dialog::CheckDlgButton(int nIDButton, UINT uCheck) {
  return ::CheckDlgButton(window_, nIDButton, uCheck);
}

BOOL Dialog::CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton) {
  return ::CheckRadioButton(window_, nIDFirstButton, nIDLastButton, nIDCheckButton);
}

BOOL Dialog::EnableDlgItem(int nIDDlgItem, BOOL bEnable) {
  return ::EnableWindow(::GetDlgItem(window_, nIDDlgItem), bEnable);
}

INT Dialog::GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton) {
  for (int i = 0; i <= nIDLastButton - nIDFirstButton; i++) {
    if (::IsDlgButtonChecked(window_, nIDFirstButton + i)) {
      return i;
    }
  }
  return 0;
}

INT Dialog::GetComboSelection(int nIDDlgItem) {
  return ::SendDlgItemMessage(window_, nIDDlgItem, CB_GETCURSEL, 0, 0);
}

HWND Dialog::GetDlgItem(int nIDDlgItem) {
  return ::GetDlgItem(window_, nIDDlgItem);
}

UINT Dialog::GetDlgItemInt(int nIDDlgItem) {
  return ::GetDlgItemInt(window_, nIDDlgItem, NULL, TRUE);
}

void Dialog::GetDlgItemText(int nIDDlgItem, LPWSTR lpString, int cchMax) {
  ::GetDlgItemText(window_, nIDDlgItem, lpString, cchMax);
}

void Dialog::GetDlgItemText(int nIDDlgItem, wstring& str) {
  int len = ::GetWindowTextLength(::GetDlgItem(window_, nIDDlgItem)) + 1;
  vector<wchar_t> buffer(len);
  ::GetDlgItemText(window_, nIDDlgItem, &buffer[0], len);
  str.assign(&buffer[0]);
}

wstring Dialog::GetDlgItemText(int nIDDlgItem) {
  int len = ::GetWindowTextLength(::GetDlgItem(window_, nIDDlgItem)) + 1;
  vector<wchar_t> buffer(len);
  ::GetDlgItemText(window_, nIDDlgItem, &buffer[0], len);
  return wstring(&buffer[0]);
}

BOOL Dialog::HideDlgItem(int nIDDlgItem) {
  return ::ShowWindow(::GetDlgItem(window_, nIDDlgItem), SW_HIDE);
}

BOOL Dialog::IsDlgButtonChecked(int nIDButton) {
  return ::IsDlgButtonChecked(window_, nIDButton);
}

BOOL Dialog::SendDlgItemMessage(int nIDDlgItem, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return ::SendDlgItemMessage(window_, nIDDlgItem, uMsg, wParam, lParam);
}

BOOL Dialog::SetComboSelection(int nIDDlgItem, int iIndex) {
  return ::SendDlgItemMessage(window_, nIDDlgItem, CB_SETCURSEL, iIndex, 0);
}

BOOL Dialog::SetDlgItemText(int nIDDlgItem, LPCWSTR lpString) {
  return ::SetDlgItemText(window_, nIDDlgItem, lpString);
}

BOOL Dialog::ShowDlgItem(int nIDDlgItem, int nCmdShow) {
  return ::ShowWindow(::GetDlgItem(window_, nIDDlgItem), nCmdShow);
}

// =============================================================================

INT_PTR CALLBACK Dialog::DialogProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  Dialog* window = reinterpret_cast<Dialog*>(WindowMap.GetWindow(hwnd));
  if (!window && uMsg == WM_INITDIALOG) {
    window = reinterpret_cast<Dialog*>(lParam);
    if (window) {
      window->SetWindowHandle(hwnd);
      WindowMap.Add(hwnd, window);
    }
  }
  if (window) {
    return window->DialogProc(hwnd, uMsg, wParam, lParam);
  } else {
    return FALSE;
  }
}

INT_PTR Dialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

INT_PTR Dialog::DialogProcDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CLOSE: {
      return OnClose();
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDOK:
          OnOK();
          return TRUE;
        case IDCANCEL:
          OnCancel();
          return TRUE;
        default:
          return OnCommand(wParam, lParam);
      }
      break;
    }
    case WM_DESTROY: {
      OnDestroy();
      break;
    }
    case WM_INITDIALOG: {
      /*if (m_PosLast.x && m_PosLast.y) {
        SetPosition(NULL, m_PosLast.x, m_PosLast.y, 0, 0, SWP_NOSIZE);
      }
      if (m_SizeLast.cx && m_SizeLast.cy) {
        SetPosition(NULL, 0, 0, m_SizeLast.cx, m_SizeLast.cy, SWP_NOMOVE);
      }*/
      return OnInitDialog();
    }
    case WM_DROPFILES: {
      OnDropFiles(reinterpret_cast<HDROP>(wParam));
      break;
    }
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE: {
      SIZE size = {0};
      OnSize(uMsg, 0, size);
      break;
    }
    case WM_GETMINMAXINFO: {
      LPMINMAXINFO lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
      SetMinMaxInfo(lpMMI);
      OnGetMinMaxInfo(lpMMI);
      break;
    }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEHOVER:
    case WM_MOUSEHWHEEL:
    case WM_MOUSELEAVE:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL: {
      LRESULT lResult = OnMouseEvent(uMsg, wParam, lParam);
      if (lResult != -1) {
        ::SetWindowLongPtrW(hwnd, DWL_MSGRESULT, lResult);
        return TRUE;
      }
      break;
    }
    case WM_MOVE: {
      POINTS pts = MAKEPOINTS(lParam);
      OnMove(&pts);
      break;
    }
    case WM_NOTIFY: {
      LRESULT lResult = OnNotify(wParam, reinterpret_cast<LPNMHDR>(lParam));
      if (lResult) {
        ::SetWindowLongPtr(hwnd, DWL_MSGRESULT, lResult);
        return TRUE;
      }
      break;
    }
    case WM_PAINT: {
      if (::GetUpdateRect(hwnd, NULL, FALSE)) {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hwnd, &ps);
        OnPaint(hdc, &ps);
        ::EndPaint(hwnd, &ps);
      } else {
        HDC hdc = ::GetDC(hwnd);
        OnPaint(hdc, NULL);
        ::ReleaseDC(hwnd, hdc);
      }
      break;
    }
    case WM_SIZE: {
      SIZE size = {LOWORD(lParam), HIWORD(lParam)};
      OnSize(uMsg, static_cast<UINT>(wParam), size);
      break;
    }
    case WM_TIMER: {
      OnTimer(static_cast<UINT_PTR>(wParam));
      break;
    }
    case WM_WINDOWPOSCHANGING: {
      LPWINDOWPOS lpWndPos = reinterpret_cast<LPWINDOWPOS>(lParam);
      SnapToEdges(lpWndPos);
      OnWindowPosChanging(lpWndPos);
      break;
    }
    default: {
      if (uMsg == WM_TASKBARCREATED || 
          uMsg == WM_TASKBARBUTTONCREATED || 
          uMsg == WM_TASKBARCALLBACK) {
            OnTaskbarCallback(uMsg, lParam);
            return FALSE;
      }
      break;
    }
  }

  return FALSE;
}

}  // namespace win