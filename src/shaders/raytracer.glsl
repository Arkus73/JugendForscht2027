#version 430 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout (rgba32f, binding = 0) uniform image2D imgOutput;

struct Camera {
    // globale Position der Kamera
    vec3 center;
    // Transformiert von Kamera-lokalen zu Weltkoordinaten
    mat4 transform;
};

struct ViewPlane {
    vec3 topLeftCorner;
    float deltaU, deltaV;
};

struct Material {
    vec3 colour;
    float smoothness;
    vec3 emissionColour;
    float emissionStrength;
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

struct Triangle {
    vec3 A, B, C;
    Material material;
};

struct AABB {
    vec4 min, max;
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
uniform ViewPlane viewplane;

#define SPHERE_COUNT 1
uniform Sphere spheres[SPHERE_COUNT];

#define CW_BACKFACE_CULLING
//#define CCW_BACKFACE_CULLING
//#define NO_CULLING
#define EPSILON 0.00001
#define MODEL_COUNT 7
layout (std430, binding = 1) readonly buffer Models {
    ivec4 meshOffsetsAndVertexCounts[MODEL_COUNT];
    mat4 modelMatrices[MODEL_COUNT];
    mat4 invModelMatrices[MODEL_COUNT];
    AABB modelAABBs[MODEL_COUNT];
    Material modelMaterials[MODEL_COUNT];
    vec4 vertices[];
};

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
    
    Ray ray;
    ray.origin = cam.center;
    
    // Licht von samplesPerPixel Strahlen wird akkumuliert. Der Mittelwert davon ist dann die Pixelfarbe.
    vec3 totalLight = vec3(0.0);
    for(int i = 0; i < samplesPerPixel; i++) {

        // Die globale Pixelkoordinate und somit auch ray.dir werden jeden Sample etwas für Anti-Aliasing gejittert
        float jitterX = randf(pixelSeed) - 0.5;
        float jitterY = randf(pixelSeed) - 0.5;
        vec3 localPixelCoord = vec3(viewplane.topLeftCorner.x + viewplane.deltaU * (texelCoord.x + 0.5 + jitterX), viewplane.topLeftCorner.y + viewplane.deltaV * (texelCoord.y + 0.5 + jitterY), viewplane.topLeftCorner.z);
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

// Möller-Trumbore-Algorithmus: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle//moller-trumbore-ray-triangle-intersection.html
HitInfo intersectionTriangle(Ray ray, Triangle triangle) {

    vec3 oa = ray.origin - triangle.A;
    // Zwei Seiten des Dreieck (Schenkel des Winkels bei A)
    vec3 E1 = triangle.B - triangle.A;
    vec3 E2 = triangle.C - triangle.A;
    
    HitInfo hitInfo;
    hitInfo.didHit = false;

    // Wichtig: Um den folgenden Code zu verstehen, muss klar gestellt werden, dass dot(A, cross(B, C)) = dot(C, cross(A, B)) = dot(B, cross(C, A)) = determinant(mat3(A, B, C))
    vec3 pvec =  cross(ray.dir, E2);
    float basicDet = dot(E1, pvec);
    //                  ^ determinant(mat3(-ray.dir, E1, E2))
    float invBasicDet = 1.0 / basicDet;
    
    #ifdef CW_BACKFACE_CULLING
    // Ist die Determinante kleiner/größer als 0, ist das Dreieck ein Backface. Ist sie nahe an 0, steht ray.dir parallel zum Dreieck (da basicDet auch = -dot(ray.dir, cross(E1, E2)) und cross(E1, E2) = normal) und es wird discarded
    if(basicDet < EPSILON) {
        return hitInfo;
    }
    #endif
    #ifdef CCW_BACKFACE_CULLING
    if(basicDet > EPSILON) {
        return hitInfo;
    }
    #endif
    #ifdef NO_CULLING
    if(abs(basicDet) < EPSILON) {
        return hitInfo;
    }
    #endif

    // Nutzung der Cramerschen Regel, um die Schnittgleichung basicMatrix * (t, u, v) = oa zu lösen (t ist der Skalar des Strahls; u, v die Baryzentrischen Koordinaten des Dreiecks)
    float u = dot(oa, pvec) * invBasicDet;
    //           ^ determinant(mat3(-ray.dir, oa, E2))
    if(u < 0.0 || u > 1.0) {
        return hitInfo;
    }

    vec3 qvec = cross(oa, E1);
    float v = dot(ray.dir, qvec) * invBasicDet;
    //           ^ determinant(mat3(-ray.dir, E1, oa))
    if(v < 0.0 || v > 1.0 || (u + v) > 1.0) {
        return hitInfo;
    }

    // Weisen die baryzentrischen Koordinaten auf einen Schnitt hin, werden die HitInfo-Daten berechnet
    float t = dot(E2, qvec) * invBasicDet;
    //          ^ determinant(mat3(oa, E1, E2))
    // Dreiecke, die hinter dem Strahl liegen, werden discarded
    if(t < 0.0) {
        return hitInfo;
    }

    hitInfo.didHit = true;
    hitInfo.dist = t;
    hitInfo.hitPoint = ray.origin + ray.dir * t;
    hitInfo.material = triangle.material;
    hitInfo.normal = normalize(cross(E1, E2));

    #ifdef NO_CULLING
    // Für Backfaces wird die Normale zum Strahl gedreht, damit Shading ordentlich klappt
    if(basicDet < 0.0) {
        hitInfo.normal *= -1.0;
    }
    #endif

    return hitInfo;
}

bool intersectionAABB(vec3 localRayOrigin, vec3 invLocalRayDir, AABB box) {
    vec3 t0 = (box.min.xyz - localRayOrigin) * invLocalRayDir;
    vec3 t1 = (box.max.xyz - localRayOrigin) * invLocalRayDir;
    
    vec3 tminv = min(t0, t1);
    vec3 tmaxv = max(t0, t1);
    
    float tMin = max(max(tminv.x, tminv.y), tminv.z);
    float tMax = min(min(tmaxv.x, tmaxv.y), tmaxv.z);
    
    return tMax >= max(0.0, tMin) && tMin <= tMax;
}

HitInfo calculateClosestHit(Ray ray) {

    HitInfo closestHit;
    closestHit.dist = 100000.0;
    closestHit.material.colour = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < SPHERE_COUNT; i++) {
        HitInfo hitInfo = intersectionSphere(ray, spheres[i]);
        if(hitInfo.didHit && hitInfo.dist < closestHit.dist) {
            closestHit = hitInfo;
        }
    }

    for(int i = 0 ; i < MODEL_COUNT; i++) {
        // Der Ray wird für die Schnitttests ins lokale Koordinatensystem des Models überführt
        vec3 localRayDir = (invModelMatrices[i] * vec4(ray.dir, 0.0)).xyz;
        vec3 localRayOrigin = (invModelMatrices[i] * vec4(ray.origin, 1.0)).xyz;
        // Wird die AABB des Models nicht geschnitten, wird es übersprungen
        vec3 localInvRayDir = 1.0 / normalize(localRayDir);
        if(!intersectionAABB(localRayOrigin, localInvRayDir, modelAABBs[i])) {
            continue;
        }

        Material material = modelMaterials[i];
        mat4 modelMatrix = modelMatrices[i];
        int meshOffset = meshOffsetsAndVertexCounts[i].x;
        int meshVertexCount = meshOffsetsAndVertexCounts[i].y;

        // Es wird über die Vertices des Meshes iteriert und je 3 Vertices der Möller-Trumbore-Algorithmus mit dem transformierten, Model-lokalen Ray durchgeführt
        Ray localRay = {localRayOrigin, localRayDir};
        mat4 normalMatrix = transpose(invModelMatrices[i]);
        for(int j = meshOffset; j < meshOffset + meshVertexCount; j += 3) {

            Triangle localTriangle = {vertices[j].xyz, vertices[j + 1].xyz, vertices[j + 2].xyz, material};
            HitInfo hitInfo = intersectionTriangle(localRay, localTriangle);

            if(hitInfo.didHit) {

                if(hitInfo.dist < closestHit.dist) {
                    closestHit = hitInfo;
                    // hitPoint wird in den globalen Raum überführt 
                    closestHit.hitPoint = ray.origin + hitInfo.dist * ray.dir;
                    // Normale wird mit der transponierten Inversen der Model-Matrix in den globalen Raum transformiert, damit die Normale trotz nicht-uniformer Skalierung senkrecht auf dem Dreieck steht
                    closestHit.normal = normalize((normalMatrix * vec4(hitInfo.normal, 0.0)).xyz);
                }
            }
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

            // Zeigt die neue Richtung ins Innere des Primitivs, wird sie umgekehrt
            if(dot(newDir, hitInfo.normal) < 0.0) {
                newDir *= -1;
            }

            Material material = hitInfo.material;
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