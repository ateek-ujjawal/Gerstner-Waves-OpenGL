#version 410

in PipelineData {
    vec3 v_vertexPosition;
    vec3 v_vertexColors;
    vec3 v_vertexNormals;
} fs_in;

out vec4 color;

// Entry point of program
void main()
{
	color = vec4(fs_in.v_vertexColors.r,fs_in.v_vertexColors.g, fs_in.v_vertexColors.b, 1.0f);
}
