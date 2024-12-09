/* Compilation on Linux: 
 g++ -std=c++17 ./src/*.cpp -o prog -I ./include/ -I./../common/thirdparty/ -lSDL2 -ldl
*/

// Third Party Libraries
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp> 

// C++ Standard Template Library (STL)
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

// Our libraries
#include "Camera.hpp"
#include "PPM.hpp"

// vvvvvvvvvvvvvvvvvvvvvvvvvv Globals vvvvvvvvvvvvvvvvvvvvvvvvvv
// Globals generally are prefixed with 'g' in this application.

// Screen Dimensions
int gScreenWidth 						= 1920;
int gScreenHeight 						= 1080;
SDL_Window* gGraphicsApplicationWindow 	= nullptr;
SDL_GLContext gOpenGLContext			= nullptr;

// Main loop flag
bool gQuit = false; // If this is quit = 'true' then the program terminates.

// shader
// The following stores the a unique id for the graphics pipeline
// program object that will be used for our OpenGL draw calls.
GLuint gGraphicsPipelineShaderProgram	= 0;
GLuint gSkyboxPipelineShaderProgram     = 0;

// OpenGL Objects
// Vertex Array Object (VAO)
// Vertex array objects encapsulate all of the items needed to render an object.
// For example, we may have multiple vertex buffer objects (VBO) related to rendering one
// object. The VAO allows us to setup the OpenGL state to render that object using the
// correct layout and correct buffers with one call after being setup.
GLuint gVertexArrayObjectFloor= 0;
GLuint gVertexArrayObjectSkybox = 0;
// Vertex Buffer Object (VBO)
// Vertex Buffer Objects store information relating to vertices (e.g. positions, normals, textures)
// VBOs are our mechanism for arranging geometry on the GPU.
GLuint  gVertexBufferObjectFloor            = 0;
GLuint  gVertexBufferObjectSkybox           = 0;

// Water texture
GLuint gTexId                    = 0;
// Cubemap texture
GLuint gCubeTexId                = 0;

// Camera
Camera gCamera;

// Floor resolution
size_t gFloorTriangles  = 0;

// Quad size
float gOceanSize = 1500.0f;
int num_of_waves = 1;

// Polygon Mode
GLenum gPolygonMode = GL_FILL;

// ^^^^^^^^^^^^^^^^^^^^^^^^ Globals ^^^^^^^^^^^^^^^^^^^^^^^^^^^



// vvvvvvvvvvvvvvvvvvv Error Handling Routines vvvvvvvvvvvvvvv
static void GLClearAllErrors(){
    while(glGetError() != GL_NO_ERROR){
    }
}

// Returns true if we have an error
static bool GLCheckErrorStatus(const char* function, int line){
    while(GLenum error = glGetError()){
        std::cout << "OpenGL Error:" << error 
                  << "\tLine: " << line 
                  << "\tfunction: " << function << std::endl;
        return true;
    }
    return false;
}

#define GLCheck(x) GLClearAllErrors(); x; GLCheckErrorStatus(#x,__LINE__);
// ^^^^^^^^^^^^^^^^^^^ Error Handling Routines ^^^^^^^^^^^^^^^



/**
* LoadShaderAsString takes a filepath as an argument and will read line by line a file and return a string that is meant to be compiled at runtime for a vertex, fragment, geometry, tesselation, or compute shader.
* e.g.
*       LoadShaderAsString("./shaders/filepath");
*
* @param filename Path to the shader file
* @return Entire file stored as a single string 
*/
std::string LoadShaderAsString(const std::string& filename){
    // Resulting shader program loaded as a single string
    std::string result = "";

    std::string line = "";
    std::ifstream myFile(filename.c_str());

    if(myFile.is_open()){
        while(std::getline(myFile, line)){
            result += line + '\n';
        }
        myFile.close();

    }

    return result;
}


/**
* CompileShader will compile any valid vertex, fragment, geometry, tesselation, or compute shader.
* e.g.
*	    Compile a vertex shader: 	CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
*       Compile a fragment shader: 	CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
*
* @param type We use the 'type' field to determine which shader we are going to compile.
* @param source : The shader source code.
* @return id of the shaderObject
*/
GLuint CompileShader(GLuint type, const std::string& source){
	// Compile our shaders
	GLuint shaderObject;

	// Based on the type passed in, we create a shader object specifically for that
	// type.
	if(type == GL_VERTEX_SHADER){
		shaderObject = glCreateShader(GL_VERTEX_SHADER);
	}else if(type == GL_FRAGMENT_SHADER){
		shaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	} else if (type == GL_TESS_CONTROL_SHADER) {
        shaderObject = glCreateShader(GL_TESS_CONTROL_SHADER);
    } else if (type == GL_TESS_EVALUATION_SHADER) {
        shaderObject = glCreateShader(GL_TESS_EVALUATION_SHADER);
    }

	const char* src = source.c_str();
	// The source of our shader
	glShaderSource(shaderObject, 1, &src, nullptr);
	// Now compile our shader
	glCompileShader(shaderObject);

	// Retrieve the result of our compilation
	int result;
	// Our goal with glGetShaderiv is to retrieve the compilation status
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

	if(result == GL_FALSE){
		int length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		char* errorMessages = new char[length]; // Could also use alloca here.
		glGetShaderInfoLog(shaderObject, length, &length, errorMessages);

		if (type == GL_VERTEX_SHADER){
			std::cout << "ERROR: GL_VERTEX_SHADER compilation failed!\n" << errorMessages << "\n";
		} else if (type == GL_FRAGMENT_SHADER){
			std::cout << "ERROR: GL_FRAGMENT_SHADER compilation failed!\n" << errorMessages << "\n";
		} else if (type == GL_TESS_CONTROL_SHADER) {
            std::cout << "ERROR: GL_TESS_CONTROL_SHADER compilation failed!\n" << errorMessages << "\n";
        } else if (type == GL_TESS_EVALUATION_SHADER) {
            std::cout << "ERROR: GL_TESS_EVALUATION_SHADER compilation failed!\n" << errorMessages << "\n";
        }
		// Reclaim our memory
		delete[] errorMessages;

		// Delete our broken shader
		glDeleteShader(shaderObject);

		return 0;
	}

  return shaderObject;
}



/**
* Creates a graphics program object (i.e. graphics pipeline) with a Vertex, Fragment Shader, TCS and TES Shaders
*
* @param vertexShaderSource Vertex source code as a string
* @param fragmentShaderSource Fragment shader source code as a string
* @param tessControlShaderSource Tessellation control shader source code as a string
* @param tessEvalShaderSource Tessellation evaluation shader source code as a string
* @return id of the program Object
*/
GLuint CreateShaderProgramWithTessellation(const std::string& vertexShaderSource, const std::string& fragmentShaderSource,
                                           const std::string& tessControlShaderSource, const std::string& tessEvalShaderSource){

    // Create a new program object
    GLuint programObject = glCreateProgram();

    // Compile our shaders
    GLuint myVertexShader   = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint myFragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint myTessControlShader = CompileShader(GL_TESS_CONTROL_SHADER, tessControlShaderSource);
    GLuint myTessEvalShader    = CompileShader(GL_TESS_EVALUATION_SHADER, tessEvalShaderSource);

    // Link our two shader programs together.
	// Consider this the equivalent of taking two .cpp files, and linking them into
	// one executable file.
    glAttachShader(programObject,myVertexShader);
    glAttachShader(programObject,myFragmentShader);
    glAttachShader(programObject, myTessControlShader);
    glAttachShader(programObject, myTessEvalShader);
    glLinkProgram(programObject);

    // Validate our program
    glValidateProgram(programObject);

    // Once our final program Object has been created, we can
	// detach and then delete our individual shaders.
    glDetachShader(programObject,myVertexShader);
    glDetachShader(programObject,myFragmentShader);
    glDetachShader(programObject,myTessControlShader);
    glDetachShader(programObject,myTessEvalShader);
	// Delete the individual shaders once we are done
    glDeleteShader(myVertexShader);
    glDeleteShader(myFragmentShader);
    glDeleteShader(myTessControlShader);
    glDeleteShader(myTessEvalShader);

    return programObject;
}

/**
* Creates a graphics program object (i.e. graphics pipeline) with a Vertex Shader and a Fragment Shader
*
* @param vertexShaderSource Vertex source code as a string
* @param fragmentShaderSource Fragment shader source code as a string
* @return id of the program Object
*/
GLuint CreateShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource){

    // Create a new program object
    GLuint programObject = glCreateProgram();

    // Compile our shaders
    GLuint myVertexShader   = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint myFragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // Link our two shader programs together.
	// Consider this the equivalent of taking two .cpp files, and linking them into
	// one executable file.
    glAttachShader(programObject,myVertexShader);
    glAttachShader(programObject,myFragmentShader);
    glLinkProgram(programObject);

    // Validate our program
    glValidateProgram(programObject);

    // Once our final program Object has been created, we can
	// detach and then delete our individual shaders.
    glDetachShader(programObject,myVertexShader);
    glDetachShader(programObject,myFragmentShader);
	// Delete the individual shaders once we are done
    glDeleteShader(myVertexShader);
    glDeleteShader(myFragmentShader);

    return programObject;
}


/**
* Create the graphics pipeline
*
* @return void
*/
void CreateGraphicsPipeline(){

    std::string vertexShaderSource      = LoadShaderAsString("./shaders/vert.glsl");
    std::string fragmentShaderSource    = LoadShaderAsString("./shaders/frag.glsl");
    std::string tessControlShaderSource = LoadShaderAsString("./shaders/gerstner_tesc.glsl");
    std::string tessEvalShaderSource    = LoadShaderAsString("./shaders/gerstner_tese.glsl");

	gGraphicsPipelineShaderProgram = CreateShaderProgramWithTessellation(vertexShaderSource,fragmentShaderSource,
                                                                         tessControlShaderSource, tessEvalShaderSource);
    
    std::string skyboxVertexShaderSource      = LoadShaderAsString("./shaders/skybox_vert.glsl");
    std::string skyboxFragmentShaderSource    = LoadShaderAsString("./shaders/skybox_frag.glsl");

    gSkyboxPipelineShaderProgram = CreateShaderProgram(skyboxVertexShaderSource, skyboxFragmentShaderSource);
}


/**
* Initialization of the graphics application. Typically this will involve setting up a window
* and the OpenGL Context (with the appropriate version)
*
* @return void
*/
void InitializeProgram(){
	// Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO)< 0){
		std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}
	
	// Setup the OpenGL Context
	// Use OpenGL 4.1 core or greater
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	// We want to request a double buffer for smooth updating.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Create an application window using OpenGL that supports SDL
	gGraphicsApplicationWindow = SDL_CreateWindow( "Tesselation",
													SDL_WINDOWPOS_UNDEFINED,
													SDL_WINDOWPOS_UNDEFINED,
													gScreenWidth,
													gScreenHeight,
													SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );

	// Check if Window did not create.
	if( gGraphicsApplicationWindow == nullptr ){
		std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}

	// Create an OpenGL Graphics Context
	gOpenGLContext = SDL_GL_CreateContext( gGraphicsApplicationWindow );
	if( gOpenGLContext == nullptr){
		std::cout << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}

	// Initialize GLAD Library
	if(!gladLoadGLLoader(SDL_GL_GetProcAddress)){
		std::cout << "glad did not initialize" << std::endl;
		exit(1);
	}
	
}

void loadCubemap(std::vector<std::string> faces)
{
    glGenTextures(1, &gCubeTexId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gCubeTexId);

    int width, height;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        PPM skyboxPPM = PPM(faces[i].c_str());
        //skyboxPPM.flipPPM();
        std::vector<uint8_t> skyboxPixelData = skyboxPPM.pixelData();
        height = skyboxPPM.getHeight();
        width = skyboxPPM.getWidth();
        if (skyboxPixelData.size() != 0)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, skyboxPixelData.data()
            );
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

/**
* Setup your geometry during the vertex specification step
*
* @return void
*/
/**
* Setup your geometry during the vertex specification step
*
* @return void
*/
void VertexSpecification(){

	// Vertex Arrays Object (VAO) Setup
	glGenVertexArrays(1, &gVertexArrayObjectFloor);
	// We bind (i.e. select) to the Vertex Array Object (VAO) that we want to work withn.
	glBindVertexArray(gVertexArrayObjectFloor);
	// Vertex Buffer Object (VBO) creation
	glGenBuffers(1, &gVertexBufferObjectFloor);

    // Generate our data for the buffer
    //GeneratePlaneBufferData();
    std::vector<GLfloat> vertexDataQuad
    {
        -gOceanSize, 0.0, -gOceanSize,     // Bottom-left vertex of quad
        -1.0, -1.0,            // texture
        0.0, 1.0, 0.0,         // normal
        gOceanSize, 0.0, -gOceanSize,      // Bottom-right vertex
        1.0, -1.0,             // texture
        0.0, 1.0, 0.0,         // normal
        gOceanSize, 0.0, gOceanSize,       // Top-right vertex
        1.0, 1.0,              // texture
        0.0, 1.0, 0.0,         // normal
        -gOceanSize, 0.0, gOceanSize,      // Top-left vertex
        -1.0, 1.0,             // texture
        0.0, 1.0, 0.0          // normal
    };

    gFloorTriangles = vertexDataQuad.size();

    glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferObjectFloor);
	glBufferData(GL_ARRAY_BUFFER, // Kind of buffer we are working with  
                                  // (e.g. GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER)
							 vertexDataQuad.size() * sizeof(GL_FLOAT), 	// Size of data in bytes
							 vertexDataQuad.data(), 						// Raw array of data
							 GL_STATIC_DRAW);	
 
    // =============================
    // offsets every 3 floats
    // v     v     v
    // 
    // x,y,z,s,t,nx,ny,nz
    //
    // |------------------| strides is '8' floats
    //
    // ============================
    // Position information (x,y,z)
	glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,sizeof(GL_FLOAT)*8,(GLvoid*)0);
    // Color information (s,t)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,sizeof(GL_FLOAT)*8,(GLvoid*)(sizeof(GL_FLOAT)*3));
    // Normal information (nx,ny,nz)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,sizeof(GL_FLOAT)*8, (GLvoid*)(sizeof(GL_FLOAT)*5));

	// Unbind our currently bound Vertex Array Object
	glBindVertexArray(0);
	// Disable any attributes we opened in our Vertex Attribute Arrray,
	// as we do not want to leave them open. 
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

    // // Texture data setup
    // // Generate Texture object
    // glGenTextures(1, &gTexId);
    // // Bind it to GL_TEXTURE_2D
    // glBindTexture(GL_TEXTURE_2D, gTexId);
    // // Populate Texture data
    // std::string fileName = "./water.ppm";
    // PPM texturePPM = PPM(fileName);
    // //texturePPM.flipPPM();
    // std::vector<uint8_t> texturePixelData = texturePPM.pixelData();
    // int height = texturePPM.getHeight();
    // int width = texturePPM.getWidth();
    // glTexImage2D(
    //     GL_TEXTURE_2D,          // 2D Texture
    //     0,                      // Mipmap level 0(highest resolution)
    //     GL_RGB,                 // Internal format
    //     width,                  // Image width
    //     height,                 // Image height
    //     0,                      // Border(must be 0)
    //     GL_RGB,                 // Image format
    //     GL_UNSIGNED_BYTE,       // Data type of each pixel
    //     texturePixelData.data() // Raw texture pixel data
    // );
    // // Generate mipmaps for our texture
    // glGenerateMipmap(GL_TEXTURE_2D);
    // // Parameters for minification, magnification, wrap on s and t using bilinear filtering
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenVertexArrays(1, &gVertexArrayObjectSkybox);
	// We bind (i.e. select) to the Vertex Array Object (VAO) that we want to work withn.
	glBindVertexArray(gVertexArrayObjectSkybox);

    std::vector<GLfloat> vertexDataSkybox
    {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };

	// Vertex Buffer Object (VBO) creation
	glGenBuffers(1, &gVertexBufferObjectSkybox);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferObjectSkybox);
	glBufferData(GL_ARRAY_BUFFER, // Kind of buffer we are working with  
                                  // (e.g. GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER)
							 vertexDataSkybox.size() * sizeof(GL_FLOAT), 	// Size of data in bytes
							 vertexDataSkybox.data(), 						// Raw array of data
							 GL_STATIC_DRAW);

    // Position information (x,y,z)
	glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,sizeof(GL_FLOAT)*3,(GLvoid*)0);

	// Unbind our currently bound Vertex Array Object
	glBindVertexArray(0);
	// Disable any attributes we opened in our Vertex Attribute Arrray,
	// as we do not want to leave them open. 
	glDisableVertexAttribArray(0);

    std::vector<std::string> faces
    {
        "./right.ppm",
        "./left.ppm",
        "./top.ppm",
        "./bottom.ppm",
        "./front.ppm",
        "./back.ppm"
    };
    loadCubemap(faces);  
}

/**
* PreDraw
* Typically we will use this for setting some sort of 'state'
* Note: some of the calls may take place at different stages (post-processing) of the
* 		 pipeline.
* @return void
*/
void PreDraw(){
    glEnable(GL_DEPTH_TEST);

    // Set the polygon fill mode
    glPolygonMode(GL_FRONT_AND_BACK,gPolygonMode);

    // Initialize clear color
    // This is the background of the screen.
    glViewport(0, 0, gScreenWidth, gScreenHeight);
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );

    //Clear color buffer and Depth Buffer
  	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // Use our shader
	glUseProgram(gGraphicsPipelineShaderProgram);

    // Model transformation by translating our object into world space
    glm::mat4 model = glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,0.0f)); 

    glPatchParameteri(GL_PATCH_VERTICES, 4);

    // Retrieve our location of our Model Matrix
    GLint u_ModelMatrixLocation = glGetUniformLocation( gGraphicsPipelineShaderProgram,"u_ModelMatrix");
    if(u_ModelMatrixLocation >=0){
        glUniformMatrix4fv(u_ModelMatrixLocation,1,GL_FALSE,&model[0][0]);
    }else{
        std::cout << "Could not find u_ModelMatrix, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }


    // Update the View Matrix
    GLint u_ViewMatrixLocation = glGetUniformLocation(gGraphicsPipelineShaderProgram,"u_ViewMatrix");
    if(u_ViewMatrixLocation>=0){
        glm::mat4 viewMatrix = gCamera.GetViewMatrix();
        glUniformMatrix4fv(u_ViewMatrixLocation,1,GL_FALSE,&viewMatrix[0][0]);
    }else{
        std::cout << "Could not find u_ViewMatrix, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }


    // Projection matrix (in perspective) 
    glm::mat4 perspective = glm::perspective(glm::radians(45.0f),
                                             (float)gScreenWidth/(float)gScreenHeight,
                                             0.1f,
                                             2000.0f);

    // Retrieve our location of our perspective matrix uniform 
    GLint u_ProjectionLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"u_Projection");
    if(u_ProjectionLocation>=0){
        glUniformMatrix4fv(u_ProjectionLocation,1,GL_FALSE,&perspective[0][0]);
    }else{
        std::cout << "Could not find u_Projection, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    // Retrieve our location of our texture sampler uniform 
    // GLint u_TextureSamplerLocation = glGetUniformLocation( gGraphicsPipelineShaderProgram,"tex");
    // if(u_TextureSamplerLocation>=0){
    //     glUniform1i(u_TextureSamplerLocation,0);
    // }else{
    //     std::cout << "Could not find tex, maybe a mispelling?\n";
    //     exit(EXIT_FAILURE);
    // }

    GLint u_SkyTextureSamplerLocation = glGetUniformLocation( gGraphicsPipelineShaderProgram,"skybox");
    if(u_SkyTextureSamplerLocation>=0){
        glUniform1i(u_SkyTextureSamplerLocation,0);
    }else{
        std::cout << "Could not find skybox, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    glm::vec3 cameraPos = glm::vec3(gCamera.GetEyeXPosition() + gCamera.GetViewXDirection(),
                                  gCamera.GetEyeYPosition() + gCamera.GetViewYDirection(),
                                  gCamera.GetEyeZPosition() + gCamera.GetViewZDirection());

    GLint u_ViewPosition = glGetUniformLocation(gGraphicsPipelineShaderProgram,"cameraPos");
    if(u_ViewPosition >=0){
        glUniform3fv(u_ViewPosition,1,&cameraPos[0]);
    }else{
        std::cout << "Could not find cameraPos, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    } 
    

    GLint u_GerstnerWavesLengthLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"num_of_waves");
    if(u_GerstnerWavesLengthLocation>=0){
        glUniform1ui(u_GerstnerWavesLengthLocation,num_of_waves);
    }else{
        std::cout << "Could not find num_of_waves, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveTimeLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"time");
    if(u_GerstnerWaveTimeLocation>=0){
        glUniform1f(u_GerstnerWaveTimeLocation,static_cast<float>(SDL_GetTicks()) / 1000.0f);
    }else{
        std::cout << "Could not find time, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    // Wave 0 ---------------------------------------
    GLint u_GerstnerWaveDirectionLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[0].direction");
    if(u_GerstnerWaveDirectionLocation>=0){
        glUniform2f(u_GerstnerWaveDirectionLocation,glm::sin(0.32f), glm::cos(0.32f));
    }else{
        std::cout << "Could not find gerstner_waves[0].direction, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveAmplitudeLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[0].amplitude");
    if(u_GerstnerWaveAmplitudeLocation>=0){
        glUniform1f(u_GerstnerWaveAmplitudeLocation,1.64f);
    }else{
        std::cout << "Could not find gerstner_waves[0].amplitude, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSteepnessLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[0].steepness");
    if(u_GerstnerWaveSteepnessLocation>=0){
        glUniform1f(u_GerstnerWaveSteepnessLocation,1.64f);
    }else{
        std::cout << "Could not find gerstner_waves[0].steepness, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveFrequencyLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[0].frequency");
    if(u_GerstnerWaveFrequencyLocation>=0){
        glUniform1f(u_GerstnerWaveFrequencyLocation,3.0f);
    }else{
        std::cout << "Could not find gerstner_waves[0].frequency, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSpeedLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[0].speed");
    if(u_GerstnerWaveSpeedLocation>=0){
        glUniform1f(u_GerstnerWaveSpeedLocation,2.0f);
    }else{
        std::cout << "Could not find gerstner_waves[0].speed, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    // Wave 1 -----------------------------------
    GLint u_GerstnerWaveDirectionLocation1= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[1].direction");
    if(u_GerstnerWaveDirectionLocation1>=0){
        glUniform2f(u_GerstnerWaveDirectionLocation1,glm::sin(0.75f), glm::cos(0.25f));
    }else{
        std::cout << "Could not find gerstner_waves[1].direction, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveAmplitudeLocation1= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[1].amplitude");
    if(u_GerstnerWaveAmplitudeLocation1>=0){
        glUniform1f(u_GerstnerWaveAmplitudeLocation1,2.5f);
    }else{
        std::cout << "Could not find gerstner_waves[1].amplitude, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSteepnessLocation1= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[1].steepness");
    if(u_GerstnerWaveSteepnessLocation1>=0){
        glUniform1f(u_GerstnerWaveSteepnessLocation1,0.5f);
    }else{
        std::cout << "Could not find gerstner_waves[1].steepness, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveFrequencyLocation1= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[1].frequency");
    if(u_GerstnerWaveFrequencyLocation1>=0){
        glUniform1f(u_GerstnerWaveFrequencyLocation1,1.0f);
    }else{
        std::cout << "Could not find gerstner_waves[1].frequency, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSpeedLocation1= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[1].speed");
    if(u_GerstnerWaveSpeedLocation1>=0){
        glUniform1f(u_GerstnerWaveSpeedLocation1,0.3f);
    }else{
        std::cout << "Could not find gerstner_waves[1].speed, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    // Wave 2 -----------------------------------
    GLint u_GerstnerWaveDirectionLocation2= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[2].direction");
    if(u_GerstnerWaveDirectionLocation2>=0){
        glUniform2f(u_GerstnerWaveDirectionLocation2,glm::sin(1.0f), glm::cos(1.0f));
    }else{
        std::cout << "Could not find gerstner_waves[2].direction, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveAmplitudeLocation2= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[2].amplitude");
    if(u_GerstnerWaveAmplitudeLocation2>=0){
        glUniform1f(u_GerstnerWaveAmplitudeLocation2,1.25f);
    }else{
        std::cout << "Could not find gerstner_waves[2].amplitude, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSteepnessLocation2= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[2].steepness");
    if(u_GerstnerWaveSteepnessLocation2>=0){
        glUniform1f(u_GerstnerWaveSteepnessLocation2,1.3f);
    }else{
        std::cout << "Could not find gerstner_waves[2].steepness, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveFrequencyLocation2= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[2].frequency");
    if(u_GerstnerWaveFrequencyLocation2>=0){
        glUniform1f(u_GerstnerWaveFrequencyLocation2,4.0f);
    }else{
        std::cout << "Could not find gerstner_waves[2].frequency, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSpeedLocation2= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[2].speed");
    if(u_GerstnerWaveSpeedLocation2>=0){
        glUniform1f(u_GerstnerWaveSpeedLocation2,4.0f);
    }else{
        std::cout << "Could not find gerstner_waves[2].speed, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    // Wave 3 -----------------------------------
    GLint u_GerstnerWaveDirectionLocation3= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[3].direction");
    if(u_GerstnerWaveDirectionLocation3>=0){
        glUniform2f(u_GerstnerWaveDirectionLocation3,glm::sin(0.5f), glm::cos(0.5f));
    }else{
        std::cout << "Could not find gerstner_waves[3].direction, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveAmplitudeLocation3= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[3].amplitude");
    if(u_GerstnerWaveAmplitudeLocation3>=0){
        glUniform1f(u_GerstnerWaveAmplitudeLocation3,6.0f);
    }else{
        std::cout << "Could not find gerstner_waves[3].amplitude, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSteepnessLocation3= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[3].steepness");
    if(u_GerstnerWaveSteepnessLocation3>=0){
        glUniform1f(u_GerstnerWaveSteepnessLocation3,2.5f);
    }else{
        std::cout << "Could not find gerstner_waves[3].steepness, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveFrequencyLocation3= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[3].frequency");
    if(u_GerstnerWaveFrequencyLocation3>=0){
        glUniform1f(u_GerstnerWaveFrequencyLocation3,2.0f);
    }else{
        std::cout << "Could not find gerstner_waves[3].frequency, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    GLint u_GerstnerWaveSpeedLocation3= glGetUniformLocation( gGraphicsPipelineShaderProgram,"gerstner_waves[3].speed");
    if(u_GerstnerWaveSpeedLocation3>=0){
        glUniform1f(u_GerstnerWaveSpeedLocation3,1.0f);
    }else{
        std::cout << "Could not find gerstner_waves[3].speed, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    glUseProgram(gSkyboxPipelineShaderProgram);

    // Update the View Matrix
    GLint u_SkyboxViewMatrixLocation = glGetUniformLocation(gSkyboxPipelineShaderProgram,"view");
    if(u_SkyboxViewMatrixLocation>=0){
        glm::mat4 viewMatrix = glm::mat4(glm::mat3(gCamera.GetViewMatrix()));
        glUniformMatrix4fv(u_SkyboxViewMatrixLocation,1,GL_FALSE,&viewMatrix[0][0]);
    }else{
        std::cout << "Could not find view, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    // Retrieve our location of our perspective matrix uniform 
    GLint u_SkyboxProjectionLocation= glGetUniformLocation(gSkyboxPipelineShaderProgram,"projection");
    if(u_SkyboxProjectionLocation>=0){
        glUniformMatrix4fv(u_SkyboxProjectionLocation,1,GL_FALSE,&perspective[0][0]);
    }else{
        std::cout << "Could not find projection, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }

    // Retrieve our location of our texture sampler uniform 
    GLint u_SkyboxTextureSamplerLocation = glGetUniformLocation(gSkyboxPipelineShaderProgram,"skybox");
    if(u_SkyboxTextureSamplerLocation>=0){
        glUniform1i(u_SkyboxTextureSamplerLocation,0);
    }else{
        std::cout << "Could not find skybox, maybe a mispelling?\n";
        exit(EXIT_FAILURE);
    }
}


/**
* Draw
* The render function gets called once per loop.
* Typically this includes 'glDraw' related calls, and the relevant setup of buffers
* for those calls.
*
* @return void
*/
void Draw(){
    glUseProgram(gGraphicsPipelineShaderProgram);
    // Enable our attributes
	glBindVertexArray(gVertexArrayObjectFloor);

    // Set texture data
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, gTexId);

    // Set skybox texture map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gCubeTexId);

    //Render data
    glDrawArrays(GL_PATCHES,0,gFloorTriangles);

    // draw skybox as last
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    glUseProgram(gSkyboxPipelineShaderProgram);
    // skybox cube
    glBindVertexArray(gVertexArrayObjectSkybox);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gCubeTexId);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default

	// Stop using our current graphics pipeline
	// Note: This is not necessary if we only have one graphics pipeline.
    glUseProgram(0);
}

/**
* Helper Function to get OpenGL Version Information
*
* @return void
*/
void getOpenGLVersionInfo(){
  std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
  std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
  std::cout << "Version: " << glGetString(GL_VERSION) << "\n";
  std::cout << "Shading language: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
}


/**
* Function called in the Main application loop to handle user input
*
* @return void
*/
void Input(){
    // Two static variables to hold the mouse position
    static int mouseX=gScreenWidth/2;
    static int mouseY=gScreenHeight/2; 

	// Event handler that handles various events in SDL
	// that are related to input and output
	SDL_Event e;
	//Handle events on queue
	while(SDL_PollEvent( &e ) != 0){
		// If users posts an event to quit
		// An example is hitting the "x" in the corner of the window.
		if(e.type == SDL_QUIT){
			std::cout << "Goodbye! (Leaving MainApplicationLoop())" << std::endl;
			gQuit = true;
		}
        if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE){
			std::cout << "ESC: Goodbye! (Leaving MainApplicationLoop())" << std::endl;
            gQuit = true;
        }
        if(e.type==SDL_MOUSEMOTION){
            // Capture the change in the mouse position
            mouseX+=e.motion.xrel;
            mouseY+=e.motion.yrel;
            gCamera.MouseLook(mouseX,mouseY);
        }
	}

    // Retrieve keyboard state
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    // if (state[SDL_SCANCODE_UP]) {
    //     SDL_Delay(250);
    //     gOceanSize += 10.0f;
    //     VertexSpecification();
    // }
    // if (state[SDL_SCANCODE_DOWN]) {
    //     SDL_Delay(250); 
    //     gOceanSize += 10.0f;
    //     VertexSpecification();
    // }

    // Camera
    // Update our position of the camera
    if (state[SDL_SCANCODE_W]) {
        gCamera.MoveForward(0.1f);
    }
    if (state[SDL_SCANCODE_S]) {
        gCamera.MoveBackward(0.1f);
    }
    if (state[SDL_SCANCODE_A]) {
        gCamera.MoveLeft(0.1f);
    }
    if (state[SDL_SCANCODE_D]) {
        gCamera.MoveRight(0.1f);
    }
    if (state[SDL_SCANCODE_1]) {
        num_of_waves = 1;
    }
    if (state[SDL_SCANCODE_2]) {
        num_of_waves = 2;
    }
    if (state[SDL_SCANCODE_3]) {
        num_of_waves = 3;
    }
    if (state[SDL_SCANCODE_4]) {
        num_of_waves = 4;
    }

    if (state[SDL_SCANCODE_TAB]) {
        SDL_Delay(250); // This is hacky in the name of simplicity,
                       // but we just delay the
                       // system by a few milli-seconds to process the 
                       // keyboard input once at a time.
        if(gPolygonMode== GL_FILL){
            gPolygonMode = GL_LINE;
        }else{
            gPolygonMode = GL_FILL;
        }
    }
}


/**
* Main Application Loop
* This is an infinite loop in our graphics application
*
* @return void
*/
void MainLoop(){

    // Little trick to map mouse to center of screen always.
    // Useful for handling 'mouselook'
    // This works because we effectively 're-center' our mouse at the start
    // of every frame prior to detecting any mouse motion.
    SDL_WarpMouseInWindow(gGraphicsApplicationWindow,gScreenWidth/2,gScreenHeight/2);
    SDL_SetRelativeMouseMode(SDL_TRUE);


	// While application is running
	while(!gQuit){
		// Handle Input
		Input();
		// Setup anything (i.e. OpenGL State) that needs to take
		// place before draw calls
		PreDraw();
		// Draw Calls in OpenGL
        // When we 'draw' in OpenGL, this activates the graphics pipeline.
        // i.e. when we use glDrawElements or glDrawArrays,
        //      The pipeline that is utilized is whatever 'glUseProgram' is
        //      currently binded.
		Draw();

		//Update screen of our specified window
		SDL_GL_SwapWindow(gGraphicsApplicationWindow);
	}
}



/**
* The last function called in the program
* This functions responsibility is to destroy any global
* objects in which we have create dmemory.
*
* @return void
*/
void CleanUp(){
	//Destroy our SDL2 Window
	SDL_DestroyWindow(gGraphicsApplicationWindow );
	gGraphicsApplicationWindow = nullptr;

    // Delete our OpenGL Objects
    glDeleteBuffers(1, &gVertexBufferObjectFloor);
    glDeleteVertexArrays(1, &gVertexArrayObjectFloor);
    glDeleteBuffers(1, &gVertexBufferObjectSkybox);
    glDeleteVertexArrays(1, &gVertexArrayObjectSkybox);

	// Delete our Graphics pipeline
    glDeleteProgram(gGraphicsPipelineShaderProgram);
    glDeleteProgram(gSkyboxPipelineShaderProgram);

	//Quit SDL subsystems
	SDL_Quit();
}


/**
* The entry point into our C++ programs.
*
* @return program status
*/
int main( int argc, char* args[] ){
    std::cout << "Use w and s keys to move forward and back\n";
    std::cout << "Use up and down to change tessellation\n";
    std::cout << "Use tab to toggle wireframe\n";
    std::cout << "Press ESC to quit\n";

	// 1. Setup the graphics program
	InitializeProgram();
	
	// 2. Setup our geometry
	VertexSpecification();
	
	// 3. Create our graphics pipeline
	// 	- At a minimum, this means the vertex and fragment shader
	CreateGraphicsPipeline();
	
	// 4. Call the main application loop
	MainLoop();	

	// 5. Call the cleanup function when our program terminates
	CleanUp();

	return 0;
}
