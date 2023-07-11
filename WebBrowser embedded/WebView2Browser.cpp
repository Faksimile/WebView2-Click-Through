#include "WebView2Browser.h"

bool WebView2Browser::Create(HWND hWndParent, WebView2CB cb) {
	hWndParent_ = hWndParent;
	cb_ = cb;
    // Locate the browser and set up the environment for WebView
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

        // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
        env->CreateCoreWebView2Controller(hWndParent_, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
            if (controller != nullptr) {
                webviewController = controller;
                webviewController->get_CoreWebView2(&webviewWindow);
            }

            // Add a few settings for the webview
            // The demo step is redundant since the values are the default settings
            ICoreWebView2Settings* Settings;
            webviewWindow->get_Settings(&Settings);
            Settings->put_IsScriptEnabled(TRUE);
            Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            Settings->put_IsWebMessageEnabled(TRUE);

			EventRegistrationToken token;
			result = webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>
				(this, &WebView2Browser::on_web_message_received_).Get(), &token);

            // Resize the WebView2 control to fit the bounds of the parent window
			RECT rcClient{};
			GetClientRect(hWndParent_, &rcClient);
			webviewController->put_Bounds(RECT{0, 0, rcClient.right, rcClient.bottom});

			// without default background color, transparent areas of html page will have white color
			COREWEBVIEW2_COLOR const trans_color{0, GetRValue(TRANS_COLOR), GetGValue(TRANS_COLOR), GetBValue(TRANS_COLOR)};
			wil::com_ptr<ICoreWebView2Controller2> controller2 = webviewController.query<ICoreWebView2Controller2>();
            result = controller2->put_DefaultBackgroundColor(trans_color);

			// without setting body background color, non-transparent areas of html page will not be visible
			wchar_t set_bkgnd_color[1024]{};
			swprintf(set_bkgnd_color, L"document.body.style.background = 'rgba(%d,%d,%d,0)'", trans_color.R, trans_color.G, trans_color.B);
			result = webviewWindow->AddScriptToExecuteOnDocumentCreated(set_bkgnd_color,
				Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(this, &WebView2Browser::on_document_created_).Get());

            return S_OK;
        }).Get());
        return S_OK;
    }).Get());

    return TRUE;
}

void WebView2Browser::Navigate(std::wstring const & szUrl) {
    webviewWindow->Navigate(szUrl.c_str());
}

HRESULT 
WebView2Browser::on_document_created_(HRESULT error, PCWSTR id) {
	if (cb_) (*cb_)();
	return S_OK;
}

HRESULT WebView2Browser::on_web_message_received_(ICoreWebView2 * sender, ICoreWebView2WebMessageReceivedEventArgs * args) {
	LPWSTR pwStr = nullptr;
	if (SUCCEEDED(args->TryGetWebMessageAsString(&pwStr))) {
		rects_from_browser_ = pwStr;
		::PostMessage(hWndParent_, WM_SET_WV2_CONTROLS, 0, (LPARAM)rects_from_browser_.c_str());
	}
	return S_OK;
}
