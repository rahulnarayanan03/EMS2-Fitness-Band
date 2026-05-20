#include "game.h"

Game::Game() {
    resetGrid();
    _difficulty = Game::MEDIUM;
    _game_state = Game::HUMAN_TURN;
}

Game::Game(Game::Difficulty difficulty) {
    resetGrid();
    _difficulty = difficulty;
    _game_state = Game::HUMAN_TURN;
}

Game::GameState Game::getGameState() {
    return _game_state;
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

bool Game::checkWin(Game::Player player) {
    if (player == Game::HUMAN) {
        for (int i = 0; i < 3; ++i) {
            if (_grid_state[i][0] == Game::OCCUPIED_X && _grid_state[i][1] == Game::OCCUPIED_X && _grid_state[i][2] == Game::OCCUPIED_X) {
                return true;
            }
            if (_grid_state[0][i] == Game::OCCUPIED_X && _grid_state[1][i] == Game::OCCUPIED_X && _grid_state[2][i] == Game::OCCUPIED_X) {
                return true;
            }
        }
        
        if (_grid_state[0][0] == Game::OCCUPIED_X && _grid_state[1][1] == Game::OCCUPIED_X && _grid_state[2][2] == Game::OCCUPIED_X) {
            return true;
        }
        if (_grid_state[0][2] == Game::OCCUPIED_X && _grid_state[1][1] == Game::OCCUPIED_X && _grid_state[2][0] == Game::OCCUPIED_X) {
            return true;
        }

        return false;
    } else {
        for (int i = 0; i < 3; ++i) {
            if (_grid_state[i][0] == Game::OCCUPIED_O && _grid_state[i][1] == Game::OCCUPIED_O && _grid_state[i][2] == Game::OCCUPIED_O) {
                return true;
            }
            if (_grid_state[0][i] == Game::OCCUPIED_O && _grid_state[1][i] == Game::OCCUPIED_O && _grid_state[2][i] == Game::OCCUPIED_O) {
                return true;
            }
        }
        
        if (_grid_state[0][0] == Game::OCCUPIED_O && _grid_state[1][1] == Game::OCCUPIED_O && _grid_state[2][2] == Game::OCCUPIED_O) {
            return true;
        }
        if (_grid_state[0][2] == Game::OCCUPIED_O && _grid_state[1][1] == Game::OCCUPIED_O && _grid_state[2][0] == Game::OCCUPIED_O) {
            return true;
        }
        
        return false;
    }
}

void Game::setDifficulty(Difficulty difficulty) {
    _difficulty = difficulty;
}

std::pair<int, int> Game::getOptimalMove() {

}

void Game::runBotMove() {

}