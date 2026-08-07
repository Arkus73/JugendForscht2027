#include <GLAD/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "camera.h"
#include "utils.h"
#include <string.h>

#define CAMERA_SPEED 3.0f
#define MOUSE_SENSITIVITY 0.1f

// Kamera, die eine Flugbewegung implementiert. Dient als Beispiel für ein Kamerasystem und kann ausgebaut/verändert/erweitert werden

void updateVectors(Camera* this) {
    this->dir[0] = cos(glm_rad(this->yaw)) * cos(glm_rad(this->pitch)); // der neue direction-Vektor wird aus yaw und pitch berechnet
    this->dir[1] = sin(glm_rad(this->pitch));
    this->dir[2] = sin(glm_rad(this->yaw)) * cos(glm_rad(this->pitch));
    glm_normalize(this->dir);

    glm_cross(this->dir, (vec3) {0.0f, 1.0f, 0.0f}, this->right);   // der neue right-Vektor wird durchs Kreuzprodukt des direction-Vektors und des nach oben zeigenden Vektors in globalen Koordinaten definiert
    glm_normalize(this->right);

    glm_cross(this->right, this->dir, this->up);    // Der neue up-Vektor wird durchs Kreuzprodukt des direction- und des right-Vektors definiert
    glm_normalize(this->up);

    // up, right und dir sind die Achsen des Kameraraumes
}

Camera* createCamera(vec3 pos, float yaw, float pitch) {

    Camera* this = malloc(sizeof(Camera));
    if(this == NULL) {
        throwException("Memory couldn't be allocated");
    }

    memcpy(this->pos, pos, sizeof(vec3));
    this->yaw = yaw;
    this->pitch = pitch;

    updateVectors(this);

    return this;
}

void destroyCamera(Camera* this) {
    free(this);
}

void getCameraView(Camera* this, mat4 dest) {
    glm_look(this->pos, this->dir, this->up, dest); // dest ist nun die LookAt-Matrix, die den globalen Raum in den Kameraraum transformiert
}

void processCameraKeyboardInput(GLFWwindow* window, Camera* this, float delta) {
    float velocity = CAMERA_SPEED * delta;
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        vec3 vel;
        glm_vec3_copy(this->dir, vel);
        glm_vec3_scale(vel, velocity, vel);
        glm_vec3_add(this->pos, vel, this->pos);
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        vec3 vel;
        glm_vec3_copy(this->dir, vel);
        glm_vec3_scale(vel, velocity, vel);
        glm_vec3_sub(this->pos, vel, this->pos);
    }
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        vec3 vel;
        glm_vec3_copy(this->right, vel);
        glm_vec3_scale(vel, velocity, vel);
        glm_vec3_add(this->pos, vel, this->pos);
    }
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        vec3 vel;
        glm_vec3_copy(this->right, vel);
        glm_vec3_scale(vel, velocity, vel);
        glm_vec3_sub(this->pos, vel, this->pos);
    }
}

void processCameraMouseInput(Camera* this, float xOffset, float yOffset) {
    this->yaw += xOffset * MOUSE_SENSITIVITY;  
    this->pitch = clamp(this->pitch + yOffset * MOUSE_SENSITIVITY, -89.0f, 89.0f);
    updateVectors(this);
}