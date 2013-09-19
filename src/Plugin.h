/*
================================================================================
 
 MD3 Exporter
 based on Pop'n'Fresh MD3 Exporter

 additional changes by coded by Pavel P. [VorteX] Timofeyev

================================================================================
*/

#ifndef __EXPORTMD3__H
#define __EXPORTMD3__H

#include "Max.h"
#include "PluginRes.h"
#include "Utils.h"

#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "stdmat.h"

#include <stdlib.h> 
#include <list>
#include <string>
#include <vector>

struct Range
{
	unsigned int first;
	unsigned int last;
};

extern TCHAR *GetString(int id);

extern HINSTANCE hInstance;

#define EXPORTMD3_CLASS_ID Class_ID(0x35a0cc95, 0x7098cbf0)

enum
{
	MULTIMATERIALS_NONE,
	MULTIMATERIALS_SKINS,
	MULTIMATERIALS_MODELS,
	MULTIMATERIALS_LAYERS
};

extern int  g_ticks_per_frame;
extern int  g_total_frames;
extern bool g_ignore_bip;
extern bool g_tag_for_pivot;
extern bool g_mesh_separate;
extern bool g_mesh_special;
extern bool g_mesh_materialasshader;
extern int  g_mesh_multimaterials;
extern bool g_show_debug;

struct ShaderMaterial
{
	char name[128];
};
extern std::vector<ShaderMaterial> g_shaders;
extern char *g_shaders_string;

extern std::list<Range>	g_frame_ranges;

TriObject* GetTriObjectFromNode(INode *node, TimeValue t, bool &deleteIt);
bool IsTriNode(INode *node, int start_time);

void ScriptBox(const char *caption, const char *description, const char *script);
void ExportResultBox(const char *caption, const char *description, const char *shadertext);

int	ExportMD3(const TCHAR *name,ExpInterface *ei, Interface *gi, BOOL suppressPrompts, DWORD options);

#endif // __EXPORTMD3__H
