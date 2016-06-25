/*
================================================================================
 
 MD3 Exporter
 based on Pop'n'Fresh MD3 Exporter

 Helper lib to deal with unicode
 coded by Pavel P. [VorteX] Timofeyev

================================================================================
*/

#include "Plugin.h"

// unicode conversion tools
#ifdef UNICODE
wchar_t *GetWC(const char *c)
{
	size_t clen = strlen(c) + 1;
	wchar_t *wc = (wchar_t *)malloc(clen * sizeof(wchar_t));
    mbstowcs(wc, c, clen);
    return wc;
}
char *GetChar(const wchar_t *w)
{
	size_t wlen = wcslen(w) + 1;
	char *cc = (char *)malloc(wlen * 4);
	wcstombs(cc, w, wlen);
    return cc;
}
#endif

// setting/getting valeus of dialog items
void GuiGetItemText(HWND hDlg, int nIDDlgItem, char *lpString, int cchMax)
{
#ifdef UNICODE
	wchar_t *ranges_unicode;
	ranges_unicode = (wchar_t *)malloc(cchMax * sizeof(wchar_t));
	GetDlgItemText(hDlg, nIDDlgItem, ranges_unicode, cchMax);
	strcpy(lpString, GetChar(ranges_unicode));
#else
	GetDlgItemText(hDlg, nIDDlgItem, lpString, cchMax);
#endif
		
}

void GuiSetItemText(HWND hDlg, int nIDDlgItem, const char *lpString)
{
#ifdef UNICODE
	wchar_t *wc = GetWC(lpString);
	SetDlgItemText(hDlg, nIDDlgItem, wc);
	free(wc);
#else
	SetDlgItemText(hDlg, nIDDlgItem, lpString);
#endif
}

void GuiSetWindowText(HWND hWnd, const char *lpString)
{
#ifdef UNICODE
	wchar_t *wc = GetWC(lpString);
	SetWindowText(hWnd, wc);
	free(wc);
#else
	SetWindowText(hWnd, lpString);
#endif
}