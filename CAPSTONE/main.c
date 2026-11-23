/*
 * PRISON ESCAPE SIMULATOR
 * CSA0217 - C Programming for Networking
 * C + Raylib
 *
 * Controls:  W A S D (or arrow keys) = move,  LEFT SHIFT = sprint
 *            Mouse = click buttons,   ESC = back to menu
 */
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum { MENU, GAME, WIN, LOSE, HIGHSCORE } GameState;

#define MAX_GUARDS 3
#define START_X 60
#define START_Y 60
#define SAVE_FILE "highscore.dat"

/* ---------- global game data ---------- */
float highScore = 9999;          /* best (lowest) escape time; 9999 = none yet */
int   playerLives = 3;

/* ---------- colours (dark prison theme from the PPT) ---------- */
static const Color BG_DARK   = {10, 14, 30, 255};
static const Color PANEL     = {22, 28, 50, 255};
static const Color FLOOR_COL = {28, 32, 48, 255};
static const Color WALL_COL  = {110, 118, 140, 255};
static const Color AMBER     = {255, 170, 0, 255};

/* ---------- helpers ---------- */
bool WallCollision(Rectangle p, Rectangle walls[], int count)
{
    for (int i = 0; i < count; i++)
        if (CheckCollisionRecs(p, walls[i])) return true;
    return false;
}

static void LoadHighScore(void)
{
    FILE *f = fopen(SAVE_FILE, "r");
    if (f) {
        float v;
        if (fscanf(f, "%f", &v) == 1 && v > 0) highScore = v;
        fclose(f);
    }
}

static void SaveHighScore(void)
{
    FILE *f = fopen(SAVE_FILE, "w");
    if (f) { fprintf(f, "%f\n", highScore); fclose(f); }
}

static bool Button(Rectangle r, const char *label, Color col, Vector2 mouse)
{
    bool hover = CheckCollisionPointRec(mouse, r);
    Color c = hover ? ColorBrightness(col, 0.25f) : col;
    DrawRectangleRec(r, c);
    DrawRectangleLinesEx(r, 2, hover ? WHITE : BLACK);
    int tw = MeasureText(label, 20);
    DrawText(label, (int)(r.x + (r.width - tw) / 2), (int)(r.y + (r.height - 20) / 2), 20, BLACK);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void DrawCentered(const char *t, int y, int size, Color c)
{
    DrawText(t, (800 - MeasureText(t, size)) / 2, y, size, c);
}

static void DrawIcon(int cx, int cy, Color c, const char *mark)
{
    DrawCircle(cx, cy, 36, c);
    DrawCircleLines(cx, cy, 36, BLACK);
    DrawText(mark, cx - MeasureText(mark, 40) / 2, cy - 20, 40, WHITE);
}

static void ResetRun(Rectangle *player, float *timer, Vector2 guards[], const Vector2 start[])
{
    player->x = START_X;
    player->y = START_Y;
    *timer = 0;
    playerLives = 3;
    for (int i = 0; i < MAX_GUARDS; i++) guards[i] = start[i];
}

/* ---------- optional scripted self-test (only compiled with -DAUTOTEST) ---------- */
#ifdef AUTOTEST
static const Vector2 route[] = {
    {80,600},{230,600},{230,150},{380,150},{380,600},{530,600},{530,150},{700,150},{740,740}
};
#define ROUTE_LEN 9
#endif

int main(void)
{
    InitWindow(800, 800, "PRISON ESCAPE");
    SetExitKey(KEY_NULL);            /* ESC should not close the window */
    SetTargetFPS(60);
    LoadHighScore();

    /* Optional menu background: used only if menu.png sits next to the game */
    Texture2D menuTex = {0};
    bool hasMenuTex = FileExists("menu.png");
    if (hasMenuTex) menuTex = LoadTexture("menu.png");

    GameState state = MENU;
    bool quit = false;

    Rectangle playBtn  = {300, 300, 200, 60};
    Rectangle scoreBtn = {300, 380, 200, 60};
    Rectangle exitBtn  = {300, 460, 200, 60};

    Rectangle player = {START_X, START_Y, 25, 25};
    Rectangle goal   = {720, 720, 40, 40};

    /* Serpentine prison maze (outer wall + 4 inner walls) */
    Rectangle walls[] =
    {
        {0,0,800,20}, {0,0,20,800}, {780,0,20,800}, {0,780,800,20},

        {150,0,20,500},
        {300,300,20,500},
        {450,0,20,500},
        {600,300,20,500}
    };
    int wallCount = 8;

    const Vector2 guardStart[MAX_GUARDS] = { {650,100}, {650,600}, {360,650} };
    Vector2 guards[MAX_GUARDS] = { {650,100}, {650,600}, {360,650} };

    float timer = 0;
    float hitCooldown = 0;           /* short invulnerability after being caught */
    int   credits = 0;
    bool  newRecord = false;

#ifdef AUTOTEST
    int frame = 0, wp = 0, shots = 0;
    const char *mode = getenv("AUTOTEST_MODE");   /* "win" or "lose" */
    bool wantWin = (mode && mode[0] == 'w');
#endif

    while (!WindowShouldClose() && !quit)
    {
        Vector2 mouse = GetMousePosition();
        float dt = GetFrameTime();

#ifdef AUTOTEST
        frame++;
        if (state == MENU && frame == 10) { ResetRun(&player,&timer,guards,guardStart); state = GAME; }
#endif

        /* ================= UPDATE ================= */
        if (state == GAME)
        {
            timer += dt;
            if (hitCooldown > 0) hitCooldown -= dt;

            float speed = 3;
            if (IsKeyDown(KEY_LEFT_SHIFT)) speed = 5;

            float dx = 0, dy = 0;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dy -= speed;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dy += speed;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dx -= speed;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx += speed;

#ifdef AUTOTEST
            dx = dy = 0;
            if (wantWin) {
                if (player.x < START_X + 1 && player.y < START_Y + 1) wp = 0;  /* respawned */
                Vector2 t = route[wp];
                float ex = t.x - player.x, ey = t.y - player.y;
                if (fabsf(ex) < 6 && fabsf(ey) < 6) { if (wp < ROUTE_LEN - 1) wp++; }
                else if (fabsf(ex) >= fabsf(ey)) dx = ex > 0 ? 5 : -5;
                else dy = ey > 0 ? 5 : -5;
                for (int i = 0; i < MAX_GUARDS; i++) {
                    float gx = player.x - guards[i].x, gy = player.y - guards[i].y;
                    if (sqrtf(gx*gx + gy*gy) < 45) {
                        dx = (fabsf(gx) > 1) ? (gx > 0 ? 5 : -5) : 0;
                        dy = (fabsf(gy) > 1) ? (gy > 0 ? 5 : -5) : 0;
                    }
                }
            }
#endif

            /* slide along walls: test X and Y separately */
            Rectangle test = player;
            test.x += dx;
            if (!WallCollision(test, walls, wallCount)) player.x = test.x;
            test = player;
            test.y += dy;
            if (!WallCollision(test, walls, wallCount)) player.y = test.y;

            /* guard chasing logic */
            for (int i = 0; i < MAX_GUARDS; i++)
            {
                Vector2 dir;
                dir.x = player.x - guards[i].x;
                dir.y = player.y - guards[i].y;
                float len = sqrtf(dir.x * dir.x + dir.y * dir.y);

                if (len < 300 && len > 0.001f)
                {
                    float vx = (dir.x / len) * 1.5f;
                    float vy = (dir.y / len) * 1.5f;

                    Rectangle g = { guards[i].x + vx, guards[i].y, 25, 25 };
                    if (!WallCollision(g, walls, wallCount)) guards[i].x += vx;
                    g = (Rectangle){ guards[i].x, guards[i].y + vy, 25, 25 };
                    if (!WallCollision(g, walls, wallCount)) guards[i].y += vy;
                }

                Rectangle guardBox = { guards[i].x, guards[i].y, 25, 25 };
                if (hitCooldown <= 0 && CheckCollisionRecs(player, guardBox))
                {
                    playerLives--;
                    if (playerLives <= 0) {
                        state = LOSE;
                    } else {
                        player.x = START_X;
                        player.y = START_Y;
                        hitCooldown = 1.5f;
                    }
                }
            }

            /* reached the exit gate */
            if (state == GAME && CheckCollisionRecs(player, goal))
            {
                newRecord = false;
                if (timer < highScore) { highScore = timer; newRecord = true; SaveHighScore(); }
                credits = (int)fmaxf(10.0f, 100.0f - timer);
                state = WIN;
            }

            if (IsKeyPressed(KEY_ESCAPE)) state = MENU;
        }

        /* ================= DRAW ================= */
        BeginDrawing();
        ClearBackground(BLACK);

        if (state == MENU)
        {
            if (hasMenuTex) {
                DrawTexture(menuTex, 0, 0, WHITE);
            } else {
                ClearBackground(BG_DARK);
                for (int x = 0; x < 800; x += 100)         /* prison bars */
                    DrawRectangle(x + 40, 0, 12, 800, (Color){30, 38, 66, 255});
                DrawRectangle(0, 145, 800, 150, (Color){10, 14, 30, 230});
                DrawCentered("PRISON", 160, 60, (Color){255, 90, 50, 255});
                DrawCentered("ESCAPE", 218, 40, AMBER);
                DrawCentered("SIMULATOR", 264, 20, RAYWHITE);
            }
            if (Button(playBtn,  "PLAY",       GREEN,  mouse)) {
                ResetRun(&player, &timer, guards, guardStart);
                hitCooldown = 0;
                state = GAME;
            }
            if (Button(scoreBtn, "HIGH SCORE", YELLOW, mouse)) state = HIGHSCORE;
            if (Button(exitBtn,  "EXIT",       RED,    mouse)) quit = true;
            DrawCentered("WASD / Arrows = move   SHIFT = sprint", 740, 18, GRAY);
        }
        else if (state == GAME)
        {
            ClearBackground(FLOOR_COL);
            /* exit gate */
            DrawRectangleRec(goal, GREEN);
            DrawRectangleLinesEx(goal, 2, WHITE);
            DrawText("EXIT", (int)goal.x + 4, (int)goal.y + 12, 16, BLACK);
            /* start cell marker */
            DrawRectangleLines(START_X - 10, START_Y - 10, 45, 45, (Color){60, 90, 160, 255});

            for (int i = 0; i < wallCount; i++) DrawRectangleRec(walls[i], WALL_COL);
            for (int i = 0; i < MAX_GUARDS; i++)
                DrawRectangle((int)guards[i].x, (int)guards[i].y, 25, 25, RED);

            /* player blinks while invulnerable */
            if (hitCooldown <= 0 || ((int)(hitCooldown * 10) % 2 == 0))
                DrawRectangleRec(player, BLUE);

            /* HUD */
            DrawRectangle(625, 25, 150, 58, (Color){0, 0, 0, 150});
            DrawText(TextFormat("TIME %.2f", timer), 633, 31, 20, WHITE);
            DrawText(TextFormat("LIVES: %d", playerLives), 633, 57, 20, RED);
        }
        else if (state == WIN)
        {
            ClearBackground(BG_DARK);
            DrawRectangle(100, 130, 600, 540, PANEL);
            DrawRectangleLinesEx((Rectangle){100, 130, 600, 540}, 2, (Color){0, 190, 120, 255});
            DrawIcon(400, 200, (Color){0, 120, 80, 255}, "*");
            DrawCentered("YOU ESCAPED!", 250, 40, GREEN);
            DrawCentered("SECURITY BREACH ARCHIVED", 298, 16, GRAY);

            DrawRectangleLinesEx((Rectangle){160, 330, 480, 90}, 2, GRAY);
            DrawText("ESCAPE TIME", 200, 345, 14, GRAY);
            DrawText(TextFormat("%.2fs", timer), 200, 370, 30, WHITE);
            DrawText("CREDITS REWARDED", 450, 345, 14, GRAY);
            DrawText(TextFormat("+%d", credits), 450, 370, 30, AMBER);
            if (newRecord) DrawCentered("NEW BEST TIME!", 432, 20, YELLOW);

            if (Button((Rectangle){300, 480, 200, 60}, "PLAY AGAIN", GREEN, mouse)) {
                ResetRun(&player, &timer, guards, guardStart);
                hitCooldown = 0;
                state = GAME;
            }
            if (Button((Rectangle){300, 560, 200, 60}, "MENU", GRAY, mouse)) state = MENU;
        }
        else if (state == LOSE)
        {
            ClearBackground(BG_DARK);
            DrawRectangle(150, 160, 500, 480, PANEL);
            DrawRectangleLinesEx((Rectangle){150, 160, 500, 480}, 2, (Color){190, 40, 40, 255});
            DrawIcon(400, 230, (Color){140, 20, 20, 255}, "!");
            DrawCentered("YOU GOT CAUGHT!", 285, 40, RED);
            DrawCentered("ALL LIVES USED UP", 335, 16, GRAY);

            DrawRectangleLinesEx((Rectangle){200, 370, 400, 70}, 2, GRAY);
            DrawCentered("SURVIVAL TIME", 380, 14, GRAY);
            DrawCentered(TextFormat("%.2f seconds of evasion", timer), 405, 20, WHITE);

            if (Button((Rectangle){300, 480, 200, 60}, "RETRY", GREEN, mouse)) {
                ResetRun(&player, &timer, guards, guardStart);
                hitCooldown = 0;
                state = GAME;
            }
            if (Button((Rectangle){300, 560, 200, 60}, "MENU", GRAY, mouse)) state = MENU;
        }
        else if (state == HIGHSCORE)
        {
            ClearBackground(BG_DARK);
            DrawCentered("BEST ESCAPE TIME", 300, 40, YELLOW);
            if (highScore >= 9999) DrawCentered("No escape recorded yet", 360, 30, WHITE);
            else DrawCentered(TextFormat("%.2f seconds", highScore), 360, 30, WHITE);
            if (Button((Rectangle){300, 450, 200, 60}, "BACK", GRAY, mouse)) state = MENU;
        }

        EndDrawing();

#ifdef AUTOTEST
        /* take screenshots at key moments, then quit */
        if (frame == 5)  { TakeScreenshot("shot_menu.png"); }
        if (frame == 60 && state == GAME) { TakeScreenshot("shot_game.png"); }
        if (state == WIN  && shots == 0) { shots = 1; TakeScreenshot("shot_win.png");  printf("RESULT: WIN time=%.2f lives=%d\n", timer, playerLives); fflush(stdout); break; }
        if (state == LOSE && shots == 0) { shots = 1; TakeScreenshot("shot_lose.png"); printf("RESULT: LOSE time=%.2f\n", timer); fflush(stdout); break; }
        if (frame > 5000) { printf("RESULT: TIMEOUT state=%d px=%.0f py=%.0f lives=%d\n", state, player.x, player.y, playerLives); break; }
#endif
    }

    if (hasMenuTex) UnloadTexture(menuTex);
    CloseWindow();
    return 0;
}
