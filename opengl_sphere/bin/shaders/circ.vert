#version 130

// input attributes of vertices
in vec3 position;	
in vec3 normal;
in vec2 texcoord;

// outputs of vertex shader = input to fragment shader
// out vec4 gl_Position: a built-in output variable that should be written in main()
out vec3 norm;	// the second output: not used yet
out vec2 tc;	// the third output: not used yet

// uniform variables
uniform float	aspect_ratio;	// to correct a distortion of the shape
uniform float	radius;			// scale of a circle
uniform mat4	model_matrix;	// 4x4 transformation matrix: explained later in the lecture
uniform mat4	projection_matrix;
uniform bool	bRotation;

void main()
{
	if(!bRotation)
	{
		gl_Position = projection_matrix * vec4( position * radius, 1 );
		gl_Position.xy *= aspect_ratio>1 ? vec2(1/aspect_ratio,1) : vec2(1,aspect_ratio);
	}
	else
	{
		gl_Position = projection_matrix * model_matrix * vec4( position * radius, 1 );
		gl_Position.xy *= aspect_ratio>1 ? vec2(1/aspect_ratio,1) : vec2(1,aspect_ratio);
	}
	// another output passed via varying variable
	norm = normal;
	tc = texcoord;
}