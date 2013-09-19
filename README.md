Darkplaces MD3 Exporter - a plugin for 3D Studio Max to export MD3 models.
Plugin code is based on Pop'N'Fresh's MD3 exporter for MAX3.

Project goals
------
- Clean up and improve for use with recent versions of 3DS MAX
- Provide reliable tool that gives warning for each thing that is wrong and needs to be fixed for correct exporting
- Export clean MD3's with correct normals and material names with no need of post-fixing
- Remove some old Quake 3-related switches (such as torso/legs exporting, object names filtering) not used with Darkplaces engine

Features of Pop'N'Fresh's part
------
- Export Quake 3 MD3 models
- Export animations (frame ranges, can set several ranges separated by ,)
- Export tags (objects which names starts with tag_)
- Export current materials as shadernames (detect root by searching for textures/ in bitmap path)

Features added
------
- Fixed normals exporting to entirely match MD3 specs (some cases with pure vertical normals wasnt exported right)
- Support for smoothing groups and edit normals modifier
- Better exporting of mirrored objects (ones having negative scale), warning are given anyway as max sometimes doesnt have valid data structures for mirrored objects, and this cant be autofixed during exporting
- Can export primitives
- Can get material name from multimaterial's first layer (some importers, like COLLADA, always sets multimaterial)
- New "create tag_pivot for pivot position" option to create tag for object's pivot (useful to store center of mass or light sampling point)
- New "export chunks" option to export separate .md3 model for each selected object
- Option to detect "collision" and "shadow" meshes, this meshes always exported 'as is' with no additional seams added (UV are null, vertex normals are null)
- Export results window with prints about missing normals, bogus mesh names, null materials etc.
- Advanced paths exporting (autoconverting \ to /, support path in material name, detects "textures/", "models/", "gfx/" as path root)
- Detects lazy object names (Box, Sphere etc.) and warns about it.
- Fixed UV exporting to use vertex channels instead of legacy getVert/getNumTVerts (sometimes this legacy structures are empty)
- Test shader script output
- Frame range defaults to 0

Build
------
To build MD3 Exporter you should get 3DS Max SDK and put the appropriate lib/ and include/ folders into sdk folder:
- for Max 7: sdk/SDK_Max7
- for Max 2012 x86: sdk/SDK_Max12
- for Max 2012 x64: sdk/SDK_Max12/x64

--------------------------------------------------------------------------------
 Version History + Changelog
--------------------------------------------------------------------------------

1.01
------
- Untested support for Max 2011

1.0
------
- Initial release. Supports Max 7 and Max 2012 (x86 and x64).