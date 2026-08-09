#include <cglm/cglm.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "sphere.h"
#include "stb_image_write.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"

#define WINDOW_HEIGHT 1000
#define WINDOW_WIDTH (WINDOW_HEIGHT * ASPECT_RATIO)

void render(Shader raytracer, unsigned int texture, int avgrRange, bool progressively);
void renderVideo(Shader raytracer, unsigned int texture, int* frameCount, int frameCountPerVideoFrame, int* videoFrameCount, int FPS, float videoLength, GLFWwindow* window);

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

    vec3 camCenter = {-10.0f, 15.0f, 20.0f};
    setVec3(raytracer, "cam.center", camCenter);
    mat4 camTransform;
    glm_lookat(camCenter, (vec3) {0.0f, 0.0f, 0.0f}, (vec3) {0.0f, 1.0f, 0.0f}, camTransform);
    glm_mat4_inv(camTransform, camTransform);
    setMatrix(raytracer, "cam.transform", camTransform);

    // Sphären und damit das Aussehen der Welt werden definiert
    Sphere* ground = createSphere(0, (vec3) {0.0f, -200.0f, 0.0f}, 200.0f, (vec3) {0.0f, 0.0f, 1.0f}, (vec3) {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f);
    uploadSphere(ground, raytracer);

    Sphere* sun = createSphere(1, (vec3) {10.0f, 10.0f, 0.0f}, 3.5f, (vec3) {1.0f, 1.0f, 1.0f}, (vec3) {1.0f, 1.0f, 1.0f}, 30.0f, 0.0f);
    uploadSphere(sun, raytracer);

    Sphere* redBall = createSphere(2, (vec3) {0.0f, 2.0f, 0.0f}, 2.0f, (vec3) {1.0f, 0.0f, 0.0f}, (vec3) {0.0f, 0.0f, 0.0f}, 0.0f, 0.6f);
    uploadSphere(redBall, raytracer);

    Sphere* whiteBall = createSphere(3, (vec3) {0.0f, 1.0f, 5.0f}, 2.0f, (vec3) {1.0f, 1.0f, 1.0f}, (vec3) {0.0f, 0.0f, 0.0f}, 0.0f, 0.6f);
    uploadSphere(whiteBall, raytracer);

    // Sonstige Uniforms werden festgelegt
    setVec2(raytracer, "imageSize", (vec2) {WINDOW_WIDTH, WINDOW_HEIGHT});
    setInt(raytracer, "maxBounceCount", 3);
    setInt(raytracer, "samplesPerPixel", 64);

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
    int frameCount = 0;
    int videoFrameCount = 0;
    
    while(!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float currentFrame = glfwGetTime();
        float delta = currentFrame - lastFrame;
        lastFrame = currentFrame;

        render(raytracer, texture, frameCount, true);
        frameCount++;
        renderVideo(raytracer, texture, &frameCount, 1024, &videoFrameCount, 1, 1.0f, window);

        // Der vorgerenderte Frame wird auf einem Quad dargestellt
        useShader(quadShader);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
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

//                                                                    v Menge an vorherigen Frames, über die interpoliert werden soll
void render(Shader raytracer, unsigned int texture, int avgrRange, bool progressively) {
    // Der Frame wird vom Raytracer auf die Textur gerendert
    static int staticSeed = 0;
    if (staticSeed == 0) {
        staticSeed = (int) time(NULL);
    }
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    useShader(raytracer);
    if(progressively) {
        setInt(raytracer, "frameSeed", (int) ts.tv_nsec);
    } else {
        setInt(raytracer, "frameSeed", staticSeed);
    }
    setInt(raytracer, "frameCount", avgrRange);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);   // Textur wird fürs Writing an Position 0 gebindet
    glDispatchCompute((WINDOW_WIDTH + 7) / 8, (WINDOW_HEIGHT + 7) / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);   // Stellt sicher, dass alle Schreiboperationen des Raytracers fertig sind, bevor die Textur wieder ausgelesen wird
}

//                                                                               v Anzahl der progressivelyRender()-Aufrufe pro Video-Frame
void renderVideo(Shader raytracer, unsigned int texture, int* frameCount, int frameCountPerVideoFrame, int* videoFrameCount, int FPS, float videoLength, GLFWwindow* window) {

    if(*frameCount == frameCountPerVideoFrame) {
        *frameCount = 0;
        *videoFrameCount += 1;

        // Pixel werden aus der Textur extrahiert
        unsigned char* pixels = malloc((size_t) (WINDOW_WIDTH * WINDOW_HEIGHT * 4));
        if(pixels == NULL) {
            throwException("Couldn't allocate memory for pixels");
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Bild wird in Datei geschrieben
        char name[30];
        snprintf(name, 30, "..\\output\\image%03d.png", *videoFrameCount);
        stbi_write_png(name, (int) WINDOW_WIDTH, WINDOW_HEIGHT, 4, pixels, (int) WINDOW_WIDTH * 4);
        free(pixels);

        float delta = 1.0 / (int) FPS;
        float time = *videoFrameCount * delta;
        // Hier Bewegung einfügen
        
        // Wurde das letzte Bild gerendert, wird das Programm geschlossen
        if(*videoFrameCount >= (int) (videoLength * FPS)) {
            glfwSetWindowShouldClose(window, true);
        }
    }
}