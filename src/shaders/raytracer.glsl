#version 430

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (rgba32f, binding = 0) uniform image2D imgOutput;

struct Camera {
    float FOV;
    // Abstand der Bildebene vom "Auge"
    float focalLength;
    float aspectRatio;
    // globale Position der Kamera
    vec3 center;
    // Transformiert von Kamera-lokalen zu Weltkoordinaten
    mat4 transform;
};

struct Material {
    vec3 colour;
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

uniform Camera cam;
uniform Sphere sphere;
uniform vec2 imageSize;

bool hitSphere(Ray ray, Sphere sphere) {
    vec3 sphereDirection = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float b = 2 * dot(ray.direction, sphereDirection);
    float c = dot(sphereDirection, sphereDirection) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;
    return discriminant >= 0;
}

void main() {
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    float planeHeight = 2 * cam.focalLength * tan(cam.FOV * 0.5);
    float planeWidth = planeHeight * cam.aspectRatio;
    vec3 topLeftCorner = vec3(-planeWidth / 2.0, planeHeight / 2.0, -cam.focalLength);
    float deltaV = -planeHeight / imageSize.y;
    float deltaU = planeWidth / imageSize.x;
    vec3 localPixelCoord = vec3(topLeftCorner.x + deltaU * (texelCoord.x + 0.5), topLeftCorner.y + deltaV * (texelCoord.y + 0.5), topLeftCorner.z);
    vec3 globalPixelCoord = vec3((cam.transform * vec4(localPixelCoord, 1.0)).xyz);
    
    Ray ray;
    ray.origin = cam.center;
    ray.direction = normalize(globalPixelCoord - cam.center);
    if(hitSphere(ray, sphere)) {
        imageStore(imgOutput, texelCoord, vec4(sphere.material.colour, 1.0));
    }
}