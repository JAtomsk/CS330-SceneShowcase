/*code referenced from SNHU CS330 Computer graphics*/

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library



// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.h"
#include "stb_image.h"





using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Jared_Semonin 3D Scene Showcase"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	const double M_PI = 3.14159265358979323846f;
	const double M_PI_2 = 1.571428571428571;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
		GLuint nVertices;
	};

	// scene vector for drawing shapes
	vector<GLMesh> scene;
	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Triangle mesh data
	GLMesh gMesh;
	GLMesh gPlaneMesh;
	GLMesh gDroidHead;
	GLMesh gHardDriveMesh;
	GLuint gDeskTexture;
	GLuint gDroidTexture;
	GLuint gDroidHeadTexture;
	GLuint gHardDriveTexture;

	// Shader program
	GLuint gProgramId;

	bool perspective = false;

	// Camera
	Camera gCamera(glm::vec3(0.0f, 0.0f, 7.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
// window size
void UResizeWindow(GLFWwindow* window, int width, int height);

void UProcessInput(GLFWwindow* window);
//keyboard and mouse input functions
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMousebuttonCallback(GLFWwindow* window, int button, int action, int mods);

// Creates the BB8 body
void UCreateMesh(GLMesh& mesh);
// Creates the Plane
void UCreatePlaneMesh(GLMesh& mesh);
// Creates the BB8 Head
void UDroidHead(GLMesh& mesh);
// Creates the hard drive
void UHardDriveMesh(GLMesh& mesh);
//destroys shaders to free memory
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
//renders the graphics
void URender();
//creates the shader program
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);



/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(460,
	layout(location = 0) in vec3 position;
layout(location = 2) in vec2 textureCoordinate;

out vec2 vertexTextureCoordinate;

//varaible of 4x4 matrix to transfrom vertex data
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
	vertexTextureCoordinate = textureCoordinate;
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(460,
	in vec2 vertexTextureCoordinate;

out vec4 fragmentColor;

uniform sampler2D uTexture;

void main()
{
	fragmentColor = texture(uTexture, vertexTextureCoordinate); // Sends texture to the GPU for rendering
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}


int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
	UCreatePlaneMesh(gPlaneMesh);
	UDroidHead(gMesh);
	UHardDriveMesh(gHardDriveMesh);


	// Create the shader program
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
		return EXIT_FAILURE;

	// Load texture
	const char* texFilename = "mydesk.png";
	if (!UCreateTexture(texFilename, gDeskTexture))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	texFilename = "droidsphere.png";
	if (!UCreateTexture(texFilename, gDroidTexture))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	texFilename = "bb8head.png";
	if (!UCreateTexture(texFilename, gDroidHeadTexture))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	texFilename = "hardrive1.png";
	if (!UCreateTexture(texFilename, gHardDriveTexture))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
	glUniform1i(glGetUniformLocation(gProgramId, "uTextureExtra"), 1);
	glUniform1i(glGetUniformLocation(gProgramId, "uDroidHeadTexture"), 2);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{

		// per-frame timing
		// -------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;
		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	UDestroyMesh(gMesh);


	// Release shader program
	UDestroyShaderProgram(gProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMousebuttonCallback);

	// GLEW: initialize

	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
	static const float cameraSpeed = 2.5f;
	//exits the program
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraOffset = cameraSpeed * gDeltaTime;

	// movement commands
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);
	// ortho and perspective view commands
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		perspective = false;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		perspective = true;

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// Function deffinition for the UMousePosition Function
// When ever the mouse moves this callback is called
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos;

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
	cout << "Mouse Cursor is at (" << xpos << "," << ypos << ")" << endl;
}

// This callback is called everytime the mouse wheel is scrolled
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}

void UMousebuttonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}

// Functioned called to render a frame
void URender()
{
	const int nrows = 10;
	const int ncols = 10;
	const int nlevels = 10;

	const float xsize = 10.0f;
	const float ysize = 10.0f;
	const float zsize = 10.0f;

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// camera/view transformation (x,y,z)
	glm::mat4 view = gCamera.GetViewMatrix();

	glm::mat4 projection;
	if (!perspective)
	{
		// changes to perspective view when p key is selected
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else
		// changes to orthographic view when p key is selected
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);

	// Set the shader to be used
	glUseProgram(gProgramId);
	// 1. Scales the object by 1
	glm::mat4 scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	// 2. Rotates shape by 15 degrees in the x axis
	glm::mat4 rotation = glm::rotate(100.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	// 3. Place object at the origin                   
	glm::mat4 translation = glm::translate(glm::vec3(0.25f, 0.25f, 2.0f));
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 model = translation * rotation * scale;

	// Retrieves and passes transform matrices to the Shader program
	GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Activate the VBOs contained within the mesh's VAO
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gDroidTexture);
	// draws BB8 body with texture
	glBindVertexArray(gMesh.vao);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// scales plane
	glm::mat4 pscale = glm::scale(glm::vec3(5.0f, 5.0f, 10.0f));
	// 2. Rotates shape by 15 degrees in the x axis
	glm::mat4 protation = glm::rotate(89.5f, glm::vec3(0.0f, 1.0f, 0.0f));
	// 3. Place object at the origin
	glm::mat4 ptranslation = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 pmodel = ptranslation * protation * pscale;

	// Retrieves and passes transform matrices to the Shader program
	GLint pmodelLoc = glGetUniformLocation(gProgramId, "model");
	GLint pviewLoc = glGetUniformLocation(gProgramId, "view");
	GLint pprojLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(pmodelLoc, 1, GL_FALSE, glm::value_ptr(pmodel));
	glUniformMatrix4fv(pviewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(pprojLoc, 1, GL_FALSE, glm::value_ptr(projection));
	

	glActiveTexture(GL_TEXTURE0);
	// draws plane with texture

	glBindTexture(GL_TEXTURE_2D, gDeskTexture);
	glBindVertexArray(gPlaneMesh.vao);
	glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_INT, (void*)0);


	// droid head draw
	
	glm::mat4 bscale = glm::scale(glm::vec3(-0.550f, -0.450f, -0.550f));
	// 2. Rotates shape by 15 degrees in the x axis
	glm::mat4 brotation = glm::rotate(3.15f, glm::vec3(0.25f, 0.0f, 1.0f));
	glm::mat4 btranslation = glm::translate(glm::vec3(0.25f, 1.25f, 2.0f));
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 bmodel = btranslation * brotation * bscale;

	// Retrieves and passes transform matrices to the Shader program
	GLint bmodelLoc = glGetUniformLocation(gProgramId, "model");
	GLint bviewLoc = glGetUniformLocation(gProgramId, "view");
	GLint bprojLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(bmodelLoc, 1, GL_FALSE, glm::value_ptr(bmodel));
	glUniformMatrix4fv(bviewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(bprojLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gDroidHeadTexture);
	glBindVertexArray(gMesh.vao);
	glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	


	// Hard drive draw
	glm::mat4 hscale = glm::scale(glm::vec3(1.75f, 3.0f, 0.25f));
	// 2. Rotates shape by 15 degrees in the x axis
	glm::mat4 hrotation = glm::rotate(50.5f, glm::vec3(0.0, 1.0f, 0.0f));
	glm::mat4 htranslation = glm::translate(glm::vec3(1.75f, 0.55f, 0.05f));
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 hmodel = htranslation * hrotation * hscale;

	// Retrieves and passes transform matrices to the Shader program
	GLint hmodelLoc = glGetUniformLocation(gProgramId, "model");
	GLint hviewLoc = glGetUniformLocation(gProgramId, "view");
	GLint hprojLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(hmodelLoc, 1, GL_FALSE, glm::value_ptr(hmodel));
	glUniformMatrix4fv(hviewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(hprojLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gHardDriveTexture);
	glBindVertexArray(gHardDriveMesh.vao);
	glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_INT, (void*)0);


	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}





// Implements the UCreateMesh function
void UDroidHead(GLMesh& mesh)
{
	// Vertex data
	GLfloat verts[] = {
		// vertex data					// index
		// top center point
		0.0f, 1.0f, 0.0f,				//0
		// ring 1
		0.0f, 0.9808f, 0.1951f,			//1
		0.0747f, 0.9808f, 0.1802f,		//2
		0.1379f, 0.9808f, 0.1379f,		//3
		0.1802f, 0.9808f, 0.0747f,		//4
		0.1951f, 0.9808, 0.0f,			//5
		0.1802f, 0.9808f, -0.0747f,		//6
		0.1379f, 0.9808f, -0.1379f,		//7
		0.0747f, 0.9808f, -0.1802f,		//8
		0.0f, 0.9808f, -0.1951f,		//9
		-0.0747f, 0.9808f, -0.1802f,	//10
		-0.1379f, 0.9808f, -0.1379f,	//11
		-0.1802f, 0.9808f, -0.0747f,	//12
		-0.1951f, 0.9808, 0.0f,			//13
		-0.1802f, 0.9808f, 0.0747f,		//14
		-0.1379f, 0.9808f, 0.1379f,		//15
		-0.0747f, 0.9808f, 0.1802f,		//16
		// ring 2
		0.0f, 0.9239f, 0.3827f,			//17
		0.1464f, 0.9239f, 0.3536f,		//18
		0.2706f, 0.9239f, 0.2706f,		//19
		0.3536f, 0.9239f, 0.1464f,		//20
		0.3827f, 0.9239f, 0.0f,			//21
		0.3536f, 0.9239f, -0.1464f,		//22
		0.2706f, 0.9239f, -0.2706f,		//23
		0.1464f, 0.9239f, -0.3536f,		//24
		0.0f, 0.9239f, -0.3827f,		//25
		-0.1464f, 0.9239f, -0.3536f,	//26
		-0.2706f, 0.9239f, -0.2706f,	//27
		-0.3536f, 0.9239f, -0.1464f,	//28
		-0.3827f, 0.9239f, 0.0f,		//29
		-0.3536f, 0.9239f, 0.1464f,		//30
		-0.2706f, 0.9239f, 0.2706f,		//31
		-0.1464f, 0.9239f, 0.3536f,		//32
		// ring 3
		0.0f, 0.8315f, 0.5556f,			//33
		0.2126f, 0.8315f, 0.5133f,		//34
		0.3928f, 0.8315f, 0.3928f,		//35
		0.5133f, 0.8315f, 0.2126f,		//36
		0.5556f, 0.8315f, 0.0f,			//37
		0.5133f, 0.8315f, -0.2126f,		//38
		0.3928f, 0.8315f, -0.3928f,		//39
		0.2126f, 0.8315f, -0.5133f,		//40
		0.0f, 0.8315f, -0.5556f,		//41
		-0.2126f, 0.8315f, -0.5133f,	//42
		-0.3928f, 0.8315f, -0.3928f,	//43
		-0.5133f, 0.8315f, -0.2126f,	//44
		-0.5556f, 0.8315f, 0.0f,		//45
		-0.5133f, 0.8315f, 0.2126f,		//46
		-0.3928f, 0.8315f, 0.3928f,		//47
		-0.2126f, 0.8315f, 0.5133f,		//48
		// ring 4
		0.0f, 0.7071f, 0.7071f,			//49
		0.2706f, 0.7071f, 0.6533f,		//50
		0.5f, 0.7071f, 0.5f,			//51
		0.6533f, 0.7071f, 0.2706f,		//52
		0.7071f, 0.7071f, 0.0f,			//53
		0.6533f, 0.7071f, -0.2706f,		//54
		0.5f, 0.7071f, -0.5f,			//55
		0.2706f, 0.7071f, -0.6533f,		//56
		0.0f, 0.7071f, -0.7071f,		//57
		-0.2706f, 0.7071f, -0.6533f,	//58
		-0.5f, 0.7071f, -0.5f,			//59
		-0.6533f, 0.7071f, -0.2706f,	//60
		-0.7071f, 0.7071f, 0.0f,		//61
		-0.6533f, 0.7071f, 0.2706f,		//62
		-0.5f, 0.7071f, 0.5f,			//63
		-0.2706f, 0.7071f, 0.6533f,		//64
		// ring 5
		0.0f, 0.5556f, 0.8315f,			//65
		0.3182f, 0.5556f, 0.7682f,		//66
		0.5879f, 0.5556f, 0.5879f,		//67
		0.7682f, 0.5556f, 0.3182f,		//68
		0.8315f, 0.5556f, 0.0f,			//69
		0.7682f, 0.5556f, -0.3182f,		//70
		0.5879f, 0.5556f, -0.5879f,		//71
		0.3182f, 0.5556f, -0.7682f,		//72
		0.0f, 0.5556f, -0.8315f,		//73
		-0.3182f, 0.5556f, -0.7682f,	//74
		-0.5879f, 0.5556f, -0.5879f,	//75
		-0.7682f, 0.5556f, -0.3182f,	//76
		-0.8315f, 0.5556f, 0.0f,		//77
		-0.7682f, 0.5556f, 0.3182f,		//78
		-0.5879f, 0.5556f, 0.5879f,		//79
		-0.3182f, 0.5556f, 0.7682f,		//80
		//ring 6
		0.0f, 0.3827f, 0.9239f,			//81
		0.3536f, 0.3827f, 0.8536f,		//82
		0.6533f, 0.3827f, 0.6533f,		//83
		0.8536f, 0.3827f, 0.3536f,		//84
		0.9239f, 0.3827f, 0.0f,			//85
		0.8536f, 0.3827f, -0.3536f,		//86
		0.6533f, 0.3827f, -0.6533f,		//87
		0.3536f, 0.3827f, -0.8536f,		//88
		0.0f, 0.3827f, -0.9239f,		//89
		-0.3536f, 0.3827f, -0.8536f,	//90
		-0.6533f, 0.3827f, -0.6533f,	//91
		-0.8536f, 0.3827f, -0.3536f,	//92
		-0.9239f, 0.3827f, 0.0f,		//93
		-0.8536f, 0.3827f, 0.3536f,		//94
		-0.6533f, 0.3827f, 0.6533f,		//95
		-0.3536f, 0.3827f, 0.8536f,		//96
		// ring 7
		0.0f, 0.1951f, 0.9808f,			//97
		0.3753f, 0.1915f, 0.9061f,		//98
		0.6935f, 0.1915f, 0.6935f,		//99
		0.9061f, 0.1915f, 0.3753f,		//100
		0.9808f, 0.1915f, 0.0f,			//101
		0.9061f, 0.1915f, -0.3753f,		//102
		0.6935f, 0.1915f, -0.6935f,		//103
		0.3753f, 0.1915f, -0.9061f,		//104
		0.0f, 0.1915f, -0.9808f,		//105
		-0.3753f, 0.1915f, -0.9061f,	//106
		-0.6935f, 0.1915f, -0.6935f,	//107
		-0.9061f, 0.1915f, -0.3753f,	//108
		-0.9808f, 0.1915f, 0.0f,		//109
		-0.9061f, 0.1915f, 0.3753f,		//110
		-0.6935f, 0.1915f, 0.6935f,		//111
		-0.3753f, 0.1915f, 0.9061f,		//112
		// ring 8
		0.0f, 0.0f, 1.0f,				//113
		0.3827f, 0.0f, 0.9239f,			//114
		0.7071f, 0.0f, 0.7071f,			//115
		0.9239f, 0.0f, 0.3827f,			//116
		1.0f, 0.0f, 0.0f,				//117
		0.9239f, 0.0f, -0.3827f,		//118
		0.7071f, 0.0f, -0.7071f,		//119
		0.3827f, 0.0f, -0.9239f,		//120
		0.0f, 0.0f, -1.0f,				//121
		-0.3827f, 0.0f, -0.9239f,		//122
		-0.7071f, 0.0f, -0.7071f,		//123
		-0.9239f, 0.0f, -0.3827f,		//124
		-1.0f, 0.0f, 0.0f,				//125
		-0.9239f, 0.0f, 0.3827f,		//126
		-0.7071, 0.0, 0.7071f,			//127
		-0.3827f, 0.0f, 0.9239f,		//128
		// ring 9
		0.0f, -0.1915f, 0.9808f,		//129
		0.3753f, -0.1915f, 0.9061f,		//130
		0.6935f, -0.1915f, 0.6935f,		//131
		0.9061f, -0.1915f, 0.3753f,		//132
		0.9808f, -0.1915f, 0.0f,		//133
		0.9061f, -0.1915f, -0.3753f,	//134
		0.6935f, -0.1915f, -0.6935f,	//135
		0.3753f, -0.1915f, -0.9061f,	//136
		0.0f, -0.1915f, -0.9808f,		//137
		-0.3753f, -0.1915f, -0.9061f,	//138
		-0.6935f, -0.1915f, -0.6935f,	//139
		-0.9061f, -0.1915f, -0.3753f,	//140
		-0.9808f, -0.1915f, 0.0f,		//141
		-0.9061f, -0.1915f, 0.3753f,	//142
		-0.6935f, -0.1915f, 0.6935f,	//143
		-0.3753f, -0.1915f, 0.9061f,	//144
		// ring 10
		0.0f, -0.3827f, 0.9239f,		//145
		0.3536f, -0.3827f, 0.8536f,		//146
		0.6533f, -0.3827f, 0.6533f,		//147
		0.8536f, -0.3827f, 0.3536f,		//148
		0.9239f, -0.3827f, 0.0f,		//149
		0.8536f, -0.3827f, -0.3536f,	//150
		0.6533f, -0.3827f, -0.6533f,	//151
		0.3536f, -0.3827f, -0.8536f,	//152
		0.0f, -0.3827f, -0.9239f,		//153
		-0.3536f, -0.3827f, -0.8536f,	//154
		-0.6533f, -0.3827f, -0.6533f,	//155
		-0.8536f, -0.3827f, -0.3536f,	//156
		-0.9239f, -0.3827f, 0.0f,		//157
		-0.8536f, -0.3827f, 0.3536f,	//158
		-0.6533f, -0.3827f, 0.6533f,	//159
		-0.3536f, -0.3827f, 0.8536f,	//160
		// ring 11
		0.0f, -0.5556f, 0.8315f,		//161
		0.3182f, -0.5556f, 0.7682f,		//162
		0.5879f, -0.5556f, 0.5879f,		//163
		0.7682f, -0.5556f, 0.3182f,		//164
		0.8315f, -0.5556f, 0.0f,		//165
		0.7682f, -0.5556f, -0.3182f,	//166
		0.5879f, -0.5556f, -0.5879f,	//167
		0.3182f, -0.5556f, -0.7682f,	//168
		0.0f, -0.5556f, -0.8315f,		//169
		-0.3182f, -0.5556f, -0.7682f,	//170
		-0.5879f, 0.5556f, -0.5879f,	//171
		-0.7682f, -0.5556f, -0.3182f,	//172
		-0.8315f, -0.5556f, 0.0f,		//173
		-0.7682f, -0.5556f, 0.3182f,	//174
		-0.5879f, -0.5556f, 0.5879f,	//175
		-0.3182f, -0.5556f, 0.7682f,	//176
		// ring 12
		0.0f, -0.7071f, 0.7071f,		//177
		0.2706f, -0.7071f, 0.6533f,		//178
		0.5f, -0.7071f, 0.5f,			//179
		0.6533f, -0.7071f, 0.2706f,		//180
		0.7071f, -0.7071f, 0.0f,		//181
		0.6533f, -0.7071f, -0.2706f,	//182
		0.5f, -0.7071f, -0.5f,			//183
		0.2706f, -0.7071f, -0.6533f,	//184
		0.0f, -0.7071f, -0.7071f,		//185
		-0.2706f, -0.7071f, -0.6533f,	//186
		-0.5f, -0.7071f, -0.5f,			//187
		-0.6533f, -0.7071f, -0.2706f,	//188
		-0.7071f, -0.7071f, 0.0f,		//189
		-0.6533f, -0.7071f, 0.2706f,	//190
		-0.5f, -0.7071f, 0.5f,			//191
		-0.2706f, -0.7071f, 0.6533f,	//192
		// ring 13
		0.0f, -0.8315f, 0.5556f,		//193
		0.2126f, -0.8315f, 0.5133f,		//194
		0.3928f, -0.8315f, 0.3928f,		//195
		0.5133f, -0.8315f, 0.2126f,		//196
		0.5556f, -0.8315f, 0.0f,		//197
		0.5133f, -0.8315f, -0.2126f,	//198
		0.3928f, -0.8315f, -0.3928f,	//199
		0.2126f, -0.8315f, -0.5133f,	//200
		0.0f, -0.8315f, -0.5556f,		//201
		-0.2126f, -0.8315f, -0.5133f,	//202
		-0.3928f, -0.8315f, -0.3928f,	//203
		-0.5133f, -0.8315f, -0.2126f,	//204
		-0.5556f, -0.8315f, 0.0f,		//205
		-0.5133f, -0.8315f, 0.2126f,	//206
		-0.3928f, -0.8315f, 0.3928f,	//207
		-0.2126f, -0.8315f, 0.5133f,	//208
		// ring 14
		0.0f, -0.9239f, 0.3827f,		//209
		0.1464f, -0.9239f, 0.3536f,		//210
		0.2706f, -0.9239f, 0.2706f,		//211
		0.3536f, -0.9239f, 0.1464f,		//212
		0.3827f, -0.9239f, 0.0f,		//213
		0.3536f, -0.9239f, -0.1464f,	//214
		0.2706f, -0.9239f, -0.2706f,	//215
		0.1464f, -0.9239f, -0.3536f,	//216
		0.0f, -0.9239f, -0.3827f,		//217
		-0.1464f, -0.9239f, -0.3536f,	//218
		-0.2706f, -0.9239f, -0.2706f,	//219
		-0.3536f, -0.9239f, -0.1464f,	//220
		-0.3827f, -0.9239f, 0.0f,		//221
		-0.3536f, -0.9239f, 0.1464f,	//222
		-0.2706f, -0.9239f, 0.2706f,	//223
		-0.1464f, -0.9239f, 0.3536f,	//224
		// ring 15
		0.0f, -0.9808f, 0.1951f,		//225
		0.0747f, -0.9808f, 0.1802f,		//226
		0.1379f, -0.9808f, 0.1379f,		//227
		0.1802f, -0.9808f, 0.0747f,		//228
		0.1951f, -0.9808, 0.0f,			//229
		0.1802f, -0.9808f, -0.0747f,	//230
		0.1379f, -0.9808f, -0.1379f,	//231
		0.0747f, -0.9808f, -0.1802f,	//232
		0.0f, -0.9808f, -0.1951f,		//233
		-0.0747f, -0.9808f, -0.1802f,	//234
		-0.1379f, -0.9808f, -0.1379f,	//235
		-0.1802f, -0.9808f, -0.0747f,	//236
		-0.1951f, -0.9808, 0.0f,		//237
		-0.1802f, -0.9808f, 0.0747f,	//238
		-0.1379f, -0.9808f, 0.1379f,	//239
		-0.0747f, -0.9808f, 0.1802f,	//240
		// bottom center point
		0.0f, -1.0f, 0.0f,				//241
	};

	// index data
	GLuint indices[] = {
		//ring 1 - top
		0,1,2,
		0,2,3,
		0,3,4,
		0,4,5,
		0,5,6,
		0,6,7,
		0,7,8,
		0,8,9,
		0,9,10,
		0,10,11,
		0,11,12,
		0,12,13,
		0,13,14,
		0,14,15,
		0,15,16,
		0,16,1,

		// ring 1 to ring 2
		1,17,18,
		1,2,18,
		2,18,19,
		2,3,19,
		3,19,20,
		3,4,20,
		4,20,21,
		4,5,21,
		5,21,22,
		5,6,22,
		6,22,23,
		6,7,23,
		7,23,24,
		7,8,24,
		8,24,25,
		8,9,25,
		9,25,26,
		9,10,26,
		10,26,27,
		10,11,27,
		11,27,28,
		11,12,28,
		12,28,29,
		12,13,29,
		13,29,30,
		13,14,30,
		14,30,31,
		14,15,31,
		15,31,32,
		15,16,32,
		16,32,17,
		16,1,17,

		// ring 2 to ring 3
		17,33,34,
		17,18,34,
		18,34,35,
		18,19,35,
		19,35,36,
		19,20,36,
		20,36,37,
		20,21,37,
		21,37,38,
		21,22,38,
		22,38,39,
		22,23,39,
		23,39,40,
		23,24,40,
		24,40,41,
		24,25,41,
		25,41,42,
		25,26,42,
		26,42,43,
		26,27,43,
		27,43,44,
		27,28,44,
		28,44,45,
		28,29,45,
		29,45,46,
		29,30,46,
		30,46,47,
		30,31,47,
		31,47,48,
		31,32,48,
		32,48,33,
		32,17,33,

		// ring 3 to ring 4
		33,49,50,
		33,34,50,
		34,50,51,
		34,35,51,
		35,51,52,
		35,36,52,
		36,52,53,
		36,37,53,
		37,53,54,
		37,38,54,
		38,54,55,
		38,39,55,
		39,55,56,
		39,40,56,
		40,56,57,
		40,41,57,
		41,57,58,
		41,42,58,
		42,58,59,
		42,43,59,
		43,59,60,
		43,44,60,
		44,60,61,
		44,45,61,
		45,61,62,
		45,46,62,
		46,62,63,
		46,47,63,
		47,63,64,
		47,48,64,
		48,64,49,
		48,33,49,

		// ring 4 to ring 5
		49,65,66,
		49,50,66,
		50,66,67,
		50,51,67,
		51,67,68,
		51,52,68,
		52,68,69,
		52,53,69,
		53,69,70,
		53,54,70,
		54,70,71,
		54,55,71,
		55,71,72,
		55,56,72,
		56,72,73,
		56,57,73,
		57,73,74,
		57,58,74,
		58,74,75,
		58,59,75,
		59,75,76,
		59,60,76,
		60,76,77,
		60,61,77,
		61,77,78,
		61,62,78,
		62,78,79,
		62,63,79,
		63,79,80,
		63,64,80,
		64,80,65,
		64,49,65,

		// ring 5 to ring 6
		65,81,82,
		65,66,82,
		66,82,83,
		66,67,83,
		67,83,84,
		67,68,84,
		68,84,85,
		68,69,85,
		69,85,86,
		69,70,86,
		70,86,87,
		70,71,87,
		71,87,88,
		71,72,88,
		72,88,89,
		72,73,89,
		73,89,90,
		73,74,90,
		74,90,91,
		74,75,91,
		75,91,92,
		75,76,92,
		76,92,93,
		76,77,93,
		77,93,94,
		77,78,94,
		78,94,95,
		78,79,95,
		79,95,96,
		79,80,96,
		80,96,81,
		80,65,81,

		// ring 6 to ring 7
		81,97,98,
		81,82,98,
		82,98,99,
		82,83,99,
		83,99,100,
		83,84,100,
		84,100,101,
		84,85,101,
		85,101,102,
		85,86,102,
		86,102,103,
		86,87,103,
		87,103,104,
		87,88,104,
		88,104,105,
		88,89,105,
		89,105,106,
		89,90,106,
		90,106,107,
		90,91,107,
		91,107,108,
		91,92,108,
		92,108,109,
		92,93,109,
		93,109,110,
		93,94,110,
		94,110,111,
		94,95,111,
		95,111,112,
		95,96,112,
		96,112,97,
		96,81,97,

		// ring 7 to ring 8
		97,113,114,
		97,98,114,
		98,114,115,
		98,99,115,
		99,115,116,
		99,100,116,
		100,116,117,
		100,101,117,
		101,117,118,
		101,102,118,
		102,118,119,
		102,103,119,
		103,119,120,
		103,104,120,
		104,120,121,
		104,105,121,
		105,121,122,
		105,106,122,
		106,122,123,
		106,107,123,
		107,123,124,
		107,108,124,
		108,124,125,
		108,109,125,
		109,125,126,
		109,110,126,
		110,126,127,
		110,111,127,
		111,127,128,
		111,112,128,
		112,128,113,
		112,97,113,

		// ring 8 to ring 9
		113,129,130,
		113,114,130,
		114,130,131,
		114,115,131,
		115,131,132,
		115,116,132,
		116,132,133,
		116,117,133,
		117,133,134,
		117,118,134,
		118,134,135,
		118,119,135,
		119,135,136,
		119,120,136,
		120,136,137,
		120,121,137,
		121,137,138,
		121,122,138,
		122,138,139,
		122,123,139,
		123,139,140,
		123,124,140,
		124,140,141,
		124,125,141,
		125,141,142,
		125,126,142,
		126,142,143,
		126,127,143,
		127,143,144,
		127,128,144,
		128,144,129,
		128,113,129,

		// ring 9 to ring 10
		129,145,146,
		129,130,146,
		130,146,147,
		130,131,147,
		131,147,148,
		131,132,148,
		132,148,149,
		132,133,149,
		133,149,150,
		133,134,150,
		134,150,151,
		134,135,151,
		135,151,152,
		135,136,152,
		136,152,153,
		136,137,153,
		137,153,154,
		137,138,154,
		138,154,155,
		138,139,155,
		139,155,156,
		139,140,156,
		140,156,157,
		140,141,157,
		141,157,158,
		141,142,158,
		142,158,159,
		142,143,159,
		143,159,160,
		143,144,160,
		144,160,145,
		144,129,145,

		// ring 10 to ring 11
		145,161,162,
		145,146,162,
		146,162,163,
		146,147,163,
		147,163,164,
		147,148,164,
		148,164,165,
		148,149,165,
		149,165,166,
		149,150,166,
		150,166,167,
		150,151,167,
		151,167,168,
		151,152,168,
		152,168,169,
		152,153,169,
		153,169,170,
		153,154,170,
		154,170,171,
		154,155,171,
		155,171,172,
		155,156,172,
		156,172,173,
		156,157,173,
		157,173,174,
		157,158,174,
		158,174,175,
		158,159,175,
		159,175,176,
		159,160,176,
		160,176,161,
		160,145,161,

		// ring 11 to ring 12
		161,177,178,
		161,162,178,
		162,178,179,
		162,163,179,
		163,179,180,
		163,164,180,
		164,180,181,
		164,165,181,
		165,181,182,
		165,166,182,
		166,182,183,
		166,167,183,
		167,183,184,
		167,168,184,
		168,184,185,
		168,169,185,
		169,185,186,
		169,170,186,
		170,186,187,
		170,171,187,
		171,187,188,
		171,172,188,
		172,188,189,
		172,173,189,
		173,189,190,
		173,174,190,
		174,190,191,
		174,175,191,
		175,191,192,
		175,176,192,
		176,192,177,
		176, 161,177,

		// ring 12 to ring 13
		177,193,194,
		177,178,194,
		178,194,195,
		178,179,195,
		179,195,196,
		179,180,196,
		180,196,197,
		180,181,197,
		181,197,198,
		181,182,198,
		182,198,199,
		182,183,199,
		183,199,200,
		183,184,200,
		184,200,201,
		184,185,201,
		185,201,202,
		185,186,202,
		186,202,203,
		186,187,203,
		187,203,204,
		187,188,204,
		188,204,205,
		188,189,205,
		189,205,206,
		189,190,206,
		190,206,207,
		190,191,207,
		191,207,208,
		191,192,208,
		192,208,193,
		192,177,193,

		// ring 13 to ring 14
		193,209,210,
		193,194,210,
		194,210,211,
		194,195,211,
		195,211,212,
		195,196,212,
		196,212,213,
		196,197,213,
		197,213,214,
		197,198,214,
		198,214,215,
		198,199,215,
		199,215,216,
		199,200,216,
		200,216,217,
		200,201,217,
		201,217,218,
		201,202,218,
		202,218,219,
		202,203,219,
		203,219,220,
		203,204,220,
		204,220,221,
		204,205,221,
		205,221,222,
		205,206,222,
		206,222,223,
		206,207,223,
		207,223,224,
		207,208,224,
		208,224,209,
		208,193,209,

		// ring 14 to ring 15
		209,225,226,
		209,210,226,
		210,226,227,
		210,211,227,
		211,227,228,
		211,212,228,
		212,228,229,
		212,213,229,
		213,229,230,
		213,214,230,
		214,230,231,
		214,215,231,
		215,231,232,
		215,216,232,
		216,232,233,
		216,217,233,
		217,233,234,
		217,218,234,
		218,234,235,
		218,219,235,
		219,235,236,
		219,220,236,
		220,236,237,
		220,221,237,
		221,237,238,
		221,222,238,
		222,238,239,
		222,223,239,
		223,239,240,
		223,224,240,
		224,240,225,
		224,209,225,

		// ring 15 - bottom
		225,226,241,
		226,227,241,
		227,228,241,
		228,229,241,
		229,239,241,
		230,231,241,
		231,232,241,
		232,233,241,
		233,234,241,
		234,235,241,
		235,236,241,
		236,237,241,
		237,238,241,
		238,239,241,
		239,240,241,
		240,225,241
	};

	// total float values per each type
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	// store vertex and index count
	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex));
	mesh.nIndices = sizeof(indices) / (sizeof(indices[0]));

	glm::vec3 normal;
	glm::vec3 vert;
	glm::vec3 center(0.0f, 0.0f, 0.0f);
	float u, v;
	std::vector<GLfloat> combined_values;

	// combine interleaved vertices, normals, and texture coords
	for (int i = 0; i < sizeof(verts) / (sizeof(verts[0])); i += 3)
	{
		vert = glm::vec3(verts[i], verts[i + 1], verts[i + 2]);
		normal = normalize(vert - center);
		u = atan2(normal.x, normal.z) / (2 * M_PI) + 0.5;
		v = normal.y * 0.5 + 0.5;
		combined_values.push_back(vert.x);
		combined_values.push_back(vert.y);
		combined_values.push_back(vert.z);
		combined_values.push_back(normal.x);
		combined_values.push_back(normal.y);
		combined_values.push_back(normal.z);
		combined_values.push_back(u);
		combined_values.push_back(v);
	}

	// Create VAO
	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBOs
	glGenBuffers(2, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the vertex buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * combined_values.size(), combined_values.data(), GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the index buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void UCreateMesh(GLMesh& mesh)
{
	// Vertex data
	GLfloat verts[] = {
		// vertex data					// index
		// top center point
		0.0f, 1.0f, 0.0f,				//0
		// ring 1
		0.0f, 0.9808f, 0.1951f,			//1
		0.0747f, 0.9808f, 0.1802f,		//2
		0.1379f, 0.9808f, 0.1379f,		//3
		0.1802f, 0.9808f, 0.0747f,		//4
		0.1951f, 0.9808, 0.0f,			//5
		0.1802f, 0.9808f, -0.0747f,		//6
		0.1379f, 0.9808f, -0.1379f,		//7
		0.0747f, 0.9808f, -0.1802f,		//8
		0.0f, 0.9808f, -0.1951f,		//9
		-0.0747f, 0.9808f, -0.1802f,	//10
		-0.1379f, 0.9808f, -0.1379f,	//11
		-0.1802f, 0.9808f, -0.0747f,	//12
		-0.1951f, 0.9808, 0.0f,			//13
		-0.1802f, 0.9808f, 0.0747f,		//14
		-0.1379f, 0.9808f, 0.1379f,		//15
		-0.0747f, 0.9808f, 0.1802f,		//16
		// ring 2
		0.0f, 0.9239f, 0.3827f,			//17
		0.1464f, 0.9239f, 0.3536f,		//18
		0.2706f, 0.9239f, 0.2706f,		//19
		0.3536f, 0.9239f, 0.1464f,		//20
		0.3827f, 0.9239f, 0.0f,			//21
		0.3536f, 0.9239f, -0.1464f,		//22
		0.2706f, 0.9239f, -0.2706f,		//23
		0.1464f, 0.9239f, -0.3536f,		//24
		0.0f, 0.9239f, -0.3827f,		//25
		-0.1464f, 0.9239f, -0.3536f,	//26
		-0.2706f, 0.9239f, -0.2706f,	//27
		-0.3536f, 0.9239f, -0.1464f,	//28
		-0.3827f, 0.9239f, 0.0f,		//29
		-0.3536f, 0.9239f, 0.1464f,		//30
		-0.2706f, 0.9239f, 0.2706f,		//31
		-0.1464f, 0.9239f, 0.3536f,		//32
		// ring 3
		0.0f, 0.8315f, 0.5556f,			//33
		0.2126f, 0.8315f, 0.5133f,		//34
		0.3928f, 0.8315f, 0.3928f,		//35
		0.5133f, 0.8315f, 0.2126f,		//36
		0.5556f, 0.8315f, 0.0f,			//37
		0.5133f, 0.8315f, -0.2126f,		//38
		0.3928f, 0.8315f, -0.3928f,		//39
		0.2126f, 0.8315f, -0.5133f,		//40
		0.0f, 0.8315f, -0.5556f,		//41
		-0.2126f, 0.8315f, -0.5133f,	//42
		-0.3928f, 0.8315f, -0.3928f,	//43
		-0.5133f, 0.8315f, -0.2126f,	//44
		-0.5556f, 0.8315f, 0.0f,		//45
		-0.5133f, 0.8315f, 0.2126f,		//46
		-0.3928f, 0.8315f, 0.3928f,		//47
		-0.2126f, 0.8315f, 0.5133f,		//48
		// ring 4
		0.0f, 0.7071f, 0.7071f,			//49
		0.2706f, 0.7071f, 0.6533f,		//50
		0.5f, 0.7071f, 0.5f,			//51
		0.6533f, 0.7071f, 0.2706f,		//52
		0.7071f, 0.7071f, 0.0f,			//53
		0.6533f, 0.7071f, -0.2706f,		//54
		0.5f, 0.7071f, -0.5f,			//55
		0.2706f, 0.7071f, -0.6533f,		//56
		0.0f, 0.7071f, -0.7071f,		//57
		-0.2706f, 0.7071f, -0.6533f,	//58
		-0.5f, 0.7071f, -0.5f,			//59
		-0.6533f, 0.7071f, -0.2706f,	//60
		-0.7071f, 0.7071f, 0.0f,		//61
		-0.6533f, 0.7071f, 0.2706f,		//62
		-0.5f, 0.7071f, 0.5f,			//63
		-0.2706f, 0.7071f, 0.6533f,		//64
		// ring 5
		0.0f, 0.5556f, 0.8315f,			//65
		0.3182f, 0.5556f, 0.7682f,		//66
		0.5879f, 0.5556f, 0.5879f,		//67
		0.7682f, 0.5556f, 0.3182f,		//68
		0.8315f, 0.5556f, 0.0f,			//69
		0.7682f, 0.5556f, -0.3182f,		//70
		0.5879f, 0.5556f, -0.5879f,		//71
		0.3182f, 0.5556f, -0.7682f,		//72
		0.0f, 0.5556f, -0.8315f,		//73
		-0.3182f, 0.5556f, -0.7682f,	//74
		-0.5879f, 0.5556f, -0.5879f,	//75
		-0.7682f, 0.5556f, -0.3182f,	//76
		-0.8315f, 0.5556f, 0.0f,		//77
		-0.7682f, 0.5556f, 0.3182f,		//78
		-0.5879f, 0.5556f, 0.5879f,		//79
		-0.3182f, 0.5556f, 0.7682f,		//80
		//ring 6
		0.0f, 0.3827f, 0.9239f,			//81
		0.3536f, 0.3827f, 0.8536f,		//82
		0.6533f, 0.3827f, 0.6533f,		//83
		0.8536f, 0.3827f, 0.3536f,		//84
		0.9239f, 0.3827f, 0.0f,			//85
		0.8536f, 0.3827f, -0.3536f,		//86
		0.6533f, 0.3827f, -0.6533f,		//87
		0.3536f, 0.3827f, -0.8536f,		//88
		0.0f, 0.3827f, -0.9239f,		//89
		-0.3536f, 0.3827f, -0.8536f,	//90
		-0.6533f, 0.3827f, -0.6533f,	//91
		-0.8536f, 0.3827f, -0.3536f,	//92
		-0.9239f, 0.3827f, 0.0f,		//93
		-0.8536f, 0.3827f, 0.3536f,		//94
		-0.6533f, 0.3827f, 0.6533f,		//95
		-0.3536f, 0.3827f, 0.8536f,		//96
		// ring 7
		0.0f, 0.1951f, 0.9808f,			//97
		0.3753f, 0.1915f, 0.9061f,		//98
		0.6935f, 0.1915f, 0.6935f,		//99
		0.9061f, 0.1915f, 0.3753f,		//100
		0.9808f, 0.1915f, 0.0f,			//101
		0.9061f, 0.1915f, -0.3753f,		//102
		0.6935f, 0.1915f, -0.6935f,		//103
		0.3753f, 0.1915f, -0.9061f,		//104
		0.0f, 0.1915f, -0.9808f,		//105
		-0.3753f, 0.1915f, -0.9061f,	//106
		-0.6935f, 0.1915f, -0.6935f,	//107
		-0.9061f, 0.1915f, -0.3753f,	//108
		-0.9808f, 0.1915f, 0.0f,		//109
		-0.9061f, 0.1915f, 0.3753f,		//110
		-0.6935f, 0.1915f, 0.6935f,		//111
		-0.3753f, 0.1915f, 0.9061f,		//112
		// ring 8
		0.0f, 0.0f, 1.0f,				//113
		0.3827f, 0.0f, 0.9239f,			//114
		0.7071f, 0.0f, 0.7071f,			//115
		0.9239f, 0.0f, 0.3827f,			//116
		1.0f, 0.0f, 0.0f,				//117
		0.9239f, 0.0f, -0.3827f,		//118
		0.7071f, 0.0f, -0.7071f,		//119
		0.3827f, 0.0f, -0.9239f,		//120
		0.0f, 0.0f, -1.0f,				//121
		-0.3827f, 0.0f, -0.9239f,		//122
		-0.7071f, 0.0f, -0.7071f,		//123
		-0.9239f, 0.0f, -0.3827f,		//124
		-1.0f, 0.0f, 0.0f,				//125
		-0.9239f, 0.0f, 0.3827f,		//126
		-0.7071, 0.0, 0.7071f,			//127
		-0.3827f, 0.0f, 0.9239f,		//128
		// ring 9
		0.0f, -0.1915f, 0.9808f,		//129
		0.3753f, -0.1915f, 0.9061f,		//130
		0.6935f, -0.1915f, 0.6935f,		//131
		0.9061f, -0.1915f, 0.3753f,		//132
		0.9808f, -0.1915f, 0.0f,		//133
		0.9061f, -0.1915f, -0.3753f,	//134
		0.6935f, -0.1915f, -0.6935f,	//135
		0.3753f, -0.1915f, -0.9061f,	//136
		0.0f, -0.1915f, -0.9808f,		//137
		-0.3753f, -0.1915f, -0.9061f,	//138
		-0.6935f, -0.1915f, -0.6935f,	//139
		-0.9061f, -0.1915f, -0.3753f,	//140
		-0.9808f, -0.1915f, 0.0f,		//141
		-0.9061f, -0.1915f, 0.3753f,	//142
		-0.6935f, -0.1915f, 0.6935f,	//143
		-0.3753f, -0.1915f, 0.9061f,	//144
		// ring 10
		0.0f, -0.3827f, 0.9239f,		//145
		0.3536f, -0.3827f, 0.8536f,		//146
		0.6533f, -0.3827f, 0.6533f,		//147
		0.8536f, -0.3827f, 0.3536f,		//148
		0.9239f, -0.3827f, 0.0f,		//149
		0.8536f, -0.3827f, -0.3536f,	//150
		0.6533f, -0.3827f, -0.6533f,	//151
		0.3536f, -0.3827f, -0.8536f,	//152
		0.0f, -0.3827f, -0.9239f,		//153
		-0.3536f, -0.3827f, -0.8536f,	//154
		-0.6533f, -0.3827f, -0.6533f,	//155
		-0.8536f, -0.3827f, -0.3536f,	//156
		-0.9239f, -0.3827f, 0.0f,		//157
		-0.8536f, -0.3827f, 0.3536f,	//158
		-0.6533f, -0.3827f, 0.6533f,	//159
		-0.3536f, -0.3827f, 0.8536f,	//160
		// ring 11
		0.0f, -0.5556f, 0.8315f,		//161
		0.3182f, -0.5556f, 0.7682f,		//162
		0.5879f, -0.5556f, 0.5879f,		//163
		0.7682f, -0.5556f, 0.3182f,		//164
		0.8315f, -0.5556f, 0.0f,		//165
		0.7682f, -0.5556f, -0.3182f,	//166
		0.5879f, -0.5556f, -0.5879f,	//167
		0.3182f, -0.5556f, -0.7682f,	//168
		0.0f, -0.5556f, -0.8315f,		//169
		-0.3182f, -0.5556f, -0.7682f,	//170
		-0.5879f, 0.5556f, -0.5879f,	//171
		-0.7682f, -0.5556f, -0.3182f,	//172
		-0.8315f, -0.5556f, 0.0f,		//173
		-0.7682f, -0.5556f, 0.3182f,	//174
		-0.5879f, -0.5556f, 0.5879f,	//175
		-0.3182f, -0.5556f, 0.7682f,	//176
		// ring 12
		0.0f, -0.7071f, 0.7071f,		//177
		0.2706f, -0.7071f, 0.6533f,		//178
		0.5f, -0.7071f, 0.5f,			//179
		0.6533f, -0.7071f, 0.2706f,		//180
		0.7071f, -0.7071f, 0.0f,		//181
		0.6533f, -0.7071f, -0.2706f,	//182
		0.5f, -0.7071f, -0.5f,			//183
		0.2706f, -0.7071f, -0.6533f,	//184
		0.0f, -0.7071f, -0.7071f,		//185
		-0.2706f, -0.7071f, -0.6533f,	//186
		-0.5f, -0.7071f, -0.5f,			//187
		-0.6533f, -0.7071f, -0.2706f,	//188
		-0.7071f, -0.7071f, 0.0f,		//189
		-0.6533f, -0.7071f, 0.2706f,	//190
		-0.5f, -0.7071f, 0.5f,			//191
		-0.2706f, -0.7071f, 0.6533f,	//192
		// ring 13
		0.0f, -0.8315f, 0.5556f,		//193
		0.2126f, -0.8315f, 0.5133f,		//194
		0.3928f, -0.8315f, 0.3928f,		//195
		0.5133f, -0.8315f, 0.2126f,		//196
		0.5556f, -0.8315f, 0.0f,		//197
		0.5133f, -0.8315f, -0.2126f,	//198
		0.3928f, -0.8315f, -0.3928f,	//199
		0.2126f, -0.8315f, -0.5133f,	//200
		0.0f, -0.8315f, -0.5556f,		//201
		-0.2126f, -0.8315f, -0.5133f,	//202
		-0.3928f, -0.8315f, -0.3928f,	//203
		-0.5133f, -0.8315f, -0.2126f,	//204
		-0.5556f, -0.8315f, 0.0f,		//205
		-0.5133f, -0.8315f, 0.2126f,	//206
		-0.3928f, -0.8315f, 0.3928f,	//207
		-0.2126f, -0.8315f, 0.5133f,	//208
		// ring 14
		0.0f, -0.9239f, 0.3827f,		//209
		0.1464f, -0.9239f, 0.3536f,		//210
		0.2706f, -0.9239f, 0.2706f,		//211
		0.3536f, -0.9239f, 0.1464f,		//212
		0.3827f, -0.9239f, 0.0f,		//213
		0.3536f, -0.9239f, -0.1464f,	//214
		0.2706f, -0.9239f, -0.2706f,	//215
		0.1464f, -0.9239f, -0.3536f,	//216
		0.0f, -0.9239f, -0.3827f,		//217
		-0.1464f, -0.9239f, -0.3536f,	//218
		-0.2706f, -0.9239f, -0.2706f,	//219
		-0.3536f, -0.9239f, -0.1464f,	//220
		-0.3827f, -0.9239f, 0.0f,		//221
		-0.3536f, -0.9239f, 0.1464f,	//222
		-0.2706f, -0.9239f, 0.2706f,	//223
		-0.1464f, -0.9239f, 0.3536f,	//224
		// ring 15
		0.0f, -0.9808f, 0.1951f,		//225
		0.0747f, -0.9808f, 0.1802f,		//226
		0.1379f, -0.9808f, 0.1379f,		//227
		0.1802f, -0.9808f, 0.0747f,		//228
		0.1951f, -0.9808, 0.0f,			//229
		0.1802f, -0.9808f, -0.0747f,	//230
		0.1379f, -0.9808f, -0.1379f,	//231
		0.0747f, -0.9808f, -0.1802f,	//232
		0.0f, -0.9808f, -0.1951f,		//233
		-0.0747f, -0.9808f, -0.1802f,	//234
		-0.1379f, -0.9808f, -0.1379f,	//235
		-0.1802f, -0.9808f, -0.0747f,	//236
		-0.1951f, -0.9808, 0.0f,		//237
		-0.1802f, -0.9808f, 0.0747f,	//238
		-0.1379f, -0.9808f, 0.1379f,	//239
		-0.0747f, -0.9808f, 0.1802f,	//240
		// bottom center point
		0.0f, -1.0f, 0.0f,				//241
	};

	// index data
	GLuint indices[] = {
		//ring 1 - top
		0,1,2,
		0,2,3,
		0,3,4,
		0,4,5,
		0,5,6,
		0,6,7,
		0,7,8,
		0,8,9,
		0,9,10,
		0,10,11,
		0,11,12,
		0,12,13,
		0,13,14,
		0,14,15,
		0,15,16,
		0,16,1,

		// ring 1 to ring 2
		1,17,18,
		1,2,18,
		2,18,19,
		2,3,19,
		3,19,20,
		3,4,20,
		4,20,21,
		4,5,21,
		5,21,22,
		5,6,22,
		6,22,23,
		6,7,23,
		7,23,24,
		7,8,24,
		8,24,25,
		8,9,25,
		9,25,26,
		9,10,26,
		10,26,27,
		10,11,27,
		11,27,28,
		11,12,28,
		12,28,29,
		12,13,29,
		13,29,30,
		13,14,30,
		14,30,31,
		14,15,31,
		15,31,32,
		15,16,32,
		16,32,17,
		16,1,17,

		// ring 2 to ring 3
		17,33,34,
		17,18,34,
		18,34,35,
		18,19,35,
		19,35,36,
		19,20,36,
		20,36,37,
		20,21,37,
		21,37,38,
		21,22,38,
		22,38,39,
		22,23,39,
		23,39,40,
		23,24,40,
		24,40,41,
		24,25,41,
		25,41,42,
		25,26,42,
		26,42,43,
		26,27,43,
		27,43,44,
		27,28,44,
		28,44,45,
		28,29,45,
		29,45,46,
		29,30,46,
		30,46,47,
		30,31,47,
		31,47,48,
		31,32,48,
		32,48,33,
		32,17,33,

		// ring 3 to ring 4
		33,49,50,
		33,34,50,
		34,50,51,
		34,35,51,
		35,51,52,
		35,36,52,
		36,52,53,
		36,37,53,
		37,53,54,
		37,38,54,
		38,54,55,
		38,39,55,
		39,55,56,
		39,40,56,
		40,56,57,
		40,41,57,
		41,57,58,
		41,42,58,
		42,58,59,
		42,43,59,
		43,59,60,
		43,44,60,
		44,60,61,
		44,45,61,
		45,61,62,
		45,46,62,
		46,62,63,
		46,47,63,
		47,63,64,
		47,48,64,
		48,64,49,
		48,33,49,

		// ring 4 to ring 5
		49,65,66,
		49,50,66,
		50,66,67,
		50,51,67,
		51,67,68,
		51,52,68,
		52,68,69,
		52,53,69,
		53,69,70,
		53,54,70,
		54,70,71,
		54,55,71,
		55,71,72,
		55,56,72,
		56,72,73,
		56,57,73,
		57,73,74,
		57,58,74,
		58,74,75,
		58,59,75,
		59,75,76,
		59,60,76,
		60,76,77,
		60,61,77,
		61,77,78,
		61,62,78,
		62,78,79,
		62,63,79,
		63,79,80,
		63,64,80,
		64,80,65,
		64,49,65,

		// ring 5 to ring 6
		65,81,82,
		65,66,82,
		66,82,83,
		66,67,83,
		67,83,84,
		67,68,84,
		68,84,85,
		68,69,85,
		69,85,86,
		69,70,86,
		70,86,87,
		70,71,87,
		71,87,88,
		71,72,88,
		72,88,89,
		72,73,89,
		73,89,90,
		73,74,90,
		74,90,91,
		74,75,91,
		75,91,92,
		75,76,92,
		76,92,93,
		76,77,93,
		77,93,94,
		77,78,94,
		78,94,95,
		78,79,95,
		79,95,96,
		79,80,96,
		80,96,81,
		80,65,81,

		// ring 6 to ring 7
		81,97,98,
		81,82,98,
		82,98,99,
		82,83,99,
		83,99,100,
		83,84,100,
		84,100,101,
		84,85,101,
		85,101,102,
		85,86,102,
		86,102,103,
		86,87,103,
		87,103,104,
		87,88,104,
		88,104,105,
		88,89,105,
		89,105,106,
		89,90,106,
		90,106,107,
		90,91,107,
		91,107,108,
		91,92,108,
		92,108,109,
		92,93,109,
		93,109,110,
		93,94,110,
		94,110,111,
		94,95,111,
		95,111,112,
		95,96,112,
		96,112,97,
		96,81,97,

		// ring 7 to ring 8
		97,113,114,
		97,98,114,
		98,114,115,
		98,99,115,
		99,115,116,
		99,100,116,
		100,116,117,
		100,101,117,
		101,117,118,
		101,102,118,
		102,118,119,
		102,103,119,
		103,119,120,
		103,104,120,
		104,120,121,
		104,105,121,
		105,121,122,
		105,106,122,
		106,122,123,
		106,107,123,
		107,123,124,
		107,108,124,
		108,124,125,
		108,109,125,
		109,125,126,
		109,110,126,
		110,126,127,
		110,111,127,
		111,127,128,
		111,112,128,
		112,128,113,
		112,97,113,

		// ring 8 to ring 9
		113,129,130,
		113,114,130,
		114,130,131,
		114,115,131,
		115,131,132,
		115,116,132,
		116,132,133,
		116,117,133,
		117,133,134,
		117,118,134,
		118,134,135,
		118,119,135,
		119,135,136,
		119,120,136,
		120,136,137,
		120,121,137,
		121,137,138,
		121,122,138,
		122,138,139,
		122,123,139,
		123,139,140,
		123,124,140,
		124,140,141,
		124,125,141,
		125,141,142,
		125,126,142,
		126,142,143,
		126,127,143,
		127,143,144,
		127,128,144,
		128,144,129,
		128,113,129,

		// ring 9 to ring 10
		129,145,146,
		129,130,146,
		130,146,147,
		130,131,147,
		131,147,148,
		131,132,148,
		132,148,149,
		132,133,149,
		133,149,150,
		133,134,150,
		134,150,151,
		134,135,151,
		135,151,152,
		135,136,152,
		136,152,153,
		136,137,153,
		137,153,154,
		137,138,154,
		138,154,155,
		138,139,155,
		139,155,156,
		139,140,156,
		140,156,157,
		140,141,157,
		141,157,158,
		141,142,158,
		142,158,159,
		142,143,159,
		143,159,160,
		143,144,160,
		144,160,145,
		144,129,145,

		// ring 10 to ring 11
		145,161,162,
		145,146,162,
		146,162,163,
		146,147,163,
		147,163,164,
		147,148,164,
		148,164,165,
		148,149,165,
		149,165,166,
		149,150,166,
		150,166,167,
		150,151,167,
		151,167,168,
		151,152,168,
		152,168,169,
		152,153,169,
		153,169,170,
		153,154,170,
		154,170,171,
		154,155,171,
		155,171,172,
		155,156,172,
		156,172,173,
		156,157,173,
		157,173,174,
		157,158,174,
		158,174,175,
		158,159,175,
		159,175,176,
		159,160,176,
		160,176,161,
		160,145,161,

		// ring 11 to ring 12
		161,177,178,
		161,162,178,
		162,178,179,
		162,163,179,
		163,179,180,
		163,164,180,
		164,180,181,
		164,165,181,
		165,181,182,
		165,166,182,
		166,182,183,
		166,167,183,
		167,183,184,
		167,168,184,
		168,184,185,
		168,169,185,
		169,185,186,
		169,170,186,
		170,186,187,
		170,171,187,
		171,187,188,
		171,172,188,
		172,188,189,
		172,173,189,
		173,189,190,
		173,174,190,
		174,190,191,
		174,175,191,
		175,191,192,
		175,176,192,
		176,192,177,
		176, 161,177,

		// ring 12 to ring 13
		177,193,194,
		177,178,194,
		178,194,195,
		178,179,195,
		179,195,196,
		179,180,196,
		180,196,197,
		180,181,197,
		181,197,198,
		181,182,198,
		182,198,199,
		182,183,199,
		183,199,200,
		183,184,200,
		184,200,201,
		184,185,201,
		185,201,202,
		185,186,202,
		186,202,203,
		186,187,203,
		187,203,204,
		187,188,204,
		188,204,205,
		188,189,205,
		189,205,206,
		189,190,206,
		190,206,207,
		190,191,207,
		191,207,208,
		191,192,208,
		192,208,193,
		192,177,193,

		// ring 13 to ring 14
		193,209,210,
		193,194,210,
		194,210,211,
		194,195,211,
		195,211,212,
		195,196,212,
		196,212,213,
		196,197,213,
		197,213,214,
		197,198,214,
		198,214,215,
		198,199,215,
		199,215,216,
		199,200,216,
		200,216,217,
		200,201,217,
		201,217,218,
		201,202,218,
		202,218,219,
		202,203,219,
		203,219,220,
		203,204,220,
		204,220,221,
		204,205,221,
		205,221,222,
		205,206,222,
		206,222,223,
		206,207,223,
		207,223,224,
		207,208,224,
		208,224,209,
		208,193,209,

		// ring 14 to ring 15
		209,225,226,
		209,210,226,
		210,226,227,
		210,211,227,
		211,227,228,
		211,212,228,
		212,228,229,
		212,213,229,
		213,229,230,
		213,214,230,
		214,230,231,
		214,215,231,
		215,231,232,
		215,216,232,
		216,232,233,
		216,217,233,
		217,233,234,
		217,218,234,
		218,234,235,
		218,219,235,
		219,235,236,
		219,220,236,
		220,236,237,
		220,221,237,
		221,237,238,
		221,222,238,
		222,238,239,
		222,223,239,
		223,239,240,
		223,224,240,
		224,240,225,
		224,209,225,

		// ring 15 - bottom
		225,226,241,
		226,227,241,
		227,228,241,
		228,229,241,
		229,239,241,
		230,231,241,
		231,232,241,
		232,233,241,
		233,234,241,
		234,235,241,
		235,236,241,
		236,237,241,
		237,238,241,
		238,239,241,
		239,240,241,
		240,225,241
	};

	// total float values per each type
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	// store vertex and index count
	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex));
	mesh.nIndices = sizeof(indices) / (sizeof(indices[0]));

	glm::vec3 normal;
	glm::vec3 vert;
	glm::vec3 center(0.0f, 0.0f, 0.0f);
	float u, v;
	std::vector<GLfloat> combined_values;

	// combine interleaved vertices, normals, and texture coords
	for (int i = 0; i < sizeof(verts) / (sizeof(verts[0])); i += 3)
	{
		vert = glm::vec3(verts[i], verts[i + 1], verts[i + 2]);
		normal = normalize(vert - center);
		u = atan2(normal.x, normal.z) / (2 * M_PI) + 0.5;
		v = normal.y * 0.5 + 0.5;
		combined_values.push_back(vert.x);
		combined_values.push_back(vert.y);
		combined_values.push_back(vert.z);
		combined_values.push_back(normal.x);
		combined_values.push_back(normal.y);
		combined_values.push_back(normal.z);
		combined_values.push_back(u);
		combined_values.push_back(v);
	}

	// Create VAO
	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBOs
	glGenBuffers(2, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the vertex buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * combined_values.size(), combined_values.data(), GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the index buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void UCreatePlaneMesh(GLMesh& mesh)
{
	// Vertex data
	GLfloat verts[] = {
		// Vertex Positions		// Normals			// Texture coords	// Index
		-1.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f,	0.0f, 1.0f,			//0
		1.0f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f,	0.0f, 0.0f,			//1
		1.0f,  0.0f, -1.0f,		0.0f, 1.0f, 0.0f,	1.0f, 0.0f,			//2
		-1.0f, 0.0f, -1.0f,		0.0f, 1.0f, 0.0f,	1.0f, 1.0f,			//3
	};

	// Index data
	GLuint indices[] = {
		0,1,2,
		0,3,2
	};

	// total float values per each type
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	// store vertex and index count
	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

	// Generate the VAO for the mesh
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);	// activate the VAO

	// Create VBOs for the mesh
	glGenBuffers(2, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends data to the GPU

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}
void UHardDriveMesh(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Positions				//Normals
		// ------------------------------------------------------

		//Back Face				//Negative Z Normal  Texture Coords.
		0.5f, 0.5f, -0.5f,		0.0f,  0.0f, -1.0f,  0.0f, 1.0f,   //0
		0.5f, -0.5f, -0.5f,		0.0f,  0.0f, -1.0f,  0.0f, 0.0f,   //1
		-0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,  1.0f, 0.0f,   //2
		-0.5f, 0.5f, -0.5f,		0.0f,  0.0f, -1.0f,  1.0f, 1.0f,   //3

		//Bottom Face			//Negative Y Normal
		-0.5f, -0.5f, 0.5f,		0.0f, -1.0f,  0.0f,  0.0f, 1.0f,  //4
		-0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  //5
		0.5f, -0.5f, -0.5f,		0.0f, -1.0f,  0.0f,  1.0f, 0.0f,  //6
		0.5f, -0.5f,  0.5f,		0.0f, -1.0f,  0.0f,  1.0f, 1.0f, //7

		//Left Face				//Negative X Normal
		-0.5f, 0.5f, -0.5f,		1.0f,  0.0f,  0.0f,  0.0f, 1.0f,      //8
		-0.5f, -0.5f,  -0.5f,	1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  //9
		-0.5f,  -0.5f,  0.5f,	1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  //10
		-0.5f,  0.5f,  0.5f,	1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  //11

		//Right Face			//Positive X Normal
		0.5f,  0.5f,  0.5f,		1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  //12
		0.5f,  -0.5f, 0.5f,		1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  //13
		0.5f, -0.5f, -0.5f,		1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  //14
		0.5f, 0.5f, -0.5f,		1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  //15

		//Top Face				//Positive Y Normal
		-0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,  0.0f, 1.0f, //16
		-0.5f,  0.5f, 0.5f,		0.0f,  1.0f,  0.0f,  0.0f, 0.0f, //17
		0.5f,  0.5f,  0.5f,		0.0f,  1.0f,  0.0f,  1.0f, 0.0f, //18
		0.5f,  0.5f,  -0.5f,	0.0f,  1.0f,  0.0f,  1.0f, 1.0f, //19

		//Front Face			//Positive Z Normal
		-0.5f, 0.5f,  0.5f,	    0.0f,  0.0f,  1.0f,  0.0f, 1.0f, //20
		-0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,  0.0f, 0.0f, //21
		0.5f,  -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,  1.0f, 0.0f, //22
		0.5f,  0.5f,  0.5f,		0.0f,  0.0f,  1.0f,  1.0f, 1.0f, //23
	};

	// Index data
	GLuint indices[] = {
		0,1,2,
		0,3,2,
		4,5,6,
		4,7,6,
		8,9,10,
		8,11,10,
		12,13,14,
		12,15,14,
		16,17,18,
		16,19,18,
		20,21,22,
		20,23,22
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(2, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]); // Activates the buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

// One problem with the current state of this project is some of these meshes are not being destroyed

void UDestroyMesh(GLMesh& mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(2, mesh.vbos);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}