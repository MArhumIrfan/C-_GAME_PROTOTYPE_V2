#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <ctime>
#include <stack>

constexpr int CHAR_W = 8;
constexpr int CHAR_H = 8;
constexpr int TOTAL_COLS = 100;
constexpr int ROWS = 60;
constexpr int NATIVE_WIDTH = TOTAL_COLS * CHAR_W;  // 800
constexpr int NATIVE_HEIGHT = ROWS * CHAR_H;       // 480

struct ResolutionPreset {
    int width;
    int height;
    std::string label;
};

const std::vector<ResolutionPreset> RESOLUTION_PRESETS = {
    { 800,  480, "800x480 (1X)" },
    { 1280, 720, "1280x720 (HD)" },
    { 1366, 768, "1366x768 (WXGA)" },
    { 1600, 960, "1600x960 (2X)" },
    { 1920, 1080, "1920x1080 (FHD)" }
};

constexpr double FIXED_TIMESTEP = 1000.0 / 60.0;

constexpr int MAP_W = 27;
constexpr int MAP_H = 27;

// --- ELEVATION-BASED DYNAMIC PALETTES (Dark -> Bright) ---
// Ground Level (0.0): Deep Dark Forest Greens
constexpr uint32_t TIER_LOW_BRIGHT  = 0xFF16A34A;
constexpr uint32_t TIER_LOW_MID     = 0xFF15803D;
constexpr uint32_t TIER_LOW_DARK    = 0xFF14532D;

// Mid Incline / Stairs (0.5): Vibrant Cyan
constexpr uint32_t TIER_MID_BRIGHT  = 0xFF38BDF8;
constexpr uint32_t TIER_MID_MID     = 0xFF0284C7;
constexpr uint32_t TIER_MID_DARK    = 0xFF0369A1;

// High Platform / Overpass (1.0+): High-Illumination Neon Lime
constexpr uint32_t TIER_HIGH_BRIGHT = 0xFF86EFAC;
constexpr uint32_t TIER_HIGH_MID    = 0xFF4ADE80;
constexpr uint32_t TIER_HIGH_DARK   = 0xFF22C55E;

constexpr uint32_t RED_GOAL_BRIGHT  = 0xFFF43F5E;
constexpr uint32_t RED_GOAL_DARK    = 0xFFBE123C;

constexpr int AUDIO_SAMPLE_RATE = 44100;
constexpr int AUDIO_BUFFER_SIZE = 1024;

enum GameState {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_SUCCESS,
    STATE_GAMEOVER
};

enum Difficulty {
    DIFF_NORMAL = 0,
    DIFF_EASY   = 1
};

const uint8_t FONT_8X8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, {0x00,0x66,0xAC,0xD8,0x36,0x6A,0x00,0x00},
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00},
    {0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00}, {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    {0x7C,0xC6,0x06,0x1C,0x30,0x66,0xFE,0x00}, {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00},
    {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00}, {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00},
    {0x7C,0xC6,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, {0xFE,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00},
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00}, {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00},
    {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x78,0x00}, {0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
    {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00}, {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},
    {0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00}, {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},
    {0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00}, {0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00},
    {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00}, {0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00},
    {0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00}, {0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00},
    {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
    {0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00}, {0x7C,0xC6,0xC6,0xC6,0xC6,0xCE,0x7C,0x06},
    {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, {0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00},
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
    {0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
    {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    {0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00}, {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}
};

struct Point { int x, y; };

struct MapCell {
    int wallType = 0;      // 0 = Air, 1 = Wall, 2 = Goal
    float floorH = 0.0f;   // 0.0 = Ground, 0.5 = Stairs, 1.0 = Overpass
    float ceilH = 2.0f;
    bool isStairs = false;
};

struct AudioState {
    float ambientPhase = 0.0f;
    float heartbeatPhase = 0.0f;
    float monsterPhase = 0.0f;
    float sanity = 100.0f;
    float monsterDist = 20.0f;
    bool isChasing = false;
    bool inGame = false;
};

void audioCallback(void* userdata, Uint8* stream, int len) {
    AudioState* audio = static_cast<AudioState*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    int samples = len / sizeof(int16_t);

    for (int i = 0; i < samples; ++i) {
        if (!audio->inGame) {
            buffer[i] = 0;
            continue;
        }

        audio->ambientPhase += (42.0f * 2.0f * 3.14159265f) / AUDIO_SAMPLE_RATE;
        if (audio->ambientPhase > 2.0f * 3.14159265f) audio->ambientPhase -= 2.0f * 3.14159265f;
        float ambient = std::sin(audio->ambientPhase) * 0.08f;

        float heartBPM = 1.0f + (100.0f - audio->sanity) / 100.0f * 2.0f;
        audio->heartbeatPhase += (heartBPM * 2.0f * 3.14159265f) / AUDIO_SAMPLE_RATE;
        if (audio->heartbeatPhase > 2.0f * 3.14159265f) audio->heartbeatPhase -= 2.0f * 3.14159265f;

        float beatEnv = 0.0f;
        float cyclePos = audio->heartbeatPhase / (2.0f * 3.14159265f);
        if (cyclePos < 0.15f) beatEnv = std::sin(cyclePos / 0.15f * 3.14159265f);
        else if (cyclePos > 0.22f && cyclePos < 0.35f) beatEnv = std::sin((cyclePos - 0.22f) / 0.13f * 3.14159265f) * 0.7f;

        float heartbeat = std::sin(audio->heartbeatPhase * 40.0f) * beatEnv * (0.35f + (100.0f - audio->sanity) / 100.0f * 0.50f);

        float monsterAudio = 0.0f;
        if (audio->monsterDist < 10.0f) {
            float proxVol = 1.0f - (audio->monsterDist / 10.0f);
            float breathFreq = audio->isChasing ? 2.5f : 0.8f;
            audio->monsterPhase += (breathFreq * 2.0f * 3.14159265f) / AUDIO_SAMPLE_RATE;
            if (audio->monsterPhase > 2.0f * 3.14159265f) audio->monsterPhase -= 2.0f * 3.14159265f;

            float noise = ((rand() % 2000) / 1000.0f - 1.0f);
            monsterAudio = noise * (std::sin(audio->monsterPhase) * 0.5f + 0.5f) * proxVol * 0.4f;
        }

        buffer[i] = static_cast<int16_t>(std::clamp(ambient + heartbeat + monsterAudio, -1.0f, 1.0f) * 32767.0f);
    }
}

class WalkAsciiElevationEngine {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* screenTexture = nullptr;
    SDL_AudioDeviceID audioDevice = 0;
    std::vector<uint32_t> pixelBuffer;
    bool isRunning = false;

    GameState currentState = STATE_TITLE;
    Difficulty currentDifficulty = DIFF_NORMAL;
    int currentResIndex = 2;
    int menuCursor = 0;

    AudioState audioState;
    MapCell worldMap[MAP_H][MAP_W];
    Point startPos;
    Point endPos;

    int currentLevel = 1;
    int totalSteps = 0;
    float levelTime = 0.0f;
    std::string deathReason = "";

    struct Player {
        float posX = 1.5f;
        float posY = 1.5f;
        float posZ = 0.0f;
        float targetPosZ = 0.0f;
        float eyeHeight = 0.5f;
        float pitch = 0.0f;

        float dirX = 1.0f;
        float dirY = 0.0f;
        float planeX = 0.0f;
        float planeY = 0.66f;
        float moveSpeed = 3.2f;
        float mouseSensitivity = 0.0022f;

        int forward = 0; // W/S (+1 / -1)
        int strafe = 0;  // A/D (-1 / +1)
        float stepAccumulator = 0.0f;
        
        float sanity = 100.0f;
        float health = 100.0f;
        bool takingDamage = false;
    } player;

    struct Monster {
        float x = 12.5f;
        float y = 12.5f;
        float speed = 1.8f;
        bool isChasing = false;
    } stalker;

    void updateWindowScale() {
        if (window) {
            int targetW = RESOLUTION_PRESETS[currentResIndex].width;
            int targetH = RESOLUTION_PRESETS[currentResIndex].height;
            SDL_SetWindowSize(window, targetW, targetH);
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    void setCaptureMouse(bool capture) {
        SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
    }

    uint32_t getElevationColor(float elevation, float dist, int side) {
        uint32_t cBright, cMid, cDark;

        if (elevation >= 0.85f) {
            cBright = TIER_HIGH_BRIGHT;
            cMid    = TIER_HIGH_MID;
            cDark   = TIER_HIGH_DARK;
        } else if (elevation >= 0.35f) {
            cBright = TIER_MID_BRIGHT;
            cMid    = TIER_MID_MID;
            cDark   = TIER_MID_DARK;
        } else {
            cBright = TIER_LOW_BRIGHT;
            cMid    = TIER_LOW_MID;
            cDark   = TIER_LOW_DARK;
        }

        if (dist < 3.0f)       return (side == 0) ? cBright : cMid;
        else if (dist < 6.5f)  return (side == 0) ? cMid : cDark;
        else                   return cDark;
    }

    void generateMazeWithOverpass() {
        for (int r = 0; r < MAP_H; ++r) {
            for (int c = 0; c < MAP_W; ++c) {
                worldMap[r][c].wallType = 1;
                worldMap[r][c].floorH = 0.0f;
                worldMap[r][c].ceilH = 2.0f;
                worldMap[r][c].isStairs = false;
            }
        }

        std::stack<Point> stack;
        startPos = { 1, 1 };
        worldMap[startPos.y][startPos.x].wallType = 0;
        stack.push(startPos);

        const int dx[4] = { 0, 0, 2, -2 };
        const int dy[4] = { -2, 2, 0, 0 };

        while (!stack.empty()) {
            Point curr = stack.top();
            std::vector<int> dirs;

            for (int i = 0; i < 4; ++i) {
                int nx = curr.x + dx[i];
                int ny = curr.y + dy[i];
                if (nx > 0 && nx < MAP_W - 1 && ny > 0 && ny < MAP_H - 1) {
                    if (worldMap[ny][nx].wallType == 1) dirs.push_back(i);
                }
            }

            if (!dirs.empty()) {
                int d = dirs[rand() % dirs.size()];
                worldMap[curr.y + dy[d] / 2][curr.x + dx[d] / 2].wallType = 0;
                worldMap[curr.y + dy[d]][curr.x + dx[d]].wallType = 0;
                stack.push({ curr.x + dx[d], curr.y + dy[d] });
            } else {
                stack.pop();
            }
        }

        int midX = MAP_W / 2;
        int midY = MAP_H / 2;

        for (int x = midX - 3; x <= midX + 3; ++x) {
            worldMap[midY][x].wallType = 0;
            worldMap[midY][x].floorH = 1.0f;
            worldMap[midY][x].ceilH = 3.0f;
        }

        worldMap[midY][midX - 4].wallType = 0;
        worldMap[midY][midX - 4].floorH = 0.5f;
        worldMap[midY][midX - 4].isStairs = true;

        worldMap[midY][midX + 4].wallType = 0;
        worldMap[midY][midX + 4].floorH = 0.5f;
        worldMap[midY][midX + 4].isStairs = true;

        endPos = { MAP_W - 2, MAP_H - 2 };
        worldMap[endPos.y][endPos.x].wallType = 2;

        player.posX = startPos.x + 0.5f;
        player.posY = startPos.y + 0.5f;
        player.posZ = 0.0f;
        player.targetPosZ = 0.0f;
        player.pitch = 0.0f;
        player.dirX = 1.0f;
        player.dirY = 0.0f;
        player.planeX = 0.0f;
        player.planeY = 0.66f;
        player.stepAccumulator = 0.0f;

        stalker.x = MAP_W / 2 + 0.5f;
        stalker.y = MAP_H / 2 + 0.5f;
        stalker.isChasing = false;
    }

    void startNewGame() {
        currentLevel = 1;
        totalSteps = 0;
        levelTime = 0.0f;
        player.sanity = 100.0f;
        player.health = 100.0f;
        generateMazeWithOverpass();
        currentState = STATE_PLAYING;
        setCaptureMouse(true);

        SDL_LockAudioDevice(audioDevice);
        audioState.inGame = true;
        audioState.sanity = 100.0f;
        audioState.monsterDist = 20.0f;
        SDL_UnlockAudioDevice(audioDevice);
    }

    void nextLevel() {
        currentLevel++;
        player.sanity = std::min(100.0f, player.sanity + 30.0f);
        player.health = std::min(100.0f, player.health + 30.0f);
        generateMazeWithOverpass();
        currentState = STATE_PLAYING;
        setCaptureMouse(true);

        SDL_LockAudioDevice(audioDevice);
        audioState.inGame = true;
        SDL_UnlockAudioDevice(audioDevice);
    }

    void drawGlyph(int col, int row, char c, uint32_t fgColor) {
        if (c < 32 || c > 127) return;
        const uint8_t* glyph = FONT_8X8[c - 32];
        int startX = col * CHAR_W;
        int startY = row * CHAR_H;

        for (int y = 0; y < CHAR_H; ++y) {
            for (int x = 0; x < CHAR_W; ++x) {
                if ((glyph[y] >> (7 - x)) & 1) {
                    pixelBuffer[(startY + y) * NATIVE_WIDTH + (startX + x)] = fgColor;
                }
            }
        }
    }

    void drawText(int col, int row, const std::string& text, uint32_t color) {
        for (size_t i = 0; i < text.size(); ++i) {
            if (col + i < TOTAL_COLS) {
                drawGlyph(col + i, row, text[i], color);
            }
        }
    }

public:
    bool init() {
        srand(static_cast<unsigned int>(time(nullptr)));

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) return false;

        window = SDL_CreateWindow(
            "Walk ASCII 3D Horror",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            RESOLUTION_PRESETS[currentResIndex].width,
            RESOLUTION_PRESETS[currentResIndex].height,
            SDL_WINDOW_SHOWN
        );

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        screenTexture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            NATIVE_WIDTH, NATIVE_HEIGHT
        );

        pixelBuffer.resize(NATIVE_WIDTH * NATIVE_HEIGHT, 0xFF000000);

        SDL_AudioSpec wantedSpec;
        SDL_zero(wantedSpec);
        wantedSpec.freq = AUDIO_SAMPLE_RATE;
        wantedSpec.format = AUDIO_S16SYS;
        wantedSpec.channels = 1;
        wantedSpec.samples = AUDIO_BUFFER_SIZE;
        wantedSpec.callback = audioCallback;
        wantedSpec.userdata = &audioState;

        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wantedSpec, nullptr, 0);
        if (audioDevice != 0) SDL_PauseAudioDevice(audioDevice, 0);

        isRunning = true;
        return true;
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) isRunning = false;

            if (currentState == STATE_PLAYING && event.type == SDL_MOUSEMOTION) {
                float rotAngle = event.motion.xrel * player.mouseSensitivity;

                float oldDirX = player.dirX;
                player.dirX = player.dirX * cos(rotAngle) - player.dirY * sin(rotAngle);
                player.dirY = oldDirX * sin(rotAngle) + player.dirY * cos(rotAngle);

                float oldPlaneX = player.planeX;
                player.planeX = player.planeX * cos(rotAngle) - player.planeY * sin(rotAngle);
                player.planeY = oldPlaneX * sin(rotAngle) + player.planeY * cos(rotAngle);

                player.pitch -= event.motion.yrel * 0.12f;
                player.pitch = std::clamp(player.pitch, -22.0f, 22.0f);
            }

            if (event.type == SDL_KEYDOWN) {
                if (currentState == STATE_TITLE) {
                    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w) {
                        menuCursor = (menuCursor - 1 + 3) % 3;
                    }
                    if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) {
                        menuCursor = (menuCursor + 1) % 3;
                    }
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        if (menuCursor == 0) startNewGame();
                        else if (menuCursor == 1) currentDifficulty = (currentDifficulty == DIFF_NORMAL) ? DIFF_EASY : DIFF_NORMAL;
                        else if (menuCursor == 2) {
                            currentResIndex = (currentResIndex + 1) % RESOLUTION_PRESETS.size();
                            updateWindowScale();
                        }
                    }
                    if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) {
                        if (menuCursor == 1) currentDifficulty = (currentDifficulty == DIFF_NORMAL) ? DIFF_EASY : DIFF_NORMAL;
                        if (menuCursor == 2) {
                            int count = RESOLUTION_PRESETS.size();
                            currentResIndex = (event.key.keysym.sym == SDLK_RIGHT)
                                ? ((currentResIndex + 1) % count)
                                : ((currentResIndex - 1 + count) % count);
                            updateWindowScale();
                        }
                    }
                }
                else if (currentState == STATE_SUCCESS) {
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) nextLevel();
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_TITLE;
                        setCaptureMouse(false);
                    }
                }
                else if (currentState == STATE_GAMEOVER) {
                    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) startNewGame();
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_TITLE;
                        setCaptureMouse(false);
                    }
                }
                else if (currentState == STATE_PLAYING) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        currentState = STATE_TITLE;
                        setCaptureMouse(false);
                    }
                }
            }
        }

        if (currentState == STATE_PLAYING) {
            const uint8_t* state = SDL_GetKeyboardState(NULL);
            player.forward = 0;
            player.strafe = 0;

            if (state[SDL_SCANCODE_W]) player.forward += 1;
            if (state[SDL_SCANCODE_S]) player.forward -= 1;
            if (state[SDL_SCANCODE_A]) player.strafe -= 1; 
            if (state[SDL_SCANCODE_D]) player.strafe += 1; 
        }
    }

    void update(double dt) {
        if (currentState != STATE_PLAYING) return;

        float dtSec = static_cast<float>(dt);
        levelTime += dtSec;
        player.takingDamage = false;

        // 1. WASD Vector Movement (Forward + Strafe)
        if (player.forward != 0 || player.strafe != 0) {
            float forwardStep = player.forward * player.moveSpeed * dtSec;
            float strafeStep  = player.strafe  * (player.moveSpeed * 0.85f) * dtSec;

            float moveX = player.dirX * forwardStep - player.dirY * strafeStep;
            float moveY = player.dirY * forwardStep + player.dirX * strafeStep;

            float bufX = (moveX > 0) ? 0.32f : -0.32f;
            float bufY = (moveY > 0) ? 0.32f : -0.32f;

            float prevX = player.posX;
            float prevY = player.posY;

            int nextTileX = int(player.posX + moveX + bufX);
            int nextTileY = int(player.posY + moveY + bufY);

            if (worldMap[int(player.posY)][nextTileX].wallType != 1) {
                float hDiff = std::abs(worldMap[int(player.posY)][nextTileX].floorH - player.posZ);
                if (hDiff <= 0.65f) player.posX += moveX;
            }

            if (worldMap[nextTileY][int(player.posX)].wallType != 1) {
                float hDiff = std::abs(worldMap[nextTileY][int(player.posX)].floorH - player.posZ);
                if (hDiff <= 0.65f) player.posY += moveY;
            }

            player.stepAccumulator += std::hypot(player.posX - prevX, player.posY - prevY);
            if (player.stepAccumulator >= 1.0f) {
                totalSteps++;
                player.stepAccumulator = 0.0f;
            }
        }

        // 2. Continuous Vertical Height Tracking
        int currTileX = int(player.posX);
        int currTileY = int(player.posY);
        player.targetPosZ = worldMap[currTileY][currTileX].floorH;
        player.posZ += (player.targetPosZ - player.posZ) * 0.25f;

        // 3. Stalker AI
        float distToMonster = std::hypot(player.posX - stalker.x, player.posY - stalker.y);

        if (distToMonster < 8.5f) {
            stalker.isChasing = true;
            float dx = (player.posX - stalker.x) / distToMonster;
            float dy = (player.posY - stalker.y) / distToMonster;

            float nx = stalker.x + dx * stalker.speed * dtSec;
            float ny = stalker.y + dy * stalker.speed * dtSec;

            if (worldMap[int(ny)][int(nx)].wallType == 0) {
                stalker.x = nx;
                stalker.y = ny;
            }

            player.sanity -= (6.0f / std::max(1.0f, distToMonster)) * dtSec;
            if (distToMonster < 1.1f) {
                player.health -= 28.0f * dtSec;
                player.takingDamage = true;
            }
        } else {
            stalker.isChasing = false;
            player.sanity -= 0.08f * dtSec;
            if (distToMonster > 12.0f) {
                player.sanity = std::min(100.0f, player.sanity + 1.0f * dtSec);
                player.health = std::min(100.0f, player.health + 0.8f * dtSec);
            }
        }

        player.sanity = std::max(0.0f, player.sanity);
        player.health = std::max(0.0f, player.health);

        if (player.sanity <= 0.0f) {
            deathReason = "LOST TO THE TERROR (SANITY DEPLETED)";
            currentState = STATE_GAMEOVER;
            setCaptureMouse(false);
            SDL_LockAudioDevice(audioDevice);
            audioState.inGame = false;
            SDL_UnlockAudioDevice(audioDevice);
            return;
        }

        if (player.health <= 0.0f) {
            deathReason = "SLAIN BY THE STALKER (HEALTH DEPLETED)";
            currentState = STATE_GAMEOVER;
            setCaptureMouse(false);
            SDL_LockAudioDevice(audioDevice);
            audioState.inGame = false;
            SDL_UnlockAudioDevice(audioDevice);
            return;
        }

        if (currTileX == endPos.x && currTileY == endPos.y) {
            currentState = STATE_SUCCESS;
            setCaptureMouse(false);
            SDL_LockAudioDevice(audioDevice);
            audioState.inGame = false;
            SDL_UnlockAudioDevice(audioDevice);
            return;
        }

        SDL_LockAudioDevice(audioDevice);
        audioState.sanity = player.sanity;
        audioState.monsterDist = distToMonster;
        audioState.isChasing = stalker.isChasing;
        SDL_UnlockAudioDevice(audioDevice);
    }

    void render3DView() {
        int viewWidth = (currentDifficulty == DIFF_EASY) ? 68 : TOTAL_COLS;
        float totalPlayerZ = player.posZ + player.eyeHeight;
        int horizon = int(ROWS / 2 + player.pitch);

        for (int col = 0; col < viewWidth; ++col) {
            float cameraX = 2.0f * col / float(viewWidth) - 1.0f;
            float rayDirX = player.dirX + player.planeX * cameraX;
            float rayDirY = player.dirY + player.planeY * cameraX;

            int mapX = int(player.posX);
            int mapY = int(player.posY);

            float deltaDistX = (rayDirX == 0) ? 1e30f : std::abs(1.0f / rayDirX);
            float deltaDistY = (rayDirY == 0) ? 1e30f : std::abs(1.0f / rayDirY);
            float sideDistX, sideDistY, perpWallDist;
            int stepX, stepY, hit = 0, side = 0;

            if (rayDirX < 0) { stepX = -1; sideDistX = (player.posX - mapX) * deltaDistX; }
            else             { stepX =  1; sideDistX = (mapX + 1.0f - player.posX) * deltaDistX; }
            if (rayDirY < 0) { stepY = -1; sideDistY = (player.posY - mapY) * deltaDistY; }
            else             { stepY =  1; sideDistY = (mapY + 1.0f - player.posY) * deltaDistY; }

            float prevFloorH = worldMap[mapY][mapX].floorH;
            float stepRiserDist = -1.0f;
            float stepFloorDiff = 0.0f;
            bool hitStepRiser = false;
            float stepElevation = 0.0f;

            while (hit == 0) {
                if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
                else                       { sideDistY += deltaDistY; mapY += stepY; side = 1; }

                if (mapX >= 0 && mapX < MAP_W && mapY >= 0 && mapY < MAP_H) {
                    float currCellFloor = worldMap[mapY][mapX].floorH;
                    if (!hitStepRiser && worldMap[mapY][mapX].wallType == 0 && std::abs(currCellFloor - prevFloorH) > 0.1f) {
                        hitStepRiser = true;
                        stepFloorDiff = currCellFloor - prevFloorH;
                        stepRiserDist = (side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
                        stepElevation = currCellFloor;
                    }
                    prevFloorH = currCellFloor;

                    if (worldMap[mapY][mapX].wallType > 0) hit = worldMap[mapY][mapX].wallType;
                } else {
                    break;
                }
            }

            if (side == 0) perpWallDist = (sideDistX - deltaDistX);
            else           perpWallDist = (sideDistY - deltaDistY);
            if (perpWallDist < 0.05f) perpWallDist = 0.05f;

            // 1. Raycasted Textured Floor 
            for (int r = horizon + 1; r < ROWS; ++r) {
                float p = r - horizon;
                float straightDist = (ROWS * totalPlayerZ) / p;
                float weight = straightDist / perpWallDist;

                float currentFloorX = weight * (player.posX + rayDirX * perpWallDist) + (1.0f - weight) * player.posX;
                float currentFloorY = weight * (player.posY + rayDirY * perpWallDist) + (1.0f - weight) * player.posY;

                int fTileX = int(currentFloorX);
                int fTileY = int(currentFloorY);

                if (fTileX >= 0 && fTileX < MAP_W && fTileY >= 0 && fTileY < MAP_H) {
                    float sampledFloorH = worldMap[fTileY][fTileX].floorH;
                    uint32_t floorColor = getElevationColor(sampledFloorH, straightDist, 0);

                    char floorGlyph = ' ';
                    if (worldMap[fTileY][fTileX].isStairs) {
                        floorGlyph = (int(straightDist * 3.0f) % 2 == 0) ? '=' : '_';
                    } else if (straightDist < 8.0f && ((fTileX + fTileY) % 2 == 0) && (col % 2 == 0)) {
                        floorGlyph = (sampledFloorH > 0.8f) ? '^' : '.';
                    }

                    if (floorGlyph != ' ') {
                        drawGlyph(col, r, floorGlyph, floorColor);
                    }
                }
            }

            // 2. Solid Walls
            float wallFloorH = (mapY >= 0 && mapY < MAP_H && mapX >= 0 && mapX < MAP_W) ? worldMap[mapY][mapX].floorH : 0.0f;
            float wallCeilH  = (mapY >= 0 && mapY < MAP_H && mapX >= 0 && mapX < MAP_W) ? worldMap[mapY][mapX].ceilH : 2.0f;

            int drawStart = horizon - int(((wallCeilH - totalPlayerZ) * ROWS) / perpWallDist);
            int drawEnd   = horizon - int(((wallFloorH - totalPlayerZ) * ROWS) / perpWallDist);

            char wallGlyph = ' ';
            if (perpWallDist <= 1.25f)      wallGlyph = '@';
            else if (perpWallDist <= 2.50f) wallGlyph = '#';
            else if (perpWallDist <= 4.00f) wallGlyph = '%';
            else if (perpWallDist <= 5.80f) wallGlyph = '*';
            else if (perpWallDist <= 7.50f) wallGlyph = '+';
            else if (perpWallDist <= 9.00f) wallGlyph = '-';
            else if (perpWallDist <= 11.0f) wallGlyph = '.';

            uint32_t wallColor;
            if (hit == 2) {
                wallColor = (side == 0) ? RED_GOAL_BRIGHT : RED_GOAL_DARK;
            } else {
                wallColor = getElevationColor(wallFloorH, perpWallDist, side);
            }

            for (int r = 0; r < ROWS; ++r) {
                if (r >= drawStart && r <= drawEnd && wallGlyph != ' ') {
                    drawGlyph(col, r, wallGlyph, wallColor);
                }
            }

            // 3. Elevation Step-Risers
            if (hitStepRiser && stepRiserDist > 0.1f && stepRiserDist < perpWallDist) {
                float lowH = std::min(prevFloorH, prevFloorH + stepFloorDiff);
                float highH = std::max(prevFloorH, prevFloorH + stepFloorDiff);

                int stepTop = horizon - int(((highH - totalPlayerZ) * ROWS) / stepRiserDist);
                int stepBottom = horizon - int(((lowH - totalPlayerZ) * ROWS) / stepRiserDist);

                uint32_t stepColor = getElevationColor(stepElevation, stepRiserDist, 0);
                char stepGlyph = (stepFloorDiff > 0) ? '=' : 'v';

                for (int r = stepTop; r <= stepBottom; ++r) {
                    if (r >= 0 && r < ROWS) {
                        drawGlyph(col, r, stepGlyph, stepColor);
                    }
                }
            }
        }

        int cx = viewWidth / 2;
        int cy = horizon;
        drawGlyph(cx, cy, '+', 0xFF94A3B8);

        if (player.takingDamage) {
            drawText(36, 28, "! ATTACKED !", RED_GOAL_BRIGHT);
        }

        std::string elevStr;
        uint32_t elevColor;
        if (player.posZ > 0.7f) {
            elevStr = "OVERPASS [HIGH]";
            elevColor = TIER_HIGH_BRIGHT;
        } else if (player.posZ > 0.2f) {
            elevStr = "STAIRS [MID]";
            elevColor = TIER_MID_BRIGHT;
        } else {
            elevStr = "GROUND [LOW]";
            elevColor = TIER_LOW_BRIGHT;
        }

        drawText(2, 2, "ELEVATION: " + elevStr + " | STEPS: " + std::to_string(totalSteps), elevColor);
        
        uint32_t hpCol = (player.health < 30.0f) ? RED_GOAL_BRIGHT : ((player.health < 60.0f) ? 0xFFF59E0B : TIER_HIGH_BRIGHT);
        drawText(2, 4, "HEALTH: " + std::to_string(int(player.health)) + "%", hpCol);

        uint32_t sanCol = (player.sanity < 30.0f) ? RED_GOAL_BRIGHT : ((player.sanity < 60.0f) ? 0xFFF59E0B : TIER_HIGH_BRIGHT);
        drawText(2, 6, "SANITY: " + std::to_string(int(player.sanity)) + "%", sanCol);

        if (currentDifficulty == DIFF_EASY) {
            renderSidebarMinimap();
        }
    }

    void renderSidebarMinimap() {
        for (int r = 0; r < ROWS; ++r) {
            drawGlyph(68, r, '|', 0xFF334155);
        }

        int miniStartX = 72;
        int miniStartY = 3;

        for (int r = 0; r < MAP_H; ++r) {
            for (int c = 0; c < MAP_W; ++c) {
                char mapCh = ' ';
                uint32_t mapCol = 0xFF1E293B;

                if (worldMap[r][c].wallType == 1) {
                    mapCh = '#';
                    mapCol = 0xFF475569;
                } else if (worldMap[r][c].isStairs) {
                    mapCh = '=';
                    mapCol = TIER_MID_BRIGHT;
                } else if (worldMap[r][c].floorH > 0.7f) {
                    mapCh = '^';
                    mapCol = TIER_HIGH_BRIGHT;
                } else if (r == startPos.y && c == startPos.x) {
                    mapCh = 'S';
                    mapCol = TIER_HIGH_BRIGHT;
                } else if (r == endPos.y && c == endPos.x) {
                    mapCh = 'E';
                    mapCol = RED_GOAL_BRIGHT;
                }

                drawGlyph(miniStartX + c, miniStartY + r, mapCh, mapCol);
            }
        }

        drawGlyph(miniStartX + int(player.posX), miniStartY + int(player.posY), 'O', 0xFF38BDF8);

        drawText(72, 32, "MODE: EASY (MINIMAP)", 0xFF94A3B8);
        drawText(72, 34, "[=] Stairs (Mid)", TIER_MID_BRIGHT);
        drawText(72, 36, "[^] Overpass (High)", TIER_HIGH_BRIGHT);
        drawText(72, 38, "[S] Start  [E] End", 0xFF64748B);
    }

    void renderTitleScreen() {
        drawText(34, 12, "==============================", TIER_HIGH_BRIGHT);
        drawText(34, 14, "     WALK ASCII 3D HORROR     ", TIER_HIGH_BRIGHT);
        drawText(34, 16, "==============================", TIER_HIGH_BRIGHT);

        std::string diffStr = (currentDifficulty == DIFF_NORMAL) ? "NORMAL (NO MINIMAP)" : "EASY (WITH MINIMAP)";
        std::string resStr = RESOLUTION_PRESETS[currentResIndex].label;

        std::string options[3] = {
            "START GAME",
            "DIFFICULTY: " + diffStr,
            "RESOLUTION: " + resStr
        };

        for (int i = 0; i < 3; ++i) {
            uint32_t col = (i == menuCursor) ? TIER_HIGH_BRIGHT : 0xFF64748B;
            std::string prefix = (i == menuCursor) ? "-> " : "   ";
            drawText(32, 24 + i * 4, prefix + options[i], col);
        }

        drawText(26, 44, "UP/DOWN: SELECT | LEFT/RIGHT: CHANGE | ENTER: START", 0xFF334155);
    }

    void renderSuccessScreen() {
        drawText(36, 12, "****************************", TIER_HIGH_BRIGHT);
        drawText(36, 14, "      MAZE COMPLETED!       ", TIER_HIGH_BRIGHT);
        drawText(36, 16, "****************************", TIER_HIGH_BRIGHT);

        drawText(34, 22, "COMPLETED LEVEL:  " + std::to_string(currentLevel), 0xFFFFFFFF);
        drawText(34, 25, "TOTAL STEPS:      " + std::to_string(totalSteps), 0xFFFFFFFF);
        drawText(34, 28, "TIME TAKEN:       " + std::to_string(int(levelTime)) + " SECONDS", 0xFFFFFFFF);
        drawText(34, 31, "REMAINING HEALTH: " + std::to_string(int(player.health)) + "%", TIER_HIGH_BRIGHT);
        drawText(34, 34, "REMAINING SANITY: " + std::to_string(int(player.sanity)) + "%", TIER_HIGH_BRIGHT);

        drawText(28, 44, "PRESS [ENTER / SPACE] TO ADVANCE TO NEXT LEVEL", TIER_LOW_BRIGHT);
        drawText(38, 47, "PRESS [ESC] FOR MAIN MENU", 0xFF64748B);
    }

    void renderGameOverScreen() {
        drawText(36, 10, "XXXXXXXXXXXXXXXXXXXXXXXXXXXX", RED_GOAL_BRIGHT);
        drawText(36, 12, "         GAME OVER          ", RED_GOAL_BRIGHT);
        drawText(36, 14, "XXXXXXXXXXXXXXXXXXXXXXXXXXXX", RED_GOAL_BRIGHT);

        drawText(28, 20, deathReason, RED_GOAL_BRIGHT);

        drawText(34, 26, "DIED AT LEVEL:    " + std::to_string(currentLevel), 0xFFCBD5E1);
        drawText(34, 29, "TOTAL STEPS:      " + std::to_string(totalSteps), 0xFFCBD5E1);
        drawText(34, 32, "SURVIVED TIME:    " + std::to_string(int(levelTime)) + " SECONDS", 0xFFCBD5E1);

        drawText(32, 42, "PRESS [ENTER / SPACE] TO TRY AGAIN", TIER_HIGH_BRIGHT);
        drawText(38, 45, "PRESS [ESC] FOR MAIN MENU", 0xFF64748B);
    }

    void render() {
        std::fill(pixelBuffer.begin(), pixelBuffer.end(), 0xFF080C14);

        if (currentState == STATE_TITLE) renderTitleScreen();
        else if (currentState == STATE_PLAYING) render3DView();
        else if (currentState == STATE_SUCCESS) renderSuccessScreen();
        else if (currentState == STATE_GAMEOVER) renderGameOverScreen();

        SDL_UpdateTexture(screenTexture, nullptr, pixelBuffer.data(), NATIVE_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    void run() {
        uint32_t previousTime = SDL_GetTicks();
        double lag = 0.0;

        while (isRunning) {
            uint32_t currentTime = SDL_GetTicks();
            lag += static_cast<double>(currentTime - previousTime);
            previousTime = currentTime;

            handleEvents();

            while (lag >= FIXED_TIMESTEP) {
                update(FIXED_TIMESTEP / 1000.0);
                lag -= FIXED_TIMESTEP;
            }

            render();
            SDL_Delay(1);
        }
    }

    void cleanup() {
        setCaptureMouse(false);
        if (audioDevice != 0) SDL_CloseAudioDevice(audioDevice);
        if (screenTexture) SDL_DestroyTexture(screenTexture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    WalkAsciiElevationEngine engine;
    if (engine.init()) engine.run();
    engine.cleanup();
    return 0;
}