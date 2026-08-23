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
    vec3 albedo;
    float metallic;
    vec3 emissionColour;
    float emissionStrength;
    vec2 roughness;
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

#define CCW_WINDING_ORDER
//#define CW_WINDING_ORDER
#define NO_CULLING
//#define CULLING

#define PI 3.14159
#define SQR(num) (num * num)

#define EPSILON 0.001
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
float randf(inout uint state);
vec3 traceRay(Ray ray, inout uint state);

/* Bezug zur Rendergleichung:

Also an sich beschreibt die Rendergleichung erstmal, wie viel Licht von einem Punkt x aus in eine bestimmte Richtung omega emittiert und reflektiert wird. Dieser Wert L(x, omega)
setzt sich zum einen aus Le(x, omega), der den Wert des in die Richtung omega emittierten Lichts beschreibt, und aus dem Integral über die BRDF, dem einfallenden Licht auf x
L(x, omega strich) und dem Kosinus des Einfallswinkels zusammen.

Die Lösung der Rendering-Gleichung erfolgt hier im Code numerisch mit der Monte-Carlo-Integration, indem mehrere "Samples" durch traceRay() generiert werden, die L(x, omega) für einen konkreten Pfad mit maximaler Tiefe
maxBounceCount darstellen. 

In traceRay() wird mithilfe der for-Schleife das Problem der unendlichen Rekursion durch die Tatsache, dass für die Berechnung von L(x, omega) immer auch L(x, omega strich) benötigt wird,
was wiederum einem weiteren L(y, x - y) aus Sicht des nächsten Schnittpunkts entspricht, gelöst. Dieses neue L(y, x - y) wird mithilfe eines durch die PDF der BRDF randomisierten abgeprallten Strahls
so lange berechnet, bis eine Lichtquelle getroffen wird und somit Le(x, omega) != 0 an dieser Stelle gilt oder bis maxBounceCount erreicht ist. Auf diesem Weg wird der throughput bei
jedem Schnitt mit (BRDF * cos(theta)) / PDF multipliziert, um am Ende mit dem emittierten Licht am Strahlenende multipliziert zu werden, um den Einfluss der BRDFs aller Schnitte 
durchzusetzen. Der Code orientiert sich folglich eher an der Pfadintegralschreibweise nach Veach anstatt der klassischen Rendergleichung.

Diese Samples werden schlussendlich gemäß der Monte Carlo Integration aufsummiert und gemittelt um das endgültige L(x, omega) zu erhalten, was als Pixelfarbe ausgegeben wird.

*/

void main() {
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    // Ein pixel- und frameabhängiger Seed wird aus dem gehashten Texel-Koordinaten mit dem Frame-Spezifischen Seed gemischt
    uint state  = (texelCoord.x * 73856093) ^ (texelCoord.y * 19349663) ^ (frameSeed * 83492791);
    
    Ray ray;
    ray.origin = cam.center;
    
    vec3 totalLight = vec3(0.0);
    for(int i = 0; i < samplesPerPixel; i++) {

        // Die globale Pixelkoordinate und somit auch ray.dir werden jeden Sample etwas für Anti-Aliasing gejittert
        float jitterX = randf(state) - 0.5;
        float jitterY = randf(state) - 0.5;
        vec3 localPixelCoord = vec3(viewplane.topLeftCorner.x + viewplane.deltaU * (texelCoord.x + 0.5 + jitterX), viewplane.topLeftCorner.y + viewplane.deltaV * (texelCoord.y + 0.5 + jitterY), viewplane.topLeftCorner.z);
        vec3 globalPixelCoord = vec3((cam.transform * vec4(localPixelCoord, 1.0)).xyz);
        ray.dir = normalize(globalPixelCoord - cam.center);

        totalLight += traceRay(ray, state);

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
    
    #if defined(CULLING) && defined(CCW_WINDING_ORDER)
    // Ist die Determinante kleiner/größer als 0, ist das Dreieck ein Backface. Ist sie nahe an 0, steht ray.dir parallel zum Dreieck (da basicDet auch = -dot(ray.dir, cross(E1, E2)) und cross(E1, E2) = normal) und es wird discarded
    if(basicDet < EPSILON) {
        return hitInfo;
    }
    #endif
    #if defined(CULLING) && defined(CW_WINDING_ORDER)
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

    #ifdef CW_WINDING_ORDER
    hitInfo.normal = -hitInfo.normal;
    #endif

    // Rays can hit the inside of closed geometry (for example the Cornell box).
    // Use a normal facing the incoming ray for the shading coordinate system.
    if(dot(hitInfo.normal, ray.dir) > 0.0) {
        hitInfo.normal = -hitInfo.normal;
    }

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
    closestHit.material.albedo = vec3(0.0, 0.0, 0.0);

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
        vec3 localInvRayDir = 1.0 / localRayDir;
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

uint nextRandom(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float randf(inout uint state) {
    return float(nextRandom(state)) / 4294967295.0;
}

vec3 randomOnUnitSphere(inout uint state) {
    float u = randf(state);
    float v = randf(state);
    float theta = u * 2.0 * PI;
    float phi = acos(2.0 * v - 1.0);
    
    float x = sin(phi) * cos(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(phi);
    
    return vec3(x, y, z);
}

// Generiere einen Punkt auf der Einheitskreisscheibe
vec2 sampleUniformDiskPolar(vec2 u) {
    float r = sqrt(u.x);
    float theta = 2.0 * PI * u.y;
    return vec2(r * cos(theta), r * sin(theta));
}

// Generiere eine von der Rauheit der Oberfläche abhängige Mikrofazettennormale wm nach https://jcgt.org/published/0007/04/01/
vec3 sampleWm(vec3 wo, vec2 alpha, inout uint state) {

    vec3 vh = normalize(vec3(alpha.x * wo.x, alpha.y * wo.y, wo.z));

    float lenSqr = SQR(vh.x) + SQR(vh.y);
    vec3 T1 = (lenSqr > 0.0) ? vec3(-vh.y, vh.x, 0.0) / sqrt(lenSqr) : vec3(1.0, 0.0, 0.0);
    vec3 T2 = cross(vh, T1);

    vec2 t = sampleUniformDiskPolar(vec2(randf(state), randf(state)));
    float s = 0.5 * (1.0 + vh.z);
    t.y = (1.0 - s) * sqrt(1.0 - SQR(t.x)) + s * t.y;

    vec3 nh = t.x * T1 + t.y * T2 + sqrt(max(0.0, 1.0 - SQR(t.x) - SQR(t.y))) * vh;
    vec3 ne = normalize(vec3(alpha.x * nh.x, alpha.y * nh.y, max(0.0, nh.z)));
    return ne;
}   

// Funktionen, die die Cosinüsse und Sinüsse der Sphärischen Koordinaten eines normalisierten Vektors zurückgeben

float cosTheta(vec3 w) {
    return w.z;
}
float cos2Theta(vec3 w) {
    return SQR(w.z);
}
float sin2Theta(vec3 w) {
    return max(0.0, 1.0 - cos2Theta(w));
}
float sinTheta(vec3 w) {
    return sqrt(sin2Theta(w));
}
float tanTheta(vec3 w) {
    return sinTheta(w) / cosTheta(w);
}
float tan2Theta(vec3 w) {
    return sin2Theta(w) / cos2Theta(w);
}
float cosPhi(vec3 w) {
    float sinTheta = sinTheta(w);
    return (sinTheta == 0.0) ? 0.0 : clamp(w.x / sinTheta, -1.0, 1.0);
}
float sinPhi(vec3 w) {
    float sinTheta = sinTheta(w);
    return (sinTheta == 0.0) ? 0.0 : clamp(w.y / sinTheta, -1.0, 1.0);
}

// Die *N*ormal-*D*istribution-*F*unction beschreibt die relative Anzahl der Mikrofazetten, die in Richtung der gesampleten Normale wm zeigen.
float GGX_NDF(vec3 wm, vec2 alpha) {
    float tan2Theta = tan2Theta(wm);
    if(isinf(tan2Theta)) return 0.0;
    float cos4Theta = SQR(cos2Theta(wm));
    float e = tan2Theta * (SQR(cosPhi(wm) / alpha.x) + SQR(sinPhi(wm) / alpha.y));
    return 1.0 / (PI * alpha.x * alpha.y * cos4Theta * SQR(1 + e));
}

// Hilffunktion lambda für die Maskierungsfunktion G1 und die zusammengesetzte Maskierungs-/Schattierungsfunktion G
float lambda(vec3 w, vec2 alpha) {
    float tan2Theta = tan2Theta(w);
    if(isinf(tan2Theta)) return 0.0;
    float alphaSqr = SQR(alpha.x * cosPhi(w)) + SQR(alpha.y * sinPhi(w));
    return (sqrt(1.0 + alphaSqr * tan2Theta) - 1.0) / 2.0;
}

float G1(vec3 w, vec2 alpha) {
    return 1.0 / (1.0 + lambda(w, alpha));
}

float G(vec3 wo, vec3 wi, vec2 alpha) {
    return 1.0 / (1.0 + lambda(wo, alpha) + lambda(wi, alpha));
}

void buildOrthonormalBasis(vec3 n, out vec3 t, out vec3 b) {
    if (n.z < -0.9999999) {
        t = vec3(0.0, -1.0, 0.0);
        b = vec3(-1.0, 0.0, 0.0);
        return;
    }
    float a = 1.0 / (1.0 + n.z);
    float h = -n.x * n.y * a;
    t = vec3(1.0 - n.x * n.x * a, h, -n.x);
    b = vec3(h, 1.0 - n.y * n.y * a, -n.y);
}

vec3 traceRay(Ray ray, inout uint state) {

    vec3 throughput = vec3(1.0); // throughput entspricht einem Faktor, der den Grad der Modifierung des Lichts einer später getroffenen Lichtquelle über den Strahlenweg beschreibt. Er bestimmt maßgeblich den Einfluss der BRDF auf incomingLight
    vec3 incomingLight = vec3(0.0); // incomingLight entspricht L(erster Schnittpunkt, -ray.dir), also dem Licht, das vom ersten Schnittpunkt Richtung Kamera emittiert und reflektiert wird 

    for(int i = 0; i < maxBounceCount; i++) {

        HitInfo hitInfo = calculateClosestHit(ray);

        if(hitInfo.didHit) {
            
            Material material = hitInfo.material;

            vec3 emittedLight = material.emissionColour * material.emissionStrength; // emittedLight entspricht dem Le(hitInfo.hitPoint, -ray.dir) des getroffenen Punktes
            incomingLight += throughput * emittedLight;
            
            if(material.emissionStrength != 0.0) {
                break;
            }

            #define COOK_TORRANCE_BRDF

            #ifdef COOK_TORRANCE_BRDF
            // Implementierung der BRDF nach dem Cook-Torrance Mikrofazettenmodell beschrieben in https://pbr-book.org/4ed/Reflection_Models/Roughness_Using_Microfacet_Theory

            // Makrooberflächennormale
            vec3 n = hitInfo.normal;
            // Beobachtungsrichtung
            vec3 woGlobal = -ray.dir;

            // wo wird für die Berechnungen in den Normalenraum transformiert
            vec3 t, b;
            buildOrthonormalBasis(n, t, b);
            mat3 localToGlobal = mat3(t, b, n);
            vec3 wo = transpose(localToGlobal) * woGlobal;

            // Die wahrgenommene Rauheit wird in die tatsächliche Rauheit alpha umgewandelt
            vec2 alpha = max(SQR(material.roughness), vec2(EPSILON));
            // Es wird eine zufällige Mikrofazettennormale generiert und der Strahl an ihr reflektiert
            vec3 wm = sampleWm(wo, alpha, state);


            // Basisreflexionschance bei Einfallswinkel 90°
            vec3 F0 = mix(vec3(0.04), material.albedo, material.metallic);
            // Fresnel-Wert, also realistische Reflexionschance abhängig vom Einfallswinkel, Schlick-Approximation
            vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - max(dot(wm, wo), 0.0), 5.0);

            float p = dot(F, vec3(0.2126, 0.7152, 0.0722));
            p = clamp(p, EPSILON, 1.0 - EPSILON);

            if(p < randf(state)) {
                // Diffuse-Anteil nach Lambert
                ray.dir = normalize(n + randomOnUnitSphere(state));
                throughput *= material.albedo * (1.0 - material.metallic) / (1.0 - p);
                
            } else {
                // Specular-Anteil nach Cook-Torrance mit VNDF-Sampling

                // Lichteinfallsrichtung
                vec3 wi = reflect(-wo, wm);    

                // Entspricht der gekürzten Form des Beitrags bei GGX VNDF Importance Sampling. Siehe Notizen für Herleitung
                throughput *= F * (G(wi, wo, alpha) / G1(wo, alpha)) / p;
                /*                                                     ^ Division durch Wahrscheinlichkeit des Pfads (-> korrekte Gewichtung bei 
                der Monte Carlo Integration/Pendant zum d(omega strich) des Integrals der Rendergleichung), Teil des / p(wi) der Formel für ein throughput-weight */

                // wi wird aus dem Normalenraum wieder in den globalen Raum transformiert
                ray.dir = normalize(localToGlobal * wi);
            }

            ray.origin = hitInfo.hitPoint + hitInfo.normal * EPSILON;

            #else
            
            float F0 = mix(0.04, max(material.albedo.r, max(material.albedo.g, material.albedo.b)), material.metallic);
            float F = F0 + (1.0 - F0) * pow(1.0 - max(dot(hitInfo.normal, -ray.dir), 0.0), 5.0);

            if(F < randf(state)) {
                // Diffuse
                ray.dir= normalize(hitInfo.normal + randomOnUnitSphere(state));
                throughput *= (material.albedo * (1.0 - material.metallic)) / (1.0 - F);
            } else {
                // Specular
                ray.dir = normalize(reflect(ray.dir, hitInfo.normal) + SQR(material.roughness.x) * randomOnUnitSphere(state));
                throughput *= mix(vec3(0.04), material.albedo, material.metallic) / F;
            }

            ray.origin = hitInfo.hitPoint + hitInfo.normal * EPSILON;

            #endif

        } else {
            break;
        }
    }
    return incomingLight;
}