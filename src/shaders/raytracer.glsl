#version 430 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
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
    vec3 emissionColour;
    float emissionStrength;
    float smoothness;
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct HitInfo {
    bool didHit;
    float dist;
    vec3 hitPoint;
    Material material;
    vec3 normal;
};

uniform Camera cam;
#define sphereCount 4
uniform Sphere spheres[sphereCount];
uniform vec2 imageSize;
// Wird beim progressiven Rendering jeden Frame gewechselt, sodass die Zufallszahlen sich jeden Frame verändern
uniform int frameSeed;
uniform int maxBounceCount;
uniform int samplesPerPixel;
uniform int frameCount;

HitInfo intersectionSphere(Ray ray, Sphere sphere);
HitInfo calculateClosestHit(Ray ray);
// Generiert eine Zufallszahl ziwschen 0.0f und 1.0f
float randf(inout uint pixelSeed);
vec3 traceRay(Ray ray, inout uint pixelSeed);

void main() {
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    // Ein pixel-spezifischer Seed wird aus dem gehashten Texel-Koordinaten mit dem Frame-Spezifischen Seed gemischt
    uint pixelSeed  = (texelCoord.x * 73856093) ^ (texelCoord.y * 19349663) ^ (frameSeed * 83492791);
    // Globale Pixel-Koordinate wird aus den Kameradaten berechnet
    float planeHeight = 2 * cam.focalLength * tan(cam.FOV * 0.5);
    float planeWidth = planeHeight * cam.aspectRatio;
    vec3 topLeftCorner = vec3(-planeWidth / 2.0, planeHeight / 2.0, -cam.focalLength);
    float deltaV = -planeHeight / imageSize.y;
    float deltaU = planeWidth / imageSize.x;
    
    Ray ray;
    ray.origin = cam.center;
    
    // Licht von samplesPerPixel Strahlen wird akkumuliert. Der Mittelwert davon ist dann die Pixelfarbe.
    vec3 totalLight;
    for(int i = 0; i < samplesPerPixel; i++) {

        // ray.dir wird jeden Sample etwas für Anti-Aliasing gejittert
        float jitterX = randf(pixelSeed) - 0.5;
        float jitterY = randf(pixelSeed) - 0.5;
        vec3 localPixelCoord = vec3(topLeftCorner.x + deltaU * (texelCoord.x + 0.5 + jitterX), topLeftCorner.y + deltaV * (texelCoord.y + 0.5 + jitterY), topLeftCorner.z);
        vec3 globalPixelCoord = vec3((cam.transform * vec4(localPixelCoord, 1.0)).xyz);
        ray.dir = normalize(globalPixelCoord - cam.center);

        totalLight += traceRay(ray, pixelSeed);

    }
    vec3 pixelColour = totalLight / samplesPerPixel;

    // Farbe wird über die letzten frameCount Frames interpoliert
    float weight = 1.0 / (frameCount + 1.0);
    vec3 average = mix(imageLoad(imgOutput, texelCoord).xyz, pixelColour, weight);
    imageStore(imgOutput, texelCoord, vec4(average, 1.0));
}

HitInfo intersectionSphere(Ray ray, Sphere sphere) {

    vec3 sphereDir = ray.origin - sphere.center;
    float a = dot(ray.dir, ray.dir);
    float b = 2 * dot(ray.dir, sphereDir);
    float c = dot(sphereDir, sphereDir) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;

    HitInfo hitInfo;
    if(discriminant >= 0) {
        // dist entspricht dem Skalar in der Geradengleichung und ist außerdem die Distanz, da ray.dir normiert ist
        float dist = (-b - sqrt(discriminant)) / (2 * a);

        // Ist dist kleiner als 0, so befindet sich der Schnittpunkt hinter dem Strahl und ist ungültig
        if(dist >= 0) {
            hitInfo.didHit = true;
            hitInfo.dist = dist;
            hitInfo.hitPoint = ray.origin + dist * ray.dir;
            hitInfo.material = sphere.material;
            hitInfo.normal = normalize(hitInfo.hitPoint - sphere.center);
            return hitInfo;
        }

    }
    hitInfo.didHit = false;
    return hitInfo;
}

HitInfo calculateClosestHit(Ray ray) {

    HitInfo closestHit;
    closestHit.dist = 100000.0;
    closestHit.material.colour = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < sphereCount; i++) {
        HitInfo hitInfo = intersectionSphere(ray, spheres[i]);
        if(hitInfo.didHit && hitInfo.dist < closestHit.dist) {
            closestHit = hitInfo;
        }
    }
    return closestHit;
}

float randf(inout uint pixelSeed) {
    pixelSeed ^= pixelSeed >> 16;
    pixelSeed *= 0x85ebca6b;
    pixelSeed ^= pixelSeed >> 13;
    pixelSeed *= 0xc2b2ae35;
    pixelSeed ^= pixelSeed >> 16;
    return pixelSeed / 4294967295.0;
}

vec3 traceRay(Ray ray, inout uint pixelSeed) {

    vec3 rayColour = vec3(1.0);
    vec3 incomingLight = vec3(0.0);

    for(int i = 0; i < maxBounceCount; i++) {

        HitInfo hitInfo = calculateClosestHit(ray);

        if(hitInfo.didHit) {

             // Der neue, reflektierte Strahl wird berechnet
            ray.origin = hitInfo.hitPoint + hitInfo.normal * 0.0001;

            vec3 newDir = vec3(1.0, 1.0, 1.0);
            int safetyLimit = 10;
            int counter = 0;
            while(newDir == vec3(1.0, 1.0, 1.0) && counter < safetyLimit) {
                float x = randf(pixelSeed) * 2 - 1;
                float y = randf(pixelSeed) * 2 - 1;
                float z = randf(pixelSeed) * 2 - 1;

                // Befindet sich die zufällig generierte neue Richtung innerhalb der Einheitssphäre, wird sie normiert und übernommen 
                if(x * x + y * y + z * z <= 1) {
                    newDir = normalize(vec3(x, y, z));
                }
                counter++;
            }

            // Werden zu viele Anläufe benötigt, wird der Fallback verwendet
            if(counter >= safetyLimit) {
                newDir = hitInfo.normal;
            }

            // Zeigt die neue Richtung ins Innere der Sphäre, wird sie umgekehrt
            if(dot(newDir, hitInfo.normal) < 0.0) {
                newDir *= -1;
            }

            Material material = hitInfo.material;
            if(i == 1) {
                //return material.colour;
            }

            if (randf(pixelSeed) <= material.smoothness) {
                ray.dir = reflect(ray.dir, hitInfo.normal);
            } else {
                ray.dir = newDir;
            }
    
             // Das Licht des Strahl wird "absorbiert" bzw. vom Objekt getintet
            rayColour *= material.colour * max(dot(hitInfo.normal, ray.dir), 0.0) * 2;
                                        //     ^ Korrigiert die Helligkeit, da der Term geringer ist, je schräger das Licht einfällt

            // Licht, das am Pixel einfällt wird aus der akkumulierten Farbe aller Objekte, 
            // die der Strahl berührt hat (rayColour) und der Farbe/Stärke der getroffenen Lichtquelle (emittedLight) berechnet
            vec3 emittedLight = material.emissionColour * material.emissionStrength;
            incomingLight += rayColour * emittedLight;
            if(material.emissionStrength != 0) {
                break;
            }
            
        } else {
            break;
        }
    }
    return incomingLight;
}