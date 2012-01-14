// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "include/cef.h"
#include "client_handler.h"
#include "cefclient.h"
#include "download_handler.h"
#include <sstream>
#include <stdio.h>
#include <string>

ClientHandler::ClientHandler()
  : m_MainHwnd(NULL),
    m_BrowserHwnd(NULL),
    m_bFormElementHasFocus(false)
{
}

ClientHandler::~ClientHandler()
{
}

void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  REQUIRE_UI_THREAD();

  AutoLock lock_scope(this);
  if(!m_Browser.get())
  {
    // We need to keep the main child window, but not popup windows
    m_Browser = browser;
    m_BrowserHwnd = browser->GetWindowHandle();
  }
}

bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
  REQUIRE_UI_THREAD();

  if (m_BrowserHwnd == browser->GetWindowHandle()) {
    // Since the main window contains the browser window, we need to close
    // the parent window instead of the browser window.
    CloseMainWindow();

    // Return true here so that we can skip closing the browser window 
    // in this pass. (It will be destroyed due to the call to close
    // the parent above.)
    return true;
  }

  // A popup browser window is not contained in another window, so we can let
  // these windows close by themselves.
  return false;
}

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  REQUIRE_UI_THREAD();

  if(m_BrowserHwnd == browser->GetWindowHandle()) {
    // Free the browser pointer so that the browser can be destroyed
    m_Browser = NULL;
  }
}

bool ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& failedUrl,
                                CefString& errorText)
{
  REQUIRE_UI_THREAD();

  if(errorCode == ERR_CACHE_MISS) {
    // Usually caused by navigating to a page with POST data via back or
    // forward buttons.
    errorText = "<html><head><title>Expired Form Data</title></head>"
                "<body><h1>Expired Form Data</h1>"
                "<h2>Your form request has expired. "
                "Click reload to re-submit the form data.</h2></body>"
                "</html>";
  } else {
    // All other messages.
    std::stringstream ss;
    ss <<       "<html><head><title>Load Failed</title></head>"
                "<body><h1>Load Failed</h1>"
                "<h2>Load of URL " << std::string(failedUrl) <<
                " failed with error code " << static_cast<int>(errorCode) <<
                ".</h2></body>"
                "</html>";
    errorText = ss.str();
  }

  return false;
}

bool ClientHandler::GetDownloadHandler(CefRefPtr<CefBrowser> browser,
                                       const CefString& mimeType,
                                       const CefString& fileName,
                                       int64 contentLength,
                                       CefRefPtr<CefDownloadHandler>& handler)
{
  REQUIRE_UI_THREAD();

  // Create the handler for the file download.
  handler = CreateDownloadHandler(this, fileName);

  // Close the browser window if it is a popup with no other document contents.
  if (browser->IsPopup() && !browser->HasDocument())
    browser->CloseBrowser();

  return true;
}

void ClientHandler::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefDOMNode> node)
{
  REQUIRE_UI_THREAD();

  // Set to true if a form element has focus.
  m_bFormElementHasFocus = (node.get() && node->IsFormControlElement());
}

bool ClientHandler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
                               KeyEventType type,
                               int code,
                               int modifiers,
                               bool isSystemKey,
                               bool isAfterJavaScript)
{
  REQUIRE_UI_THREAD();

  if (isAfterJavaScript && !m_bFormElementHasFocus && code == 0x20) {
    // Special handling for the space character if a form element does not have
    // focus.
    if (type == KEYEVENT_RAWKEYDOWN) {
      browser->GetMainFrame()->ExecuteJavaScript(
          "alert('You pressed the space bar!');", "", 0);
    }
    return true;
  }

  return false;
}

bool ClientHandler::OnDragStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask)
{
  REQUIRE_UI_THREAD();

  // Forbid dragging of image files.
  if (dragData->IsFile()) {
    std::string fileExt = dragData->GetFileExtension();
    if (fileExt == ".png" || fileExt == ".jpg" || fileExt == ".gif")
      return true;
  }

  return false;
}

bool ClientHandler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                DragOperationsMask mask)
{
  REQUIRE_UI_THREAD();

  // Forbid dragging of link URLs.
  if (dragData->IsLink())
    return true;

  return false;
}

void ClientHandler::NotifyDownloadComplete(const CefString& fileName)
{
  SetLastDownloadFile(fileName);
  SendNotification(NOTIFY_DOWNLOAD_COMPLETE);
}

void ClientHandler::NotifyDownloadError(const CefString& fileName)
{
  SetLastDownloadFile(fileName);
  SendNotification(NOTIFY_DOWNLOAD_ERROR);
}

void ClientHandler::SetMainHwnd(CefWindowHandle hwnd)
{
  AutoLock lock_scope(this);
  m_MainHwnd = hwnd;
}

void ClientHandler::SetLastDownloadFile(const std::string& fileName)
{
  AutoLock lock_scope(this);
  m_LastDownloadFile = fileName;
}

std::string ClientHandler::GetLastDownloadFile()
{
  AutoLock lock_scope(this);
  return m_LastDownloadFile;
}

bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
                                  const CefPopupFeatures& popupFeatures,
                                  CefWindowInfo& windowInfo,
                                  const CefString& url,
                                  CefRefPtr<CefClient>& client,
                                  CefBrowserSettings& settings)
{
  REQUIRE_UI_THREAD();
  std::string urlStr = url;
  if(urlStr.find("chrome-devtools:") == std::string::npos) {
    // Show all popup windows excluding DevTools in the current window.
    windowInfo.m_dwStyle &= ~WS_VISIBLE;
    client = new ClientPopupHandler(m_Browser);
  }
  return false;
}

// Disables right-click menus.
bool ClientHandler::OnBeforeMenu(CefRefPtr<CefBrowser> browser,
                                const CefMenuInfo& menuInfo)
{
  return true;
}

void ClientHandler::SendNotification(NotificationType type)
{
  UINT id;
  switch(type)
  {
  case NOTIFY_CONSOLE_MESSAGE:
    id = ID_WARN_CONSOLEMESSAGE;
    break;
  case NOTIFY_DOWNLOAD_COMPLETE:
    id = ID_WARN_DOWNLOADCOMPLETE;
    break;
  case NOTIFY_DOWNLOAD_ERROR:
    id = ID_WARN_DOWNLOADERROR;
    break;
  default:
    return;
  }
  PostMessage(m_MainHwnd, WM_COMMAND, id, 0);
}

void ClientHandler::CloseMainWindow()
{
  ::PostMessage(m_MainHwnd, WM_CLOSE, 0, 0);
}
