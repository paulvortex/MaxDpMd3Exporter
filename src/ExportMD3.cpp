/*
================================================================================
 
 MD3 Exporter
 based on Pop'n'Fresh MD3 Exporter

 additional changes by coded by Pavel P. [VorteX] Timofeyev

================================================================================
*/

#include "Plugin.h"

BOOL exportSelected;

std::vector<ShaderMaterial> g_shaders;
char *g_shaders_string = NULL;

struct ExportNode
{
	int i;
	char *name;
};

struct ExportUV
{
	ExportUV() : filled(false) {};
	float u,v;
	int	  tvert;
	bool  filled;
};

struct ExportVertex
{
	int vert;
	Point3  normal;
	int     normaladdr;
	boolean normalfilled; 
};

struct ExportTriangle
{
	union
	{
		struct 
		{
			int a,b,c;
		};
		int e[3];
	};
};

/*
================================================================================
 
 SCENE ENUMERATING

================================================================================
*/

class MySceneEntry
{
public:
	MySceneEntry(INode *n, Object *o) { node = n; obj = o; next = NULL; }

	INode *node;
	Object *obj;
	MySceneEntry *next;
};

class SceneEnumProc : public ITreeEnumProc
{
public:
	SceneEnumProc(IScene *scene, TimeValue t, Interface *i);
	~SceneEnumProc();

	Interface *		i;

	MySceneEntry *	head;
	MySceneEntry *	tail;

	IScene		*	theScene;
	int				count;
	TimeValue		time;
	int				Count() { return count; }
	void			Append(INode *node, Object *obj);
	int				callback( INode *node );
	Box3			Bound();

	MySceneEntry *	operator[](int index);
};

SceneEnumProc::SceneEnumProc(IScene *scene, TimeValue t, Interface *i)
{
	time		= t;
	theScene	= scene;
	count		= 0;
	head = tail = NULL;
	this->i		= i;

	theScene->EnumTree(this);
}

SceneEnumProc::~SceneEnumProc()
{
	while(head)
	{
		MySceneEntry *next = head->next;
		delete head;
		head = next;
	}
	head = tail = NULL;
	count = 0;	
}

int SceneEnumProc::callback(INode *node)
{
	std::list<std::string>::iterator name_i;
	const char *nodename;
	
	if (exportSelected && node->Selected() == FALSE)
		return TREE_CONTINUE;

	Object *obj = node->EvalWorldState(time).obj;
	nodename = node->GetName();
	if (obj->CanConvertToType(triObjectClassID))
	{
		if ((g_ignore_bip && !_tcsncmp("Bip", nodename, 3)) || !_tcsncmp("*", nodename, 1))
			return TREE_CONTINUE;
		Append(node, obj);
	}
	return TREE_CONTINUE; // keep on enumeratin
}

void SceneEnumProc::Append(INode *node, Object *obj)
{
	MySceneEntry *entry = new MySceneEntry(node, obj);

	if (tail)
		tail->next = entry;
	tail = entry;
	if (!head)
		head = entry;
	count++;	
}

Box3 SceneEnumProc::Bound()
{
	Box3 bound;

	bound.Init();
	MySceneEntry *e = head;
	
	ViewExp *vpt = i->GetViewport(NULL);
	
	while(e)
	{
		Box3 bb;
		e->obj->GetWorldBoundBox(time, e->node, vpt, bb);
		bound += bb;
		e = e->next;
	}

	return bound;
}

MySceneEntry *SceneEnumProc::operator[](int index)
{
	MySceneEntry *e = head;

	while(index)
	{
		e = e->next;
		index--;
	}
	return e;
}

//////////////////////////////////////////////////////////////////////////////////
// Messaging
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Actual Export
//////////////////////////////////////////////////////////////////////////////////

// put material into file
void putMaterial(const char *name, Mtl *mtl, Texmap *tex, FILE *file)
{
	int i, l;
	const char *subname;
	char s[2048];

	// once we have the filename we mangle it. We search for 'models' and if found we remove the filename to that part.
	// Thus if we have "C:/quake3/baseq3/models/mapobjects/model.md3"
	// It becomes "models/mapobjects/model.md3" otherwise we just leave it.
	// Also we do convert slashes to backSlashes and remove extension
	subname = NULL;

	// strip extension
	strcpy(s, name);
	l = (int)strlen(s) - 1;
	while(l >= 0 && s[l] != '.') l--;
	if (l != 0) s[l] = 0;

	// convert slashes
	l = (int)strlen(s);
	for (i = 0; i < l; i++)
		if (s[i] == '\\')
			s[i] = '/';

	// find a local path
                         subname = strstr( s, "models/" );
	if (subname == NULL) subname = strstr( s, "textures/" );
	if (subname == NULL) subname = strstr( s, "particles/" );
	if (subname == NULL) subname = strstr( s, "gfx/" );
	if (!subname)
		subname = name;
	putChars(subname, 64, file);

	// write material
	bool shader_written = false;
	for (i = 0; i < (int)g_shaders.size(); i++)
	{
		if (!_tcsncmp(g_shaders[i].name, subname, sizeof(g_shaders[i].name)))
		{
			shader_written = true;
			break;
		}
	}
	if (!shader_written)
	{
		ShaderMaterial NewSh;
		strncpy(NewSh.name, subname, sizeof(NewSh.name));
		g_shaders.push_back(NewSh);
	}
}

int ExportQuake3Model(const TCHAR *filename, ExpInterface *ei, Interface *gi, int start_time, std::list<ExportNode> lTags, std::list<ExportNode> lMeshes)
{
	FILE *file;
	int i, j, totalTags, totalMeshes, current_time = 0;
	long pos_current, totalTris = 0, totalVerts = 0;
	std::list<Range>::iterator range_i;
	std::vector<Point3> lFrameBBoxMin;
	std::vector<Point3> lFrameBBoxMax;
	long pos_tagstart;
	long pos_tagend;
	long pos_filesize;
	long pos_framestart;
	char *filepath;
	int lazynamesfixed = 0;
	const Point3 x_axis(1, 0, 0);
	const Point3 z_axis(0, 0, 1);

	SceneEnumProc checkScene(ei->theScene, start_time, gi);
	totalTags = (int)lTags.size();
	if (g_tag_for_pivot)
		totalTags++;
	totalMeshes = (int)lMeshes.size();

	// open file
	filepath = ConvertStr(filename, tolower);
	file = fopen(filepath, "wb");
	if (!file)
	{
		ExportError("Cannot open file '%s'.", filepath);
		return FALSE;
	}
	ExportDebug("%s:", filepath);

	// sync pattern and version
	putChars("IDP3", 4, file);
	put32(15, file);
	putChars(filename, 64, file);
	put32(0, file);   // flags
	
	// MD3 header
	ExportState("Writing MD3 header");
	put32(g_total_frames, file);      // how many frames
	put32(totalTags, file);	  // tagsnum
	put32(totalMeshes, file); // meshnum
	put32(1, file);   // maxskinnum
	put32(108, file); // headersize
	pos_tagstart = ftell(file); put32(0, file);   // tagstart
	pos_tagend	= ftell(file);  put32(256, file); // tagend
	pos_filesize = ftell(file); put32(512, file); // filesize
	ExportDebug("    %i frames, %i tags, %i meshes", g_total_frames, totalTags, totalMeshes);

	// frame info
	// bbox arrays get filled while exported mesh and written back then
	ExportState("Writing frame info");
	pos_framestart = ftell(file);
	lFrameBBoxMin.resize(g_total_frames);
	lFrameBBoxMax.resize(g_total_frames);
	for (i = 0; i < g_total_frames; i++)
	{
		ExportState("Writing info for frame %i of %i", i, g_total_frames);
		putFloat(-1.0f, file);	// bbox min vector
		putFloat(-1.0f, file);
		putFloat(-1.0f, file);	
		putFloat( 1.0f, file); // bbox max vector
		putFloat(1.0f, file);
		putFloat(1.0f, file);
		putFloat(0.0f, file);  // local origin (usually 0 0 0)
		putFloat(0.0f, file);
		putFloat(0.0f, file);
		putFloat(1.0f, file);  // radius of bounding sphere
		putChars("", 16, file);
	}

	// tags
	pos_current = ftell(file);
	fseek(file, pos_tagstart, SEEK_SET);
	put32(pos_current, file);
	fseek(file, pos_current, SEEK_SET);
	
	// for each frame range cycle all frames and write out each tag
	long pos_tags = pos_current;
	if (totalTags)
	{
		long current_frame = 0;
		ExportState("Writing %i tags", totalTags);
		for (range_i = g_frame_ranges.begin(); range_i != g_frame_ranges.end(); range_i++)
		{
			for (i = (*range_i).first; i <= (int)(*range_i).last; i++, current_frame++)
			{
				SceneEnumProc current_scene(ei->theScene, i * g_ticks_per_frame, gi);
				current_time = current_scene.time;

				// write out tags
				if (lTags.size())
				{
					for (std::list<ExportNode>::iterator tag_i = lTags.begin(); tag_i != lTags.end(); tag_i++)
					{
						INode *node	= current_scene[tag_i->i]->node;
						Matrix3	tm = node->GetObjTMAfterWSM(current_time);

						ExportState("Writing '%s' frame %i of %i", tag_i->name, i, g_total_frames);

						// tagname
						putChars(tag_i->name, 64, file);
						// origin, rotation matrix
						Point3 row = tm.GetRow(3);
						putFloat(row.x, file);
						putFloat(row.y, file);
						putFloat(row.z, file);
						row = tm.GetRow(0);
						putFloat(row.x, file);
						putFloat(row.y, file);
						putFloat(row.z, file);
						row = tm.GetRow(1);
						putFloat(row.x, file);
						putFloat(row.y, file);
						putFloat(row.z, file);
						row = tm.GetRow(2);
						putFloat(row.x, file);
						putFloat(row.y, file);
						putFloat(row.z, file);
					}
				}

				// write the center of mass tag_pivot which is avg of all objects's pivots
				if (g_tag_for_pivot)
				{
					ExportState("Writing 'tag_pivot' frame %i of %i", i, g_total_frames);

					// write the null data as tag_pivot need to be written after actual geometry
					// (it needs information on frame bound boxes to get proper blendings)
					putChars("tag_pivot", 64, file);
					putFloat(0, file);
					putFloat(0, file);
					putFloat(0, file);
					putFloat(1, file);
					putFloat(0, file);
					putFloat(0, file);
					putFloat(0, file);
					putFloat(1, file);
					putFloat(0, file);
					putFloat(0, file);
					putFloat(0, file);
					putFloat(1, file);
				}
			}
		}
	}

	// write the tag object offsets
	pos_current = ftell(file);
	fseek(file, pos_tagend, SEEK_SET);
	put32(pos_current, file);
	fseek(file, pos_current, SEEK_SET);

	// allocate the structs used to calculate tag_pivot
	std::vector<Point3> tag_pivot_origin;
	std::vector<double> tag_pivot_volume;
	if (g_tag_for_pivot)
	{
		tag_pivot_origin.resize(g_total_frames);
		tag_pivot_volume.resize(g_total_frames);
	}

	// mesh objects
	// for each mesh object write uv and frames
	SceneEnumProc scratch(ei->theScene, start_time, gi);
	ExportState("Writing %i meshes", (int)lMeshes.size());
	for (std::list<ExportNode>::iterator mesh_i = lMeshes.begin(); mesh_i != lMeshes.end(); mesh_i++)
	{
		bool needsDel;

		ExportState("Start mesh #%i", mesh_i);
		INode *node = checkScene[mesh_i->i]->node;
		Matrix3 tm	= node->GetObjTMAfterWSM(start_time);
		TriObject *tri = GetTriObjectFromNode(node, start_time, needsDel);
		if (!tri)
			continue;
		Mesh &mesh = tri->GetMesh();
		      mesh.checkNormals(TRUE);

		// fix lazy object names
		ExportState("Attempt to fix mesh name '%s'", mesh_i->name);
		char meshname[64];
		memset(meshname, 0, 64);
		strncpy(meshname, mesh_i->name, min(63, strlen(mesh_i->name)));
		if (!_tcsncmp("Box", meshname, 3)    || !_tcsncmp("Sphere", meshname, 6)  || !_tcsncmp("Cylinder", meshname, 8) ||
            !_tcsncmp("Torus", meshname, 5)  || !_tcsncmp("Cone", meshname, 4)    || !_tcsncmp("GeoSphere", meshname, 9) ||
			!_tcsncmp("Tube", meshname, 4)   || !_tcsncmp("Pyramid", meshname, 7) || !_tcsncmp("Plane", meshname, 5) ||
			!_tcsncmp("Teapot", meshname, 6) || !_tcsncmp("Object", meshname, 6))
		{
name_conflict:
			lazynamesfixed++;
			if (lazynamesfixed == 1)
				strcpy(meshname, "base");
			else
				sprintf(meshname, "base%i", lazynamesfixed);

			// check if it's not used by another mesh
			for (std::list<ExportNode>::iterator m_i = lMeshes.begin(); m_i != lMeshes.end(); m_i++)
				if (!_tcsncmp(m_i->name, meshname, strlen(meshname)))
					goto name_conflict;
			// approve name
			ExportWarning("Lazy object name '%s' (mesh renamed to '%s').", node->GetName(), meshname);
		}

		// special mesh check
		bool shadow_or_collision = false;
		if (g_mesh_special)
			  if (!_tcsncmp("collision", meshname, 9) || !_tcsncmp("shadow", meshname, 6))
				shadow_or_collision = true;

		// get material
		char *shadername = NULL;
		Texmap *tex = 0;
		Mtl *mtl = 0;
		if (!shadow_or_collision)
		{
			mtl = node->GetMtl();
			if (mtl)
			{
				// check for multi-material
				if (mtl->IsMultiMtl())
				{
					// check if it's truly multi material
					// we do support multi-material with only one texture (some importers set it)
					bool multi_material = false;
					MtlID matId = mesh.faces[0].getMatID();
					for (i = 1; i < mesh.getNumFaces(); i++)
						if (mesh.faces[i].getMatID() != matId)
							multi_material = true;

					if (multi_material)
						if (g_mesh_multimaterials == MULTIMATERIALS_NONE)
							ExportWarning("Object '%s' is multimaterial and using multiple materials on its faces, that case is not yet supported (truncating to first submaterial).", node->GetName());
					
					// switch to submaterial
					mtl = mtl->GetSubMtl(matId);
				}

				// get shader from material if supplied
				if (g_mesh_materialasshader && (strstr(mtl->GetName(), "/") != NULL || strstr(mtl->GetName(), "\\") != NULL))
					shadername = (char *)mtl->GetName();
				else
				{
					// get texture
					tex = mtl->GetSubTexmap(ID_DI);
					if (tex)
					{
						if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00))
						{
							shadername = ((BitmapTex *)tex)->GetMapName();
							if (shadername == NULL || !shadername[0])
								ExportWarning("Object '%s' material '%s' has no bitmap.", tex->GetName(), node->GetName());
						}
						else
						{
							tex = NULL;
							ExportWarning("Object '%s' has material with wrong texture type (only Bitmap are supported).", node->GetName());
						}
					}
					else
						ExportWarning("Object '%s' has material but no texture.", node->GetName());
				}
			}
			else
				ExportWarning("Object '%s' has no material.", node->GetName());
		}

		long pos_meshstart = ftell(file);

		// surface object
		ExportState("Writing mesh '%s' header", meshname);
		putChars("IDP3", 4, file);
		putChars(meshname, 64, file);
		put32(0, file); // flags
		put32(g_total_frames, file);                          // framecount
		put32(1, file);                                       // skincount
		long pos_vertexnum = ftell(file); put32(0, file);     // vertexcount
		put32(mesh.getNumFaces(), file);                      // trianglecount
		long pos_trianglestart = ftell(file); put32(0, file); // start triangles
		put32(108, file);                                     // header size
		long pos_texvecstart = ftell(file); put32(0, file);   // texvecstart
		long pos_vertexstart = ftell(file); put32(16, file);  // vertexstart
		long pos_meshsize = ftell(file); put32(32, file);	  // meshsize

		// write out a single 'skin'
		ExportState("Writing mesh %s texture", meshname);
		if (shadow_or_collision)
			putChars(meshname, 64, file);
		else if (shadername) 
			putMaterial(shadername, mtl, tex, file);
		else
			putChars("noshader", 64, file);
		put32(0, file); // flags

		// build geometry
		ExportState("Building vertexes/triangles");
		std::vector<ExportVertex>vVertexes;
		std::vector<ExportTriangle>vTriangles;
		vVertexes.resize(mesh.getNumVerts());
		int vExtraVerts = mesh.getNumVerts();
		for (i = 0; i < mesh.getNumVerts(); i++)
		{
			vVertexes[i].vert = i;
			vVertexes[i].normalfilled = false;
			// todo: check for coicident verts
		}
		int vNumExtraVerts = 0;

		// check normals
		if (!mesh.normalsBuilt && !shadow_or_collision)
			ExportWarning("Object '%s' does not have normals contructed.", node->GetName());

		// get info for triangles
		const float normal_epsilon = 0.01f;
		vTriangles.resize(mesh.getNumFaces());
		for (i = 0; i < mesh.getNumFaces(); i++)
		{
			DWORD smGroup = mesh.faces[i].getSmGroup();
			ExportState("Mesh %s: checking normals for face %i of %i", meshname, i, mesh.getNumFaces());
			for (j = 0; j < 3; j++)
			{
				int vert = mesh.faces[i].getVert(j);
				vTriangles[i].e[j] = vert;
				// find a right normal for this vertex and save its 'address'
				int vni;
				Point3 vn;
				if (!mesh.normalsBuilt || shadow_or_collision)
				{
					vn.Set(0, 0, 0);
					vni = 0;
				}
				else
				{
					int numNormals;
					RVertex *rv = mesh.getRVertPtr(vert);
					if (rv && rv->rFlags & SPECIFIED_NORMAL)
					{
						vn = rv->rn.getNormal();
						vni = 0;
					}
					else if (rv && (numNormals = rv->rFlags & NORCT_MASK) && smGroup)
					{
						// If there is only one vertex is found in the rn member.
						if (numNormals == 1)
						{
							vn = rv->rn.getNormal();
							vni = 0;
						}
						else
						{
							// If two or more vertices are there you need to step through them
							// and find the vertex with the same smoothing group as the current face.
							// You will find multiple normals in the ern member.
							for (int k = 0; k < numNormals; k++)
							{
								if (rv->ern[k].getSmGroup() & smGroup)
								{
									vn = rv->ern[k].getNormal();
									vni = 1 + k;
								}
							}
						}
					}
					else
					{
						// Get the normal from the Face if no smoothing groups are there
						vn = mesh.getFaceNormal(i);
						vni = 0 - (i + 1);
					}
				}

				// subdivide to get all normals right
				if (!vVertexes[vert].normalfilled)
				{
					vVertexes[vert].normal = vn;
					vVertexes[vert].normaladdr = vni;
					vVertexes[vert].normalfilled = true;
				}
				else if ((vVertexes[vert].normal - vn).Length() >= normal_epsilon)
				{
					// current vertex not matching normal - it was already filled by different smoothing group
					// find a vert in extra verts in case it was already created
					bool vert_found = false;
					for (int ev = vExtraVerts; ev < (int)vVertexes.size(); ev++)
					{
						if (vVertexes[ev].vert == vert && (vVertexes[ev].normal - vn).Length() < normal_epsilon)
						{
							vert_found = true;
							vTriangles[i].e[j] = ev;
							break;
						}
					}
					// we havent found a vertex, create new
					if (!vert_found)
					{
						ExportVertex NewVert;
						NewVert.vert = vVertexes[vert].vert;
						NewVert.normal = vn;
						NewVert.normaladdr = vni;
						NewVert.normalfilled = true;
						vTriangles[i].e[j] = (int)vVertexes.size();
						vVertexes.push_back(NewVert);
						vNumExtraVerts++;
					}
				}
			}
		}
		int vNumExtraVertsForSmoothGroups = vNumExtraVerts;

		// generate UV map
		// VorteX: use direct maps reading since getNumTVerts()/getTVert is deprecated
		//  max sets two default mesh maps: 0 - vertex color, 1 : UVW, 2 & up are custom ones
		ExportState("Building UV map");
		std::vector<ExportUV>vUVMap;
		vUVMap.resize(vVertexes.size());
		int meshMap = 1;
		if (!mesh.mapSupport(meshMap) || !mesh.getNumMapVerts(meshMap) || shadow_or_collision)
		{
			for (i = 0; i < mesh.getNumVerts(); i++)
			{
				vUVMap[i].u = 0.5;
				vUVMap[i].v = 0.5;
			}
			if (!shadow_or_collision)
				ExportWarning("No UV mapping was found on object '%s'.", node->GetName());
		}
		else
		{
			UVVert *meshUV = mesh.mapVerts(meshMap);
			for (i = 0; i < (int)vTriangles.size(); i++)
			{
				ExportState("Mesh %s: converting tvert for face %i of %i", meshname, i, (int)vTriangles.size());
				// for 3 face vertexes
				for (j = 0; j < 3; j++)
				{
					int vert = vTriangles[i].e[j];
					int tv = mesh.tvFace[i].t[j];
					UVVert &UV = meshUV[tv];

					if (!vUVMap[vert].filled)
					{
						// fill uvMap vertex
						vUVMap[vert].u = UV.x;
						vUVMap[vert].v = UV.y;
						vUVMap[vert].filled = true;
						vUVMap[vert].tvert = tv;
					}
					else if (tv != vUVMap[vert].tvert)
					{
						// uvMap slot for this vertex has been filled
						// we should arrange triangle to other vertex, which not filled and having same shading and uv
						// check if any of the extra vertices can fit
						bool vert_found = false;
						for (int ev = vExtraVerts; ev < (int)vVertexes.size(); ev++)
						{
							if (vVertexes[ev].vert == vert && vUVMap[vert].u == UV.x &&vUVMap[vert].v == UV.y  && (vVertexes[ev].normal - vVertexes[vert].normal).Length() < normal_epsilon)
							{
								vert_found = true;
								vTriangles[i].e[j] = vVertexes[ev].vert;
								break;
							}
						}
						if (!vert_found)
						{
							// create new vert
							ExportVertex NewVert;
							NewVert.vert = vVertexes[vert].vert;
							NewVert.normal = vVertexes[vert].normal;
							NewVert.normaladdr = vVertexes[vert].normaladdr;
							NewVert.normalfilled = vVertexes[vert].normalfilled;
							vTriangles[i].e[j] = (int)vVertexes.size();
							vVertexes.push_back(NewVert);
							vNumExtraVerts++;
							// create new TVert
							ExportUV newUV;
							newUV.filled = true;
							newUV.u = UV.x;
							newUV.v = UV.y;
							newUV.tvert = tv;
							vUVMap.push_back(newUV);
						}
					}
				}
			}
		}
		int vNumExtraVertsForUV = (vNumExtraVerts - vNumExtraVertsForSmoothGroups);

		// print some debug stats
		ExportDebug("    mesh %s: %i vertexes +%i SG +%i UV, %i triangles", meshname, ((int)vVertexes.size() - vNumExtraVerts), vNumExtraVertsForSmoothGroups, vNumExtraVertsForUV, (int)vTriangles.size());

		// fill in triangle start
		pos_current = ftell(file);
		fseek(file, pos_trianglestart, SEEK_SET);
		put32(pos_current - pos_meshstart, file);
		fseek(file, pos_current, SEEK_SET);

		// detect if object have negative scale (mirrored)
		// in this canse we should rearrange triangles counterclockwise
		// so stuff will not be inverted
		ExportState("Mesh %s: writing %i triangles", meshname, (int)vTriangles.size());
		if (DotProd(CrossProd(tm.GetRow(0), tm.GetRow(1)), tm.GetRow(2)) < 0.0)
		{
			ExportWarning("Object '%s' is mirrored (having negative scale on it's transformation)", node->GetName());
			for (i = 0; i < (int)vTriangles.size(); i++)
			{
				put32(vTriangles[i].b, file);	// vertex index
				put32(vTriangles[i].c, file);	// for 3 vertices
				put32(vTriangles[i].a, file);	// of triangle
			}
		}
		else
		{
			for (i = 0; i < (int)vTriangles.size(); i++)
			{
				put32(vTriangles[i].a, file);	// vertex index
				put32(vTriangles[i].c, file);	// for 3 vertices
				put32(vTriangles[i].b, file);	// of triangle
			}
		}

		// fill in texvecstart
		// write out UV mapping coords.
		ExportState("Mesh %s: writing %i UV vertexes", meshname, (int)vUVMap.size());
		pos_current = ftell(file);
		fseek(file, pos_texvecstart, SEEK_SET);
		put32(pos_current - pos_meshstart, file);
		fseek(file, pos_current, SEEK_SET);
		for (i = 0; i < (int)vUVMap.size(); i++)
		{
			putFloat(vUVMap[i].u, file); // texture coord u,v
			putFloat(1.0f - vUVMap[i].v, file);	// for vertex
		}
		vUVMap.clear();

		// fill in vertexstart
		pos_current = ftell(file);
		fseek(file, pos_vertexstart, SEEK_SET);
		put32(pos_current - pos_meshstart, file);
		fseek(file, pos_current, SEEK_SET);

		// fill in vertexnum
		pos_current = ftell(file);
		fseek(file, pos_vertexnum, SEEK_SET);
		put32((int)vVertexes.size(), file);
		fseek(file, pos_current, SEEK_SET);

		// write out for each frame the position of each vertex
		long current_frame = 0;
		ExportState("Mesh %s: writing %i frames", meshname, g_total_frames);
		for (range_i = g_frame_ranges.begin(); range_i != g_frame_ranges.end(); range_i++)
		{
			for (i = (*range_i).first; i <= (int)(*range_i).last; i++, current_frame++)
			{
				bool _needsDel;
				SceneEnumProc current_scene(ei->theScene, i * g_ticks_per_frame, gi);
				current_time = current_scene.time;
				INode *_node = current_scene[mesh_i->i]->node;
				TriObject *_tri	= GetTriObjectFromNode(_node, current_time, _needsDel);
				if (!_tri)
					continue;
				Mesh &_mesh	= _tri->GetMesh();
				      _mesh.checkNormals(TRUE);
				Matrix3 _tm	= _node->GetObjTMAfterWSM(current_time);

				ExportState("Mesh %s: writing frame %i of %i", meshname, current_frame, g_total_frames);

				Point3 BoxMin(0, 0, 0);
				Point3 BoxMax(0, 0, 0);
				for (j = 0; j < (int)vVertexes.size(); j++) // number of vertices
				{
					ExportState("Mesh %s: transform vertex %i of %i", meshname, j, (int)vVertexes.size());

					int vert = vVertexes[j].vert;
					Point3 &v = _tm.PointTransform(_mesh.getVert(vert));

					// populate bbox data
					if (!shadow_or_collision)
					{
						BoxMin.x = min(BoxMin.x, v.x);
						BoxMin.y = min(BoxMin.y, v.y);
						BoxMin.z = min(BoxMin.z, v.z);
						BoxMax.x = max(BoxMax.x, v.x);
						BoxMax.y = max(BoxMax.y, v.y);
						BoxMax.z = max(BoxMax.z, v.z);
					}

					// write vertex
					double f;
					f = v.x * 64.0f; if (f < -32768.0) f = -32768.0; if (f > 32767.0) f = 32767.0; put16((short)f, file);
					f = v.y * 64.0f; if (f < -32768.0) f = -32768.0; if (f > 32767.0) f = 32767.0; put16((short)f, file);
					f = v.z * 64.0f; if (f < -32768.0) f = -32768.0; if (f > 32767.0) f = 32767.0; put16((short)f, file);

					// get normal
					ExportState("Mesh %s: transform vertex normal %i of %i", meshname, j, (int)vVertexes.size());
					Point3 n;
					if (!vVertexes[j].normalfilled || !_mesh.normalsBuilt)
						n = _mesh.getNormal(vert);
					else
					{
						RVertex *rv = _mesh.getRVertPtr(vert);
						if (vVertexes[j].normaladdr < 0)
							n = _mesh.getFaceNormal((0 - vVertexes[j].normaladdr) - 1);
						else if (vVertexes[j].normaladdr == 0)
							n = rv->rn.getNormal();
						else 
							n = rv->ern[vVertexes[j].normaladdr - 1].getNormal();
					}
					Point3 &nt = _tm.VectorTransform(n).Normalize();

					// encode a normal vector into a 16-bit latitude-longitude value
					double lng = acos(nt.z) * 255 / (2 * pi);
					double lat = atan2(nt.y, nt.x) * 255 / (2 * pi);
					put16((((int)lat & 0xFF) << 8) | ((int)lng & 0xFF), file);
				}

				// blend the pivot positions for tag_pivot using mesh's volumes for blending power
				if (g_tag_for_pivot && !shadow_or_collision)
				{
					ExportState("Mesh %s: writing tag_pivot", meshname);

					Point3 Size = BoxMax - BoxMin;
					double BoxVolume = pow(Size.x * Size.y * Size.z, 0.333f);

					// blend matrices
					float blend = (float)(BoxVolume / (BoxVolume + tag_pivot_volume[current_frame]));
					float iblend = 1 - blend;
					tag_pivot_volume[current_frame]   = tag_pivot_volume[current_frame] + BoxVolume;
					Point3 row = _tm.GetRow(3) - _node->GetObjOffsetPos();
					tag_pivot_origin[current_frame].x = tag_pivot_origin[current_frame].x * iblend + row.x * blend;
					tag_pivot_origin[current_frame].y = tag_pivot_origin[current_frame].y * iblend + row.y * blend;
					tag_pivot_origin[current_frame].z = tag_pivot_origin[current_frame].z * iblend + row.z * blend;
				}

				// populate bbox data for frames
				lFrameBBoxMin[current_frame].x = min(lFrameBBoxMin[current_frame].x, BoxMin.x);
				lFrameBBoxMin[current_frame].y = min(lFrameBBoxMin[current_frame].y, BoxMin.y);
				lFrameBBoxMin[current_frame].z = min(lFrameBBoxMin[current_frame].z, BoxMin.z);
				lFrameBBoxMax[current_frame].x = max(lFrameBBoxMax[current_frame].x, BoxMax.x);
				lFrameBBoxMax[current_frame].y = max(lFrameBBoxMax[current_frame].y, BoxMax.y);
				lFrameBBoxMax[current_frame].z = max(lFrameBBoxMax[current_frame].z, BoxMax.z);

				// delete the working object, if necessary.
				if (_needsDel)
					delete _tri;
			}
		}

		// delete if necessary
		if (needsDel)
			delete tri;

		// fill in meshsize
		pos_current = ftell(file);
		fseek(file, pos_meshsize, SEEK_SET);
		put32(pos_current - pos_meshstart, file);
		fseek(file, pos_current, SEEK_SET);

		// reset back to first frame
		SceneEnumProc scratch(ei->theScene, start_time, gi);
		totalTris += (long)vTriangles.size();
		totalVerts += (long)vVertexes.size();
		vTriangles.clear();
		vVertexes.clear();
	}

	// write tag_pivot
	ExportState("Writing tag_pivot positions");
	if (g_tag_for_pivot)
	{
		pos_current = ftell(file);
		long current_frame = 0;
		for (range_i = g_frame_ranges.begin(); range_i != g_frame_ranges.end(); range_i++)
		{
			for (i = (*range_i).first; i <= (int)(*range_i).last; i++, current_frame++)
			{
				fseek(file, pos_tags + totalTags*112*current_frame + (int)lTags.size()*112 + 64, SEEK_SET);
				// origin
				putFloat(tag_pivot_origin[current_frame].x, file);
				putFloat(tag_pivot_origin[current_frame].y, file);
				putFloat(tag_pivot_origin[current_frame].z, file);
			}
		}
		fseek(file, pos_current, SEEK_SET);
	}
	tag_pivot_volume.clear();
	tag_pivot_origin.clear();

	// write frame data
	ExportState("Writing culling info");
	long current_frame = 0;
	pos_current = ftell(file);
	for (range_i = g_frame_ranges.begin(); range_i != g_frame_ranges.end(); range_i++)
	{
		for (i = (*range_i).first; i <= (int)(*range_i).last; i++, current_frame++)
		{
			fseek(file, pos_framestart + current_frame*56, SEEK_SET);
			putFloat(lFrameBBoxMin[current_frame].x, file);	// bbox min vector
			putFloat(lFrameBBoxMin[current_frame].y, file);
			putFloat(lFrameBBoxMin[current_frame].z, file);	
			putFloat(lFrameBBoxMax[current_frame].x, file); // bbox max vector
			putFloat(lFrameBBoxMax[current_frame].y, file);
			putFloat(lFrameBBoxMax[current_frame].z, file);
			putFloat(0, file); // local origin (usually 0 0 0)
			putFloat(0, file);
			putFloat(0, file);
			putFloat(max(lFrameBBoxMin[current_frame].Length(), lFrameBBoxMax[current_frame].Length()) , file); // radius of bounding sphere
		}
	}
	fseek(file, pos_current, SEEK_SET);
	lFrameBBoxMin.clear();
	lFrameBBoxMax.clear();

	// fill in filesize
	pos_current = ftell(file);
	fseek(file, pos_filesize, SEEK_SET);
	put32(pos_current, file);
	fseek(file, pos_current, SEEK_SET);

	fclose(file);

	ExportDebug("    total: %i vertexes, %i triangles", totalVerts, totalTris);

	return TRUE;
}

int ExportMD3(const TCHAR *filename, ExpInterface *ei, Interface *gi, BOOL suppressPrompts, DWORD options)
{
	int result = 1, i, j, start_time;
	std::list<ExportNode> lTags;
	std::list<ExportNode> lMeshes;
	std::string shaderString;
	ExportNode exportNode;
	HWND exportWindow;

	g_shaders.clear();

	exportWindow = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_EXPORTING), GetActiveWindow(), 0);
	exportSelected = (options & SCENE_EXPORT_SELECTED) ? TRUE : FALSE;
	StartExportMessages(exportWindow);

	// any frames to export specified?
	if (g_frame_ranges.empty())
		ExportError("No frames to export.");
	else
	{
		// make sure there are nodes we're interested in!
		// ask the scene to enumerate all its nodes so we can determine if there are any we can use
		start_time = g_ticks_per_frame * (*(g_frame_ranges.begin())).first;
		SceneEnumProc checkScene(ei->theScene, start_time, gi);
		if (!checkScene.Count())
			ExportError("No data to export.");
		else if (g_mesh_separate)
		{
			// get the meshes and tags we want to export
			char filebase[128];
			char filepath[2048];
			char sepfile[2048];
			char *fn;
			fn = const_cast<char *>(filename);
			FileBase(fn, filebase);
			FilePath(fn, filepath);
			for (i = 0; i < checkScene.Count(); i++)
			{
				INode *node	= checkScene[i]->node;
				if (!_tcsncmp("*", node->GetName(), 1))
					continue;
				if (!_tcsncmp("tag_", node->GetName(), 4))
					continue;
				if (g_mesh_special && (strstr(node->GetName(), "_collision") || strstr(node->GetName(), "_shadow")))
					continue;
				if (!IsTriNode(node, start_time))
					continue;
		
				// add base mesh
				exportNode.i = i;
				exportNode.name = "base";
				lMeshes.push_back(exportNode);

				// honor _collision and _shadow meshes
				if (g_mesh_special)
				{
					char collisionNodeName[512], shadowNodeName[512];
					sprintf(collisionNodeName, "%s_collision", node->GetName());
					sprintf(shadowNodeName, "%s_shadow", node->GetName());
					for (j = 0; j < checkScene.Count(); j++)
					{
						INode *_node = checkScene[j]->node;
						if (!_tcscmp(collisionNodeName, _node->GetName()))
						{		
							exportNode.i = j;
							exportNode.name = "collision";
							lMeshes.push_back(exportNode);
							continue;
						}
						if (!_tcscmp(shadowNodeName, _node->GetName()))
						{
							exportNode.i = j;
							exportNode.name = "shadow";
							lMeshes.push_back(exportNode);
						}
					}
				}

				// export
				sprintf(sepfile, "%s%s_%s.md3", filepath, filebase, node->GetName());
				result = ExportQuake3Model(sepfile, ei, gi, start_time, lTags, lMeshes);
				lTags.clear();
				lMeshes.clear();
			}
		}
		else
		{
			for (i = 0; i < checkScene.Count(); ++i)
			{
				INode *node	= checkScene[i]->node;
				exportNode.i = i;
				exportNode.name = node->GetName();
				// any object whose name begins with * will not be exported
				if (!_tcsncmp("*", exportNode.name, 1))
					continue;
				// ignore parts (use only if exporting separate models)
				if (g_mesh_special && (strstr(exportNode.name, "_collision") || strstr(exportNode.name, "_shadow")))
					continue;
				// consider any object whose name starts with 'tag_' to be a tag
				if (_tcsncmp("tag_", exportNode.name, 4) == 0)
				{
					lTags.push_back(exportNode);
					continue;
				}
				// check if mesh has triangles
				if (IsTriNode(node, start_time))
					lMeshes.push_back(exportNode);
			}
			result = ExportQuake3Model(filename, ei, gi, start_time, lTags, lMeshes);
			lTags.clear();
			lMeshes.clear();
		}

		// generate shader string
		if (!g_shaders_string)
			g_shaders_string = (char *)malloc(32768);
		memset(g_shaders_string, 0, 32768);
		static const char *shader_template = 
			"%s%s\r\n"
			"{\r\n"
			"	dpmeshcollisions\r\n"
			"	dpglossintensitymod 1\r\n"
			"	dpglossexponentmod 1\r\n"
			"	{\r\n"
			"		map \"%s\"\r\n"
			"	}\r\n"
			"	{\r\n"
			"		map $lightmap\r\n"
			"	}\r\n"
			"}\r\n"
			"\r\n";
		for (i = 0; i < (int)g_shaders.size(); i++)
			sprintf(g_shaders_string, shader_template, g_shaders_string, g_shaders[i].name, g_shaders[i].name);
		ShowExportMessages(g_shaders_string);
	}

	DestroyWindow(exportWindow);
	return result;
}