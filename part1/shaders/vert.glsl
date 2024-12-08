#version 410
// From Vertex Buffer Object (VBO)
// The only thing that can come 'in', that is
// what our shader reads, the first part of the
// graphics pipeline.
layout(location=0) in vec3 position;
layout(location=1) in vec2 texCoords;
layout(location=2) in vec3 vertexNormals;

// Uniform variables
uniform mat4 u_ModelMatrix;

// Pass vertex colors into the fragment shader
//out vec3 v_vertexPosition;
//out vec3 v_vertexColors;
//out vec3 v_vertexNormals;

out PipelineData {
    vec3 v_vertexPosition;
    vec2 v_texCoords;
    vec3 v_vertexNormals;
} vs_out;

void main()
{
  vec4 rotate_normal = u_ModelMatrix * vec4(vertexNormals, 0.0);
  vec4 world_pos = u_ModelMatrix * vec4(position, 1.0);

  vs_out.v_vertexPosition = world_pos.xyz;
  vs_out.v_texCoords = texCoords;
  vs_out.v_vertexNormals = rotate_normal.xyz;

  //v_vertexColors = vertexColors;
  //v_vertexNormals= vertexNormals;


  //vec4 newPosition = u_Projection * u_ViewMatrix * u_ModelMatrix * vec4(position,1.0f);
  //                                                                  // Don't forget 'w'
	//gl_Position = vec4(newPosition.x, newPosition.y, newPosition.z, newPosition.w);
}


