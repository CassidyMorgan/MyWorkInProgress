#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GLFW/Header.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "3D Scene"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Texture
    GLuint gTextureIdPink;
    GLuint gTextureIdGranite;
    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader programs
    GLuint gPyramidProgramId;
    GLuint gLampProgramId;
    GLuint gPlaneProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 7.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
    glm::vec3 gPyramidPosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gPyramidScale(2.0f);
    glm::vec3 gPlanePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gPlaneScale(5.0f);

    // Pyramid and light color
    //m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(1.5f, 7.5f, 4.0f);
    glm::vec3 gLightScale(0.3f);

    // Perspective to Orthographic View
    bool gIsViewOrthographic = true;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

/* Plane Vertex Shader Source Code*/
const GLchar* planeVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Plane Fragment Shader Source Code*/
const GLchar* planeFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing pyramid color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture1; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 1.0f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; // Set specular light strength
    float highlightSize = 8.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture1, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Pyramid Vertex Shader Source Code*/
const GLchar* pyramidVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Pyramid Fragment Shader Source Code*/
const GLchar* pyramidFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing pyramid color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 1.0f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; // Set specular light strength
    float highlightSize = 8.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller shape) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
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

    // Create the shader programs
    if (!UCreateShaderProgram(planeVertexShaderSource, planeFragmentShaderSource, gPlaneProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(pyramidVertexShaderSource, pyramidFragmentShaderSource, gPyramidProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "resources/textures/NeonPinkPlastic.jpg";
    if (!UCreateTexture(texFilename, gTextureIdPink))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "resources/textures/granite.jpg";
    if (!UCreateTexture(texFilename, gTextureIdGranite))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gPyramidProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gPyramidProgramId, "uTexture"), 0);
    glUseProgram(gPlaneProgramId);
    glUniform1i(glGetUniformLocation(gPyramidProgramId, "uTexture1"), 1);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
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

    // Release texture
    UDestroyTexture(gTextureIdPink);
    UDestroyTexture(gTextureIdGranite);

    // Release shader programs
    UDestroyShaderProgram(gPlaneProgramId);
    UDestroyShaderProgram(gPyramidProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
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
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
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

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && gTexWrapMode != GL_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureIdPink);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_REPEAT;

        cout << "Current Texture Wrapping Mode: REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && gTexWrapMode != GL_MIRRORED_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureIdPink);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_MIRRORED_REPEAT;

        cout << "Current Texture Wrapping Mode: MIRRORED REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_EDGE)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureIdPink);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_EDGE;

        cout << "Current Texture Wrapping Mode: CLAMP TO EDGE" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_BORDER)
    {
        float color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

        glBindTexture(GL_TEXTURE_2D, gTextureIdPink);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_BORDER;

        cout << "Current Texture Wrapping Mode: CLAMP TO BORDER" << endl;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
    {
        gUVScale += 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
    {
        gUVScale -= 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }

    // Change perspective view to orthographic
    static bool isPKeyDown = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !gIsViewOrthographic) 
    {
        gIsViewOrthographic = true;
        //orthogonal view
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && gIsViewOrthographic)
    {
        gIsViewOrthographic = false;
        glMatrixMode(GL_PROJECTION); // keep it like this
        glLoadIdentity();
        gluPerspective(45.0, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
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
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate the cube VAO (used by cube and lamp)
    glBindVertexArray(gMesh.vao);

    // Pyramid: draw pyramid
    //----------------
    // Set the shader to be used
    glUseProgram(gPyramidProgramId);

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = glm::translate(gPyramidPosition) * glm::scale(gPyramidScale);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gPyramidProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gPyramidProgramId, "view");
    GLint projLoc = glGetUniformLocation(gPyramidProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Pyramid Shader program for the pyramid color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gPyramidProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gPyramidProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gPyramidProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gPyramidProgramId, "viewPosition");

    // Pass color, light, and camera data to the Pyramid Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gPyramidProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdPink);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

    // Plane: draw plane
//----------------
// Set the shader to be used
    glUseProgram(gPlaneProgramId);

    // Model matrix: transformations are applied right-to-left order
    model = glm::translate(gPlanePosition) * glm::scale(gPlaneScale);

    // camera/view transformation
    view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gPlaneProgramId, "model");
    viewLoc = glGetUniformLocation(gPlaneProgramId, "view");
    projLoc = glGetUniformLocation(gPlaneProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Pyramid Shader program for the pyramid color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gPlaneProgramId, "objectColor");
    lightColorLoc = glGetUniformLocation(gPlaneProgramId, "lightColor");
    lightPositionLoc = glGetUniformLocation(gPlaneProgramId, "lightPos");
    viewPositionLoc = glGetUniformLocation(gPlaneProgramId, "viewPosition");

    // Pass color, light, and camera data to the Pyramid Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    UVScaleLoc = glGetUniformLocation(gPlaneProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureIdGranite);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller pyramid used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

    // Deactivate the Vertex Array Object and shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions          //Normals
        // ------------------------------------------------------
        //Front Left Face    //Negative Z Normal  Texture Coords.
        //Triangular Prism (Lotion Bottle)
        //Triangle 1 Left Face 
         0.35f,  1.0f,  -0.2f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Top Left Vertex 1  
         0.45f,  0.2f,  -0.25f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Left Back Vertex 2 
         0.45f,  0.2f,  -0.15f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Left Front Vertex 3 
        //Triangle 2 Right Face
         0.75f,  0.2f,  -0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Bottom Right Back Vertex 5 
         0.85f,  1.0f,  -0.2f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Top Right Vertex 0 
         0.75f,  0.2f,  -0.15f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Right Front Vertex 4 
        //Triangle 3 Back Face
         0.85f,  1.0f,  -0.2f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // Top Right Vertex 0 
         0.35f,  1.0f,  -0.2f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // Top Left Vertex 1 
         0.75f,  0.2f,  -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // Bottom Right Back Vertex 5 
        //Triangle 4 Back Face
         0.35f,  1.0f,  -0.2f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // Top Left Vertex 1 
         0.45f,  0.2f,  -0.25f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // Bottom Left Back Vertex 2 
         0.75f,  0.2f,  -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // Bottom Right Back Vertex 5        
        //Triangle 5 Front Face
         0.85f,  1.0f,  -0.2f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // Top Right Vertex 0 
         0.35f,  1.0f,  -0.2f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // Top Left Vertex 1  
         0.45f,  0.2f,  -0.15f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // Bottom Left Front Vertex 3 
        //Triangle 6 Front Face
         0.85f,  1.0f,  -0.2f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // Top Right Vertex 0 
         0.45f,  0.2f,  -0.15f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // Bottom Left Front Vertex 3 
         0.75f,  0.2f,  -0.15f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // Bottom Right Front Vertex 4 
        //Triangle 7 Bottom Face
         0.75f,  0.2f,  -0.25f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Bottom Right Back Vertex 5 
         0.45f,  0.2f,  -0.25f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Left Back Vertex 2 
         0.45f,  0.2f,  -0.15f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Left Front Vertex 3 
        //Triangle 8 Bottom Face
         0.75f,  0.2f,  -0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Bottom Right Back Vertex 5 
         0.45f,  0.2f,  -0.15f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Left Front Vertex 3 
         0.75f,  0.2f,  -0.15f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Right Front Vertex 4 

        //Cylinder (Lotion Bottle Cap)
        //Bottom Circle
         //Triangle 1
         0.6f,  0.01f, -0.2f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.45f, 0.01f, -0.25f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Left Back Vertex 2 
         0.43f,  0.01f, -0.2f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Left Vertex 1    
         //Triangle 2
         0.6f,  0.01f, -0.2f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.6f,  0.01f, -0.26f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Center Back Vertex 3 
         0.45f, 0.01f, -0.25f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Left Back Vertex 2 
         //Triangle 3
         0.6f,  0.01f, -0.2f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.75f, 0.01f, -0.25f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // Bottom Right Back Vertex 4 
         0.6f,  0.01f, -0.26f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // Bottom Center Back Vertex 3 
         //Triangle 4
         0.6f,  0.01f, -0.2f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.77f,  0.01f, -0.2f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // Bottom Right Vertex 5 
         0.75f, 0.01f, -0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // Bottom Right Back Vertex 4 
         //Triangle 5
         0.6f,  0.01f, -0.2f,   0.0f, -1.0f,  0.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.75f, 0.01f, -0.15f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f, // Bottom Right Front Vertex 6 
         0.77f,  0.01f, -0.2f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // Bottom Right Vertex 5 
         //Triangle 6
         0.6f,  0.01f, -0.2f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.6f,  0.01f, -0.14f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // Bottom Center Front Vertex 7 
         0.75f, 0.01f, -0.15f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // Bottom Right Front Vertex 6 
         //Triangle 7
         0.6f,  0.01f, -0.2f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.45f, 0.01f, -0.15f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Left Front Vertex 8 
         0.6f,  0.01f, -0.14f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Bottom Center Front Vertex 7 
         //Triangle 8
         0.6f,  0.01f, -0.2f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Center Bottom Vertex 9 
         0.43f, 0.01f, -0.2f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Left Vertex 1  
         0.45f, 0.01f, -0.15f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Left Front Vertex 8 

         //The circles connecting planes
         //Plane 1 Front
         0.6f,  0.01f, -0.14f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // Bottom Center Front Vertex 7 
         0.6f,  0.2f,  -0.14f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // Top Center Front Vertex 7 
         0.45f, 0.01f, -0.15f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // Bottom Left Front Vertex 8 
         0.6f,  0.2f,  -0.14f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // Top Center Front Vertex 7 
         0.45f, 0.2f,  -0.15f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // Top Left Front Vertex 8 
         0.45f, 0.01f, -0.15f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // Bottom Left Front Vertex 8 
         //Plane 2 Front
         0.75f, 0.01f, -0.15f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // Bottom Right Front Vertex 6 
         0.75f, 0.2f,  -0.15f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f,// Bottom Right Front Vertex 6 
         0.6f,  0.01f, -0.14f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // Bottom Center Front Vertex 7 
         0.75f, 0.2f,  -0.15f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // Bottom Right Front Vertex 6 
         0.6f,  0.2f,  -0.14f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // Bottom Center Front Vertex 7 
         0.6f,  0.01f, -0.14f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // Bottom Center Front Vertex 7 
         //Plane 3 Left Front
         0.45f, 0.01f, -0.15f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,// Bottom Left Front Vertex 8 
         0.45f, 0.2f,  -0.15f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Bottom Left Front Vertex 8 
         0.43f, 0.01f, -0.2f,    -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Left Vertex 1  
         0.45f, 0.2f,  -0.15f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Bottom Left Front Vertex 8 
         0.43f, 0.2f,  -0.2f,    -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,// Bottom Left Vertex 1 
         0.43f, 0.01f, -0.2f,    -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Left Vertex 1 
         //Plane 4 Right Front
         0.77f, 0.01f, -0.2f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Bottom Right Vertex 5 
         0.77f, 0.2f,  -0.2f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Top Right Vertex 5 
         0.75f, 0.01f, -0.15f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // Bottom Right Front Vertex 6 
         0.77f, 0.2f,  -0.2f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // Top Right Vertex 5 
         0.75f, 0.2f,  -0.15f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // Top Right Front Vertex 6 
         0.75f, 0.01f, -0.15f,  1.0f,  0.0f, 0.0f,   0.0f, 1.0f, // Bottom Right Front Vertex 6 
         //Plane 5 Back
         0.75f, 0.01f, -0.25f,  0.0f, -1.0f, 0.0f,   1.0f, 0.0f, // Bottom Right Back Vertex 4 
         0.75f, 0.2f,  -0.25f,  0.0f, -1.0f, 0.0f,   1.0f, 1.0f, // Top Right Back Vertex 4 
         0.6f,  0.01f, -0.26f,  0.0f, -1.0f, 0.0f,   0.0f, 1.0f, // Bottom Center Back Vertex 3 
         0.75f, 0.2f,  -0.25f,  0.0f, -1.0f, 0.0f,   1.0f, 0.0f, // Top Right Back Vertex 4 
         0.6f,  0.2f,  -0.26f,  0.0f, -1.0f, 0.0f,   1.0f, 1.0f, // Top Center Back Vertex 3 
         0.6f,  0.01f, -0.26f,  0.0f, -1.0f, 0.0f,   0.0f, 1.0f, // Bottom Center Back Vertex 3 
         //Plane 6 Back
         0.6f,  0.01f, -0.26f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Bottom Center Back Vertex 3 
         0.6f,  0.2f,  -0.26f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // Top Center Back Vertex 3 
         0.45f, 0.01f, -0.25f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f, // Bottom Left Back Vertex 2 
         0.6f,  0.2f,  -0.26f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Top Center Back Vertex 3 
         0.45f, 0.2f,  -0.25f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // Top Left Back Vertex 2 
         0.45f, 0.01f, -0.25f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f, // Bottom Left Back Vertex 2 
         //Plane 7 Back Left
         0.45f, 0.01f, -0.25f, -1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // Bottom Left Back Vertex 2 
         0.45f, 0.2f,  -0.25f, -1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Left Back Vertex 2 
         0.43f, 0.01f, -0.2f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f, // Bottom Left Vertex 1  
         0.45f, 0.2f,  -0.25f, -1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // Top Left Back Vertex 2 
         0.43f, 0.2f,  -0.2f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Left Vertex 1  
         0.43f, 0.01f, -0.2f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f, // Bottom Left Vertex 1 
         //Plane 8 Back Right
         0.77f, 0.01f, -0.2f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // Bottom Right Vertex 5 
         0.77f, 0.2f,  -0.2f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right Vertex 5 
         0.75f, 0.01f, -0.25f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f, // Bottom Right Back Vertex 4 
         0.77f, 0.2f,  -0.2f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // Top Right Vertex 5 
         0.75f, 0.2f,  -0.25f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right Back Vertex 4 
         0.75f, 0.01f, -0.25f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f, // Bottom Right Back Vertex 4      

         //Top Circle
         //Triangle 1
         0.6f,  0.2f, -0.2f,  0.0f, 0.0f, -1.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.45f, 0.2f, -0.25f, 0.0f, 0.0f, -1.0f,   1.0f, 1.0f, // Top Left Back Vertex 2 
         0.43f, 0.2f, -0.2f,  0.0f, 0.0f, -1.0f,   0.0f, 1.0f, // Top Left Vertex 1    
         //Triangle 2
         0.6f,  0.2f, -0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.6f,  0.2f, -0.26f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // Top Center Back Vertex 3 
         0.45f, 0.2f, -0.25f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,// Top Left Back Vertex 2 
         //Triangle 3
         0.6f,  0.2f, -0.2f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.75f, 0.2f, -0.25f, -1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right Back Vertex 4 
         0.6f,  0.2f, -0.26f, -1.0f, 0.0f, 0.0f,   0.0f, 1.0f, // Top Center Back Vertex 3 
         //Triangle 4
         0.6f,  0.2f, -0.2f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.77f, 0.2f, -0.2f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right Vertex 5 
         0.75f, 0.2f, -0.25f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f,// Top Right Back Vertex 4 
         //Triangle 5
         0.6f,  0.2f, -0.2f,  0.0f, -1.0f, 0.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.75f, 0.2f, -0.15f, 0.0f, -1.0f, 0.0f,   1.0f, 1.0f, // Top Right Front Vertex 6 
         0.77f, 0.2f, -0.2f, 0.0f,  -1.0f, 0.0f,   0.0f, 1.0f, // Top Right Vertex 5 
         //Triangle 6
         0.6f,  0.2f, -0.2f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.6f,  0.2f, -0.14f, 0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // Top Center Front Vertex 7 
         0.75f, 0.2f, -0.15f, 0.0f, 1.0f, 0.0f,   0.0f, 1.0f, // Top Right Front Vertex 6 
         //Triangle 7
         0.6f,  0.2f, -0.2f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.45f, 0.2f, -0.15f, 0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // Top Left Front Vertex 8 
         0.6f,  0.2f, -0.14f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f, // Top Center Front Vertex 7 
         //Triangle 8
         0.6f,  0.2f, -0.2f,  0.0f, 0.0f, -1.0f,   1.0f, 0.0f, // Center Top Vertex 9 
         0.43f, 0.2f, -0.2f,  0.0f, 0.0f, -1.0f,   1.0f, 1.0f, // Top Left Vertex 1  
         0.45f, 0.2f, -0.15f, 0.0f, 0.0f, -1.0f,   0.0f, 1.0f,// Top Left Front Vertex 8

        //Plane
         //Triangle 1 Back Triangle
        -2.0f, 0.0f,  2.0f,  0.0f, 0.0f, -1.0f,   1.0f, 0.0f, // Bottom Right Back Vertex 1 GREEN
        -2.0f, 0.0f, -2.0f,  0.0f, 0.0f, -1.0f,   1.0f, 1.0f, // Bottom Left Front Vertex 2 GREEN
         2.0f, 0.0f, -2.0f,  0.0f, 0.0f, -1.0f,   0.0f, 1.0f, // Bottom Right Front Vertex 3 GREEN
         //Triangle 2 Front Triangle
        -2.0f, 0.0f,  2.0f,  0.0f, 0.0f,  1.0f,   1.0f, 0.0f, // Bottom Right Back Vertex 1 GREEN
         2.0f, 0.0f, -2.0f,  0.0f, 0.0f,  1.0f,   1.0f, 1.0f, // Bottom Right Front Vertex 3 GREEN
         2.0f, 0.0f,  2.0f,  0.0f, 0.0f,  1.0f,   0.0f, 1.0f, // Bottom Right Front Vertex 4 GREEN


    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

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


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
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
        glBindTexture(GL_TEXTURE_2D, 1);

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
