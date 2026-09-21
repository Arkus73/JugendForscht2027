# Jugend Forscht Projekt 2027 Log und TODO

## TODO:

### Path Tracer Features:
- [ X ] Grobe Erstimplementierung (ID: 0)
- [  ] Erweiterung der Model-Pipeline zum Laden von Modellen aus Dateien (ID: 5)

### Path Tracer Optimierung
- [  ] Erweiterung des AABB-Culling zu einer BVH (ID: 1)
- [  ] Implementierung eines kantenerhaltenden Denoisers (A-Trous oder Bilateral? Siehe auch ID 3) (ID: 4)
- [  ] Schauen, ob Werte für den/im Raytracer zu oft berechnet werden oder diese vorberechnet werden können (ID: 7)
- [ X ] Implementierung von VNDF Importance Sampling (ID: 10)
- [  ] Erweiterung des Importance Sampling zu MIS mit NEE (ID: 14)

### [ X ] Path Tracer PBR Materialien
- [ X ] Einfaches Cosine-Weighted Sampling (ID: 6)
- [ X ] Fresnel implementieren (ID: 9)
- [ X ] Materialeigenschaften zu albedo, roughness, metallic erweitern (ID: 11)
- [ X ] Eine verbesserte, physikalisch korrekte BRDF implementieren (Torrance-Sparrow) (ID: 12)
- [ X ] Lichtbrechende, transmissive Materialien durch eine BSDF simulieren (ID: 13)

### Erweiterung zum spektralen Path Tracing

### Recherche
- [  ] Bei der THWS Schweinfurt Bib Ausweis beantragen (ID: 2)
    - [  ] Raytracing Gems ausleihen (ID: 3)
- [ X ] Recherche zur Rendering Equation (ID: 8)
- [ X ] Recherche zur Theorie und Implementierung des spektralen Path Tracings (ID: 15)
- [  ] Recherche zu 


## LOGS:

### [Bis 2026-08-16] - Start und grobe Implementierung des Raytracers - [TODO ID 0]

- Rendering Features:
    - Intersection Funktionen (Ray-AABB/Sphäre/Dreieck (Möller-Trumbore Algorithmus))
    - Schattierung und Path-Tracing (-> traceRay())
    - Specular Reflections durch smoothness Komponente im Material-Struct und Modifizierung des Reflexionscodes in traceRay()
    - Anti-Aliasing durch Sub-Pixel Jittering

- Engine und Pipeline:
    - OpenGL Setup
    - Hilfsfunktions zum Speichern mehrerer Renders (für Video-Rendering oder Trainingsdaten-Generierung)
    - C-Structs für Materialien, Sphären und Models
    - Model-Pipeline
    
- Optimierung des Raytracers:
    - Backface-Culling
    - Triangle Intersection im Model- statt Worldspace aufgrund der massiven Rechenlast durch Modelmatrix-Multiplikationen mit den Dreiecks-Vertices
    - AABB-Culling und dessen integration in die Model-Pipeline

### [2026-08-17 bis 2026-08-23] - Tieferes mathematisches Verständnis und eine physikalisch korrekte BRDF - [TODO ID 6, 8, 11, 9, 12, 10]

- Verringerung des Rauschens und somit Verbesserung der Bildqualität, sowie Verbesserung der Performance durch die Implementierung eines einfachen Cosine-Weighted Sampling Verfahrens
- Recherche und Verständnis der Rendergleichung und Verbindung mit dem Code durch Kommentare
- Vorbereitung des Codes für die Cook Torrance BRDF durch erweiterte Materialeigenschaften und den Fresnel und Verbesserung des vereinfachten Modells durch diese
- Implementierung der Cook Torrance BRDF mit VNDF Importance Sampling, anfängliche Schwierigkeiten mit sampleWm, aber schließlich Übernahme der Implementierung aus dem offiziellen Paper

### [2026-08-25 bis 2026-09-21] - Erweiterung der BRDF zur BSDF - [TODO ID 13, 15]
- Erweiterung der Materialeigenschaften mit transmittance und IOR zur Vorbereitung auf die BSDF
- Sauberere Sphere-Pipeline, nun ähnlich zur Model-Pipeline mit einer SSBO gehandelt
- Implementierung einer BTDF und scheitern aufgrund von Versuch der Schachtelung. Funktioniert mit sauberer Trennung in opake (BRDF) und transmissive (BSDF) Objekte.
- Korrekte Modifizierung des throughputs durch gekürzte Gewichte
- Unerwartet schwache Refraktion bei Sphäre und anscheinend kein Austritt des Lichtstrahls? -> Backface-Intersection bei Sphären implementiert
- Eigenartig dunkle Akzente bei Glaswürfel. Vielleicht Strahl durch TIRs darin gefangen, bevor? -> Wahrscheinlicher Grund, kein Bug. Phänomen verschwindet bei erhöhter maxBounceCount

### [2026-09-21 bis] - Spektrales Path Tracing - [TODO ID ]
