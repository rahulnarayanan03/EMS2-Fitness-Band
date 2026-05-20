#include "game.h"

Game::Game() {
    resetGrid();
    _difficulty = Game::MEDIUM;
}

Game::Game(Game::Difficulty difficulty) {
    resetGrid();
    _difficulty = difficulty;
}

Game::GridState Game::getCellState(int row, int col) {
    if (row > 3 || col > 3 || row < 1 || col < 1) {
        Serial.println("Warning: row/col out of bounds");
        return Game::FREE;
    } else {
        return _grid_state[row-1][col-1];
    }
}

void Game::placeO(int row, int col) {
    _grid_state[row-1][col-1] = Game::OCCUPIED_O;
    return;
}

void Game::placeX(int row, int col) {
    _grid_state[row-1][col-1] = Game::OCCUPIED_X;
    return;
}

void Game::resetGrid() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            _grid_state[i][j] = Game::FREE;
        }
    }
}

bool Game::isBoardFull() {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (_grid_state[i][j] == Game::FREE) {
                return false;
            }
        }
    }
    return true;
}

void Game::setDifficulty(Difficulty difficulty) {
    _difficulty = difficulty;
}

std::pair<int, int> Game::getOptimalMove() {

}

void Game::runBotMove() {

}