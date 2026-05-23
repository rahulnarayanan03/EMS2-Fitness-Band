#include "game.h"

Game::Game() {
    resetGame();
    _difficulty = Game::MEDIUM;
    _game_state = Game::IDLE;
}

Game::Game(Game::Difficulty difficulty) {
    resetGame();
    _difficulty = difficulty;
    _game_state = Game::IDLE;
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
    if (row > 3 || col > 3 || row < 1 || col < 1) {
        Serial.println("Warning: row/col out of bounds");
        return;
    } else {
        _grid_state[row-1][col-1] = Game::OCCUPIED_O;
        return;
    }
}

void Game::placeX(int row, int col) {
    if (row > 3 || col > 3 || row < 1 || col < 1) {
        Serial.println("Warning: row/col out of bounds");
        return;
    } else {
        _grid_state[row-1][col-1] = Game::OCCUPIED_X;
        return;
    }
}

void Game::resetGame() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            _grid_state[i][j] = Game::FREE;
        }
    }

    _game_state = Game::IDLE;
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

void Game::playGame() {
    _game_state = Game::HUMAN_TURN;
    mode_pressable = false;
}

void Game::endGame() {
    _game_state = Game::IDLE;
    mode_pressable = true;
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

int Game::minimax(int depth, bool isMaximizing, int alpha, int beta)
    {
        int score = 0;

        if (checkWin(Game::HUMAN))
        {
            return -1;
        }
        else if (checkWin(Game::BOT))
        {
            return 1;
        }
        else if (isBoardFull())
        {
            return 0;
        }

        if (isMaximizing)
        {
            int bestScore = INT_MIN;
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (_grid_state[i][j] == Game::FREE)
                    {
                        _grid_state[i][j] = Game::OCCUPIED_O;
                        int currentScore = minimax(depth + 1, false, alpha, beta);
                        _grid_state[i][j] = Game::FREE;
                        bestScore = std::max(bestScore, currentScore);
                        alpha = std::max(alpha, bestScore);

                        if (beta <= alpha)
                            break;
                    }
                }
            }
            score = bestScore;
        }
        else
        {
            int bestScore = INT_MAX;
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (_grid_state[i][j] == Game::FREE)
                    {
                        _grid_state[i][j] = Game::OCCUPIED_X;
                        int currentScore = minimax(depth + 1, true, alpha, beta);
                        _grid_state[i][j] = Game::FREE;
                        bestScore = std::min(bestScore, currentScore);
                        beta = std::min(beta, bestScore);

                        if (beta <= alpha)
                            break;
                    }
                }
            }
            score = bestScore;
        }

        return score;
    }

void Game::runBotMove() {
        int bestScore = INT_MIN;
        int bestMoveRow = -1;
        int bestMoveCol = -1;

        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (_grid_state[i][j] == Game::FREE)
                {
                    _grid_state[i][j] = Game::OCCUPIED_O; // Bot's move
                    int moveScore = minimax(0, false, INT_MIN, INT_MAX);
                    _grid_state[i][j] = Game::FREE;

                    if (moveScore > bestScore)
                    {
                        bestScore = moveScore;
                        bestMoveRow = i;
                        bestMoveCol = j;
                    }
                }
            }
        }

        if (bestMoveRow != -1 && bestMoveCol != -1)
        {
            placeO(bestMoveRow, bestMoveCol);
        }
    }