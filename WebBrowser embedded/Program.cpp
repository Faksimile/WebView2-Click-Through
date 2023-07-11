#include <Windows.h>
#include <tchar.h>
#include <vector>

#include "WebView2Browser.h"

HINSTANCE hInst = nullptr;
HWND hWndMain = nullptr;
TCHAR* szWndTitleMain = _T("Toggling click-through on transparent Webview2 browser");
TCHAR* szWndClassMain = _T("WndClass_EmbeddedWB");

void BrowserCreated();
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

std::vector<RECT> rects;

void parse_rects(std::wstring const & input) {
	rects.clear();
	if (input.empty()) return;

	size_t p = 0;
	while (true) {
		size_t p1 = input.find(L'[', p);
		size_t p2 = input.find(L']', p1);
		if (p1 == std::wstring::npos || p2 == std::wstring::npos || p2 <= p1) return;

		int x, y, w, h;
		if (swscanf_s(input.c_str() + p1 + 1, L"%d,%d,%d,%d", &x, &y, &w, &h) != 4) return;

		rects.push_back(RECT {x, y, x + w, y + h});
		p = p2;
	}
}


WebView2Browser webBrowser;


INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT) {

	OleInitialize(NULL);

	hInst = hInstance;

	WNDCLASS wc{};
	wc.hbrBackground = CreateSolidBrush(TRANS_COLOR);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = hInst;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = szWndClassMain;
	RegisterClass(&wc);

	hWndMain = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, 
							  szWndClassMain, szWndTitleMain, 
							  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							  NULL, NULL, hInst, NULL);

	SetLayeredWindowAttributes(hWndMain, TRANS_COLOR, 0, LWA_COLORKEY);

	if (!webBrowser.Create(hWndMain, BrowserCreated)) {
		::MessageBox(hWndMain, L"Creating browser window failed", L"", MB_ICONERROR | MB_OK);
		return 0;
	}

	for (MSG msg; GetMessage(&msg, NULL, 0, 0);) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

void BrowserCreated() {
	// load html-page
	WCHAR rawPath[MAX_PATH];
	GetModuleFileNameW(hInst, rawPath, MAX_PATH);
	std::wstring path = std::wstring(_T("file:///")) + rawPath;
	std::size_t index = path.find_last_of(L"\\") - 1;
	index = path.find_last_of(L"\\", index) + 1;
	path.replace(index, path.length(), L"SamplePage.htm");
	webBrowser.Navigate(path.c_str());

	// start timer to periodically check whether mouse is over a html control
	::SetTimer(hWndMain, 0, 30, 0); 
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

		// notification from browser that there are new rectangular control areas
		// parse the rect infos
		case WM_SET_WV2_CONTROLS: {
			parse_rects((PCWSTR)lParam);
			return 0;
		}

		// for simplicity we use a timer to periodically check
		// whether the mouse is over a control in the html page
		// when the mouse is over a control, WS_EX_TRANSPARENT style is removed to make it clickable
		// when the mouse is over a transparent background area, the WS_EX_TRANSPARENT style is added
		case WM_TIMER: { 
			// there doesn't seem to be an API to retrieve the Webview2 HWND directly, so use this hack
			static HWND wv2_wnd = ::FindWindowEx(hWnd, 0, _T("Chrome_WidgetWin_0"), 0);
			if (!wv2_wnd) return 0;

			bool click_through = true;	
			// check whether mouse is over any html controls
			for (auto const & r: rects) {
				POINT pt{};
				::GetCursorPos(&pt);
				::ScreenToClient(wv2_wnd, &pt);
				if (PtInRect(&r,pt) == TRUE) {
					click_through = false;
					break;
				}
			}
			if (click_through) {
				// check whether mouse is over caption and make it clickable if so
				RECT rc {};
				::GetWindowRect(hWnd, &rc);
				rc.bottom = rc.top + ::GetSystemMetrics(SM_CYDLGFRAME) + int(::GetSystemMetrics(SM_CYCAPTION));
				POINT pt {};
				::GetCursorPos(&pt);
				click_through = !PtInRect(&rc, pt);
			}

			// toggle WS_EX_TRANSPARENT when mouse moves over control or transparent background
			static bool cur_click_through = false; // default state of Webview2 window is that click-through is disabled 
			if (click_through != cur_click_through) {
				cur_click_through = click_through;
				auto const xs = GetWindowLong(hWnd, GWL_EXSTYLE);
				SetWindowLong(hWnd, GWL_EXSTYLE,  click_through ? (xs | WS_EX_TRANSPARENT) : (xs & ~WS_EX_TRANSPARENT));
			}
		} break;

		case WM_DESTROY:
			::ExitProcess(0);
			break;
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
