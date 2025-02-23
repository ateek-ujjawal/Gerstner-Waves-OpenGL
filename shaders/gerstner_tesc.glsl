#version 410

// Tessellation control shader
// Generate tessellated geometry using the 4 control points provided for a quad from the
// vertex shader. These control points are then passed onto a primitive generator which
// generates more geometry based on the inner and outer tessellation levels provided.

layout(vertices = 4) out;

in VertexData {
    vec3 v_vertexPosition;
    vec3 v_vertexNormals;
} tc_in[];

out VertexData {
    vec3 v_vertexPosition;
    vec3 v_vertexNormals;
} tc_out[];

void main() {
    // Just forward the vertex attributes through the GL pipeline.
    tc_out[gl_InvocationID].v_vertexPosition = tc_in[gl_InvocationID].v_vertexPosition;
    tc_out[gl_InvocationID].v_vertexNormals = tc_in[gl_InvocationID].v_vertexNormals;

    float tess_level = 100.0;

    // Define inner and outer tessellation levels
    // For quads it has 2 inner levels and 4 outer levels
    gl_TessLevelInner[0] = tess_level;
    gl_TessLevelInner[1] = tess_level;
    gl_TessLevelOuter[0] = tess_level; 
    gl_TessLevelOuter[1] = tess_level;
    gl_TessLevelOuter[2] = tess_level; 
    gl_TessLevelOuter[3] = tess_level;
}