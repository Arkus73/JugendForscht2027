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
    vec3 albedoSpectrumCoeffs;
    float metallic;
    vec2 roughness;
    float transmittance;
    float IOR;
    float emissionSpectrum[3];
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
    float currentIOR;
    float distInMaterial;
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
layout (std430, binding = 2) readonly buffer Spheres {
    vec4 sphereSpatialData[SPHERE_COUNT];
    Material sphereMaterials[SPHERE_COUNT];
};

#define CCW_WINDING_ORDER
//#define CW_WINDING_ORDER
//#define NO_CULLING
#define CULLING

#define PI 3.14159
#define SQR(num) ((num) * (num))

#define EPSILON 0.001
#define MODEL_COUNT 8
layout (std430, binding = 1) readonly buffer Models {
    ivec4 meshOffsetsAndVertexCounts[MODEL_COUNT];
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

Also an sich beschreibt die Rendergleichung erstmal, wie viel Licht von einem Punkt x aus in eine bestimmte Richtung wo emittiert und reflektiert wird. Dieser Wert L(x, wo)
setzt sich zum einen aus Le(x, wo), der den Wert des in die Richtung omega emittierten Lichts beschreibt, und aus dem Integral über die BRDF, dem einfallenden Licht auf x
L(x, wi) und dem Kosinus des Einfallswinkels zusammen.

Die Lösung der Rendering-Gleichung erfolgt hier im Code numerisch mit der Monte-Carlo-Integration, indem mehrere "Samples" durch traceRay() generiert werden, die L(x, wo) für einen konkreten Pfad mit maximaler Tiefe
maxBounceCount darstellen. 

In traceRay() wird mithilfe der for-Schleife das Problem der unendlichen Rekursion durch die Tatsache, dass für die Berechnung von L(x, wo) immer auch L(x, wi) benötigt wird,
was wiederum einem weiteren L(y, x - y) aus Sicht des nächsten Schnittpunkts entspricht, gelöst. Dieses neue L(y, x - y) wird mithilfe eines durch eine, auf die BxDF abgestimmte, PDF randomisierten abgeprallten Strahls
so lange berechnet, bis eine Lichtquelle getroffen wird und somit Le(x, wo) != 0 an dieser Stelle gilt oder bis maxBounceCount erreicht ist. Auf diesem Weg wird der throughput bei
jedem Schnitt mit dem (gekürzten) Gewicht (BRDF * cosTheta(wi)) / PDF multipliziert, um am Ende mit dem emittierten Licht am Strahlenende multipliziert zu werden, um den Einfluss der BRDFs aller Schnitte 
durchzusetzen. Der Code orientiert sich folglich eher an der Pfadintegralschreibweise nach Veach anstatt der klassischen Rendergleichung.

Diese Samples werden schlussendlich gemäß der Monte Carlo Integration aufsummiert und gemittelt um das endgültige L(x, wo) zu erhalten, was als Pixelfarbe ausgegeben wird.
*/

void main() {
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    // Ein pixel- und frameabhängiger Seed wird aus den Texel-Koordinaten mit dem Frame-Spezifischen Seed gemischt
    uint state = (texelCoord.x * gl_NumWorkGroups.x + texelCoord.y) * frameSeed;
    
    Ray ray = {cam.center, vec3(0.0), 1.0, 0.0};
    
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
    // Ist die Diskriminante kleiner als 0, so gibt es keine Schnittpunkte
    if(discriminant >= 0) {
        // dist entspricht dem Skalar in der Geradengleichung und ist außerdem die Distanz, da ray.dir normiert ist
        float distA = (-b - sqrt(discriminant)) / (2 * a);
        float distB = (-b + sqrt(discriminant)) / (2 * a);
        float dist;

        // Ist dist kleiner als 0, so befindet sich der Schnittpunkt hinter dem Strahl und ist ungültig
        if(distA >= 0.0) {
            dist = distA;
        } else if(distB >= 0.0) {
            dist = distB;
        } else {
            hitInfo.didHit = false;
            return hitInfo;
        }

        hitInfo.didHit = true;
        hitInfo.dist = dist;
        hitInfo.hitPoint = ray.origin + dist * ray.dir;
        hitInfo.material = sphere.material;
        hitInfo.normal = normalize(hitInfo.hitPoint - sphere.center);
        return hitInfo;

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
    
    bool transmissive = triangle.material.transmittance > EPSILON;
    #if defined(CULLING) && defined(CCW_WINDING_ORDER)
    // Ist die Determinante kleiner/größer als 0, ist das Dreieck ein Backface. Ist sie nahe an 0, steht ray.dir parallel zum Dreieck (da basicDet auch = -dot(ray.dir, cross(E1, E2)) und cross(E1, E2) = normal) und es wird discarded. Ist das Dreieck durchsichtig, passiert das nicht
    if(transmissive) {
        if(abs(basicDet) < EPSILON) {
            return hitInfo;
        }
    } else if(basicDet < EPSILON) {
        return hitInfo;
    }
    #endif
    #if defined(CULLING) && defined(CW_WINDING_ORDER)
    if(transmissive) {
        if(abs(basicDet) < EPSILON) {
            return hitInfo;
        }
    } else if(basicDet > EPSILON) {
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
    closestHit.material.albedoSpectrumCoeffs = vec3(0.0, 0.0, 0.0);

    for(int i = 0; i < SPHERE_COUNT; i++) {
        Sphere sphere = {sphereSpatialData[i].xyz, sphereSpatialData[i].w, sphereMaterials[i]};
        HitInfo hitInfo = intersectionSphere(ray, sphere);
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
        int meshOffset = meshOffsetsAndVertexCounts[i].x;
        int meshVertexCount = meshOffsetsAndVertexCounts[i].y;

        // Es wird über die Vertices des Meshes iteriert und je 3 Vertices der Möller-Trumbore-Algorithmus mit dem transformierten, Model-lokalen Ray durchgeführt
        Ray localRay = {localRayOrigin, localRayDir, ray.currentIOR, ray.distInMaterial};
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
float absCosTheta(vec3 w) {
    return abs(w.z);
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
    vec3 up = (abs(n.z) < 0.9) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    t = normalize(cross(up, n));
    b = cross(t, n);
}

bool sameHemisphere(vec3 w1, vec3 w2) {
    return w1.z * w2.z > 0.0;
}

// Quetscht das Albedo Spektrum in den Bereich [0;1], um die Energieerhaltung zu gewährleisten
float S(float x) {
    return 0.5 + (x / (2 * sqrt(1 + SQR(x))));
}

float evalAlbedoSpectrum(vec3 albedoSpectrumCoeffs, float lambda) {
    return S(albedoSpectrumCoeffs.x * SQR(lambda) + albedoSpectrumCoeffs.y * lambda + albedoSpectrumCoeffs.z);
}

vec3 traceRay(Ray ray, inout uint state) {

    vec3 throughput = vec3(1.0); // throughput entspricht einem Faktor, der den Grad der Modifierung des Lichts einer später getroffenen Lichtquelle über den Strahlenweg beschreibt. Er bestimmt maßgeblich den Einfluss der BRDF auf incomingLight
    vec3 incomingLight = vec3(0.0); // incomingLight entspricht L(erster Schnittpunkt, -ray.dir), also dem Licht, das vom ersten Schnittpunkt Richtung Kamera emittiert und reflektiert wird 

    for(int i = 0; i < maxBounceCount; i++) {

        HitInfo hitInfo = calculateClosestHit(ray);

        if(hitInfo.didHit) {
            
            Material material = hitInfo.material;
            
            // Ist das Material eine Lichtquelle?
            if(material.emissionSpectrum[0] >= -EPSILON) {
                vec3 emittedLight = vec3(material.emissionSpectrum[0], material.emissionSpectrum[1], material.emissionSpectrum[2]); // emittedLight entspricht dem Le(hitInfo.hitPoint, -ray.dir) des getroffenen Punktes
                incomingLight += throughput * emittedLight;
                break;
            }

            // Beobachtungsrichtung
            vec3 woGlobal = -ray.dir;

            // Makrooberflächennormale
            vec3 geometricNormal = hitInfo.normal;
            bool frontface = dot(geometricNormal, woGlobal) > 0.0;
            // ausgerichtete Normale
            vec3 n = geometricNormal;
            if(!frontface) {
                n = -n;
            }

            // wo wird für die Berechnungen in den Normalenraum transformiert
            vec3 t, b;
            buildOrthonormalBasis(n, t, b);
            mat3 localToGlobal = mat3(t, b, n);
            //      transpose entspricht hier inverse, da die Spaltenvektoren orthonormal sind
            vec3 wo = transpose(localToGlobal) * woGlobal;

            // Die wahrgenommene Rauheit wird in die tatsächliche Rauheit alpha umgewandelt
            vec2 alpha = max(SQR(material.roughness), vec2(EPSILON));
            // Es wird eine zufällige Mikrofazettennormale generiert
            vec3 wm = sampleWm(wo, alpha, state);

            vec3 albedo = vec3(evalAlbedoSpectrum(material.albedoSpectrumCoeffs, 700.0), evalAlbedoSpectrum(material.albedoSpectrumCoeffs, 540.0), evalAlbedoSpectrum(material.albedoSpectrumCoeffs, 460.0));
            // BxDF und PDF aus https://pbr-book.org/4ed/Reflection_Models/Roughness_Using_Microfacet_Theory; Siehe Notizen für gekürzte Throughput-Weight Herleitungen
            if(material.transmittance < EPSILON) {
                // BRDF

                // Basisspekularreflexionsanteil bei Einfallswinkel 90°
                vec3 F0 = mix(vec3(0.04), albedo, material.metallic);
                // Fresnel-Wert, also realistischer Spekularreflexionsanteil abhängig vom Einfallswinkel, Schlick-Approximation
                vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - min(dot(wm, wo), 1.0 - EPSILON), 5.0);

                float pSpecular = dot(F, vec3(0.2126, 0.7152, 0.0722));

                if(pSpecular < randf(state)) {
                    // Diffuse-Anteil nach Lambert
                    ray.dir = normalize(geometricNormal + randomOnUnitSphere(state));
                    throughput *= albedo * (1.0 - F) / ((1.0 - pSpecular));
                    
                } else {
                    // Specular-Anteil nach Torrance-Sparrow
                    // Lichteinfallsrichtung
                    vec3 wi = reflect(-wo, wm);    
                    if(!sameHemisphere(wo, wi)) return vec3(0.0);

                    throughput *= (F * dot(wo, wm) * G(wi, wo, alpha)) / (G1(wo, alpha) * max(dot(wo, wm), 0.0) * pSpecular);

                    // wi wird aus dem Normalenraum wieder in den globalen Raum transformiert
                    ray.dir = normalize(localToGlobal * wi);
                }

                ray.origin = hitInfo.hitPoint + geometricNormal * EPSILON;

            } else {
                // BSDF
                float etao = ray.currentIOR;
                float etai = (frontface) ? material.IOR : 1.0;
                float eta = etao / etai;

                vec3 wiRefracted = refract(-wo, wm, eta);
                bool TIR = dot(wiRefracted, wiRefracted) < EPSILON;

                float F;
                // Abfangen von möglichen Totalreflexionen (TIR)
                if(TIR) {
                    F = 1.0;
                } else {
                    float F0 = pow((etai - etao) / (etai + etao), 2.0);
                    F = F0 + (1.0 - F0) * pow(1.0 - min(dot(wm, wo), 1.0 - EPSILON), 5.0);
                }

                vec3 wi;
                if(F < randf(state)) {
                    wi = wiRefracted;
                    throughput *= (G(wi, wo, alpha) * abs(dot(wi, wm))) / (G1(wo, alpha) * max(dot(wo, wm), 0.0));
                    ray.currentIOR = etai;
                } else {
                    wi = reflect(-wo, wm);
                    throughput *= (dot(wo, wm) * G(wi, wo, alpha)) / (G1(wo, alpha) * max(dot(wo, wm), 0.0));
                }
                
                vec3 wiGlobal = normalize(localToGlobal * wi);
                ray.dir = wiGlobal;
                // Je nachdem, ob der Strahl innerhalb oder außerhalb eines Objekts weiterfliegt, wird der Startpunkt zur Verhinderung von Fehlern durch Fließkommaungenauigkeiten modifiziert
                ray.origin = (dot(wiGlobal, geometricNormal) < 0.0) ? hitInfo.hitPoint - geometricNormal * EPSILON : hitInfo.hitPoint + geometricNormal * EPSILON;
            }


        } else {
            break;
        }
    }
    return incomingLight;
}