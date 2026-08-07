#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"
#include "stb_image.h"

GLFWwindow* InitAndCreateWindow(int windowWidth, int windowHeight, const char* title) {

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, title, NULL, NULL);    // Fenster wird erstellt
    if(window == NULL) {
        glfwTerminate();
        throwException("Failed to create a window");
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {  // GLAD wird initialisiert
        throwException("Failed to initialise GLAD");
    }    

    return window;
}

void default_framebuffer_size_callback(GLFWwindow* window, int width, int height) {     // Callback-Funktion bei Window Resize
    glViewport(0, 0, width, height);    // Viewport wird an neue Fenstergröße angepasst
}

void __noreturn throwException(char* msg) {
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}

float __const clamp(float value, float min, float max) {
    if(value < min) {
        return min;
    } else if(value > max) {
        return max;
    } else {
        return value;
    }
}

float randf(float lowerBound, float upperBound) {
    srand((unsigned int) time(NULL));
    return (((float) rand() / (float) RAND_MAX) * (upperBound - lowerBound)) + lowerBound;
}

bool randomise(float probability) {
    srand((unsigned int) time(NULL));
    return probability > ((float) rand() / (float) RAND_MAX);
}

unsigned int createTextureFromFile(const char* dir, int filter) {

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (filter == GL_LINEAR) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(dir, &width, &height, &nrChannels, 0); 
    if(data != NULL) {
        stbi_set_flip_vertically_on_load(true);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, (nrChannels == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data); 
        glGenerateMipmap(GL_TEXTURE_2D);   
    } else {
        throwException("Couldn't load image");
    }
    stbi_image_free(data);

    return texture;
}

int __const modulo(int a, int b) {
    return (a % b + b) % b;
}