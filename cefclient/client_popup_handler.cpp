// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "windows.h"
#include "shellapi.h"
#include "include/cef.h"
#include "client_popup_handler.h"
#include "util.h"

ClientPopupHandler::ClientPopupHandler(CefRefPtr<CefBrowser> parentBrowser)
  : m_ParentBrowser(parentBrowser)
{
  ASSERT(m_ParentBrowser.get());
}

ClientPopupHandler::~ClientPopupHandler()
{
}


bool ClientPopupHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        CefRefPtr<CefRequest> request,
                                        NavType navType,
                                        bool isRedirect)
{
  REQUIRE_UI_THREAD();
    
  // Load the request in the the default browser.
  LPCWSTR url;
  url = LPCWSTR(request->GetURL().c_str());
  ShellExecute(NULL,L"open",url,NULL,NULL,SW_SHOWNORMAL);
  browser->CloseBrowser();
  m_ParentBrowser = NULL;

  return true;
}
