#version 130

// inputs from vertex shader
in vec2 tc;	// used for texture coordinate visualization

// output of the fragment shader
out vec4 fragColor;

// shader's global variables, called the uniform variables
uniform int solid_color;

void main()
{
	if(solid_color == 0)
		fragColor = vec4(tc.xy,0,1);
	else if(solid_color == 1)
		fragColor = vec4(tc.xxx, 1);
	else if(solid_color == 2)
		fragColor = vec4(tc.yyy, 1);
}