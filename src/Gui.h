// Gui.h
// Helper lib to deal with unicode

// MAX2013, MAX2014 SDK and so on require unicode to be forced
#if _3DSMAX_VERSION >= 2013
	#ifndef UNICODE
		#error MAX 2013 and higher requires unicode charset! Please update your project config. 
	#endif
#endif

// unicode conversion tools
#ifdef UNICODE
	char	*GetChar(const wchar_t *c);;
#else
	#define  GetChar(s) s
#endif

// setting/getting values of dialog items
void GuiGetItemText(HWND hDlg, int nIDDlgItem, char *lpString, int cchMax);
void GuiSetItemText(HWND hDlg, int nIDDlgItem, const char *lpString);
void GuiSetWindowText(HWND hWnd, const char *lpString);