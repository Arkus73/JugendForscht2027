#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "shader.h"
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#define WINDOW_HEIGHT 480
#define WINDOW_WIDTH WINDOW_HEIGHT * ASPECT_RATIO

void __destructor cleanup();

int main(int argc, char** argv) {

    glfwInit();
    GLFWwindow* window = InitAndCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raytracer");
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, default_framebuffer_size_callback);
    glfwSetWindowAspectRatio(window, 16, 9);
    glfwSwapInterval(1);    // Vsync

    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Oben links
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // Oben rechts
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f, // Unten rechts

        -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // Oben links
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // Unten rechts
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f // Unten links
    };

    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 5 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 5 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    Shader quadShader = createShader("..\\src\\shaders\\quadVertex.glsl", "..\\src\\shaders\\quadFragment.glsl");
    useShader(quadShader);
    setInt(quadShader, "tex", 0);

    Shader raytracer = createComputeShader("..\\src\\shaders\\raytracer.glsl");
    useShader(raytracer);
    // Camera wird gesetzt
    setFloat(raytracer, "cam.FOV", glm_rad(45));
    setFloat(raytracer, "cam.focalLength", 1.0f);
    setFloat(raytracer, "cam.aspectRatio", ASPECT_RATIO);
    vec3 camCenter = {0.0f, 0.0f, 0.0f};
    setVec3(raytracer, "cam.center", camCenter);
    mat4 camTransform;
    glm_mat4_identity(camTransform);
    glm_translate(camTransform, camCenter);
    setMatrix(raytracer, "cam.transform", camTransform);
    setVec2(raytracer, "imageSize", (vec2) {WINDOW_WIDTH, WINDOW_HEIGHT});
    // Sphäre wird gesetzt
    setVec3(raytracer, "sphere.center", (vec3) {0.0f, 2.0f, -5.0f});
    setFloat(raytracer, "sphere.radius", 1.0f);
    setVec3(raytracer, "sphere.material.colour", (vec3) {1.0f, 1.0f, 1.0f});

    // Textur, die vom Raytracer bearbeitet und später dargestellt wird
    unsigned int raytracedRender;
    glGenTextures(1, &raytracedRender);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, raytracedRender);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

    while(!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Der Frame wird vom Raytracer auf die Textur raytracedRender gerendert
        useShader(raytracer);
        glBindImageTexture(0, raytracedRender, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);   // Textur wird fürs Writing an Position 0 gebindet
        glDispatchCompute(WINDOW_WIDTH, WINDOW_HEIGHT, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);    // Stellt sicher, dass alle Schreiboperationen des Raytracers fertig sind, bevor die Textur wieder ausgelesen wird

        // Der vorgerenderte Frame wird auf einem Quad dargestellt
        useShader(quadShader);
        glBindTexture(GL_TEXTURE_2D, raytracedRender);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    return EXIT_SUCCESS;
}

void cleanup() {
    glfwTerminate();
}