#include "cgmath.h"			// slee's simple math library
#include "cgut.h"			// slee's OpenGL utility
#include "GL/glfw3.h"
#include "GL/glad.h"


//*******************************************************************
// global constants
static const char*	window_name = "cgbase - circle";
static const char*	vert_shader_path = "../bin/shaders/circ.vert";
static const char*	frag_shader_path = "../bin/shaders/circ.frag";
uint				NUM_TESS = 36;		// initial tessellation factor

//*******************************************************************
// window objects
GLFWwindow*	window = nullptr;
ivec2		window_size = ivec2(1024, 576);	// initial window size

//*******************************************************************
// OpenGL objects
GLuint	program = 0;	// ID holder for GPU program
GLuint	vertex_buffer = 0;	// ID holder for vertex buffer
GLuint	index_buffer = 0;	// ID holder for index buffer

//*******************************************************************
// global variables
int		frame = 0;				// index of rendering frames
int    solid_color = 0;
float	radius = 1.0f;

bool	bUseIndexBuffer = true;
bool	bWireframe = false;
bool    bRotation = false;   // this is the default

//*******************************************************************
// holder of vertices and indices
std::vector<vertex>	vertex_list;	// host-side vertices
std::vector<uint>	index_list;		// host-side indices

//*******************************************************************
void update()
{
	// update simulation
	float t = float(glfwGetTime())*0.5f;
	mat4 view_projection_matrix =
	{
		0, 1, 0, 0,
		0, 0, 1, 0,
	   -1, 0, 0, 1,
		0, 0, 0, 1
	};


	mat4 rotation_matrix =				// explained later (in the transformation lecture)
	{
		cos(t), -sin(t), 0, 0,
		sin(t), cos(t), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// update uniform variables in vertex/fragment shaders
	GLint uloc;

	uloc = glGetUniformLocation(program, "projection_matrix"); if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, view_projection_matrix);
	uloc = glGetUniformLocation(program, "solid_color");		if (uloc > -1) glUniform1i(uloc, solid_color);
	uloc = glGetUniformLocation(program, "bRotation");		if (uloc > -1) glUniform1i(uloc, bRotation);
	uloc = glGetUniformLocation(program, "aspect_ratio");		if (uloc > -1) glUniform1f(uloc, window_size.x / float(window_size.y));
	uloc = glGetUniformLocation(program, "radius");			if (uloc > -1) glUniform1f(uloc, radius);
	uloc = glGetUniformLocation(program, "model_matrix");		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, rotation_matrix);
}

void render()
{
	// clear screen (with background color) and clear depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// notify GL that we use our own program
	glUseProgram(program);

	// bind vertex attributes to your shader program
	const char*	vertex_attrib[] = { "position", "normal", "texcoord" };
	size_t		attrib_size[] = { sizeof(vertex().pos), sizeof(vertex().norm), sizeof(vertex().tex) };
	for (size_t k = 0, kn = std::extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1])
	{
		GLuint loc = glGetAttribLocation(program, vertex_attrib[k]); if (loc >= kn) continue;
		glEnableVertexAttribArray(loc);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
	}

	// render vertices: trigger shader programs to process vertex data
	if (bUseIndexBuffer)
	{
		if (index_buffer) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
		glDrawElements(GL_TRIANGLES, index_list.size(), GL_UNSIGNED_INT, nullptr);
	}
	else
	{
		glDrawArrays(GL_TRIANGLES, 0, NUM_TESS * (NUM_TESS * 2) * 2 * 3); // NUM_TESS = N
	}

	// swap front and back buffers, and display to screen
	glfwSwapBuffers(window);
}

void reshape(GLFWwindow* window, int width, int height)
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width, height);
	glViewport(0, 0, width, height);
}

void print_help()
{
	printf("[help]\n");
	printf("- press ESC or 'q' to terminate the program\n");
	printf("- press F1 or 'h' to see help\n");
	printf("- press 'w' to toggle wireframe\n");
	printf("- press 'd' to toggle (tc.xy, 0) > (tc.xxx, 1) > (tc.yyy, 1)\n");
	printf("- press 'r' to rotate the sphere\n");

	printf("\n");
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	void update_vertex_buffer(uint N);	// forward declaration
	void update_sphere_vertices(uint N);	// forward declaration

	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)	glfwSetWindowShouldClose(window, GL_TRUE);
		else if (key == GLFW_KEY_H || key == GLFW_KEY_F1)	print_help();
		else if (key == GLFW_KEY_W)
		{
			bWireframe = !bWireframe;
			glPolygonMode(GL_FRONT_AND_BACK, bWireframe ? GL_LINE : GL_FILL);
			printf("> using %s mode\n", bWireframe ? "wireframe" : "solid");
		}

		else if (key == GLFW_KEY_D)
		{
			solid_color = (solid_color + 1) % 3;
			if (solid_color == 0)
				printf("> using (texcord.xy, 0, 1)\n");
			else if (solid_color == 1)
				printf("> using (texcord.xxx, 1)\n");
			else if (solid_color == 2)
				printf("> using (texcord.yyy, 1)\n");
		}

		else if (key == GLFW_KEY_R)
		{
			bRotation = !bRotation;
		}
	}
}

void mouse(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT&&action == GLFW_PRESS)
	{
		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
		printf("> Left mouse button pressed at (%d, %d)\n", int(pos.x), int(pos.y));
	}
}

void motion(GLFWwindow* window, double x, double y)
{
}

void update_vertex_buffer(uint N)
{
	// clear and create new buffers
	if (vertex_buffer)	glDeleteBuffers(1, &vertex_buffer);	vertex_buffer = 0;
	if (index_buffer)	glDeleteBuffers(1, &index_buffer);	index_buffer = 0;

	// check exceptions
	if (vertex_list.empty()) { printf("[error] vertex_list is empty.\n"); return; }

	// create buffers
	if (bUseIndexBuffer)
	{
		index_list.clear();
		for (uint i = 0; i < N; i++)
		{
			for (uint k = 0; k < N * 2; k++)
			{

				index_list.push_back((N * 2 + 1) * i + (k + 1));
				index_list.push_back((N * 2 + 1) * (i + 1) + k);
				index_list.push_back((N * 2 + 1) * (i + 1) + (k + 1));

				index_list.push_back((N * 2 + 1) * (i + 1) + k);
				index_list.push_back((N * 2 + 1) * i + (k + 1));
				index_list.push_back((N * 2 + 1) * i + k);


			}
		}

		// generation of vertex buffer: use vertex_list as it is
		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vertex_list.size(), &vertex_list[0], GL_STATIC_DRAW);

		// geneation of index buffer
		glGenBuffers(1, &index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*index_list.size(), &index_list[0], GL_STATIC_DRAW);
	}
	else
	{
		std::vector<vertex> triangle_vertices;
		for (uint i = 0; i < N; i++)
		{
			for (uint k = 0; k < N * 2; k++)
			{
				triangle_vertices.push_back(vertex_list[(N * 2 + 1) * i + (k + 1)]);
				triangle_vertices.push_back(vertex_list[(N * 2 + 1) * (i + 1) + k]);
				triangle_vertices.push_back(vertex_list[(N * 2 + 1) * (i + 1) + (k + 1)]);

				triangle_vertices.push_back(vertex_list[(N * 2 + 1) * (i + 1) + k]);
				triangle_vertices.push_back(vertex_list[(N * 2 + 1) * i + (k + 1)]);
				triangle_vertices.push_back(vertex_list[(N * 2 + 1) * i + k]);
			}
		}

		// generation of vertex buffer: use triangle_vertices instead of vertex_list
		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*triangle_vertices.size(), &triangle_vertices[0], GL_STATIC_DRAW);

	}
}

void update_sphere_vertices(uint N)
{
	vertex_list.clear();

	for (uint i = 0; i <= N; i++)
	{
		float alpha = PI * 1.0f / float(N) * float(i);
		for (uint k = 0; k <= N * 2; k++)
		{
			float beta = PI * 2.0f / float(N * 2) * float(k);
			vertex_list.push_back
			({
				vec3(radius * sin(alpha) * cos(beta), radius * sin(alpha) * sin(beta), radius * cos(alpha)),  //vertex position
				vec3(sin(alpha) * cos(beta), sin(alpha) * sin(beta), cos(alpha)),                             //normal vector facing your eye
				vec2(beta / (2 * PI), 1 - alpha / PI)														  //texture coordinate
			});
		}
	}
}

bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glLineWidth(1.0f);
	glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);	// set clear color
	glEnable(GL_CULL_FACE);								// turn on backface culling
	glEnable(GL_DEPTH_TEST);								// turn on depth tests

	// define the position of four corner vertices
	update_sphere_vertices(NUM_TESS);

	// create vertex buffer; called again when index buffering mode is toggled
	update_vertex_buffer(NUM_TESS);

	return true;
}

void user_finalize()
{
}

void main(int argc, char* argv[])
{
	// initialization
	if (!glfwInit()) { printf("[error] failed in glfwInit()\n"); return; }

	// create window and initialize OpenGL extensions
	if (!(window = cg_create_window(window_name, window_size.x, window_size.y))) { glfwTerminate(); return; }
	if (!cg_init_extensions(window)) { glfwTerminate(); return; }	// init OpenGL extensions

	// initializations and validations of GLSL program
	if (!(program = cg_create_program(vert_shader_path, frag_shader_path))) { glfwTerminate(); return; }	// create and compile shaders/program
	if (!user_init()) { printf("Failed to user_init()\n"); glfwTerminate(); return; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback(window, reshape);	// callback for window resizing events
	glfwSetKeyCallback(window, keyboard);			// callback for keyboard events
	glfwSetMouseButtonCallback(window, mouse);	// callback for mouse click inputs
	glfwSetCursorPosCallback(window, motion);		// callback for mouse movements

	// enters rendering/event loop
	for (frame = 0; !glfwWindowShouldClose(window); frame++)
	{
		glfwPollEvents();	// polling and processing of events
		update();			// per-frame update
		render();			// per-frame render
	}

	// normal termination
	user_finalize();
	glfwDestroyWindow(window);
	glfwTerminate();
}
