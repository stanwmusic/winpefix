// MainWindowWin.cpp
// Copyright (c) 2015 Arkadiusz Bokowy
//
// This file is a part of an WinPEFix.
//
// This projected is licensed under the terms of the MIT license.

#include "MainWindowWin.h"

#include <algorithm>
#include <shlwapi.h>
#include <windowsx.h>

#include "PELinkFix.h"

using std::sort;
using std::unique;


namespace acp {
	/* Strings encoding is a portability nightmare, specially when it comes to
	 * Windows. In order to keep core engine as much universal as possible, we
	 * need to convert between wide characters and narrow ones. */

	std::string encode(const std::wstring &wstr) {
		if (wstr.empty())
			return std::string();
		int size = WideCharToMultiByte(CP_THREAD_ACP, 0, &wstr[0],
				(int)wstr.size(), NULL, 0, NULL, NULL);
		std::string str(size, 0);
		WideCharToMultiByte(CP_THREAD_ACP, 0, &wstr[0],
				(int)wstr.size(), &str[0], size, NULL, NULL);
		return str;
	}

	std::wstring decode(const std::string &str) {
		if (str.empty())
			return std::wstring();
		int size = MultiByteToWideChar(CP_THREAD_ACP, 0, &str[0],
				(int)str.size(), NULL, 0);
		std::wstring wstr (size, 0);
		MultiByteToWideChar(CP_THREAD_ACP, 0, &str[0],
				(int)str.size(), &wstr[0], size);
		return wstr;
	}

}


MainWindow::MainWindow(LPCTSTR lpTemplateName) :
		prevWidth(0),
		prevHeight(0) {

	create(lpTemplateName);

	hWndSelectButon = GetDlgItem(dialog(), IDC_SELECT);
	hWndProcessButton = GetDlgItem(dialog(), IDC_PROCESS);
	hWndEditbox = GetDlgItem(dialog(), IDC_TEXT);

	enableProcessing(FALSE);

}

LRESULT MainWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SELECT:
			selectFiles();
			break;
		case IDC_PROCESS:
			process();
			break;
		}
		break;

	case WM_GETMINMAXINFO: {
		MINMAXINFO *mmi = (MINMAXINFO *)lParam;
		LONG base = GetDialogBaseUnits();
		mmi->ptMinTrackSize.x = MulDiv(150, LOWORD(base), 4);
		mmi->ptMinTrackSize.y = MulDiv(75, HIWORD(base), 8);
		break;
	}

	case WM_SIZE:
		if (wParam == SIZE_RESTORED)
			maintainLayout(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_CLOSE:
		DestroyWindow(dialog());
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	}

	return 0;
}

VOID MainWindow::selectFiles() {

	OPENFILENAME ofn = { 0 };
	TCHAR szFileName[MAX_PATH] = TEXT("");

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = dialog();
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = sizeof(szFileName);
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_EXPLORER;

	if (!GetOpenFileName(&ofn))
		return;

	vector<wstring> lfiles;

	LPTSTR lpstr = ofn.lpstrFile + lstrlen(ofn.lpstrFile) + 1;
	if (!*lpstr)
		lfiles.push_back(ofn.lpstrFile);
	else
		for (; *lpstr; lpstr += lstrlen(lpstr) + 1) {
			TCHAR file[MAX_PATH];
			lfiles.push_back(PathCombine(file, ofn.lpstrFile, lpstr));
		}

	files.insert(files.end(), lfiles.begin(), lfiles.end());
	sort(files.begin(), files.end());
	files.erase(unique(files.begin(), files.end()), files.end());

	enableProcessing(TRUE);

	consoleLog(TEXT("Selected files:"));
	for (auto i = lfiles.begin(); i != lfiles.end(); i++)
		consoleLog((TEXT(" - ") + *i).c_str());

}

VOID MainWindow::process() {

	consoleLog(TEXT("Processing..."));
	for (auto i = files.begin(); i != files.end(); i++) {
		consoleLog((TEXT(" - ") + *i).c_str());

		PELinkFix pe(acp::encode(*i).c_str());
		if (!pe.process())
			consoleLog((TEXT("error: ") + acp::decode(pe.getErrorString())).c_str());
	}

	files.clear();
	enableProcessing(FALSE);
	consoleLog(TEXT("Done."));

}

VOID MainWindow::consoleLog(LPCTSTR message) {

	INT length = GetWindowTextLength(hWndEditbox);
	length += lstrlen(message) + 2 + 1;

	TCHAR *logdata = new TCHAR[length];
	GetWindowText(hWndEditbox, logdata, length);

	lstrcat(logdata, message);
	lstrcat(logdata, TEXT("\r\n"));

	SetWindowText(hWndEditbox, logdata);
	SendMessage(hWndEditbox, EM_LINESCROLL, 0, 100);

	delete[] logdata;
}

VOID MainWindow::consoleClear() {
	SetWindowText(hWndEditbox, NULL);
}

/* Maintain window widgets layout during the resize action. Take in mind,
 * that this method is tightly coupled with the dialog template. */
VOID MainWindow::maintainLayout(INT width, INT height) {

	if (prevWidth && prevHeight) {

		RECT rect;
		POINT pt;

		INT dx = width - prevWidth;
		INT dy = height - prevHeight;

		GetWindowRect(hWndSelectButon, &rect);
		pt.x = rect.left; pt.y = rect.top;
		ScreenToClient(dialog(), &pt);
		SetWindowPos(hWndSelectButon, 0, pt.x + dx, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		GetWindowRect(hWndProcessButton, &rect);
		pt.x = rect.left; pt.y = rect.top;
		ScreenToClient(dialog(), &pt);
		SetWindowPos(hWndProcessButton, 0, pt.x + dx, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		GetWindowRect(hWndEditbox, &rect);
		pt.x = rect.right - rect.left;
		pt.y = rect.bottom - rect.top;
		SetWindowPos(hWndEditbox, 0, 0, 0, pt.x + dx, pt.y + dy, SWP_NOMOVE | SWP_NOZORDER);
	}

	prevWidth = width;
	prevHeight = height;
}

VOID MainWindow::enableProcessing(BOOL enabled) {
	EnableWindow(hWndProcessButton, enabled);
}
