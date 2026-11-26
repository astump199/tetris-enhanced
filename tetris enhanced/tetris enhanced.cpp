// tetris enhanced.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace sf;

const int COLS = 10;
const int ROWS = 20;
const int BLOCK_SIZE = 30;
const int SIDEBAR_WIDTH = 150;

// 0 = empty, 1-7 = locked tetromino colors
int board[ROWS][COLS] = { 0 };

const Color PIECE_COLORS[8] = {
    Color(25, 25, 30),    // 0: Empty grid background
    Color::Cyan,          // 1: I-piece
    Color::Blue,          // 2: J-piece
    Color(255, 165, 0),   // 3: L-piece (Orange)
    Color::Yellow,        // 4: O-piece
    Color::Green,         // 5: S-piece
    Color::Magenta,       // 6: T-piece
    Color::Red            // 7: Z-piece
};

const int SHAPES[7][4][2] = {
    {{0,1}, {1,1}, {2,1}, {3,1}}, // I
    {{0,0}, {0,1}, {1,1}, {2,1}}, // J
    {{2,0}, {0,1}, {1,1}, {2,1}}, // L
    {{1,0}, {2,0}, {1,1}, {2,1}}, // O
    {{1,0}, {2,0}, {0,1}, {1,1}}, // S
    {{1,0}, {0,1}, {1,1}, {2,1}}, // T
    {{0,0}, {1,0}, {1,1}, {2,1}}  // Z
};

struct Piece {
    int type;       // 0 to 6
    int x, y;       // Top-left anchor on the 10x20 grid
    int blocks[4][2];
};

bool isValidPosition(const Piece& p) {
    for (int i = 0; i < 4; i++) {
        int bx = p.x + p.blocks[i][0];
        int by = p.y + p.blocks[i][1];

        if (bx < 0 || bx >= COLS || by >= ROWS) {
            return false;
        }
        if (by >= 0 && board[by][bx] != 0) {
            return false;
        }
    }
    return true;
}

void rotatePiece(Piece& p) {
    if (p.type == 3) return; // O-piece does not rotate

    Piece rotated = p;
    int px = 1, py = 1;
    for (int i = 0; i < 4; i++) {
        int relX = p.blocks[i][0] - px;
        int relY = p.blocks[i][1] - py;
        rotated.blocks[i][0] = px - relY;
        rotated.blocks[i][1] = py + relX;
    }

    if (isValidPosition(rotated)) {
        p = rotated;
    }
    else {
        rotated.x += 1;
        if (isValidPosition(rotated)) { p = rotated; return; }
        rotated.x -= 2;
        if (isValidPosition(rotated)) { p = rotated; return; }
    }
}

void lockPiece(const Piece& p) {
    for (int i = 0; i < 4; i++) {
        int bx = p.x + p.blocks[i][0];
        int by = p.y + p.blocks[i][1];
        if (by >= 0 && by < ROWS && bx >= 0 && bx < COLS) {
            board[by][bx] = p.type + 1;
        }
    }
}

int clearLines() {
    int linesCleared = 0;
    for (int r = ROWS - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < COLS; c++) {
            if (board[r][c] == 0) {
                full = false;
                break;
            }
        }

        if (full) {
            linesCleared++;
            for (int rowAbove = r; rowAbove > 0; rowAbove--) {
                for (int c = 0; c < COLS; c++) {
                    board[rowAbove][c] = board[rowAbove - 1][c];
                }
            }
            for (int c = 0; c < COLS; c++) {
                board[0][c] = 0;
            }
            r++;
        }
    }
    return linesCleared;
}

Piece spawnPiece() {
    Piece p;
    p.type = rand() % 7;
    p.x = 3;
    p.y = 0;
    for (int i = 0; i < 4; i++) {
        p.blocks[i][0] = SHAPES[p.type][i][0];
        p.blocks[i][1] = SHAPES[p.type][i][1];
    }
    return p;
}

int main() {
    srand(static_cast<unsigned>(time(0)));

    int windowWidth = (COLS * BLOCK_SIZE) + SIDEBAR_WIDTH;
    int windowHeight = ROWS * BLOCK_SIZE;
    RenderWindow window(VideoMode(windowWidth, windowHeight), "Tetris - Level 1");
    window.setFramerateLimit(60);

    Piece current = spawnPiece();
    Piece next = spawnPiece();

    // Block Skip state: 0 = Ready, 1..3 = drops remaining before next skip
    int skipCooldown = 0;
    const int SKIP_COOLDOWN_MAX = 3;

    Clock fallClock;
    float fallDelay = 0.6f;
    int score = 0;
    bool gameOver = false;

    std::cout << "=== TETRIS LEVEL 1 ===" << std::endl;
    std::cout << "Controls: Left/Right = Move | Up = Rotate | Down = Soft Drop | Space = Hard Drop | Shift = Block Skip" << std::endl;
    std::cout << "Block Skip: READY" << std::endl;

    RectangleShape cellShape(Vector2f(BLOCK_SIZE - 2, BLOCK_SIZE - 2));

    // Preview box dimensions & position in the sidebar
    const int PREVIEW_BOX_SIZE = 4 * BLOCK_SIZE;
    const float PREVIEW_X = (COLS * BLOCK_SIZE) + (SIDEBAR_WIDTH - PREVIEW_BOX_SIZE) / 2.0f;
    const float PREVIEW_Y = 30.0f;

    RectangleShape previewFrame(Vector2f(PREVIEW_BOX_SIZE, PREVIEW_BOX_SIZE));
    previewFrame.setPosition(PREVIEW_X, PREVIEW_Y);
    previewFrame.setFillColor(Color(20, 20, 25));
    previewFrame.setOutlineThickness(2.0f);
    previewFrame.setOutlineColor(Color(70, 70, 85));

    // 3 visual charge pips below the preview box to show Block Skip cooldown
    RectangleShape pipShape(Vector2f(32.0f, 12.0f));
    pipShape.setOutlineThickness(1.5f);
    pipShape.setOutlineColor(Color(80, 80, 95));

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }

            if (!gameOver && event.type == Event::KeyPressed) {
                Piece test = current;

                if (event.key.code == Keyboard::Left) {
                    test.x -= 1;
                    if (isValidPosition(test)) current = test;
                }
                else if (event.key.code == Keyboard::Right) {
                    test.x += 1;
                    if (isValidPosition(test)) current = test;
                }
                else if (event.key.code == Keyboard::Up) {
                    rotatePiece(current);
                }
                else if (event.key.code == Keyboard::Down) {
                    test.y += 1;
                    if (isValidPosition(test)) {
                        current = test;
                        fallClock.restart();
                    }
                }
                else if (event.key.code == Keyboard::Space) {
                    while (isValidPosition(test)) {
                        current = test;
                        test.y += 1;
                    }
                    fallClock.restart();
                }
                else if (event.key.code == Keyboard::LShift || event.key.code == Keyboard::RShift) {
                    if (skipCooldown == 0) {
                        int skippedType = next.type;
                        next = spawnPiece();
                        // Guarantee a different piece type so the skip feels responsive
                        while (next.type == skippedType) {
                            next = spawnPiece();
                        }
                        skipCooldown = SKIP_COOLDOWN_MAX;
                        std::cout << "Block Skip used! Drop 3 pieces to recharge." << std::endl;
                    }
                    else {
                        std::cout << "Block Skip on cooldown: " << skipCooldown << " drop(s) remaining." << std::endl;
                    }
                }
            }
        }

        // Gravity tick
        if (!gameOver && fallClock.getElapsedTime().asSeconds() >= fallDelay) {
            Piece test = current;
            test.y += 1;

            if (isValidPosition(test)) {
                current = test;
            }
            else {
                lockPiece(current);

                // Decrement Block Skip cooldown each time a piece is dropped/locked
                if (skipCooldown > 0) {
                    skipCooldown--;
                    if (skipCooldown == 0) {
                        std::cout << "Block Skip: READY!" << std::endl;
                    }
                    else {
                        std::cout << "Block Skip recharging: " << skipCooldown << " drop(s) remaining." << std::endl;
                    }
                }

                int cleared = clearLines();
                if (cleared > 0) {
                    score += cleared * 100;
                    std::cout << "Lines cleared: " << cleared << " | Score: " << score << std::endl;
                }

                // Promote next piece to current and generate a new next piece
                current = next;
                next = spawnPiece();

                if (!isValidPosition(current)) {
                    gameOver = true;
                    std::cout << "GAME OVER! Final Score: " << score << std::endl;
                }
            }
            fallClock.restart();
        }

        // Render
        window.clear(Color(15, 15, 20));

        // 1. Draw the 10x20 grid and locked blocks
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                cellShape.setPosition(c * BLOCK_SIZE + 1, r * BLOCK_SIZE + 1);
                cellShape.setFillColor(PIECE_COLORS[board[r][c]]);
                window.draw(cellShape);
            }
        }

        // 2. Draw the active falling piece
        if (!gameOver) {
            cellShape.setFillColor(PIECE_COLORS[current.type + 1]);
            for (int i = 0; i < 4; i++) {
                int bx = current.x + current.blocks[i][0];
                int by = current.y + current.blocks[i][1];
                if (by >= 0) {
                    cellShape.setPosition(bx * BLOCK_SIZE + 1, by * BLOCK_SIZE + 1);
                    window.draw(cellShape);
                }
            }
        }

        // 3. Draw the Next Piece preview box & queued piece
        // Highlight preview border cyan when Block Skip is ready
        previewFrame.setOutlineColor(skipCooldown == 0 ? Color::Cyan : Color(70, 70, 85));
        window.draw(previewFrame);

        float xOffset = (next.type == 0 || next.type == 3) ? 0.0f : (BLOCK_SIZE / 2.0f);
        float yOffset = BLOCK_SIZE;

        cellShape.setFillColor(PIECE_COLORS[next.type + 1]);
        for (int i = 0; i < 4; i++) {
            float px = PREVIEW_X + xOffset + (next.blocks[i][0] * BLOCK_SIZE) + 1;
            float py = PREVIEW_Y + yOffset + (next.blocks[i][1] * BLOCK_SIZE) + 1;
            cellShape.setPosition(px, py);
            window.draw(cellShape);
        }

        // 4. Draw the 3 Block Skip cooldown pips under the preview box
        int chargedPips = SKIP_COOLDOWN_MAX - skipCooldown; // 0 to 3 filled pips
        for (int i = 0; i < SKIP_COOLDOWN_MAX; i++) {
            pipShape.setPosition(PREVIEW_X + i * 44.0f, PREVIEW_Y + PREVIEW_BOX_SIZE + 16.0f);
            if (i < chargedPips) {
                // Bright green when all 3 are charged (ready), yellow while charging
                pipShape.setFillColor(skipCooldown == 0 ? Color(50, 220, 120) : Color(230, 180, 40));
            }
            else {
                pipShape.setFillColor(Color(30, 30, 38));
            }
            window.draw(pipShape);
        }

        window.display();
    }

    return 0;
}