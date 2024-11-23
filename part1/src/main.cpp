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

// vvvvvvvvvvvvvvvvvvvvvvvvvv Globals vvvvvvvvvvvvvvvvvvvvvvvvvv
// Globals generally are prefixed with 'g' in this application.

// Screen Dimensions
int gScreenWidth 						= 640;
int gScreenHeight 						= 480;
SDL_Window* gGraphicsApplicationWindow 	= nullptr;
SDL_GLContext gOpenGLContext			= nullptr;

// Main loop flag
bool gQuit = false; // If this is quit = 'true' then the program terminates.

// shader
// The following stores the a unique id for the graphics pipeline
// program object that will be used for our OpenGL draw calls.
GLuint gGraphicsPipelineShaderProgram	= 0;

// OpenGL Objects
// Vertex Array Object (VAO)
// Vertex array objects encapsulate all of the items needed to render an object.
// For example, we may have multiple vertex buffer objects (VBO) related to rendering one
// object. The VAO allows us to setup the OpenGL state to render that object using the
// correct layout and correct buffers with one call after being setup.
GLuint gVertexArrayObjectFloor= 0;
// Vertex Buffer Object (VBO)
// Vertex Buffer Objects store information relating to vertices (e.g. positions, normals, textures)
// VBOs are our mechanism for arranging geometry on the GPU.
GLuint  gVertexBufferObjectFloor            = 0;

// Camera
Camera gCamera;

// Draw wireframe mode
GLenum gPolygonMode = GL_LINE;

// Floor resolution
size_t gFloorResolution = 10;
size_t gFloorTriangles  = 0;

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
* Creates a graphics program object (i.e. graphics pipeline) with a Vertex Shader and a Fragment Shader
*
* @param vertexShaderSource Vertex source code as a string
* @param fragmentShaderSource Fragment shader source code as a string
* @return id of the program Object
*/
GLuint CreateShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource,
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
* Create the graphics pipeline
*
* @return void
*/
void CreateGraphicsPipeline(){

    std::string vertexShaderSource      = LoadShaderAsString("./shaders/vert.glsl");
    std::string fragmentShaderSource    = LoadShaderAsString("./shaders/frag.glsl");
    std::string tessControlShaderSource = LoadShaderAsString("./shaders/gerstner.tesc");
    std::string tessEvalShaderSource    = LoadShaderAsString("./shaders/gerstner.tese");

	gGraphicsPipelineShaderProgram = CreateShaderProgram(vertexShaderSource,fragmentShaderSource,
                                                         tessControlShaderSource, tessEvalShaderSource);
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


struct Vertex{
    float x,y,z;    // position
	float r,g,b; 	// color
	float nx,ny,nz; // normals
};  

struct Triangle{
    Vertex vertices[3]; // 3 vertices per triangle
};



// Return a value that is a mapping between the current range and a new range.
// Source: https://www.arduino.cc/reference/en/language/functions/math/map/
float map_linear(float x, float in_min, float in_max, float out_min, float out_max){
    return (x-in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Pass in an unsigned integer representing the number of
// rows and columns in the plane (e.g. resolution=00)
// The plane is 'flat' so the 'y' position will be 0.0f;
std::vector<Triangle> generatePlane(size_t resolution=0){
    
    // Generate a grid of points
    std::vector<Vertex> gridPoints;
    for (int z = 0; z < gFloorResolution; z++) {
        for (int x = 0; x < gFloorResolution; x++) {
            Vertex point;

            // Colors for the point
            point.g = 6.0f;
            point.r = point.b = 0.0f;

            // Interpolated x and z values in the range -3.0f to 3.0f
            point.x = map_linear(x, 0, gFloorResolution, -3.0f, 3.0f);
            point.y = -1.0f;
            point.z = -map_linear(z, 0, gFloorResolution, -3.0f, 3.0f);

            // Surface normals for the point
            point.nx = 0.0f;
            point.ny = 1.0f;
            point.nz = 0.0f;

            // Push this point on the grid
            gridPoints.push_back(point);
        }
    }

    // Store the resulting plane 
    std::vector<Triangle> result;
    for (int z = 0; z < gFloorResolution - 1; z++) {
        for (int x = 0; x < gFloorResolution - 1; x++) {

            // Get vertex index of a quad consisting of four points
            int idx1 = z * gFloorResolution + x;
            int idx2 = idx1 + 1;
            int idx3 = idx1 + gFloorResolution;
            int idx4 = idx1 + gFloorResolution + 1;

            // Make two triangles that represent a quad using upper
            // and lower halves of it.
            Triangle upperHalf, lowerHalf;

            lowerHalf.vertices[0] = gridPoints[idx1];
            lowerHalf.vertices[1] = gridPoints[idx2];
            lowerHalf.vertices[2] = gridPoints[idx3];

            upperHalf.vertices[0] = gridPoints[idx2];
            upperHalf.vertices[1] = gridPoints[idx4];
            upperHalf.vertices[2] = gridPoints[idx3];

            // Push generated triangles to a vector
            result.push_back(lowerHalf);
            result.push_back(upperHalf);
        }
    }

    return result;
}




// Regenerate the flat plane
void GeneratePlaneBufferData(){
    // Generate a plane with the resolution 
    std::vector<Triangle> mesh = generatePlane(gFloorResolution); 

		std::vector<GLfloat> vertexDataFloor;
        for (Triangle t : mesh) {
            for (Vertex v : t.vertices) {
                vertexDataFloor.push_back(v.x);
                vertexDataFloor.push_back(v.y);
                vertexDataFloor.push_back(v.z);
                vertexDataFloor.push_back(v.r);
                vertexDataFloor.push_back(v.g);
                vertexDataFloor.push_back(v.b);
                vertexDataFloor.push_back(v.nx);
                vertexDataFloor.push_back(v.ny);
                vertexDataFloor.push_back(v.nz);
            }
        }

		// Store size in a global so you can later determine how many
		// vertices to draw in glDrawArrays;
		// TODO: You need to verify to yourself if this 'size' represents the number of 'vertices' or not -- think about it.
    gFloorTriangles = vertexDataFloor.size();

		glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferObjectFloor);
		glBufferData(GL_ARRAY_BUFFER, 						// Kind of buffer we are working with 
																							// (e.g. GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER)
							 vertexDataFloor.size() * sizeof(GL_FLOAT), 	// Size of data in bytes
							 vertexDataFloor.data(), 											// Raw array of data
							 GL_STATIC_DRAW);															// How we intend to use the data
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
        -1.0, 0.0, -1.0,     // Bottom-left vertex of quad
        0.0, 0.0, 1.0,       // color
        0.0, 1.0, 0.0,       // normal
        1.0, 0.0, -1.0,      // Bottom-right vertex
        0.0, 0.0, 1.0,       // color
        0.0, 1.0, 0.0,       // normal
        1.0, 0.0, 1.0,       // Top-right vertex
        0.0, 0.0, 1.0,       // color
        0.0, 1.0, 0.0,        // normal
        -1.0, 0.0, 1.0,      // Top-left vertex
        0.0, 0.0, 1.0,        // color
        0.0, 1.0, 0.0        // normal
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
    // x,y,z,r,g,b,nx,ny,nz
    //
    // |------------------| strides is '9' floats
    //
    // ============================
    // Position information (x,y,z)
	glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,sizeof(GL_FLOAT)*9,(void*)0);
    // Color information (r,g,b)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,sizeof(GL_FLOAT)*9,(GLvoid*)(sizeof(GL_FLOAT)*3));
    // Normal information (nx,ny,nz)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,sizeof(GL_FLOAT)*9, (GLvoid*)(sizeof(GL_FLOAT)*6));

	// Unbind our currently bound Vertex Array Object
	glBindVertexArray(0);
	// Disable any attributes we opened in our Vertex Attribute Arrray,
	// as we do not want to leave them open. 
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}


/**
* PreDraw
* Typically we will use this for setting some sort of 'state'
* Note: some of the calls may take place at different stages (post-processing) of the
* 		 pipeline.
* @return void
*/
void PreDraw(){
	// Disable depth test and face culling.
    glEnable(GL_DEPTH_TEST);                    // NOTE: Need to enable DEPTH Test
    glDisable(GL_CULL_FACE);

    // Set the polygon fill mode
    glPolygonMode(GL_FRONT_AND_BACK,gPolygonMode);

    // Initialize clear color
    // This is the background of the screen.
    glViewport(0, 0, gScreenWidth, gScreenHeight);
    glClearColor( 0.0f, 0.0f, 0.0f, 1.f );

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
                                             20.0f);

    // Retrieve our location of our perspective matrix uniform 
    GLint u_ProjectionLocation= glGetUniformLocation( gGraphicsPipelineShaderProgram,"u_Projection");
    if(u_ProjectionLocation>=0){
        glUniformMatrix4fv(u_ProjectionLocation,1,GL_FALSE,&perspective[0][0]);
    }else{
        std::cout << "Could not find u_Perspective, maybe a mispelling?\n";
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
    // Enable our attributes
	glBindVertexArray(gVertexArrayObjectFloor);

    //Render data
    glDrawArrays(GL_PATCHES,0,gFloorTriangles);

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
    //     gFloorResolution+=1;
    //     std::cout << "Resolution:" << gFloorResolution << std::endl;
    //     GeneratePlaneBufferData();
    // }
    // if (state[SDL_SCANCODE_DOWN]) {
    //     SDL_Delay(250); 
    //     gFloorResolution-=1;
    //     if(gFloorResolution<=1){
    //         gFloorResolution=1;
    //     }
    //     std::cout << "Resolution:" << gFloorResolution << std::endl;
    //     GeneratePlaneBufferData();
    // }

    // Camera
    // Update our position of the camera
    if (state[SDL_SCANCODE_W]) {
        gCamera.MoveForward(0.01f);
    }
    if (state[SDL_SCANCODE_S]) {
        gCamera.MoveBackward(0.01f);
    }
    if (state[SDL_SCANCODE_A]) {
        gCamera.MoveLeft(0.01f);
    }
    if (state[SDL_SCANCODE_D]) {
        gCamera.MoveRight(0.01f);
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

	// Delete our Graphics pipeline
    glDeleteProgram(gGraphicsPipelineShaderProgram);

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
