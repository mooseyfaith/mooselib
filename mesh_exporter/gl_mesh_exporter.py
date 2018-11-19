bl_info = {
	"name": "Export GL Mesh",
	"description": "Exports very specific opengl mesh buffer format.",
	"author": "Tafil Kajtazi",
	"version": (1, 0),
	"blender": (2, 70),
	"location": "File > Export",
	"category": "Import-Export"}

import bpy
import math
import mathutils
import time

def average_add(average, delta):
	average[0] += 1

	for i in range(3):
		average[i + 1] += delta[i]

def average_end(average):
	for i in range(3):
		average[i + 1] /= average[0]

class GLMeshExporter(bpy.types.Operator):
	"""Export GL Mesh"""      				# blender will use this as a tooltip for menu items and buttons.
	bl_idname = "object.gl_mesh_export"        # unique identifier for buttons and menu items to reference.
	bl_label = "Export gl mesh (.glm)"         # display name in the interface.

	filepath = bpy.props.StringProperty(subtype="FILE_PATH", default="*.glm")


	# export options ##########################################################

	apply_modifiers = bpy.props.BoolProperty(
		name        = "Apply Visible Modifiers",
		description = "",
		default     = True,
		)

	with_tangents = bpy.props.BoolProperty(
		name        = "Export Tangents",
		description = """Add Vertex Buffer Attribute 'tangent' for all vertecies.
						 (used for Normal Mapping)""",
		default     = False,
		)

	def draw(self, context):
		layout = self.layout

		row = layout.row()
		row.prop(self, "apply_modifiers")

		row = layout.row()
		row.prop(self, "with_tangents")
	
	# collect unique vertices and assign them continuoes indices

	def insert_unique_vertex(self, mesh, uvs, vertex_index, polygon_index_plus_one, loop_index):
		new_vertex = {
			'vertex_index': vertex_index,
			'polygon_index_plus_one': polygon_index_plus_one,
			'loop_index': loop_index,
			'index': self.unique_vertex_count
		}

		# flat polygon vertices are allways unique
		if polygon_index_plus_one != 0:
			self.unique_vertices[self.unique_vertex_count] = new_vertex
			self.unique_vertex_count += 1

			return new_vertex['index']

		if uvs:
			vertex = (mesh.loops[loop_index].vertex_index, uvs[loop_index].uv[:])
		else:
			vertex = (mesh.loops[loop_index].vertex_index)

		if vertex in self.smooth_vertices:
			return self.smooth_vertices[vertex]

		self.smooth_vertices[vertex] = self.unique_vertex_count
		self.unique_vertices[self.unique_vertex_count] = new_vertex
		self.unique_vertex_count += 1

		return self.unique_vertex_count - 1

	# formatting ##############################################################

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

	def swap_yz(self, vector):
		return (vector[0], vector[2], -vector[1])

	def negative(self, vector):
		return (-vector[0], -vector[1], -vector[2])

	def write_vertex_attributes(self, mesh, active_uvs, vertex_index, polygon_index_plus_one, loop_index, has_uv, has_bones):
		# swizlle position and normal so that y and z are swapped (as it should be...)

		self.write("%.6f %.6f %.6f " % self.swap_yz(mesh.vertices[vertex_index].co[:]))
			
		if mesh.vertex_colors:
			self.write("u8 4 %d %d %d %d " % int(255 * mesh.vertex_colors[vertex_index].co[:]))

		if polygon_index_plus_one == 0:
			vertex = mesh.vertices[vertex_index]
		else:
			vertex = mesh.polygons[polygon_index_plus_one - 1]

		self.write("%.6f %.6f %.6f " % self.swap_yz(vertex.normal[:]))

		if self.with_tangents:
			self.write("%.6f %.6f %.6f " % self.swap_yz(mesh.loops[loop_index].tangent[:]))

		if has_uv:
			self.write("%.6f %.6f " % active_uvs[loop_index].uv[:])

		if has_bones:
			self.write_bone_attributes(mesh.vertices[vertex_index])

	def write_bone_attributes(self, vertex):
		bones = []
		
		for group_element in vertex.groups:
			bones.append((group_element.weight, group_element.group))
					
		if len(vertex.groups) < 4:
			for i in range(len(vertex.groups), 4):
				bones.append((0.0, 0))
					
		elif len(vertex.groups) > 4:
			self.report({'WARNING'}, "warning: vertex belongs to more then 4 vertex groups, only the top 4 groups with the highest weights will be used")			
			# search for the top 4 most influential bones and only use them
			bones = sorted(bones, key=lambda bone_element: bone_element[0])

			total_weight = 0.0
			for bone_element in bones:
				total_weight += bone_element[0]

			# normalize bone_weight so that the total_weight of the top 4 bones adds up to 1.0
			if total_weight > 0.0:
				for bone_element in bones:
					bone_element = (bone_element[0] / total_weight, bone_element[1]);

		self.write("u8 4 ")

		for i in range(4):
			self.write("%d " % bones[i][1])
					
		for i in range(4):
			self.write("%.6f " % bones[i][0])

	def execute(self, context):
		# reset temporary data here, stored in class to avoid passing around
		self.alignment = ""
		self.file = None
		self.new_line_pending = False
		#self.unique_ids = []
	
		object = context.object
		#mesh = object.data
		mesh = object.to_mesh(context.scene, self.apply_modifiers, 'PREVIEW')
		triangles = []

		mesh.calc_normals_split()

		if self.with_tangents:
			mesh.calc_tangents()

		has_uv = mesh.uv_layers.active != None
		active_uvs = None
		if has_uv:
			active_uvs = mesh.uv_layers.active.data
		
		start_time = time.time()

		smooth_vertex_count = 0
		flat_vertex_count = 0
		for polygon in mesh.polygons:
			vertex_count = len(polygon.vertices)
			if (vertex_count < 3) or (vertex_count > 4):
				continue

			if polygon.use_smooth:
				smooth_vertex_count += vertex_count
			else:
				flat_vertex_count += vertex_count

		self.report({'INFO'}, "polygon count: %d, flat vertex count: %d, smooth vertex count: up to %d" % (len(mesh.polygons), flat_vertex_count, smooth_vertex_count))

		#  reserve enough space
		self.unique_vertices = [None] * (flat_vertex_count + smooth_vertex_count)
		self.unique_vertex_count = 0
		self.smooth_vertices = {}

		for polygon in mesh.polygons:
			polygon_index_plus_one = polygon.index + 1
			if polygon.use_smooth:
				polygon_index_plus_one = 0

			if len(polygon.vertices) == 3:
				for vertex_index, loop_index in zip(polygon.vertices, polygon.loop_indices):
					triangles.append(self.insert_unique_vertex(mesh, active_uvs, vertex_index, polygon_index_plus_one, loop_index))
			
			elif len(polygon.vertices) == 4:
				# only insert once, since multiply inserts will split vertices of flat polygons
				index_0 = self.insert_unique_vertex(mesh, active_uvs, polygon.vertices[0], polygon_index_plus_one, polygon.loop_indices[0])
				index_1 = self.insert_unique_vertex(mesh, active_uvs, polygon.vertices[1], polygon_index_plus_one, polygon.loop_indices[1])
				index_2 = self.insert_unique_vertex(mesh, active_uvs, polygon.vertices[2], polygon_index_plus_one, polygon.loop_indices[2])
				index_3 = self.insert_unique_vertex(mesh, active_uvs, polygon.vertices[3], polygon_index_plus_one, polygon.loop_indices[3])

				triangles.append(index_0)
				triangles.append(index_1)
				triangles.append(index_2)

				triangles.append(index_0)
				triangles.append(index_2)
				triangles.append(index_3)
			else:		
				self.report({'WARNING'}, "skipping polygon with %d sides, please use triangles and quads only" % len(polygon.vertices))

		self.report({'INFO'}, "unique vertex count: %d, triangle index count: %d, triangle count: %d" % (self.unique_vertex_count, len(triangles), len(triangles) / 3))

		triangles_build_time = time.time() - start_time
		self.report({'INFO'}, "triangles build time: %f" % triangles_build_time)

		self.file = open(self.filepath, 'w')
		
		self.write("# OpenGL Mesh Data")
		self.new_line()

		self.new_line()
		self.write("vertex_buffers ")
		self.open_array(1)
		self.new_line()

		attribue_count = 2 # has at least position, normal

		if self.with_tangents:
			attribue_count += 1

		if mesh.vertex_colors:
			attribue_count += 1

		if mesh.uv_layers:
			attribue_count += 1

		has_bones = len(object.vertex_groups) > 0
		if has_bones:
			attribue_count += 2

		self.new_line()
		self.write("vertex_buffer ")
		self.open_array(attribue_count)
		self.new_line()
		
		self.write("position 3 f32 0 0")
		self.new_line()

		if mesh.vertex_colors:
			self.write("color 4 u8 1 0") #normalized unsinged byte
			self.new_line()

		self.write("normal 3 f32 0 0")
		self.new_line()

		if self.with_tangents:
			self.write("tangent 3 f32 0 0")
			self.new_line()
		
		if has_uv:
			self.write("uv0 2 f32 0 0")
			self.new_line()

		if has_bones:
			self.write("bone_index 4 u8 0 0")
			self.new_line()
			self.write("bone_weight 4 f32 0 0")
			self.new_line()
	
		self.close_array()
		self.open_array(self.unique_vertex_count)
		self.new_line()

		for index in range(self.unique_vertex_count):
			vertex_index           = self.unique_vertices[index]['vertex_index']
			polygon_index_plus_one = self.unique_vertices[index]['polygon_index_plus_one']
			loop_index             = self.unique_vertices[index]['loop_index']

			self.write_vertex_attributes(mesh, active_uvs, vertex_index, polygon_index_plus_one, loop_index, has_uv, has_bones)

			self.new_line()

		self.unique_vertices = None

		self.close_array() # vertex_buffer data
		self.new_line()

		self.close_array()
		self.write("# vertex buffers")
		self.new_line()

		self.new_line()
		self.write("indices ")
		self.open_array(len(triangles))
		self.new_line()

		corner_count = 0
		triangles_in_line_count = 0
		max_triangles_in_line_count = 1

		for i in triangles:
			self.write("%d" % i)

			corner_count += 1
			if corner_count == 3:
				corner_count = 0
				triangles_in_line_count += 1

				if triangles_in_line_count == max_triangles_in_line_count:
					triangles_in_line_count = 0
					self.new_line()
				else:
					self.write("  ")
			else:
				self.write(" ")
		
		if triangles_in_line_count != 0:
			self.new_line()

		self.close_array()
		self.new_line()

		for modifier in object.modifiers:
			if modifier.type == 'ARMATURE':
				armature = modifier.object.data
            
				self.write("bones ")
				self.open_array(len(armature.bones))
				self.new_line()

				root_bone = None
				for bone in armature.bones:
					self.write('bone "%s" ' % bone.name)

					if bone.parent == None:
						self.write('root ')
						root_bone = bone
						#break
					else:
						self.write('parent "%s" ' % bone.parent.name)

					transform = bone.matrix_local.transposed() # local to armature not relative to parent

					self.write("%.6f %.6f %.6f " % self.swap_yz(transform[0][:-1]))
					self.write("%.6f %.6f %.6f " % self.swap_yz(transform[1][:-1]))
					self.write("%.6f %.6f %.6f " % self.swap_yz(transform[2][:-1]))
					self.write("%.6f %.6f %.6f " % self.swap_yz(transform[3][:-1]))

					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.x_axis[:]))
					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.y_axis[:]))
					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.z_axis[:]))
					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.head_local[:]))

					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.x_axis[:]))
					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.z_axis[:]))
					#self.write("%.6f %.6f %.6f " % self.swap_yz(self.negative(bone.y_axis[:])))
					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.head[:]))

					#self.write("%.6f %.6f %.6f " % (bone.x_axis[:]))
					#self.write("%.6f %.6f %.6f " % (bone.z_axis[:]))
					#self.write("%.6f %.6f %.6f " % self.negative(bone.y_axis[:]))
					#self.write("%.6f %.6f %.6f " % self.swap_yz(bone.head[:]))

					self.new_line()
            
				self.close_array()
				self.new_line()

				#if for whatever reason the bones are not in depth first search order use this:
				'''
				self.write("bones_hirarchy ")
				self.open_array(len(armature.bones))
				self.new_line()

				self.write('bone "%s" root' % root_bone.name)
				#self.open_array(len(root_bone.children))
				self.new_line()
            
				path = [ (0, root_bone) ]
            
				while len(path):
					next_child_index, bone = path[-1]
                
					if next_child_index < len(bone.children):
						path[-1] = (next_child_index + 1, bone)
						bone = bone.children[next_child_index]
						path.append((0, bone))

						self.write('bone "%s" parent "%s"' % (bone.name, path[-2][1].name))
						#self.open_array(len(bone.children))
						self.new_line()
					else:
						path = path[:-1]
						#self.close_array()
						#self.new_line()

				self.close_array()
				self.new_line()
				'''

		self.new_line()
		self.write("draw_calls ")
		self.open_array(1)
		self.new_line()

		self.write("triangles 0 %d" % len(triangles))
		self.new_line()
		
		self.close_array()
		self.new_line()
				
		self.file.close()

		write_time = time.time() - start_time - triangles_build_time
		self.report({'INFO'}, "write time %f" % write_time)

		return {'FINISHED'}
	
	
	def invoke(self, context, event):
		context.window_manager.fileselect_add(self)
		return {'RUNNING_MODAL'}


# Only needed if you want to add into a dynamic menu
def menu_func(self, context):
	self.layout.operator_context = 'INVOKE_DEFAULT'
	self.layout.operator(GLMeshExporter.bl_idname, text="Export GL Mesh (.glm)")


def register():
	bpy.utils.register_class(GLMeshExporter)
	bpy.types.INFO_MT_file_export.append(menu_func)


def unregister():
	bpy.types.INFO_MT_file_export.remove(menu_func)
	bpy.utils.unregister_class(GLMeshExporter)


if __name__ == "__main__":
	register()