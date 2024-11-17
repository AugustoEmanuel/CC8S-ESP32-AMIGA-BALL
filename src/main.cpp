#include <Arduino.h>
#include "base/graphics.hpp"
#include "base/n64controller.hpp"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "base/sprite.hpp"
#include "res/ball.hpp"
#include "res/ball_shadow.hpp"
#include <WiFi.h>
#include "res/bounce.hpp"

#define PI 3.14159265359
#define RED 0xFF0000
#define WHITE 0xFFFFFF
#define ROTATE_ANGLE 0.5

// Define the sprite size and other constants
#define BALL_WIDTH 61
#define BALL_HEIGHT 61

// Global variables
int ballSprite;
int white_start = 1;
float ball_x = 5.0;
float parabel_x = 0.0;
int ball_dir = 1;
float parabel_dir = 9.25;
int shadowX, shadowY;
const int dacPin = 25;
int bounceIndex = sizeof(bounce);
// unsigned int the_ball[BALL_WIDTH * BALL_HEIGHT]; // Placeholder for sprite data

// Function declarations
// void background();
// void cycle_palette();
// void plot_line(float x);
// void update(float delta);
// void load_blank(const char* type, int width, int height);

// // Simulate loading a blank sprite (you would load a real image here)
// void load_blank(const char* type, int width, int height) {
//     // In an actual program, you would load an image or initialize an array for the sprite
//     printf("Loading sprite of type %s with size %dx%d\n", type, width, height);
// }

// Function to check if a pixel (px, py) is inside or on the edge of a circle
inline bool isPixelOverlappingCircle(int px, int py, int cx, int cy, int radius) {
    // Calculate the squared distance between the pixel and the circle's center
    int dx = px - cx;
    int dy = py - cy;
    int distanceSquared = dx * dx + dy * dy;

    // Check if the distance squared is less than or equal to radius squared
    return distanceSquared <= radius * radius;
}

inline void drawPixelForCircle(int x, int y){
    if(x >= 32 && x <= 288 && y <= 208 && (x%8 == 0 || y%8 == 0)){
        GFX::setPixel(x, y, 11);
        return;
    }
    GFX::setPixel(x, y, 9);
}

void drawCircle(int centerX, int centerY, int radius) {
    int x = 0;
    int y = radius;
    int d = 1 - radius;

    // Draw lines between points to fill the circle
    while (x <= y) {
        // Draw horizontal lines between the points in each octant
        for (int i = centerX - x; i <= centerX + x; i++) {
            drawPixelForCircle(i, centerY + y);
            drawPixelForCircle(i, centerY - y);
        }
        for (int i = centerX - y; i <= centerX + y; i++) {
            drawPixelForCircle(i, centerY + x);
            drawPixelForCircle(i, centerY - x);
        }

        // Update decision parameter and coordinates
        if (d < 0) {
            d = d + 2 * x + 3;
        } else {
            d = d + 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void drawLine(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        GFX::setPixel(x0, y0, isPixelOverlappingCircle(x0, y0, shadowX, shadowY, 64) ? 11 : 10);  // Set the pixel at (x0, y0)

        // If the start point equals the end point, we're done
        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Function to simulate drawing a background
void drawBackground() {
    // Placeholder function to simulate drawing a background
    // You would typically use graphics API calls here

    // Drawing grid-like background
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            // Simulate drawing a rectangle (in real code, you would call a graphics function here)

            if(x >= 32 && x <= 288 && y <= 208 && (x%8 == 0 || y%8 == 0)){
                GFX::setPixel(x, y, 10);
                continue;
            }
            GFX::setPixel(x, y, 8);
            // printf("Drawing rectangle at (%d, %d)\n", x, y);
        }
    }
}


// // Function to cycle palette colors (simplified version)
uint64_t palTimer = 0;
void cycle_palette() {
    if(esp_timer_get_time() - palTimer < 30000) return;

    uint16_t * palPtr = GFX::getPalettePtr();
    
    if(ball_dir < 0){
        uint16_t temp = *palPtr;
        for (int ci = 0; ci < 7; ci++) {
            *(palPtr + ci) = *(palPtr + ci + 1);
        }
        *(palPtr + 7) = temp;
    }
    else{
        uint16_t temp = *(palPtr + 7); // Store the last element
        for (int ci = 7; ci > 0; ci--) {
            *(palPtr + ci) = *(palPtr + ci - 1);
        }
        *palPtr = temp;
    }
    palTimer = esp_timer_get_time();
}

// Main game loop update function
void update(float delta) {
    drawBackground();

    parabel_x += parabel_dir * delta;
    float ball_y = parabel_x * parabel_x * 3;
    if (ball_y >= 72.0) {
        ball_y = 72.0;
        parabel_x = sqrt(ball_y / 3);
        parabel_dir = -parabel_dir;
    }

    ball_y += 32;

    if (ball_x <= 16) {
        ball_x = 16;
        ball_dir = -ball_dir;
        bounceIndex = 44;
    } else if (ball_x >= 176) {
        ball_x = 176;
        ball_dir = -ball_dir;
        bounceIndex = 44;
    }

    ball_x += ball_dir * delta * 25;
    // cycle_palette();

    if (ball_y >= 72.0) {
        bounceIndex = 44;
    }

    SPR::setPosX(ballSprite, ball_x);
    SPR::setPosY(ballSprite, ball_y);

    shadowX = ball_x + 96, shadowY = ball_y + 64;
    drawCircle(shadowX, shadowY, 64);

    int step = -16;
    for(int i = 32; i <= 288; i++){
        if(i%8 != 0) continue;
        drawLine(i, 208, i + step, 236);
        step += 1;
    }

    int x0 = 32;
    int x1 = 288;
    int y = 208;
    step = 4;
    for(int i = 0; i < 3; i++){
        y  += step * (i+1);

        drawLine(x0 - (step * (i+1)), y, x1 + (step * (i+1)), y);
    }
}

void loop2(void * p);
void playOnDac(void * p);
void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(921600);
    N64C::init();
    N64C::checkForFactoryStart();
    GFX::init();
    GFX::clearFrameBuffer();
    SPR::init();

    ballSprite = SPR::addSprite(&ball_sprite, 0, 0);
    uint16_t * palPtr = GFX::getPalettePtr();
    for(int i = 0; i < 4; i++){
        *(palPtr+i) = 0x1F;
    }

    for(int i = 4; i < 8; i++){
        *(palPtr+i) = 0x3FFF;
    }

    *(palPtr+10) = 0x3C1F;
    *(palPtr+9) = 0x2318;
    *(palPtr+8) = 0x3BDE;
    *(palPtr+11) = 0x3018;

    xTaskCreatePinnedToCore(
        loop2,         // Task function
        "PALCYCLE",       // Task name
        4096,                      // Stack size
        NULL,                      // Parameter passed to the task
        23,                         // Priority
        NULL,     // Task handle
        0                          // Core 1
    );
}

void loop() {
    update(0.07f);
    SPR::update();
    GFX::executeRoutines();
    GFX::updateScreen();
}

void loop2(void * p){
    while(true){
        cycle_palette();
        vTaskDelay(1);
    }
}
