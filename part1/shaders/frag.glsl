#version 410

uniform sampler2D tex;

in PipelineData {
    vec3 v_vertexPosition;
    vec2 v_texCoords;
    vec3 v_vertexNormals;
} fs_in;

out vec4 color;

// Entry point of program
void main()
{
    vec3 diffuseColor =  texture(tex, fs_in.v_texCoords).rgb;
	color = vec4(diffuseColor, 1.0f);
}
