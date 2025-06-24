#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define GRID_SIZE 20
#define GRID_WIDTH (WINDOW_WIDTH / GRID_SIZE)
#define GRID_HEIGHT (WINDOW_HEIGHT / GRID_SIZE)

// Digit patterns for numbers (5x3)
const int DIGIT_WIDTH = 3;
const int DIGIT_HEIGHT = 5;
const int DIGITS[10][5][3] = {
    {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // 0
    {{0,1,0},{1,1,0},{0,1,0},{0,1,0},{1,1,1}}, // 1
    {{1,1,1},{0,0,1},{1,1,1},{1,0,0},{1,1,1}}, // 2
    {{1,1,1},{0,0,1},{1,1,1},{0,0,1},{1,1,1}}, // 3
    {{1,0,1},{1,0,1},{1,1,1},{0,0,1},{0,0,1}}, // 4
    {{1,1,1},{1,0,0},{1,1,1},{0,0,1},{1,1,1}}, // 5
    {{1,1,1},{1,0,0},{1,1,1},{1,0,1},{1,1,1}}, // 6
    {{1,1,1},{0,0,1},{0,1,0},{1,0,0},{1,0,0}}, // 7
    {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,1,1}}, // 8
    {{1,1,1},{1,0,1},{1,1,1},{0,0,1},{1,1,1}}  // 9
};

typedef struct {
    int x, y;
} Point;

typedef struct Node {
    Point pos;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    Point dir;
    int length;
} Snake;

typedef struct {
    Snake snake;
    Point food;
    int score;
    float speed;
    float moveTimer;
    int paused;
    int gameOver;
} Game;

void initSnake(Snake* snake) {
    Node* node = malloc(sizeof(Node));
    node->pos.x = GRID_WIDTH / 2;
    node->pos.y = GRID_HEIGHT / 2;
    node->next = NULL;
    snake->head = node;
    snake->tail = node;
    snake->dir.x = 1;
    snake->dir.y = 0;
    snake->length = 1;
}

void freeSnake(Snake* snake) {
    if (!snake) return;
    Node* current = snake->head;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    snake->head = NULL;
    snake->tail = NULL;
    snake->length = 0;
}

int pointEquals(Point a, Point b) {
    return a.x == b.x && a.y == b.y;
}

int isOnSnake(Snake* snake, Point p) {
    Node* current = snake->head;
    while (current) {
        if (pointEquals(current->pos, p)) return 1;
        current = current->next;
    }
    return 0;
}

void placeFood(Game* game) {
    Point p;
    do {
        p.x = rand() % GRID_WIDTH;
        p.y = rand() % GRID_HEIGHT;
    } while (isOnSnake(&game->snake, p));
    game->food = p;
}

void resetGame(Game* game) {
    if (game->snake.head != NULL) {
        freeSnake(&game->snake);
    }
    initSnake(&game->snake);
    placeFood(game);
    game->score = 0;
    game->speed = 5.0f;
    game->moveTimer = 0.0f;
    game->paused = 0;
    game->gameOver = 0;
}

void addHead(Snake* snake, Point newPos) {
    Node* node = malloc(sizeof(Node));
    node->pos = newPos;
    node->next = snake->head;
    snake->head = node;
    if (snake->length == 0) snake->tail = node;
    snake->length++;
}

void removeTail(Snake* snake) {
    if (!snake->head) return;
    if (snake->head == snake->tail) {
        free(snake->tail);
        snake->head = snake->tail = NULL;
        snake->length = 0;
        return;
    }
    Node* current = snake->head;
    while (current->next != snake->tail) {
        current = current->next;
    }
    free(snake->tail);
    snake->tail = current;
    snake->tail->next = NULL;
    snake->length--;
}

void handleInput(Game* game, SDL_Event* e) {
    if (e->type == SDL_KEYDOWN) {
        if (game->gameOver && e->key.keysym.sym == SDLK_r) {
            resetGame(game);
            return;
        }
        switch (e->key.keysym.sym) {
            case SDLK_UP:
                if (game->snake.dir.y != 1) game->snake.dir = (Point){0, -1};
                break;
            case SDLK_DOWN:
                if (game->snake.dir.y != -1) game->snake.dir = (Point){0, 1};
                break;
            case SDLK_LEFT:
                if (game->snake.dir.x != 1) game->snake.dir = (Point){-1, 0};
                break;
            case SDLK_RIGHT:
                if (game->snake.dir.x != -1) game->snake.dir = (Point){1, 0};
                break;
            case SDLK_p:
                game->paused = !game->paused;
                break;
        }
    }
}

void updateGame(Game* game, float deltaTime) {
    if (game->paused || game->gameOver) return;
    game->moveTimer += deltaTime;
    float interval = 1.0f / game->speed;
    if (game->moveTimer >= interval) {
        game->moveTimer -= interval;
        Point newHead = {
            game->snake.head->pos.x + game->snake.dir.x,
            game->snake.head->pos.y + game->snake.dir.y
        };
        if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT || isOnSnake(&game->snake, newHead)) {
            game->gameOver = 1;
            return;
        }
        addHead(&game->snake, newHead);
        if (pointEquals(newHead, game->food)) {
            game->score++;
            if (game->score % 5 == 0) game->speed += 1.0f;
            placeFood(game);
        } else {
            removeTail(&game->snake);
        }
    }
}

void drawPixel(SDL_Renderer* r, int x, int y) {
    SDL_Rect p = {x, y, 4, 4};
    SDL_RenderFillRect(r, &p);
}

void drawDigit(SDL_Renderer* r, int digit, int x, int y) {
    if (digit < 0 || digit > 9) return;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    for (int i = 0; i < DIGIT_HEIGHT; i++)
        for (int j = 0; j < DIGIT_WIDTH; j++)
            if (DIGITS[digit][i][j]) drawPixel(r, x + j * 5, y + i * 5);
}

void drawNumber(SDL_Renderer* r, int num, int x, int y) {
    if (num == 0) {
        drawDigit(r, 0, x, y);
        return;
    }
    int digits[10], len = 0;
    while (num > 0) {
        digits[len++] = num % 10;
        num /= 10;
    }
    for (int i = len - 1; i >= 0; i--) {
        drawDigit(r, digits[i], x + (len - 1 - i) * 20, y);
    }
}

void drawScore(SDL_Renderer* r, int score) {
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    const char* label = "SCORE:";
    int x = 10;
    for (int i = 0; label[i]; i++) {
        SDL_RenderDrawLine(r, x, 10, x + 10, 10); // minimalist "text" bar
        x += 20;
    }
    drawNumber(r, score, x, 10);
}

void drawMessage(SDL_Renderer* r, const char* msg) {
    int len = 0;
    while (msg[len]) len++;
    int x = (WINDOW_WIDTH - len * 20) / 2;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    for (int i = 0; i < len; i++) {
        SDL_RenderDrawLine(r, x + i * 20, WINDOW_HEIGHT / 2, x + i * 20 + 10, WINDOW_HEIGHT / 2);
    }
}

void render(Game* game, SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    // Draw food (rounded)
    SDL_SetRenderDrawColor(r, 255, 80, 80, 255);
    SDL_Rect food = {
        game->food.x * GRID_SIZE + 4,
        game->food.y * GRID_SIZE + 4,
        GRID_SIZE - 8,
        GRID_SIZE - 8
    };
    SDL_RenderFillRect(r, &food);

    // Draw snake
    Node* curr = game->snake.head;
    int i = 0;
    while (curr) {
        Uint8 green = 255 - i * (200 / game->snake.length);
        SDL_SetRenderDrawColor(r, 0, green, 0, 255);
        SDL_Rect s = {
            curr->pos.x * GRID_SIZE,
            curr->pos.y * GRID_SIZE,
            GRID_SIZE,
            GRID_SIZE
        };
        SDL_RenderFillRect(r, &s);
        curr = curr->next;
        i++;
    }

    drawScore(r, game->score);
    if (game->paused) drawMessage(r, "PAUSED");
    else if (game->gameOver) drawMessage(r, "GAME OVER - R TO RESTART");

    SDL_RenderPresent(r);
}

int main() {
    srand((unsigned int)time(NULL));
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init Failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* w = SDL_CreateWindow("Snake Game in C",
                                     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!w) {
        printf("Window Failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
    if (!r) {
        printf("Renderer Failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(w);
        SDL_Quit();
        return 1;
    }

    Game game = {0};
    resetGame(&game);
    Uint32 last = SDL_GetTicks();
    int quit = 0;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = 1;
            handleInput(&game, &e);
        }

        Uint32 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        last = now;

        updateGame(&game, dt);
        render(&game, r);

        Uint32 frameTime = SDL_GetTicks() - now;
        if (frameTime < 16) SDL_Delay(16 - frameTime);
    }

    freeSnake(&game.snake);
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}
