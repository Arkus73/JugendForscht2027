#ifndef CAMERA_H
#define CAMERA_H

typedef struct {
    vec3 pos;
    vec3 dir;
    vec3 right;
    vec3 up;

    float yaw, pitch;   // yaw ist der Winkel, der die Rotation um die x-Achse beschreibt, pitch der um die y-Achse
} Camera;

Camera* createCamera(vec3 pos, float yaw, float pitch);
void destroyCamera(Camera* this);
void getCameraView(Camera* this, mat4 dest);
void processCameraKeyboardInput(GLFWwindow* window, Camera* this, float delta);
void processCameraMouseInput(Camera* this, float xOffset, float yOffset);

#endif