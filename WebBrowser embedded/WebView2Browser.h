#pragma once

#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"
#include <string>

using namespace Microsoft::WRL;
	
UINT const WM_SET_WV2_CONTROLS = WM_USER;

// In order to be able to toggle click-through on- and off (window is always transparent)
// 1) It's necessary that the manifest doesn't have any compatibility flags
// 2) The window does can't have the WS_CLIPCHILDREN style
// 3) the transparency color can't have the same R and B values
COLORREF const TRANS_COLOR = RGB(0xDF, 0xFE, 0xEF); 

typedef void (*WebView2CB)();

class WebView2Browser {
public:
    WebView2Browser() = default;
    bool Create(HWND hWndParent, WebView2CB cb);
    void Navigate(std::wstring const & szUrl);

protected:
	HRESULT on_document_created_(HRESULT error, PCWSTR id);
	HRESULT on_web_message_received_(ICoreWebView2 * sender, ICoreWebView2WebMessageReceivedEventArgs * args);

private:
	wil::com_ptr<ICoreWebView2Controller> webviewController;
	wil::com_ptr<ICoreWebView2> webviewWindow;

    HWND hWndParent_ = 0;
	WebView2CB cb_ = 0;
	std::wstring rects_from_browser_;
};
