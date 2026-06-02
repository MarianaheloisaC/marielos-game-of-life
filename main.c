#include <stdio.h>
#include "raylib.h"
#include <math.h>
#include <stdbool.h>

typedef enum { LIGHT, DARK } TemaFonte;

//------------------------------------------------------------------------------------
// Variáveis globais
//------------------------------------------------------------------------------------
#define ESPACO_EXTRA 10
#define LARGURA_UNIVERSO 50 + ESPACO_EXTRA * 2
#define ALTURA_UNIVERSO 30 + ESPACO_EXTRA * 2

const int largura_janela = 1000;
const int altura_janela = 600;
const int tamanho_celula = 20;

bool universo[ALTURA_UNIVERSO][LARGURA_UNIVERSO] = {0};
bool rodando = false;

float periodo_universo = 0.2f;
float timer_universo = 0.0f;
float timer_teclagem = 0.0f;
float cooldown_periodo = 0.12f;

Color cor_celula_morta = {255, 255, 255, 255};
Color cor_celula_viva = {0, 0, 0, 255};
Color cor_texto_forte = {0, 0, 0, 50};
Color cor_texto_fraco = {0, 0, 0, 32};

TemaFonte temas_fonte[10] = {DARK, DARK, DARK, DARK, LIGHT,
                             LIGHT, LIGHT, DARK, DARK, DARK};

Color paletas[10][2] = {
    {{255, 255, 255, 255}, {0, 0, 0, 255}},
    {{231, 6, 55, 255}, {255, 230, 174, 255}},
    {{149, 245, 249, 255}, {219, 65, 106, 255}},
    {{255, 58, 114, 255}, {2, 31, 83, 255}},
    {{22, 23, 26, 255}, {123, 115, 222, 255}},
    {{17, 17, 17, 255}, {121, 255, 139, 255}},
    {{3, 26, 65, 255}, {96, 212, 255, 255}},
    {{180, 130, 214, 255}, {57, 45, 126, 255}},
    {{226, 178, 143, 255}, {148, 76, 74, 255}},
    {{243, 161, 166, 255}, {255, 212, 212, 255}}
};

Camera2D camera = { 0 };

//------------------------------------------------------------------------------------
// Protótipos
//------------------------------------------------------------------------------------
void inverte_celula(int x, int y);
void trata_click();
void atualiza_universo();
void renderiza_universo();
void trata_teclagem();
int conta_vizinhos(bool universo[ALTURA_UNIVERSO][LARGURA_UNIVERSO], int x, int y);
void trocaTemaFonte(TemaFonte tema);

//------------------------------------------------------------------------------------
// MAIN
//------------------------------------------------------------------------------------
int main(void) {
    InitWindow(largura_janela, altura_janela, "Marielô's Game of Life");

    camera.zoom = 1.0f;
    camera.offset = (Vector2){ largura_janela/2.0f, altura_janela/2.0f };
    camera.target = (Vector2){
        (LARGURA_UNIVERSO - ESPACO_EXTRA * 2) * tamanho_celula / 2.0f,
        (ALTURA_UNIVERSO  - ESPACO_EXTRA * 2) * tamanho_celula / 2.0f
    };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        // --- Update ---
        float speed = 5.0f / camera.zoom;

        if (IsKeyDown(KEY_D)) camera.target.x += speed;
        if (IsKeyDown(KEY_A)) camera.target.x -= speed;
        if (IsKeyDown(KEY_W)) camera.target.y -= speed;
        if (IsKeyDown(KEY_S)) camera.target.y += speed;

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            float zoomSpeed = 0.1f;
            camera.zoom += wheel * zoomSpeed;
            if (camera.zoom < 0.2f) camera.zoom = 0.2f;
            if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        }

        timer_teclagem += GetFrameTime();
        trata_teclagem();

        if (rodando) {
            timer_universo += GetFrameTime();
            if (timer_universo >= periodo_universo) {
                atualiza_universo();
                timer_universo = 0.0f;
            }
        }

        trata_click();

        // --- Draw ---
        BeginDrawing();
        ClearBackground(cor_celula_morta);

        BeginMode2D(camera);
        renderiza_universo();
        EndMode2D();

        // UI por cima (fora do Mode2D para não ser afetada pela câmera)
        DrawText("aperte ESPAÇO para rodar", 320, 288, 24, cor_texto_forte);
        DrawText("aperte 0 - 9 para mudar a paleta de cores", 250, 322, 22, cor_texto_fraco);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

//------------------------------------------------------------------------------------
// INPUT TECLADO
//------------------------------------------------------------------------------------
void trata_teclagem() {

    // SPACE — toggle seguro com flag estática
    static bool space_held = false;

    if (IsKeyDown(KEY_SPACE)) {
        if (!space_held) {
            rodando = !rodando;
            space_held = true;
        }
    } else {
        space_held = false;
    }

    // Velocidade — pressionada única
    if (IsKeyPressed(KEY_MINUS)) {
        periodo_universo += 0.02f;
        timer_teclagem = -0.4f;
    }

    if (IsKeyPressed(KEY_EQUAL)) {
        if (periodo_universo - 0.02f >= 0.02f)
            periodo_universo -= 0.02f;
        timer_teclagem = -0.4f;
    }

    // Paletas — DEVE vir depois do espaço para não consumir KEY_SPACE da fila
    int tecla = GetKeyPressed();
    if (tecla >= KEY_ZERO && tecla <= KEY_NINE) {
        int idx = tecla - KEY_ZERO;
        cor_celula_morta = paletas[idx][0];
        cor_celula_viva  = paletas[idx][1];
        trocaTemaFonte(temas_fonte[idx]);
    }

    // Segurar para mudar velocidade continuamente
    if (timer_teclagem >= cooldown_periodo) {
        if (IsKeyDown(KEY_MINUS))
            periodo_universo += 0.02f;

        if (IsKeyDown(KEY_EQUAL))
            if (periodo_universo - 0.02f >= 0.02f)
                periodo_universo -= 0.02f;

        timer_teclagem = 0.0f;
    }
}

//------------------------------------------------------------------------------------
// MOUSE
//------------------------------------------------------------------------------------
void trata_click() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 posicao_click = GetScreenToWorld2D(GetMousePosition(), camera);
        int x = (int)floor(posicao_click.x / tamanho_celula) + ESPACO_EXTRA;
        int y = (int)floor(posicao_click.y / tamanho_celula) + ESPACO_EXTRA;
        printf("click em x=%d y=%d\n", x, y); // linha temporária
        inverte_celula(x, y);
    }
}

// CORREÇÃO: validação de bounds antes de inverter
void inverte_celula(int x, int y) {
    if (x >= 0 && x < LARGURA_UNIVERSO &&
        y >= 0 && y < ALTURA_UNIVERSO)
        universo[y][x] = !universo[y][x];
}

//------------------------------------------------------------------------------------
// LÓGICA DO UNIVERSO
//------------------------------------------------------------------------------------
void atualiza_universo() {
    bool universo_paralelo[ALTURA_UNIVERSO][LARGURA_UNIVERSO];

    for (int i = 0; i < ALTURA_UNIVERSO; i++)
        for (int j = 0; j < LARGURA_UNIVERSO; j++)
            universo_paralelo[i][j] = universo[i][j];

    for (int i = 0; i < ALTURA_UNIVERSO; i++) {
        for (int j = 0; j < LARGURA_UNIVERSO; j++) {
            int vizinhos = conta_vizinhos(universo_paralelo, j, i);

            if (universo[i][j] && (vizinhos < 2 || vizinhos > 3))
                universo[i][j] = false;
            else if (vizinhos == 3)
                universo[i][j] = true;
        }
    }
}

void renderiza_universo() {
    for (int i = ESPACO_EXTRA; i < ALTURA_UNIVERSO - ESPACO_EXTRA; i++) {
        for (int j = ESPACO_EXTRA; j < LARGURA_UNIVERSO - ESPACO_EXTRA; j++) {
            if (universo[i][j]) {
                int x = j - ESPACO_EXTRA;
                int y = i - ESPACO_EXTRA;
                DrawRectangle(
                    x * tamanho_celula,
                    y * tamanho_celula,
                    tamanho_celula,
                    tamanho_celula,
                    cor_celula_viva
                );
            }
        }
    }
}

int conta_vizinhos(bool universo[ALTURA_UNIVERSO][LARGURA_UNIVERSO], int x, int y) {
    int numero_vizinhos = 0;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;

            int nx = x + j;
            int ny = y + i;

            if (nx >= 0 && nx < LARGURA_UNIVERSO &&
                ny >= 0 && ny < ALTURA_UNIVERSO)
                numero_vizinhos += universo[ny][nx];
        }
    }
    return numero_vizinhos;
}

//------------------------------------------------------------------------------------
// TEMA
//------------------------------------------------------------------------------------
void trocaTemaFonte(TemaFonte tema) {
    switch (tema) {
        case DARK:
            cor_texto_forte = (Color){0, 0, 0, 50};
            cor_texto_fraco = (Color){0, 0, 0, 32};
            break;
        case LIGHT:
            cor_texto_forte = (Color){255, 255, 255, 50};
            cor_texto_fraco = (Color){255, 255, 255, 32};
            break;
    }
}
