import bpy

bl_info = {
	"name": "p2fx Blender Scripts",
	"author": "hero",
	"version": (1, 0, 0),
	"blender": (3, 5, 0),
	"location": "File > Import/Export",
	"description": "For inter-operation with P2FX.",
	#"warning": "",
	"category": "Import-Export",
}

from . import utils, import_p2gr, export_p2gr

classes = (
	import_p2gr.P2grImporter,
	export_p2gr.P2grExporter,
)

def menu_func_import_p2gr(self, context):
	self.layout.operator(import_p2gr.P2grImporter.bl_idname, text="P2FX p2fxGameRecord (.p2gr)")

def menu_func_export_p2gr(self, context):
	self.layout.operator(export_p2gr.P2grExporter.bl_idname, text="P2FX P2GR Batch Export (.fbx)")

def register():
	from bpy.utils import register_class
	for cls in classes:
		register_class(cls)
	
	bpy.types.TOPBAR_MT_file_import.append(menu_func_import_p2gr)
	bpy.types.TOPBAR_MT_file_export.append(menu_func_export_p2gr)

def unregister():
	from bpy.utils import unregister_class
	for cls in reversed(classes):
		unregister_class(cls)
	
	bpy.types.TOPBAR_MT_file_import.remove(menu_func_import_p2gr)
	bpy.types.TOPBAR_MT_file_export.remove(menu_func_export_p2gr)

if __name__ == "__main__":
	unregister()
	register()
