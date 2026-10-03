#ifndef UTILS_H
#define UTILS_H

#include <GLFW/glfw3.h>
#include <stdbool.h>

#define ASPECT_RATIO (16.0f / 9.0f)
#define SPECTRAL_RESOLUTION (3)
#define MIN_WAVELENGTH (400)
#define MAX_WAVELENGTH (800)
#define WAVELENGTH_STEP ((float)(MAX_WAVELENGTH - MIN_WAVELENGTH) / (float)(SPECTRAL_RESOLUTION - 1.0f))

#define __cleanup(func) __attribute__((cleanup(func)))  // __cleanup bevor einer Pointer-Deklaration sorgt dafür, dass auf einen Pointer zu diesem Pointer die angegebene Funktion wird 
#define __constructor __attribute__((constructor))  // Markiert Funktionen, die automatisch vor main() ausgeführt werden
#define __destructor __attribute__((destructor))    // Markiert Funktionen, die automatisch nach Beendigung des Programms ausgeführt werden
#define __pure __attribute__((pure))    // Markiert Funktionen, die ihre Ausgabe nur aus den Parametern und gelesenem globalen Speicher berechnen und den globalen Speicher nicht verändern
#define __const __attribute__((const))  // Markiert Funktionen, die ihre Ausgabe ausschließlich auch ihren Parametern berechnet und sonst nichts verändert oder liest
#define __noreturn __attribute__((noreturn))    // Markiert Funktionen, die das Programm beenden, also exit() aufrufen

GLFWwindow* InitAndCreateWindow(int windowWidth, int windowHeight, const char* title);  // Initialisiert Glad und erstellt ein Fenster
void default_framebuffer_size_callback(GLFWwindow* window, int width, int height);     // Callback-Funktion bei Window Resize
void __noreturn throwException(char* msg);
float __const clamp(float value, float min, float max);
float randf(float lowerBound, float upperBound);
bool randomise(float probability);
unsigned int createTextureFromFile(const char* dir, int filter);
int __const modulo(int a, int b);

#endif