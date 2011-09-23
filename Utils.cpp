/*
================================================================================
 
 MD3 Exporter
 based on Pop'n'Fresh MD3 Exporter

 additional changes by coded by Pavel P. [VorteX] Timofeyev

================================================================================
*/

#include "Plugin.h"

#include <direct.h>
#include <commdlg.h>
#include <iostream> 
#include <stdio.h>
#include <stdarg.h>

/*
================================================================================
 
 DISK IO
 
 Copyright (C) Matthew 'pagan' Baranowski & Sander 'FireStorm' van Rossen
 distributed under GNU General Public License

================================================================================
*/

void putLittle16(short num , FILE *f)
{
	union
	{
		struct
		{
	        unsigned char b1, b2;
		};
		unsigned short i1;
	}u;

	u.i1 = num;
	fputc( u.b1, f );
	fputc( u.b2, f );
}

short getLittle16(FILE *f)
{
	union
	{
		struct
		{
	        unsigned char b1, b2;
		};
		unsigned short i1;
	}u;
	u.b1 = fgetc(f);
	u.b2 = fgetc(f);
	return u.i1;
}

void putLittle32(long num , FILE *f)
{
	union 
	{
		struct
		{
	        unsigned char b1, b2, b3, b4;
		};
		unsigned long i1;
	} u;
	u.i1 = num;
	fputc( u.b1, f );
	fputc( u.b2, f );
	fputc( u.b3, f );
	fputc( u.b4, f );
}

long getLittle32(FILE *f)
{
	union 
	{
		struct
		{
	        unsigned char b1, b2, b3, b4;
		};
		unsigned long i1;
	} u;
	u.b1 = fgetc(f);
	u.b2 = fgetc(f);
	u.b3 = fgetc(f);
	u.b4 = fgetc(f);
	return u.i1;
}

void putLittleFloat(float num, FILE *f)//32bit floating point number
{
	union 
	{
		struct
		{
	        unsigned char  b1, b2, b3, b4;
		};
		float	 f1;
	}u;
	u.f1 = num;
	fputc( u.b1, f );
	fputc( u.b2, f );
	fputc( u.b3, f );
	fputc( u.b4, f );
}

float getLittleFloat(FILE *f) //32bit floating point number
{
	union 
	{
		struct
		{
			unsigned char b1, b2, b3, b4;
		};
		float f1;
	}u;
	u.b1 = fgetc(f);
	u.b2 = fgetc(f);
	u.b3 = fgetc(f);
	u.b4 = fgetc(f);
	return u.f1;
}

void putBig16(short num , FILE *f)
{
	union 
	{
		struct
		{
	        unsigned char  b1, b2, b3, b4;
		};
		short i1;
	}u;
	u.i1 = num;
	fputc( u.b2, f );
	fputc( u.b1, f );
}

short getBig16 (FILE *f)
{
	union 
	{
		struct
		{
	        unsigned char b1, b2;
		};
		unsigned short i1;
	} u;
	u.b2 = fgetc(f);
	u.b1 = fgetc(f);
	return u.i1;
}

void putBig32( long num, FILE *f)
{
	union 
	{
		struct
		{
	        unsigned char b1, b2, b3, b4;
		};
		long i1;
	}u;
	u.i1 = num;
	fputc( u.b4, f );
	fputc( u.b3, f );
	fputc( u.b2, f );
	fputc( u.b1, f );
}

long getBig32(FILE *f)
{
	union 
	{
		struct
		{
	        unsigned char b1, b2, b3, b4;
		};
		unsigned long i1;
	}u;
	u.b4 = fgetc(f);
	u.b3 = fgetc(f);
	u.b2 = fgetc(f);
	u.b1 = fgetc(f);
	return u.i1;
}

void putBigFloat(float num, FILE *f) //32bit floating point number
{
	union 
	{
		struct
		{
	        unsigned char b1, b2, b3, b4;
		};
		float f1;
	}u;
	u.f1 = num;
	fputc( u.b4, f );
	fputc( u.b3, f );
	fputc( u.b2, f );
	fputc( u.b1, f );
}

float getBigFloat(FILE *f) //32bit floating point number
{
	union 
	{
		struct
		{
			unsigned char b1, b2, b3, b4;
		};
		float f1;
	}u;
	u.b4 = fgetc(f);
	u.b3 = fgetc(f);
	u.b2 = fgetc(f);
	u.b1 = fgetc(f);
	return u.f1;
}

void putChars(const char *c, int size, FILE *f)
{
	int i, l;

	l = (int)strlen(c);
	for (i = 0; i < l && i < size; i++)
		fputc(c[i], f);
	while (i < size)
	{
		fputc(0, f);
		i++;
	}
}

/*
================================================================================
 
 STRING UTILS

================================================================================
*/

char ConvertASCIIC[8192];
char *ConvertStr(const char *filename, int func(int c))
{
	int i;

	ConvertASCIIC[0] = 0;
	for (i = 0; i < (int)strlen(filename); i++)
		ConvertASCIIC[i] = func(filename[i]);
	ConvertASCIIC[i] = 0;
	return ConvertASCIIC;
}

void FilePath (char *path, char *dest)
{
	char *src;

	src = path + strlen(path) - 1;
	// back up until a \ or the start
	while (src != path && (*(src-1) != '/' && *(src-1) != '\\'))
		src--;
	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void FileBase (char *path, char *dest)
{
	char *src, *ext = NULL;

	src = path + strlen(path) - 1;
	// back up until a \ or the start
	while(src != path && (*(src-1) != '/' && *(src-1) != '\\'))
		src--;
	while(*src)
	{
		if (*src == '.')
			ext = dest;
		*dest++ = *src++;
	}
	if (ext)
		*ext = 0;
	else
		*dest = 0;
}

/*
================================================================================
 
 EXPORT MESSAGES

================================================================================
*/

struct ExportMsg
{
	char message[512];
	bool iserror;
	bool isdebug;
};

std::vector<ExportMsg> ExportMessages;
HWND ExportMessagesWindow;
bool ExportMessagesWarnings;

void StartExportMessages(HWND wnd)
{
	ExportMessages.clear();
	ExportMessagesWindow = wnd;
	ExportMessagesWarnings = false;
}

void ExportError(const char *text, ...)
{
	va_list argptr;
	ExportMsg msg;

	msg.iserror = true;
	msg.isdebug = false;
	msg.isdebug = true;
	va_start(argptr, text);
	vsprintf(msg.message, text, argptr);
	va_end(argptr);
	ExportMessages.push_back(msg);
}

void ExportWarning(const char *text, ...)
{
	va_list argptr;
	ExportMsg msg;

	ExportMessagesWarnings = true;
	msg.iserror = false;
	msg.isdebug = false;
	va_start(argptr, text);
	vsprintf(msg.message, text, argptr);
	va_end(argptr);
	ExportMessages.push_back(msg);
}

void ExportMessage(const char *text, ...)
{
	va_list argptr;
	ExportMsg msg;

	msg.iserror = false;
	msg.isdebug = false;
	va_start(argptr, text);
	vsprintf(msg.message, text, argptr);
	va_end(argptr);
	ExportMessages.push_back(msg);
}

void ExportDebug(const char *text, ...)
{
	va_list argptr;
	ExportMsg msg;

	msg.iserror = false;
	msg.isdebug = true;
	va_start(argptr, text);
	vsprintf(msg.message, text, argptr);
	va_end(argptr);
	SetDlgItemText( ExportMessagesWindow, IDC_TEXT_EXPORTING, msg.message);
	ExportMessages.push_back(msg);
}

void ExportState(const char *text, ...)
{
	va_list argptr;
	char msg[512];

	if (!g_show_debug)
		return;

	va_start(argptr, text);
	vsprintf(msg, text, argptr);
	va_end(argptr);
	SetDlgItemText( ExportMessagesWindow, IDC_TEXT_EXPORTING, msg);
}

void ShowExportMessages(const char *shadertext)
{
	char c[512], m_err[8192], m_msg[8192], m_debug[8192], m[24576];
	bool anyerrors = false;

	if (!ExportMessages.size())
		return;

	strcpy(m_err, "");
	strcpy(m_msg, "");
	strcpy(m_debug, "");

	// grab messages
	for (int i = 0; i < (int)ExportMessages.size(); i++)
	{
		if (ExportMessages[i].isdebug)
			sprintf(m_debug, "%s%s\r\n", m_debug, ExportMessages[i].message);
		else if (ExportMessages[i].iserror)
			sprintf(m_err, "%s %s\r\n", m_err, ExportMessages[i].message);
		else
			sprintf(m_msg, "%s %s\r\n", m_msg, ExportMessages[i].message);
	}

	// connect together
	strcpy(m, "");
	if (strcmp(m_err, ""))
	{
		anyerrors = true;
		strcpy(c, "Error message!");
		sprintf(m, "%s\r\n", m_err);
		if (strcmp(m_msg, ""))
			sprintf(m, "%sMessages:\r\n%s\r\n", m, m_msg);
	}
	else
	{
		strcpy(c, "Export succesful!");
		if (strcmp(m_msg, ""))
			sprintf(m, "%s--- Fix This Stuff ---\r\n%s\r\n", m, m_msg);
	}
	if (strcmp(m_debug, ""))
		sprintf(m, "%s--- Export Stats ---\r\n%s\r\n", m, m_debug);

	// show
#if 1
	ExportResultBox(c, m, shadertext);
#else
	if (anyerrors)
		MessageBox(GetActiveWindow(), m, c, MB_OK | MB_ICONERROR);
	else if (ExportMessagesWarnings)
		MessageBox(GetActiveWindow(), m, c, MB_OK | MB_ICONWARNING);
	else
		MessageBox(GetActiveWindow(), m, c, MB_OK | MB_ICONINFORMATION);
#endif

	// clean up
	StartExportMessages(ExportMessagesWindow);
}
