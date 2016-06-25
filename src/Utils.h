/*
================================================================================
 
 MD3 Exporter
 based on Pop'n'Fresh MD3 Exporter

 additional changes by coded by Pavel P. [VorteX] Timofeyev

================================================================================
*/

#ifndef UTILS_H
#define UTILS_H

/*
================================================================================
 
 DISK I/O

================================================================================
*/

void  putLittle16(short num , FILE *f);
short getLittle16(FILE *f);
void  putLittle32(long num , FILE *f);
long  getLittle32(FILE *f);
void  putLittleFloat(float num , FILE *f);
float getLittleFloat(FILE *f);
void  putBig16(short num , FILE *f);
short getBig16(FILE *f);
void  putBig32(long num , FILE *f);
long  getBig32(FILE *f);
void  putBigFloat(float num , FILE *f);
float getBigFloat(FILE *f);
void  putChars(const char *c, int size, FILE *f);

#define get16 getLittle16
#define get32 getLittle32
#define getFloat getLittleFloat
#define put16 putLittle16
#define put32 putLittle32
#define putFloat putLittleFloat

/*
================================================================================
 
 EXPORT MESSAGES

================================================================================
*/

void StartExportMessages(HWND wnd);
void ExportError(const char *text, ...);
void ExportWarning(const char *text, ...);
void ExportMessage(const char *text, ...);
void ExportDebug(const char *text, ...);
void ExportState(const char *text, ...);
void ShowExportMessages(const char *shadertext);

#endif