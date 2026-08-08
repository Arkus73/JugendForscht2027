#include <cglm/cglm.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "sphere.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"

#define WINDOW_HEIGHT 480
#define WINDOW_WIDTH WINDOW_HEIGHT * ASPECT_RATIO

void render(Shader raytracer, unsigned int texture, int frameCount);

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

    vec3 camCenter = {3.0f, 0.0f, 5.0f};
    setVec3(raytracer, "cam.center", camCenter);
    mat4 camTransform;
    glm_lookat(camCenter, (vec3) {2.0f, 0.0f, 0.0f}, (vec3) {0.0f, 1.0f, 0.0f}, camTransform);
    glm_mat4_inv(camTransform, camTransform);
    setMatrix(raytracer, "cam.transform", camTransform);

    // Sphären und damit das Aussehen der Welt werden definiert
    setVec3(raytracer, "spheres[0].center", (vec3) {4.0f, 0.0f, -12.0f});
    setFloat(raytracer, "spheres[0].radius", 3.0f);
    setVec3(raytracer, "spheres[0].material.colour", (vec3) {0.0f, 0.0f, 1.0f});
    setVec3(raytracer, "spheres[0].material.emissionColour", (vec3) {0.0f, 0.0f, 0.0f});
    setFloat(raytracer, "spheres[0].material.emissionStrength", 0.0);

    vec3 spherePos = {-2.0f, 0.0f, -5.0f};
    setVec3(raytracer, "spheres[1].center", spherePos);
    setFloat(raytracer, "spheres[1].radius", 2.0f);
    setVec3(raytracer, "spheres[1].material.colour", (vec3) {0.0f, 0.0f, 0.0f});
    setVec3(raytracer, "spheres[1].material.emissionColour", (vec3) {1.0f, 1.0f, 1.0f});
    setFloat(raytracer, "spheres[1].material.emissionStrength", 8.0);

    // Sonstige Uniforms werden festgelegt
    setVec2(raytracer, "imageSize", (vec2) {WINDOW_WIDTH, WINDOW_HEIGHT});
    setInt(raytracer, "maxBounceCount", 3);
    setInt(raytracer, "samplesPerPixel", 10);

    // Textur, die vom Raytracer bearbeitet und später dargestellt wird
    unsigned int texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

    float lastFrame = glfwGetTime();
    int frameCount = 1;
    while(!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float currentFrame = glfwGetTime();
        float delta = currentFrame - lastFrame;
        lastFrame = currentFrame;

        render(raytracer, texture, frameCount);
        frameCount++;

        // Der vorgerenderte Frame wird auf einem Quad dargestellt
        useShader(quadShader);
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    glfwDestroyWindow(window);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(quadShader.ID);
    glDeleteProgram(raytracer.ID);

    return EXIT_SUCCESS;
}

void render(Shader raytracer, unsigned int texture, int frameCount) {
    // Der Frame wird vom Raytracer auf die Textur gerendert
    useShader(raytracer);
    setInt(raytracer, "randSeed", (int) time(NULL) * (int) glfwGetTime());
    setInt(raytracer, "frameCount", frameCount);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);   // Textur wird fürs Writing an Position 0 gebindet
    glDispatchCompute(WINDOW_WIDTH, WINDOW_HEIGHT, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);    // Stellt sicher, dass alle Schreiboperationen des Raytracers fertig sind, bevor die Textur wieder ausgelesen wird
}