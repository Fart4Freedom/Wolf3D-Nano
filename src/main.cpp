#include <Arduino.h>
#include <avr/pgmspace.h>

#include "SPI_Display.h"
#include "config.h"

#if defined (E1M1)
#include "E1M1.h"
#elif defined (E1M2)
#include "E1M2.h"
#elif defined(E1M3)
#include "E1M3.h"
#elif defined(E1M4)
#include "E1M4.h"
#elif defined(E1M5)
#include "E1M5.h"
#elif defined(E1M6)
#include "E1M6.h"
#elif defined(E1M7)
#include "E1M7.h"
#elif defined(E1M8)
#include "E1M8.h"
#elif defined(E1M9)
#include "E1M9.h"
#elif defined(E1M10)
#include "E1M10.h"
#endif 


#ifdef USE_16BPP_BUFFER
uint16_t columnBuffer[RAYCASTER_SCREEN_HEIGHT][BUFFER_WIDTH];
#else
uint8_t columnBuffer[RAYCASTER_SCREEN_HEIGHT][BUFFER_WIDTH];
#endif

bool noclipEnabled = false;

struct RaycastResult {
    float dist = 0.0f;
    uint8_t textureId = 0;   // zero-based index into wall_textures[]
    uint8_t side = 0;        // 0 = X/vertical face, 1 = Y/horizontal face
    bool hit = false;
    bool isDoor = false;
    int mapX = 0;
    int mapY = 0;
    int stepX = 0;
    int stepY = 0;
};

float playerX = WL6_PLAYER_X;
float playerY = WL6_PLAYER_Y;
float playerDirX = WL6_PLAYER_DIR_X;
float playerDirY = WL6_PLAYER_DIR_Y;
float planeX =  WL6_PLAYER_PLANE_X;
float planeY = FOV; //WL6_PLAYER_PLANE_Y;
float moveSpeed = 0.3f;
float rotSpeed = 0.3f;

#ifdef DISPLAY_FPS
unsigned long frameStartTime = 0;
unsigned long frameCount = 0;
unsigned long fps = 0;
#endif

SPI_DISPLAY display;

#ifndef WL6_MAX_ACTIVE_DOORS
#define WL6_MAX_ACTIVE_DOORS 12
#endif

#ifndef WL6_DOOR_AUTO_CLOSE_MS
#define WL6_DOOR_AUTO_CLOSE_MS 5000UL
#endif

#ifndef WL6_DOOR_ANIM_MS
#define WL6_DOOR_ANIM_MS 550UL
#endif

#define WL6_DOOR_FULL_OPEN 255
#define WL6_DOOR_PASSABLE_OPEN 240

struct DoorState {
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t open = 0;          // 0 = closed, 255 = fully open
    bool targetOpen = false;
    bool active = false;
    unsigned long openedAt = 0;
};

DoorState doorStates[WL6_MAX_ACTIVE_DOORS];
unsigned long lastDoorUpdateMs = 0;

#if defined(BUTTON_USE)
#define WL6_USE_BUTTON_PIN BUTTON_USE
#elif defined(BUTTON_ACTION)
#define WL6_USE_BUTTON_PIN BUTTON_ACTION
#elif defined(BUTTON_FIRE)
#define WL6_USE_BUTTON_PIN BUTTON_FIRE
#elif defined(BUTTON_A)
#define WL6_USE_BUTTON_PIN BUTTON_A
#elif defined(BUTTON_SELECT)
#define WL6_USE_BUTTON_PIN BUTTON_SELECT
#endif

#ifdef WL6_USE_BUTTON_PIN
bool lastUseButtonDown = false;
#endif

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------
void clearColumnBuffer();
void drawWallColumn(int x, int chunkStart);
void flipBuffer(int chunkStart);
void renderScreen();
void updateUser();
void updateDoors();
bool openDoorInFrontOfPlayer();
bool openDoorAt(uint8_t x, uint8_t y);

// -----------------------------------------------------------------------------
// Map / tile helpers
// -----------------------------------------------------------------------------
inline uint8_t getBlockAt(int x, int y) {
    if (x < 0 || y < 0 || x >= WL6_MAP_W || y >= WL6_MAP_H) {
        return 1; // solid outside map, texture tile 1
    }
    return pgm_read_byte(&world_map[(uint16_t)y * WL6_MAP_W + (uint16_t)x]);
}

inline bool isWallTile(uint8_t tile) {
    return WL6_IS_TEXTURE_TILE(tile);
}

inline bool isDoorTile(uint8_t tile) {
    return WL6_IS_DOOR(tile);
}

inline bool isSpecialSolidTile(uint8_t tile) {
    return tile == WL6_TILE_PUSHWALL ||
           tile == WL6_TILE_EXIT ||
           tile == WL6_TILE_UNKNOWN;
}

inline bool isSolidTile(uint8_t tile) {
    return isWallTile(tile) || isDoorTile(tile) || isSpecialSolidTile(tile);
}

inline uint8_t tileToTexture(uint8_t tile) {
    // world_map stores normal wall texture tiles as 1..WL6_NUM_WALL_TEXTURES.
    // wall_textures[] is zero-based, so subtract 1.
    return tile - 1;
}

inline uint8_t doorToTexture(uint8_t tile) {
#if WL6_TEX_LOCKED_DOOR >= 0
    if (tile == WL6_TILE_DOOR_LOCKED) return (uint8_t)WL6_TEX_LOCKED_DOOR;
#endif
#if WL6_TEX_ELEVATOR_DOOR >= 0
    if (tile == WL6_TILE_DOOR_ELEVATOR) return (uint8_t)WL6_TEX_ELEVATOR_DOOR;
#endif
#if WL6_TEX_GREEN_DOOR >= 0
    return (uint8_t)WL6_TEX_GREEN_DOOR;
#else
    (void)tile;
    return 0;
#endif
}

inline bool isSolidForDoorOrient(uint8_t tile) {
    // Doors sit between solid wall blocks. Do not count doors themselves as walls
    // for orientation, only actual wall/special solid tiles.
    return isWallTile(tile) || isSpecialSolidTile(tile);
}

inline uint8_t getDoorSideFromMap(int mapX, int mapY, uint8_t fallbackSide) {
    bool leftSolid  = isSolidForDoorOrient(getBlockAt(mapX - 1, mapY));
    bool rightSolid = isSolidForDoorOrient(getBlockAt(mapX + 1, mapY));
    bool upSolid    = isSolidForDoorOrient(getBlockAt(mapX, mapY - 1));
    bool downSolid  = isSolidForDoorOrient(getBlockAt(mapX, mapY + 1));

    // Wolf3D door geometry is a thin slab in the CENTER of the door cell.
    // Walls above/below -> vertical slab: |  (hit as side 0 / X-facing).
    if (upSolid && downSolid) return 0;

    // Walls left/right -> horizontal slab: --- (hit as side 1 / Y-facing).
    if (leftSolid && rightSolid) return 1;

    // Some custom maps may not have perfect surrounding walls.
    return fallbackSide;
}

inline bool isPushwallAt(uint8_t x, uint8_t y) {
#if WL6_NUM_PUSHWALLS > 0
    for (uint8_t i = 0; i < WL6_NUM_PUSHWALLS; i++) {
        WL6_Pushwall pw;
        memcpy_P(&pw, &wl6_pushwalls[i], sizeof(WL6_Pushwall));
        if (pw.x == x && pw.y == y) return true;
    }
#else
    (void)x;
    (void)y;
#endif
    return false;
}

inline uint8_t getColorIndex(uint8_t textureX, uint8_t textureY, uint8_t textureId) {
    // WL6.h stores row-major 32x32 indexed textures:
    // wall_textures[(tex * 1024) + y * 32 + x]
    return pgm_read_byte(&wall_textures[
        ((uint16_t)textureId * WL6_TEX_PIXELS) +
        ((uint16_t)textureY * WL6_TEX_W) +
        textureX
    ]);
}

// -----------------------------------------------------------------------------
// Door helpers
// -----------------------------------------------------------------------------
inline DoorState *findDoorState(uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < WL6_MAX_ACTIVE_DOORS; i++) {
        if (doorStates[i].active && doorStates[i].x == x && doorStates[i].y == y) {
            return &doorStates[i];
        }
    }
    return NULL;
}

DoorState *getOrAllocDoorState(uint8_t x, uint8_t y) {
    DoorState *d = findDoorState(x, y);
    if (d) return d;

    for (uint8_t i = 0; i < WL6_MAX_ACTIVE_DOORS; i++) {
        if (!doorStates[i].active) {
            doorStates[i].x = x;
            doorStates[i].y = y;
            doorStates[i].open = 0;
            doorStates[i].targetOpen = false;
            doorStates[i].active = true;
            doorStates[i].openedAt = 0;
            return &doorStates[i];
        }
    }

    // Reuse a fully closed inactive-looking slot if the small cache is full.
    for (uint8_t i = 0; i < WL6_MAX_ACTIVE_DOORS; i++) {
        if (doorStates[i].open == 0 && !doorStates[i].targetOpen) {
            doorStates[i].x = x;
            doorStates[i].y = y;
            doorStates[i].openedAt = 0;
            return &doorStates[i];
        }
    }

    // Cache is full of moving/open doors. Refuse to open another one.
    return NULL;
}

inline uint8_t getDoorOpenByte(int x, int y) {
    if (x < 0 || y < 0 || x >= WL6_MAP_W || y >= WL6_MAP_H) return 0;
    DoorState *d = findDoorState((uint8_t)x, (uint8_t)y);
    return d ? d->open : 0;
}

inline float getDoorOpenAmount(int x, int y) {
    return (float)getDoorOpenByte(x, y) * (1.0f / 255.0f);
}

inline bool canPlayerOpenDoorTile(uint8_t tile) {
    // Later: return false for locked/key doors when the player lacks the key.
    return isDoorTile(tile);
}

inline bool isPlayerInsideTile(uint8_t x, uint8_t y) {
    return ((uint8_t)playerX == x && (uint8_t)playerY == y);
}

bool openDoorAt(uint8_t x, uint8_t y) {
    uint8_t tile = getBlockAt(x, y);
    if (!canPlayerOpenDoorTile(tile)) return false;

    DoorState *d = getOrAllocDoorState(x, y);
    if (!d) return false;

    d->targetOpen = true;
    d->openedAt = millis();
    return true;
}

bool openDoorInFrontOfPlayer() {
    // Use a few samples so the button works both close to the centered door slab
    // and slightly farther back in the corridor.
    const float samples[] = {0.55f, 0.85f, 1.15f, 1.45f};

    for (uint8_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        int x = (int)floorf(playerX + playerDirX * samples[i]);
        int y = (int)floorf(playerY + playerDirY * samples[i]);
        if (x < 0 || y < 0 || x >= WL6_MAP_W || y >= WL6_MAP_H) continue;
        if (isDoorTile(getBlockAt(x, y))) {
            return openDoorAt((uint8_t)x, (uint8_t)y);
        }
    }

    // Fallback: adjacent cardinal tile based on the dominant view axis.
    int x = (int)floorf(playerX);
    int y = (int)floorf(playerY);
    if (fabsf(playerDirX) > fabsf(playerDirY)) {
        x += (playerDirX >= 0.0f) ? 1 : -1;
    } else {
        y += (playerDirY >= 0.0f) ? 1 : -1;
    }

    if (x < 0 || y < 0 || x >= WL6_MAP_W || y >= WL6_MAP_H) return false;
    if (!isDoorTile(getBlockAt(x, y))) return false;
    return openDoorAt((uint8_t)x, (uint8_t)y);
}

void updateDoors() {
    unsigned long now = millis();
    if (lastDoorUpdateMs == 0) lastDoorUpdateMs = now;
    unsigned long dt = now - lastDoorUpdateMs;
    lastDoorUpdateMs = now;

    // Fixed-point animation: 0..255 over WL6_DOOR_ANIM_MS.
    uint16_t animStep = (uint16_t)((dt * 255UL) / WL6_DOOR_ANIM_MS);
    if (animStep == 0 && dt > 0) animStep = 1;
    if (animStep > 255) animStep = 255;

    for (uint8_t i = 0; i < WL6_MAX_ACTIVE_DOORS; i++) {
        DoorState &d = doorStates[i];
        if (!d.active) continue;

        if (d.targetOpen && d.open >= WL6_DOOR_FULL_OPEN) {
            if ((unsigned long)(now - d.openedAt) >= WL6_DOOR_AUTO_CLOSE_MS &&
                !isPlayerInsideTile(d.x, d.y)) {
                d.targetOpen = false;
            }
        }

        if (d.targetOpen) {
            uint16_t v = (uint16_t)d.open + animStep;
            d.open = (v >= WL6_DOOR_FULL_OPEN) ? WL6_DOOR_FULL_OPEN : (uint8_t)v;
        } else {
            if (isPlayerInsideTile(d.x, d.y)) {
                d.open = WL6_DOOR_FULL_OPEN;
                continue;
            }
            if (d.open > animStep) d.open -= (uint8_t)animStep;
            else d.open = 0;

            if (d.open == 0) {
                // Closed door no longer needs an active SRAM slot.
                d.active = false;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Player / movement
// -----------------------------------------------------------------------------

inline bool canMoveTo(float x, float y) {
    if (noclipEnabled) return true;

    if (x - COLLISION_BUFFER < 0 || x + COLLISION_BUFFER >= WL6_MAP_W ||
        y - COLLISION_BUFFER < 0 || y + COLLISION_BUFFER >= WL6_MAP_H) {
        return false;
    }

    int minX = (int)(x - COLLISION_BUFFER);
    int maxX = (int)(x + COLLISION_BUFFER);
    int minY = (int)(y - COLLISION_BUFFER);
    int maxY = (int)(y + COLLISION_BUFFER);

    for (int mapX = minX; mapX <= maxX; mapX++) {
        for (int mapY = minY; mapY <= maxY; mapY++) {
            uint8_t tile = getBlockAt(mapX, mapY);

            if (isDoorTile(tile)) {
                if (getDoorOpenByte(mapX, mapY) < WL6_DOOR_PASSABLE_OPEN) {
                    return false;
                }
                continue;
            }

            if (isWallTile(tile) || isSpecialSolidTile(tile)) {
                float dx = x - (mapX + 0.5f);
                float dy = y - (mapY + 0.5f);
                float distance = sqrtf(dx * dx + dy * dy);

                if (distance < COLLISION_BUFFER + 0.7071f) {
                    return false;
                }
            }
        }
    }
    return true;
}

void updateUser() {
    if (digitalRead(BUTTON_UP) == LOW) {
        float newX = playerX + playerDirX * moveSpeed;
        float newY = playerY + playerDirY * moveSpeed;
        if (canMoveTo(newX, playerY)) playerX = newX;
        if (canMoveTo(playerX, newY)) playerY = newY;
    }

    if (digitalRead(BUTTON_DOWN) == LOW) {
        float newX = playerX - playerDirX * moveSpeed;
        float newY = playerY - playerDirY * moveSpeed;
        if (canMoveTo(newX, playerY)) playerX = newX;
        if (canMoveTo(playerX, newY)) playerY = newY;
    }

    if (digitalRead(BUTTON_LEFT) == LOW) {
        float sinVal = sinf(-rotSpeed);
        float cosVal = cosf(-rotSpeed);
        float oldDirX = playerDirX;
        playerDirX = playerDirX * cosVal - playerDirY * sinVal;
        playerDirY = oldDirX * sinVal + playerDirY * cosVal;
        float oldPlaneX = planeX;
        planeX = planeX * cosVal - planeY * sinVal;
        planeY = oldPlaneX * sinVal + planeY * cosVal;
    }

    if (digitalRead(BUTTON_RIGHT) == LOW) {
        float sinVal = sinf(rotSpeed);
        float cosVal = cosf(rotSpeed);
        float oldDirX = playerDirX;
        playerDirX = playerDirX * cosVal - playerDirY * sinVal;
        playerDirY = oldDirX * sinVal + playerDirY * cosVal;
        float oldPlaneX = planeX;
        planeX = planeX * cosVal - planeY * sinVal;
        planeY = oldPlaneX * sinVal + planeY * cosVal;
    }

#ifdef WL6_USE_BUTTON_PIN
    bool useDown = (digitalRead(WL6_USE_BUTTON_PIN) == LOW);
    if (useDown && !lastUseButtonDown) {
        openDoorInFrontOfPlayer();
    }
    lastUseButtonDown = useDown;
#endif
}

// -----------------------------------------------------------------------------
// Raycaster
// -----------------------------------------------------------------------------
inline RaycastResult traceRay(float rayDirX, float rayDirY) {
    RaycastResult r;

    int mapX = (int)playerX;
    int mapY = (int)playerY;

    float deltaDistX = (rayDirX == 0.0f) ? LARGE_FLOAT : fabsf(1.0f / rayDirX);
    float deltaDistY = (rayDirY == 0.0f) ? LARGE_FLOAT : fabsf(1.0f / rayDirY);

    float sideDistX, sideDistY;
    int stepX, stepY;

    if (rayDirX < 0.0f) {
        stepX = -1;
        sideDistX = (playerX - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0f - playerX) * deltaDistX;
    }

    if (rayDirY < 0.0f) {
        stepY = -1;
        sideDistY = (playerY - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0f - playerY) * deltaDistY;
    }

    for (uint8_t guard = 0; guard < 128; guard++) {
        uint8_t ddaSide;

        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            ddaSide = 0;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            ddaSide = 1;
        }

        uint8_t tile = getBlockAt(mapX, mapY);

        if (isDoorTile(tile)) {
            uint8_t doorSide = getDoorSideFromMap(mapX, mapY, ddaSide);
            float doorOpen = getDoorOpenAmount(mapX, mapY);

            float doorDist = 0.0f;
            float hitAlongDoor = 0.0f;

            if (doorSide == 0) {
                // Vertical centered door slab: | inside the map cell.
                // Plane is x = mapX + 0.5.
                if (fabsf(rayDirX) < 0.00001f) continue;

                float plane = mapX + 0.5f;
                doorDist = (plane - playerX) / rayDirX;
                if (doorDist <= 0.001f) continue;

                float hitY = playerY + doorDist * rayDirY;
                if (hitY < mapY || hitY >= mapY + 1.0f) continue;

                hitAlongDoor = hitY - floorf(hitY);
            } else {
                // Horizontal centered door slab: --- inside the map cell.
                // Plane is y = mapY + 0.5.
                if (fabsf(rayDirY) < 0.00001f) continue;

                float plane = mapY + 0.5f;
                doorDist = (plane - playerY) / rayDirY;
                if (doorDist <= 0.001f) continue;

                float hitX = playerX + doorDist * rayDirX;
                if (hitX < mapX || hitX >= mapX + 1.0f) continue;

                hitAlongDoor = hitX - floorf(hitX);
            }

            // Sliding-door gap.  The visible panel starts after the open amount,
            // so rays through the opened part continue past the door cell.
            if (hitAlongDoor < doorOpen) continue;

            r.dist = fabsf(doorDist);
            r.textureId = doorToTexture(tile);
            r.side = doorSide;
            r.hit = true;
            r.isDoor = true;
            r.mapX = mapX;
            r.mapY = mapY;
            r.stepX = stepX;
            r.stepY = stepY;
            return r;
        }

        if (isWallTile(tile)) {
            float wallDist = (ddaSide == 0)
                ? (mapX - playerX + (1 - stepX) / 2.0f) / rayDirX
                : (mapY - playerY + (1 - stepY) / 2.0f) / rayDirY;

            if (wallDist <= 0.001f) continue;

            r.dist = fabsf(wallDist);
            r.textureId = tileToTexture(tile);
            r.side = ddaSide;
            r.hit = true;
            r.isDoor = false;
            r.mapX = mapX;
            r.mapY = mapY;
            r.stepX = stepX;
            r.stepY = stepY;
            return r;
        }

        if (isSpecialSolidTile(tile)) {
            float wallDist = (ddaSide == 0)
                ? (mapX - playerX + (1 - stepX) / 2.0f) / rayDirX
                : (mapY - playerY + (1 - stepY) / 2.0f) / rayDirY;

            if (wallDist <= 0.001f) continue;

            r.dist = fabsf(wallDist);
            r.textureId = 0;
            r.side = ddaSide;
            r.hit = true;
            r.isDoor = false;
            r.mapX = mapX;
            r.mapY = mapY;
            r.stepX = stepX;
            r.stepY = stepY;
            return r;
        }
    }

    return r;
}

inline bool shouldUseDoorSideTexture(const RaycastResult &hit) {
#if WL6_TEX_DOOR_SIDE < 0
    (void)hit;
    return false;
#else
    if (hit.isDoor) return false;

    int adjX = hit.mapX;
    int adjY = hit.mapY;

    if (hit.side == 0) {
        // Vertical wall face. Check the tile across that face.
        adjX = hit.mapX - hit.stepX;
        adjY = hit.mapY;
    } else {
        // Horizontal wall face. Check the tile across that face.
        adjX = hit.mapX;
        adjY = hit.mapY - hit.stepY;
    }

    uint8_t adjTile = getBlockAt(adjX, adjY);
    if (!isDoorTile(adjTile)) return false;

    uint8_t doorSide = getDoorSideFromMap(adjX, adjY, hit.side);

    // Vertical door slab | touches horizontal wall faces above/below.
    // Horizontal door slab --- touches vertical wall faces left/right.
    return doorSide != hit.side;
#endif
}

void drawWallColumn(int x, int chunkStart) {
    float cameraX = 2.0f * x / (float)RAYCASTER_SCREEN_WIDTH - 1.0f;
    float rayDirX = playerDirX + planeX * cameraX;
    float rayDirY = playerDirY + planeY * cameraX;

    RaycastResult hit = traceRay(rayDirX, rayDirY);
    if (!hit.hit || hit.dist <= 0.01f) return;

    int lineHeight = (int)(RAYCASTER_SCREEN_HEIGHT / hit.dist);
    if (lineHeight <= 0) return;

    int drawStart = max(0, -lineHeight / 2 + RAYCASTER_SCREEN_HEIGHT / 2);
    int drawEnd = min(RAYCASTER_SCREEN_HEIGHT - 1,
                      lineHeight / 2 + RAYCASTER_SCREEN_HEIGHT / 2);

    float wallHitPos = (hit.side == 0)
        ? playerY + hit.dist * rayDirY
        : playerX + hit.dist * rayDirX;
    wallHitPos -= floorf(wallHitPos);

    if (hit.isDoor) {
        // Same sliding offset as traceRay(): the texture moves with the door
        // panel instead of stretching across the whole cell opening.
        wallHitPos -= getDoorOpenAmount(hit.mapX, hit.mapY);
        wallHitPos -= floorf(wallHitPos);
    }

    int textureX = (int)(wallHitPos * WL6_TEX_W);
    if (textureX < 0) textureX = 0;
    if (textureX >= WL6_TEX_W) textureX = WL6_TEX_W - 1;

    if (hit.side == 0 && rayDirX > 0.0f) textureX = WL6_TEX_W - textureX - 1;
    if (hit.side == 1 && rayDirY < 0.0f) textureX = WL6_TEX_W - textureX - 1;

#if WL6_TEX_DOOR_SIDE >= 0
    // Door-side/jamb texture belongs on the neighboring wall face, not as fake
    // strips on the door face.
    if (shouldUseDoorSideTexture(hit)) {
        hit.textureId = (uint8_t)WL6_TEX_DOOR_SIDE;
    }
#endif

    int bufferX = x - chunkStart;
    if (bufferX < 0 || bufferX >= BUFFER_WIDTH) return;

    int textureStep = (WL6_TEX_H << 8) / lineHeight;
    int texturePos = (drawStart - RAYCASTER_SCREEN_HEIGHT / 2 + lineHeight / 2) * textureStep;

    for (int y = drawStart; y <= drawEnd; y++) {
        int textureY = texturePos >> 8;
        if (textureY < 0) textureY = 0;
        if (textureY >= WL6_TEX_H) textureY = WL6_TEX_H - 1;

        uint8_t colorIndex = getColorIndex((uint8_t)textureX,
                                           (uint8_t)textureY,
                                           hit.textureId);
#ifdef USE_16BPP_BUFFER
        columnBuffer[y][bufferX] = WL6_COLOR565(colorIndex);
#else
        columnBuffer[y][bufferX] = colorIndex;
#endif
        texturePos += textureStep;
    }
}

// -----------------------------------------------------------------------------
// Screen / display
// -----------------------------------------------------------------------------
void clearColumnBuffer() {
    for (int x = 0; x < BUFFER_WIDTH; x++) {
        for (int y = 0; y < RAYCASTER_SCREEN_HEIGHT; y++) {
#ifdef USE_16BPP_BUFFER
            uint16_t idx = (y < RAYCASTER_SCREEN_HEIGHT / 2) ? CEILING_COLOR_16 : FLOOR_COLOR_16;
            columnBuffer[y][x] = WL6_COLOR565(idx);
#else
            columnBuffer[y][x] = (y < RAYCASTER_SCREEN_HEIGHT / 2) ? CEILING_COLOR : FLOOR_COLOR;
#endif
        }
    }
}


#ifdef LOW_RES_RENDER_2X
// Write "BUFFER_WIDTH" vertical lines to the screen, scaled by 100%.
void flipBuffer(int chunkStart) {
    for (int x = 0; x < BUFFER_WIDTH; x++) {
        int screenX = chunkStart + x;
        if (screenX >= RAYCASTER_SCREEN_WIDTH) break;

        // We need to scale horizontally as well, so for each column, we write it twice
        for (int scaledX = screenX * 2; scaledX < (screenX * 2 + 2); scaledX++) {
            if (scaledX >= RAYCASTER_SCREEN_WIDTH * 2) break;

            // Set the address window for the scaled column (draw two columns for horizontal scaling)
            display.setViewport(scaledX, 0, scaledX, (RAYCASTER_SCREEN_HEIGHT * 2) - 1);
            

            digitalWrite(TFT_DC, HIGH); // send data
            digitalWrite(TFT_CS, LOW);  // select display

            // Write each pixel twice (vertically scaled)
            for (int y = 0; y < RAYCASTER_SCREEN_HEIGHT; y++) {
#ifdef USE_16BPP_BUFFER                  
                uint16_t color = columnBuffer[y][x];
#else
                uint16_t color = pgm_read_word(wolf3d_palette_rgb565 +  columnBuffer[y][x]);
#endif 
                // Write each pixel twice vertically
                for (int scaledY = y * 2; scaledY < (y * 2 + 2); scaledY++) {
                    // Send the color to the display
                    SPDR = color >> 8;  // Send upper byte
                    while (!(SPSR & (1 << SPIF)));  // Wait for transmission to complete
                    SPDR = color & 0xFF;  // Send lower byte
                    while (!(SPSR & (1 << SPIF)));  // Wait for transmission to complete
                }
            }

            digitalWrite(TFT_CS, HIGH); // deselect display
        }
    }
}
#else
// Write "BUFFER_WIDTH" columns to the screen. 
void flipBuffer(int chunkStart) {
    for (int x = 0; x < BUFFER_WIDTH; x++) {
        int screenX = chunkStart + x;
        if (screenX >= VIEWPORT_WIDTH) break;

        // Set the address window for the entire column
        display.setViewport(screenX, 0, screenX, RAYCASTER_SCREEN_HEIGHT - 1);
     //   digitalWrite(TFT_DC, HIGH); // send data
     //   digitalWrite(TFT_CS, LOW);  // select display
       
        PORTB |=  (1 << PB0); 
        PORTD &= ~(1 << PD7); 

        //write the pixel to the display 
#ifdef FLIPP_Y_IN_SOFTWARE
        for (int y = RAYCASTER_SCREEN_HEIGHT - 1; y >= 0; --y) 
#else        
        for (int y = 0; y < RAYCASTER_SCREEN_HEIGHT; y++) 
#endif 
        {
#ifdef USE_16BPP_BUFFER            
            uint16_t color = columnBuffer[y][x];
#else
            uint16_t color = pgm_read_word(wolf3d_palette_rgb565 +  columnBuffer[y][x]);
#endif             
            SPDR = color >> 8;
            while (!(SPSR & (1 << SPIF)));
            SPDR = color & 0xFF;
            while (!(SPSR & (1 << SPIF)));
        }

       // digitalWrite(TFT_CS, HIGH); // deselect display
           PORTD |= (1 << PD7); 

    }
}
#endif


void renderScreen() {
    for (int chunkStart = 0; chunkStart < RAYCASTER_SCREEN_WIDTH; chunkStart += BUFFER_WIDTH + COLUMN_SKIP) {
        int chunkEnd = min(chunkStart + BUFFER_WIDTH, RAYCASTER_SCREEN_WIDTH);

        clearColumnBuffer();

        for (int x = chunkStart; x < chunkEnd; x++) {
            drawWallColumn(x, chunkStart);
        }

        flipBuffer(chunkStart);
    }
}

// -----------------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------------
void setup() {
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_LEFT, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT, INPUT_PULLUP);
#ifdef WL6_USE_BUTTON_PIN
    pinMode(WL6_USE_BUTTON_PIN, INPUT_PULLUP);
#endif

    display.begin();
    display.clearScreen();

#ifdef DISPLAY_FPS
    frameStartTime = millis();
#endif

   // initPlayer();
}

void loop() {
#ifdef DISPLAY_FPS
    frameCount++;
    if (millis() - frameStartTime >= 1000) {
        fps = frameCount;
        frameCount = 0;
        frameStartTime = millis();
    }
#endif
    
    updateDoors();
    openDoorInFrontOfPlayer();
    updateUser();
    renderScreen();

#ifdef DISPLAY_FPS
    display.clearRect(0, SCREEN_HEIGHT - 7, 50, 7);
    display.setCursor(0, SCREEN_HEIGHT - 7);
    display.setTextColor(TFT_GREEN);
    display.setTextSize(1);
    display.print("FPS: ");
    display.print(fps);
#endif

#ifdef DISPLAY_DEBUG_POS
    display.clearRect(0, FPS_Y, 80, 7);
    display.setCursor(0, FPS_Y); 
    display.setTextColor(TFT_GREEN); 
    display.setTextSize(1);
    display.print("x: ");
    display.print((int)playerX);
    display.print(" y: ");
    display.print((int)playerY);
 
#endif

}
