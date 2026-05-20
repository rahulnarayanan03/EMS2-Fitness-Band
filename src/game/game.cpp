#include "game.h"

Game::Game() {

}

Game::Game(Game::Difficulty difficulty) {
    resetGrid();
}

Game::GridState Game::getCellState(int row, int col) {
    if (row > 3 || col > 3 || row < 1 || col < 1) {
        Serial.println("Warning: row/col out of bounds");
        return Game::FREE;
    } else {
        return game_state[row-1][col-1];
    }
}

void Game::placeO(int row, int col) {
    game_state[row-1][col-1] = Game::OCCUPIED_O;
    return;
}

void Game::placeX(int row, int col) {
    game_state[row-1][col-1] = Game::OCCUPIED_X;
    return;
}

void Game::resetGrid() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            game_state[i][j] = Game::FREE;
        }
    }
}