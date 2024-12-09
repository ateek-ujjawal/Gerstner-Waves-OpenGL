#version 410

// Tessellation control shader: we decide how much we should tessellate a quad geometry.
// All tessellation in OpenGL is done according to an "abstract patch", which is a quad,
// in this case. The outer tessellation level decides how many edges to spawn in each of
// the edges of the quad, and the inner tessellation level say how many inner primitives
// to spawn (in this case, quads). These settings are passed over to the primitive tess.
// unit that will generate more geometry for us.

layout(vertices = 4) out;

in PipelineData {
    vec3 v_vertexPosition;
    vec2 v_texCoords;
    vec3 v_vertexNormals;
} tc_in[];

out PipelineData {
    vec3 v_vertexPosition;
    vec2 v_texCoords;
    vec3 v_vertexNormals;
} tc_out[];

void main() {
    // Just forward the vertex attributes through the GL pipeline.
    tc_out[gl_InvocationID].v_vertexPosition = tc_in[gl_InvocationID].v_vertexPosition;
    tc_out[gl_InvocationID].v_texCoords = tc_in[gl_InvocationID].v_texCoords;
    tc_out[gl_InvocationID].v_vertexNormals = tc_in[gl_InvocationID].v_vertexNormals;

    float tess_level = 100.0;

    gl_TessLevelInner[0] = tess_level;
    gl_TessLevelInner[1] = tess_level;
    gl_TessLevelOuter[0] = tess_level; 
    gl_TessLevelOuter[1] = tess_level;
    gl_TessLevelOuter[2] = tess_level; 
    gl_TessLevelOuter[3] = tess_level;
}