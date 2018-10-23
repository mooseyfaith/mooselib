bl_info = {
	"name": "Export Animation",
	"description": "Exports very specific animation format.",
	"author": "Tafil Kajtazi",
	"version": (1, 0),
	"blender": (2, 70),
	"location": "File > Export",
	"category": "Import-Export"}

tool_tip = "Export Animations (.anim)"

import bpy
import math
import mathutils

class TK_Animation_Exporter(bpy.types.Operator):
	"""Export Animation"""      # blender will use this as a tooltip for menu items and buttons.
	bl_idname = "object.tk_anim_export"        # unique identifier for buttons and menu items to reference.
	bl_label = tool_tip         # display name in the interface.

	filepath = bpy.props.StringProperty(subtype="FILE_PATH")
	
	def unique_vertex_index(self, vertex_tupel):
		if vertex_tupel in self.unique_ids:
			return self.unique_ids.index(vertex_tupel)
		else:
			self.unique_ids.append(vertex_tupel)
			return len(self.unique_ids) - 1

	def open_array(self, count):
		self.write("%d { " % count)
		self.alignment += "\t"

	def close_array(self):
		self.alignment = self.alignment[:-1]
		self.write("} ")

	def new_line(self):
		if self.new_line_pending:
			self.file.write("\n" + self.alignment)
		
		self.new_line_pending = True

	def write(self, text):
		if self.new_line_pending:
			self.file.write("\n" + self.alignment)
			self.new_line_pending = False

		self.file.write(text)

	def execute(self, context): # execute() is called by blender when running the operator
		# reset temporary data here, stored in class to avoid passing around
		self.alignment = ""
		self.file = None
		self.new_line_pending = False
		self.unique_ids = []

		self.file = open(self.filepath, 'w')
		
		self.write("# Animation Data")
		self.new_line()

		# ToDo: read framerate from blender settings
		framerate = 60
		self.write("framerate %d" % framerate)
		self.new_line()

		actions = bpy.data.actions
		self.write("animations ")
		self.open_array(len(actions))

		for action in actions:
			if len(action.fcurves) == 0:
				continue

			action_frame_start, action_frame_end = action.fcurves[0].range()

			self.new_line()
			self.write('animation "%s" ' % action.name)
						
			self.open_array(len(action.groups))
			self.new_line()

			for group in action.groups:
				self.write('"%s" ' % group.name)

				has_translation = False
				has_rotation = False
				has_scale = False

				for curve in group.channels:
					if curve.data_path.endswith("location"):
						has_translation = True
            
					if curve.data_path.endswith("rotation_quaternion"):
						has_rotation = True
                
					if curve.data_path.endswith("scale"):
						has_scale = True
                
					curve_frame_start, curve_frame_end = curve.range()
        
					if curve_frame_start < action_frame_start:
						action_frame_start = curve_frame_start
        
					if curve_frame_end > action_frame_end:
						action_frame_end = curve_frame_end

				group_channel_count = 0
				
				if has_translation:
					group_channel_count += 1

				if has_rotation:
					group_channel_count += 1
				
				if has_scale:
					group_channel_count += 1

				self.open_array(group_channel_count)

				if has_translation:
					self.write("translation ")
	            
				if has_rotation:
					self.write("rotation ")
            
				if has_scale:
					self.write("scale ")

				self.close_array()
				self.new_line()
    
			self.close_array()

			self.write("frames %d " % int(action_frame_start))
			self.open_array(int(action_frame_end - action_frame_start) + 1)

			for frame in range(int(action_frame_start), int(action_frame_end) + 1):
				self.new_line()

				for group in action.groups:
					for curve in group.channels:
						self.write("%.6f " % curve.evaluate(frame))

					self.new_line()
					continue

					translation = [ 0.0, 0.0, 0.0 ]
					rotation	= [ 0.0, 0.0, 0.0, 0.0 ]
					scale		= [ 0.0, 0.0, 0.0 ]

					has_translation = False
					has_rotation = False
					has_scale = False

					# swizzle so that y and z coordinates are swapped (as it should be ...)
					# this is gonna be slow, but better here than in importer
					for curve in group.channels:
						if curve.data_path.endswith("location"):
							if curve.array_index == 1:
								translation[2] = -curve.evaluate(frame)
							elif curve.array_index == 2:
								translation[1] = curve.evaluate(frame)
							else:
								translation[curve.array_index] = curve.evaluate(frame)

							has_translation = True

						elif curve.data_path.endswith("rotation_quaternion"):
							if curve.array_index == 2:
								rotation[3] = -curve.evaluate(frame)
							elif curve.array_index == 3:
								rotation[2] = curve.evaluate(frame)
							else:
								rotation[curve.array_index] = curve.evaluate(frame)

							has_rotation = True

						elif curve.data_path.endswith("scale"):
							if curve.array_index == 1:
								scale[2] = -curve.evaluate(frame)
							elif curve.array_index == 2:
								scale[1] = curve.evaluate(frame)
							else:
								scale[curve.array_index] = curve.evaluate(frame)

							has_scale = True
					
					if has_translation:
						self.write("%.6f %.6f %.6f " % tuple(translation))
	            
					if has_rotation:
						self.write("%.6f %.6f %.6f %.6f " % tuple(rotation))

					if has_scale:
						self.write("%.6f %.6f %.6f " % tuple(scale))

					self.new_line()
				
			self.close_array()
			self.new_line()

		self.close_array()
				
		self.file.close()

		return {'FINISHED'}
	
	
	def invoke(self, context, event):
		context.window_manager.fileselect_add(self)
		return {'RUNNING_MODAL'}


# Only needed if you want to add into a dynamic menu
def menu_func(self, context):
	self.layout.operator_context = 'INVOKE_DEFAULT'
	self.layout.operator(TK_Animation_Exporter.bl_idname, text = tool_tip)


def register():
	bpy.utils.register_class(TK_Animation_Exporter)
	bpy.types.INFO_MT_file_export.append(menu_func)


def unregister():
	bpy.utils.unregister_class(TK_Animation_Exporter)


if __name__ == "__main__":
	register()