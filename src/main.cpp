#include <string>
#include <unistd.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "chip8.h"

#define PIXEL_SIZE 20

void setupGraphics();

void setupInput();

void drawGraphics();

void readInputs();

chip8 myChip8;
GLubyte *pixels = new GLubyte[CHIP_8_SCREEN_HEIGHT * CHIP_8_SCREEN_WIDTH * 3];

GLuint shaderProgram;
GLFWwindow *window;
GLuint VAO, VBO, EBO;
GLuint texture;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: PROGRAM chip8application\n\n");
        return 1;
    }

    setupGraphics();

    setupInput();

    myChip8.initialize();
    myChip8.loadGame(argv[1]);

    while (!glfwWindowShouldClose(window)) {
        myChip8.emulateCycle();

        if (myChip8.drawFlag)
            drawGraphics();

        usleep(1 / 60 * 1000000);
    }

    // Destroy everything
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    delete[] pixels;

    return 0;
}

void setupGraphics() {
    int width = CHIP_8_SCREEN_WIDTH * PIXEL_SIZE;
    int height = CHIP_8_SCREEN_HEIGHT * PIXEL_SIZE;

    // Initialize GLFW
    glfwInit();

    // Tell GLFW what version of OpenGL is use
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Tell GLFW the profile to use
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the window and set it as the current context
    window = glfwCreateWindow(width, height, "CHIP-8", NULL, NULL);
    if (window == NULL)
        throw "Failed to create GLFW window";
    glfwMakeContextCurrent(window);

    // Load Glad
    gladLoadGL();

    // Specify the viewport of OpenGL in the window
    glViewport(0, 0, width, height);

    // Load shader
    shaderProgram = glCreateProgram();
    {
        std::string vertexShaderSCodeString = "#version 330 core\n"
                                              "\n"
                                              "layout (location = 0) in vec2 aPos;\n"
                                              "layout (location = 1) in vec3 aColor;\n"
                                              "layout (location = 2) in vec2 aTex;\n"
                                              "\n"
                                              "out vec3 color;\n"
                                              "out vec2 texCoord;\n"
                                              "\n"
                                              "void main() {\n"
                                              "    gl_Position = vec4(aPos, .0f, 1.0f);\n"
                                              "    color = aColor;\n"
                                              "    texCoord = aTex;\n"
                                              "}";
        std::string fragmentShaderSCodeString = "#version 330 core\n"
                                                "\n"
                                                "out vec4 FragColor;\n"
                                                "\n"
                                                "in vec3 color;\n"
                                                "in vec2 texCoord;\n"
                                                "\n"
                                                "uniform sampler2D tex0;\n"
                                                "\n"
                                                "void main() {\n"
                                                "    FragColor = texture(tex0, texCoord);\n"
                                                "}";

        const char *vertexShaderSCodeCharA = vertexShaderSCodeString.c_str();
        const char *fragmentShaderSCodeCharA = fragmentShaderSCodeString.c_str();

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSCodeCharA, NULL);
        glCompileShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSCodeCharA, NULL);
        glCompileShader(fragmentShader);

        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    GLfloat vertices[] = {
            // Position |     Color       | Texcoords
            -1.f, 1.f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
            1.f, 1.f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
            1.f, -1.f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
            -1.f, -1.f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
    };

    GLuint elements[] = {
            0, 1, 2,
            2, 3, 0
    };

    // Generate the VAO, VBO and EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind the VAO as the current Vertex Array Object
    glBindVertexArray(VAO);

    // Bind the VBO as the current Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Insert the vertices into the VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Bind the EBO as the current Element Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // Insert the indices into the EBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    // Configure the Vertex Attribute so that OpenGL knows how to read the VBO
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *) 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *) (2 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *) (5 * sizeof(float)));
    // Enable the Vertex Attribute
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create the texture
    texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Clear screen
    memset(pixels, 0, CHIP_8_SCREEN_WIDTH * CHIP_8_SCREEN_HEIGHT * 3 * sizeof(char));

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CHIP_8_SCREEN_WIDTH, CHIP_8_SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 (GLvoid *) pixels);
    GLuint texUni = glGetUniformLocation(shaderProgram, "tex0");
    glUseProgram(shaderProgram);
    glUniform1i(texUni, 0);
}

void drawGraphics() {
    // Update pixels
    for (int y = 0; y < CHIP_8_SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < CHIP_8_SCREEN_WIDTH; ++x) {
            pixels[y * CHIP_8_SCREEN_WIDTH * 3 + x * 3 + 0] = pixels[y * CHIP_8_SCREEN_WIDTH * 3 + x * 3 + 1] = pixels[
                    y * CHIP_8_SCREEN_WIDTH * 3 + x * 3 + 2] = myChip8.gfx[y * CHIP_8_SCREEN_WIDTH + x] * 255;
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CHIP_8_SCREEN_WIDTH, CHIP_8_SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 (GLvoid *) pixels);

    // Update GLFW
    glClearColor(.0f, .0f, .0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);

    glfwPollEvents();

    myChip8.drawFlag = false;
}

void key_event(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // PRESS
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) myChip8.key[0x1] = 1;
    else if (key == GLFW_KEY_2 && action == GLFW_PRESS) myChip8.key[0x2] = 1;
    else if (key == GLFW_KEY_3 && action == GLFW_PRESS) myChip8.key[0x3] = 1;
    else if (key == GLFW_KEY_4 && action == GLFW_PRESS) myChip8.key[0xC] = 1;

    else if (key == GLFW_KEY_Q && action == GLFW_PRESS) myChip8.key[0x4] = 1;
    else if (key == GLFW_KEY_W && action == GLFW_PRESS) myChip8.key[0x5] = 1;
    else if (key == GLFW_KEY_E && action == GLFW_PRESS) myChip8.key[0x6] = 1;
    else if (key == GLFW_KEY_R && action == GLFW_PRESS) myChip8.key[0xD] = 1;

    else if (key == GLFW_KEY_A && action == GLFW_PRESS) myChip8.key[0x7] = 1;
    else if (key == GLFW_KEY_S && action == GLFW_PRESS) myChip8.key[0x8] = 1;
    else if (key == GLFW_KEY_D && action == GLFW_PRESS) myChip8.key[0x9] = 1;
    else if (key == GLFW_KEY_F && action == GLFW_PRESS) myChip8.key[0xE] = 1;

    else if (key == GLFW_KEY_Z && action == GLFW_PRESS) myChip8.key[0xA] = 1;
    else if (key == GLFW_KEY_X && action == GLFW_PRESS) myChip8.key[0x0] = 1;
    else if (key == GLFW_KEY_C && action == GLFW_PRESS) myChip8.key[0xB] = 1;
    else if (key == GLFW_KEY_V && action == GLFW_PRESS) myChip8.key[0xF] = 1;

        // RELEASE
    else if (key == GLFW_KEY_1 && action == GLFW_RELEASE) myChip8.key[0x1] = 0;
    else if (key == GLFW_KEY_2 && action == GLFW_RELEASE) myChip8.key[0x2] = 0;
    else if (key == GLFW_KEY_3 && action == GLFW_RELEASE) myChip8.key[0x3] = 0;
    else if (key == GLFW_KEY_4 && action == GLFW_RELEASE) myChip8.key[0xC] = 0;

    else if (key == GLFW_KEY_Q && action == GLFW_RELEASE) myChip8.key[0x4] = 0;
    else if (key == GLFW_KEY_W && action == GLFW_RELEASE) myChip8.key[0x5] = 0;
    else if (key == GLFW_KEY_E && action == GLFW_RELEASE) myChip8.key[0x6] = 0;
    else if (key == GLFW_KEY_R && action == GLFW_RELEASE) myChip8.key[0xD] = 0;

    else if (key == GLFW_KEY_A && action == GLFW_RELEASE) myChip8.key[0x7] = 0;
    else if (key == GLFW_KEY_S && action == GLFW_RELEASE) myChip8.key[0x8] = 0;
    else if (key == GLFW_KEY_D && action == GLFW_RELEASE) myChip8.key[0x9] = 0;
    else if (key == GLFW_KEY_F && action == GLFW_RELEASE) myChip8.key[0xE] = 0;

    else if (key == GLFW_KEY_Z && action == GLFW_RELEASE) myChip8.key[0xA] = 0;
    else if (key == GLFW_KEY_X && action == GLFW_RELEASE) myChip8.key[0x0] = 0;
    else if (key == GLFW_KEY_C && action == GLFW_RELEASE) myChip8.key[0xB] = 0;
    else if (key == GLFW_KEY_V && action == GLFW_RELEASE) myChip8.key[0xF] = 0;
}

void setupInput() {
    glfwSetKeyCallback(window, key_event);
}
