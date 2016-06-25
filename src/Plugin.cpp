/*
================================================================================
 
 MD3 Exporter
 based on Pop'n'Fresh MD3 Exporter

 additional changes by coded by Pavel P. [VorteX] Timofeyev

================================================================================
*/

#include "Plugin.h"

int	g_ticks_per_frame;
int g_total_frames = 0;

bool g_ignore_bip = false;	
bool g_tag_for_pivot = true;
bool g_mesh_separate = false;
bool g_mesh_special = true;
bool g_mesh_materialasshader = true;
int  g_mesh_multimaterials;
bool g_show_debug = false;

std::list<FrameRange> g_frame_ranges;

class ExportPlugin : public SceneExport
{
	public:
		static HWND hParams;

		int   ExtCount();     // Number of extensions supported 
		const TCHAR * Ext(int n);     // Extension #n (i.e. "ASC")
		const TCHAR * LongDesc();     // Long ASCII description (i.e. "Ascii Export") 
		const TCHAR * ShortDesc();    // Short ASCII description (i.e. "Ascii")
		const TCHAR * AuthorName();    // ASCII Author name
		const TCHAR * CopyrightMessage();   // ASCII Copyright message 
		const TCHAR * OtherMessage1();   // Other message #1
		const TCHAR * OtherMessage2();   // Other message #2
		unsigned int Version();     // Version number * 100 (i.e. v3.01 = 301) 
		void	ShowAbout(HWND hWnd);  // Show DLL's "About..." box
		int		DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0); // Export	file
		BOOL	SupportsOptions(int ext, DWORD options);
		//Constructor/Destructor
		ExportPlugin();
		~ExportPlugin();		
};

class ExportMD3ClassDesc: public ClassDesc2
{
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new ExportPlugin(); }
	const TCHAR *	ClassName() { return _T("ExportMD3"); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return EXPORTMD3_CLASS_ID; }
	const TCHAR* 	Category() { return _T("Quake MD3 export"); }
	const TCHAR*	InternalName() { return _T("ExportPlugin"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle
};

static ExportMD3ClassDesc ExportMD3Desc;
ClassDesc2* GetExportMD3Desc() {return &ExportMD3Desc;}

static INT_PTR CALLBACK ExportMD3OptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	static ExportPlugin *imp = NULL;
	char ranges_named[1024];
	char *token;
	char *subtoken;
	FrameRange range;

	switch(message)
	{
		case WM_INITDIALOG:
			imp = (ExportPlugin *)lParam;
			CenterWindow(hWnd, GetParent(hWnd));
			SendDlgItemMessage(hWnd, IDC_CHECK_IGNOREBIP, BM_SETCHECK,	(WPARAM) false, 0L);
			SendDlgItemMessage(hWnd, IDC_CHECK_RECENTER, BM_SETCHECK, (WPARAM) true, 0L);
			SendDlgItemMessage(hWnd, IDC_CHECK_MESHSEPARATE, BM_SETCHECK, (WPARAM) false, 0L);
			SendDlgItemMessage(hWnd, IDC_CHECK_COLLISION, BM_SETCHECK, (WPARAM) true, 0L);
			SendDlgItemMessage(hWnd, IDC_CHECK_DEBUG, BM_SETCHECK, (WPARAM) false, 0L);
			SendDlgItemMessage(hWnd, IDC_CHECK_MATERIALNAMESASSHADERS, BM_SETCHECK, (WPARAM) true, 0L);
			SendDlgItemMessage(hWnd, IDC_MULTIMATERIAL_NONE, BM_SETCHECK, (WPARAM) true, 0L);
			GuiSetItemText(hWnd, IDC_EDIT_FRAMES, "0");
			return TRUE;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (LOWORD(wParam) == IDC_BUTTON_EXPORT)
				{
					g_ignore_bip = (SendDlgItemMessage(hWnd, IDC_CHECK_IGNOREBIP, BM_GETCHECK, 0, 0L) == BST_CHECKED);
					g_tag_for_pivot = (SendDlgItemMessage(hWnd, IDC_CHECK_RECENTER, BM_GETCHECK, 0, 0L) == BST_CHECKED);
					g_mesh_separate = (SendDlgItemMessage(hWnd, IDC_CHECK_MESHSEPARATE, BM_GETCHECK, 0, 0L) == BST_CHECKED);
					g_mesh_special = (SendDlgItemMessage(hWnd, IDC_CHECK_MESHSPECIAL, BM_GETCHECK, 0, 0L) == BST_CHECKED);
					g_mesh_materialasshader = (SendDlgItemMessage(hWnd, IDC_CHECK_MATERIALNAMESASSHADERS, BM_GETCHECK, 0, 0L) == BST_CHECKED);
					g_show_debug = (SendDlgItemMessage(hWnd, IDC_CHECK_DEBUG, BM_GETCHECK, 0, 0L) == BST_CHECKED);
					if (SendDlgItemMessage(hWnd, IDC_MULTIMATERIAL_SKINFILES, BM_GETCHECK, 0, 0L) == BST_CHECKED)
						g_mesh_multimaterials = MULTIMATERIALS_SKINS;
					else if (SendDlgItemMessage(hWnd, IDC_MULTIMATERIAL_SKINMODELS, BM_GETCHECK, 0, 0L) == BST_CHECKED)
						g_mesh_multimaterials = MULTIMATERIALS_MODELS;
					else
						g_mesh_multimaterials = MULTIMATERIALS_NONE;
					g_total_frames = 0;
					g_frame_ranges.clear();
					GuiGetItemText(hWnd, IDC_EDIT_FRAMES, ranges_named, sizeof(ranges_named) - 1);

					token = strtok(ranges_named, ",");
					while(token != NULL)
					{
						// get a range
						subtoken = strchr(token, '-');
						if (subtoken != NULL)
						{
							// find a range
							*subtoken = '\0';
							range.first	= atoi(token);
							range.last = atoi(subtoken+1);
						}
						else
						{
							// a single value
							range.first	= atoi(token);
							range.last = range.first;
						}	
						g_total_frames += range.last - range.first + 1;
						g_frame_ranges.push_back(range);
						token = strtok(NULL, ",");
					}
					EndDialog(hWnd, FALSE);
				}
				else if (LOWORD(wParam) == IDCANCEL)
					EndDialog(hWnd, FALSE);
			}
			return TRUE;
		case WM_CLOSE:
			return TRUE;
	}
	return FALSE;
}

ExportPlugin::ExportPlugin()
{

}

ExportPlugin::~ExportPlugin() 
{

}

int ExportPlugin::ExtCount()
{
	return 1;
}

const TCHAR *ExportPlugin::Ext(int n)
{		
	return _T("md3");
}

const TCHAR *ExportPlugin::LongDesc()
{
	return _T("Darkplaces MD3 Model exporting plugin");
}
	
const TCHAR *ExportPlugin::ShortDesc() 
{			
	return _T("Darkplaces MD3 Exporter");
}

const TCHAR *ExportPlugin::AuthorName()
{			
	return _T("Pop'N'Fresh and Pavel [VorteX] Timofeyev");
}

const TCHAR *ExportPlugin::CopyrightMessage() 
{	
	return _T("");
}

const TCHAR *ExportPlugin::OtherMessage1() 
{		
	return _T("");
}

const TCHAR *ExportPlugin::OtherMessage2() 
{		
	return _T("");
}

unsigned int ExportPlugin::Version()
{				
	return 100;
}

void ExportPlugin::ShowAbout(HWND hWnd)
{			
}

int	ExportPlugin::DoExport(const TCHAR *name, ExpInterface *ei, Interface *gi, BOOL suppressPrompts, DWORD options)
{
	int result;

	g_ticks_per_frame = GetTicksPerFrame();

	if (!suppressPrompts)
		DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PANEL), GetActiveWindow(), ExportMD3OptionsDlgProc, (LPARAM)this);

	result = ExportMD3(name, ei, gi, suppressPrompts, options);

	return result;
}
	
BOOL ExportPlugin::SupportsOptions(int ext, DWORD options)
{
	return(options == SCENE_EXPORT_SELECTED) ? TRUE : FALSE;
}

// Return a pointer to a TriObject given an INode or return NULL
// if the node cannot be converted to a TriObject
TriObject* GetTriObjectFromNode(INode *node, TimeValue t, bool &deleteIt)
{
	deleteIt = false;
	Object *obj = node->EvalWorldState(t).obj;
	if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
	{ 
		TriObject *tri = (TriObject *) obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != tri)
			deleteIt = true;
		return tri;
	}
	return NULL;
}

// check if particular node has triangles
bool IsTriNode(INode *node, int start_time)
{
	bool needsDel;
	bool res = false;
	
	TriObject *tri = GetTriObjectFromNode(node, start_time, needsDel);
	if (!tri)
		return false;
	if (tri->GetMesh().numFaces != 0)
		res = true;
	if (needsDel)
		delete tri;
	return res;
}

/*
================================================================================
 
 SCRIPT DIALOG BOX

================================================================================
*/

struct ScriptBoxParms
{
	const char *caption;
	const char *description;
	const char *script;
};

static INT_PTR CALLBACK ScriptBoxProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ScriptBoxParms *ScriptParms = (ScriptBoxParms *)lParam;
	switch(message)
	{
		case WM_INITDIALOG:
			CenterWindow(hWnd, GetParent(hWnd));
			GuiSetWindowText(hWnd, ScriptParms->caption);
			GuiSetItemText(hWnd, IDC_SCRIPT, ScriptParms->script);
			GuiSetItemText(hWnd, IDC_SCRIPT_DESC, ScriptParms->description);
			return TRUE;
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
				if (LOWORD(wParam) == IDOK)
					PostMessage(hWnd, WM_CLOSE, 0, 0);
			return TRUE;
			break;
		case WM_CLOSE:
			delete ScriptParms;
			EndDialog(hWnd, FALSE);
			return TRUE;
			break;
	}
	return FALSE;
}

// show script dialog box
void ScriptBox(const char *caption, const char *description, const char *text)
{
	ScriptBoxParms *ScriptParms;
	
	ScriptParms = new ScriptBoxParms;
	ScriptParms->caption = caption;
	ScriptParms->description = description;
	ScriptParms->script = text;
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_BOX), GetActiveWindow(), ScriptBoxProc, (LPARAM)ScriptParms);
}

/*
================================================================================
 
 SCRIPT EXPORT DONE DIALOG

================================================================================
*/

struct ScriptExportBoxParms
{
	const char *caption;
	const char *description;
	const char *shader;
};

static INT_PTR CALLBACK ScriptExportBoxProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ScriptExportBoxParms *ScriptParms = (ScriptExportBoxParms *)lParam;
	switch(message)
	{
		case WM_INITDIALOG:
			CenterWindow(hWnd, GetParent(hWnd));
			GuiSetWindowText(hWnd, ScriptParms->caption);
			GuiSetItemText(hWnd, IDC_EXPORT_LOG, ScriptParms->description);
			GuiSetItemText(hWnd, IDC_EXPORT_SHADERTEXT, ScriptParms->shader);
			return TRUE;
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (LOWORD(wParam) == IDC_EXPORT_CLOSE)
					PostMessage(hWnd, WM_CLOSE, 0, 0);
				else if (LOWORD(wParam) == IDC_EXPORT_SHADER)
					ScriptBox("Sample .shader text", "This is basic material script to use with Darkplaces engine.\r\nCopy this text and place in appropriate .shader file.", g_shaders_string);
			}
			return TRUE;
			break;
		case WM_CLOSE:
			delete ScriptParms;
			EndDialog(hWnd, FALSE);
			return TRUE;
			break;
	}
	return FALSE;
}

// show script dialog box
void ExportResultBox(const char *caption, const char *description, const char *shadertext)
{
	ScriptExportBoxParms *ScriptParms;
	
	ScriptParms = new ScriptExportBoxParms;
	ScriptParms->caption = caption;
	ScriptParms->description = description;
	ScriptParms->shader = shadertext;
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DONE), GetActiveWindow(), ScriptExportBoxProc, (LPARAM)ScriptParms);
}
