#include <cglm/cglm.h>
#include "colours.h"
#include "dynamicArray.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "model.h"
#include "rgb2spec.h"
#include "shader.h"
#include "sphere.h"
#include "stb_image_write.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"

#define WINDOW_HEIGHT 600
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

    // Viewplane-Werte werden aus Kamerawerten berechnet und an den Compute Shader gesendet
    float FOV = glm_rad(65.0f);
    // Abstand der Bildebene vom "Auge"
    float focalLength = 1.0f;

    float planeHeight = 2 * focalLength * tan(FOV * 0.5f);
    float planeWidth = planeHeight * ASPECT_RATIO;
    vec3 topLeftCorner = {-planeWidth / 2.0f, planeHeight / 2.0f, -focalLength};
    float deltaV = -planeHeight / WINDOW_HEIGHT;
    float deltaU = planeWidth / WINDOW_WIDTH;

    setVec3(raytracer, "viewplane.topLeftCorner", topLeftCorner);
    setFloat(raytracer, "viewplane.deltaU", deltaU);
    setFloat(raytracer, "viewplane.deltaV", deltaV);

    // Nötige Kameradaten werden an den Shader geschickt
    vec3 camCenter = {3.0f, 0.0f, 14.9f};
    setVec3(raytracer, "cam.center", camCenter);
    mat4 camTransform;
    glm_lookat(camCenter, (vec3) {0.0f, 0.0f, 0.0f}, (vec3) {0.0f, 1.0f, 0.0f}, camTransform);
    glm_mat4_inv(camTransform, camTransform);
    setMatrix(raytracer, "cam.transform", camTransform);

    // Szene wird definiert

    DynamicArray* materialInstanceTracker = createDynamicArray(sizeof(Material*), 1);
    DynamicArray* sphereInstanceTracker = createDynamicArray(sizeof(Sphere*), 1);
    ModelInstanceTracker* modelInstanceTracker = initModelInstanceTracker();

    float darkSpectrum[SPECTRAL_RESOLUTION], brightSpectrum[SPECTRAL_RESOLUTION];
    for(int i = 0; i < SPECTRAL_RESOLUTION; i++) {
        darkSpectrum[i] = 0.0f;
        brightSpectrum[i] = 17.5f;
    }
    darkSpectrum[0] = -1.0f; // Kennzeichen eines nicht leutenden Materials

    // LUT zur Überführung der RGB-Farben in Spektrumskoeffizienten wird geladen
    RGB2Spec* LUT = rgb2spec_load("..\\input\\srgb.coeff");

    vec3 redCoeff, greenCoeff, blueCoeff, bleenCoeff, whiteCoeff;
    rgb2spec_fetch(LUT, RED, redCoeff);
    rgb2spec_fetch(LUT, GREEN, greenCoeff);
    rgb2spec_fetch(LUT, BLUE, blueCoeff);
    rgb2spec_fetch(LUT, BLEEN, bleenCoeff);
    rgb2spec_fetch(LUT, WHITE, whiteCoeff);

    Material* lightMaterial = createMaterial(whiteCoeff, (vec2) {1.0f, 1.0f}, 0.0f, 0.0f, 1.0f, brightSpectrum, NULL, materialInstanceTracker);
    Material* whiteMaterial = createMaterial(whiteCoeff, (vec2) {1.0f, 1.0f}, 0.0f, 0.0f, 1.0f, darkSpectrum, NULL, materialInstanceTracker);
    Material* refractiveMaterial = createMaterial(whiteCoeff, (vec2) {0.0f, 0.0f}, 0.0f, 1.0f, 2.0f, darkSpectrum, NULL, materialInstanceTracker);
    Material* redMaterial = createMaterial(redCoeff, (vec2) {1.0f, 1.0f}, 0.0f, 0.0f, 1.0f, darkSpectrum, NULL, materialInstanceTracker);
    Material* greenMaterial = createMaterial(greenCoeff, (vec2) {1.0f, 1.0f}, 0.0f, 0.0f, 1.0f, darkSpectrum, NULL, materialInstanceTracker);
    Material* blueMaterial = createMaterial(blueCoeff, (vec2) {1.0f, 1.0f}, 0.0f, 0.0f, 1.0f, darkSpectrum, NULL, materialInstanceTracker);
    Material* bleenMaterial = createMaterial(bleenCoeff, (vec2) {1.0f, 1.0f}, 0.0f, 0.0f, 1.0f, darkSpectrum, NULL, materialInstanceTracker);

    rgb2spec_free(LUT);

    Sphere* sphere = createSphere((vec3) {0.0f, 0.0f, 10.0f}, 1.0f, refractiveMaterial, false, sphereInstanceTracker);

    unsigned int sphereSSBO = prepareSphereSSBO(sphereInstanceTracker);
    uploadAllSpheres(sphereInstanceTracker, sphereSSBO);
    disposeStaticSpheres(sphereInstanceTracker);

    vec4 vertices[] = {

        // Koordinaten (X, Y, Z)          // Dummy/Padding (W)

        // Rückseite (Z = -0.5) - Blick von außen (Richtung +Z)
        -0.5f, -0.5f, -0.5f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f,
        0.5f,  0.5f, -0.5f, 1.0f,
        0.5f,  0.5f, -0.5f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f,

        // Vorderseite (Z = 0.5) - Blick von außen (Richtung -Z)
        -0.5f, -0.5f,  0.5f, 1.0f,
        0.5f, -0.5f,  0.5f, 1.0f,
        0.5f,  0.5f,  0.5f, 1.0f,
        0.5f,  0.5f,  0.5f, 1.0f,
        -0.5f,  0.5f,  0.5f, 1.0f,
        -0.5f, -0.5f,  0.5f, 1.0f,

        // Linke Seite (X = -0.5) - Blick von außen (Richtung +X)
        -0.5f,  0.5f,  0.5f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f,
        -0.5f, -0.5f,  0.5f, 1.0f,
        -0.5f,  0.5f,  0.5f, 1.0f,

        // Rechte Seite (X = 0.5) - Blick von außen (Richtung -X)
        0.5f,  0.5f,  0.5f, 1.0f,
        0.5f, -0.5f,  0.5f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f,
        0.5f,  0.5f, -0.5f, 1.0f,
        0.5f,  0.5f,  0.5f, 1.0f,

        // Unterseite (Y = -0.5) - Blick von außen (Richtung +Y)
        -0.5f, -0.5f, -0.5f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f,
        0.5f, -0.5f,  0.5f, 1.0f,
        0.5f, -0.5f,  0.5f, 1.0f,
        -0.5f, -0.5f,  0.5f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f,

        // Oberseite (Y = 0.5) - Blick von außen (Richtung -Y)
        -0.5f,  0.5f, -0.5f, 1.0f,
        -0.5f,  0.5f,  0.5f, 1.0f,
        0.5f,  0.5f,  0.5f, 1.0f,
        0.5f,  0.5f,  0.5f, 1.0f,
        0.5f,  0.5f, -0.5f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f
    };

    Model* lightSource = createModel(modelInstanceTracker, vertices, 36, lightMaterial, (vec3) {0.0f, 9.75f, 0.0f}, (vec3) {7.0f, 0.5f, 7.0f}, false);

    Model* backWall = createModel(modelInstanceTracker, vertices, 36, whiteMaterial, (vec3) {0.0f, 0.0f, -15.5f}, (vec3) {20.0f, 20.0f, 1.0f}, false);
    Model* frontWall = createModel(modelInstanceTracker, vertices, 36, whiteMaterial, (vec3) {0.0f, 0.0f, 15.5f}, (vec3) {20.0f, 20.0f, 1.0f}, false);

    Model* topWall = createModel(modelInstanceTracker, vertices, 36, whiteMaterial, (vec3) {0.0f, 10.5f, 0.0f}, (vec3) {20.0f, 1.0f, 30.0f}, false);
    Model* bottomWall = createModel(modelInstanceTracker, vertices, 36, greenMaterial, (vec3) {0.0f, -10.5f, 0.0f}, (vec3) {20.0f, 1.0f, 30.0f}, false);

    Model* leftWall = createModel(modelInstanceTracker, vertices, 36, redMaterial, (vec3) {-10.5f, 0.0f, 0.0f}, (vec3) {1.0f, 20.0f, 30.0f}, false);
    Model* rightWall = createModel(modelInstanceTracker, vertices, 36, blueMaterial, (vec3) {10.5f, 0.0f, 0.0f}, (vec3) {1.0f, 20.0f, 30.0f}, false);

    Model* refractiveCube = createModel(modelInstanceTracker, vertices, 36, bleenMaterial, (vec3) {0.0f, 0.0f, 5.0f}, (vec3) {1.0f, 1.0f, 1.0f}, false);

    unsigned int modelSSBO = prepareModelSSBO(modelInstanceTracker);

    uploadAllModels(modelInstanceTracker, modelSSBO);
    disposeStaticModels(modelInstanceTracker);

    disposeMaterialBlueprints(materialInstanceTracker);

    // Sonstige Uniforms werden festgelegt
    setInt(raytracer, "maxBounceCount", 6);
    setInt(raytracer, "samplesPerPixel", 16);

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

        useShader(raytracer);
        render(raytracer, texture, frameCount, true);
        frameCount++;
        //printf("%d\n", frameCount);
        //renderVideo(raytracer, texture, &frameCount, 512, &videoFrameCount, 1, 1.0f, window);

        // Der vorgerenderte Frame wird auf einem Quad dargestellt
        useShader(quadShader);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(quadShader.ID);
    glDeleteProgram(raytracer.ID);

    deinitModelInstanceTracker(modelInstanceTracker);
    destroyDynamicArray(materialInstanceTracker);
    destroyDynamicArray(sphereInstanceTracker);

    glfwTerminate();
    glfwDestroyWindow(window);

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
        char name[40];
        snprintf(name, 40, "..\\output\\image%03d.png", *videoFrameCount);
        stbi_write_png(name, (int) WINDOW_WIDTH, WINDOW_HEIGHT, 4, pixels, (int) WINDOW_WIDTH * 4);
        free(pixels);

        float delta = 1.0 / (int) FPS;
        float time = *videoFrameCount * delta;
        // Hier Bewegung einfügen

        
        // Wurde das letzte Bild gerendert, wird das Programm geschlossen
        if(*videoFrameCount >= (int) (videoLength * FPS)) {
            glfwSetWindowShouldClose(window, true);
            printf("Total rendering time: %.2f\n", (glfwGetTime()));
            printf("Average rendering time per frame: %.2f", glfwGetTime() / (float) *videoFrameCount);
        }
    }
}