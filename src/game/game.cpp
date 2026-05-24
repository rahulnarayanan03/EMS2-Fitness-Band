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

std::pair<int, int> Game::getRowColTouched(uint16_t tx, uint16_t ty) {
    std::pair<int, int> touch_rowcol;

    // If the touch is along the first row
    if (ty >= GAME_Y && ty <= GAME_Y + GAME_SIZE/3) {
        // Check first column
        if (tx >= GAME_X && tx <= GAME_X + GAME_SIZE/3) {
            touch_rowcol.first = 1;
            touch_rowcol.second = 1;
            return touch_rowcol;
        // Check second column
        } else if (tx >= GAME_X + GAME_SIZE/3 && tx <= GAME_X + 2*GAME_SIZE/3) {
            touch_rowcol.first = 1;
            touch_rowcol.second = 2;
            return touch_rowcol;
        // Check third column
        } else if (tx >= GAME_X + 2*GAME_SIZE/3 && tx <= GAME_X + 3*GAME_SIZE/3) {
            touch_rowcol.first = 1;
            touch_rowcol.second = 3;
            return touch_rowcol;
        } else {
            touch_rowcol.first = 0;
            touch_rowcol.second = 0;
            return touch_rowcol;
        }
    // Check second row
    } else if (ty >= GAME_Y + GAME_SIZE/3 && ty <= GAME_Y + 2*GAME_SIZE/3) {
        // Check first column
        if (tx >= GAME_X && tx <= GAME_X + GAME_SIZE/3) {
            touch_rowcol.first = 2;
            touch_rowcol.second = 1;
            return touch_rowcol;
        // Check second column
        } else if (tx >= GAME_X + GAME_SIZE/3 && tx <= GAME_X + 2*GAME_SIZE/3) {
            touch_rowcol.first = 2;
            touch_rowcol.second = 2;
            return touch_rowcol;
        // Check third column
        } else if (tx >= GAME_X + 2*GAME_SIZE/3 && tx <= GAME_X + 3*GAME_SIZE/3) {
            touch_rowcol.first = 2;
            touch_rowcol.second = 3;
            return touch_rowcol;
        } else {
            touch_rowcol.first = 0;
            touch_rowcol.second = 0;
            return touch_rowcol;
        }
    // Check third row
    } else if (ty >= GAME_Y + 2*GAME_SIZE/3 && ty <= GAME_Y + 3*GAME_SIZE/3) {
        // Check first column
        if (tx >= GAME_X && tx <= GAME_X + GAME_SIZE/3) {
            touch_rowcol.first = 3;
            touch_rowcol.second = 1;
            return touch_rowcol;
        // Check second column
        } else if (tx >= GAME_X + GAME_SIZE/3 && tx <= GAME_X + 2*GAME_SIZE/3) {
            touch_rowcol.first = 3;
            touch_rowcol.second = 2;
            return touch_rowcol;
        // Check third column
        } else if (tx >= GAME_X + 2*GAME_SIZE/3 && tx <= GAME_X + 3*GAME_SIZE/3) {
            touch_rowcol.first = 3;
            touch_rowcol.second = 3;
            return touch_rowcol;
        } else {
            touch_rowcol.first = 0;
            touch_rowcol.second = 0;
            return touch_rowcol;
        }
    } else {
        touch_rowcol.first = 0;
        touch_rowcol.second = 0;
        return touch_rowcol;
    }
}

std::pair<int, int> Game::getLastBotMove() {
    std::pair<int, int> last_bot_move;
    last_bot_move.first = last_bot_row_col.first;
    last_bot_move.second = last_bot_row_col.second;
    return last_bot_move;
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
        _game_state = Game::BOT_TURN;
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
    resetGame();
    _game_state = Game::HUMAN_TURN;
    mode_pressable = false;
}

void Game::endGame() {
    _game_state = Game::IDLE;
    mode_pressable = true;
}

bool Game::isModePressable() {
    return mode_pressable;
}

bool Game::checkWin(Game::Player player) {
    if (player == Game::HUMAN) {
        for (int i = 0; i < 3; ++i) {
            if (_grid_state[i][0] == Game::OCCUPIED_X && _grid_state[i][1] == Game::OCCUPIED_X && _grid_state[i][2] == Game::OCCUPIED_X) {
                _game_state = Game::HUMAN_WIN;
                return true;
            }
            if (_grid_state[0][i] == Game::OCCUPIED_X && _grid_state[1][i] == Game::OCCUPIED_X && _grid_state[2][i] == Game::OCCUPIED_X) {
                _game_state = Game::HUMAN_WIN;
                return true;
            }
        }
        
        if (_grid_state[0][0] == Game::OCCUPIED_X && _grid_state[1][1] == Game::OCCUPIED_X && _grid_state[2][2] == Game::OCCUPIED_X) {
            _game_state = Game::HUMAN_WIN;
            return true;
        }
        if (_grid_state[0][2] == Game::OCCUPIED_X && _grid_state[1][1] == Game::OCCUPIED_X && _grid_state[2][0] == Game::OCCUPIED_X) {
            _game_state = Game::HUMAN_WIN;
            return true;
        }

        return false;
    } else {
        for (int i = 0; i < 3; ++i) {
            if (_grid_state[i][0] == Game::OCCUPIED_O && _grid_state[i][1] == Game::OCCUPIED_O && _grid_state[i][2] == Game::OCCUPIED_O) {
                _game_state = Game::BOT_WIN;
                return true;
            }
            if (_grid_state[0][i] == Game::OCCUPIED_O && _grid_state[1][i] == Game::OCCUPIED_O && _grid_state[2][i] == Game::OCCUPIED_O) {
                _game_state = Game::BOT_WIN;
                return true;
            }
        }
        
        if (_grid_state[0][0] == Game::OCCUPIED_O && _grid_state[1][1] == Game::OCCUPIED_O && _grid_state[2][2] == Game::OCCUPIED_O) {
            _game_state = Game::BOT_WIN;
            return true;
        }
        if (_grid_state[0][2] == Game::OCCUPIED_O && _grid_state[1][1] == Game::OCCUPIED_O && _grid_state[2][0] == Game::OCCUPIED_O) {
            _game_state = Game::BOT_WIN;
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

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (_grid_state[i][j] == Game::FREE) {
                    _grid_state[i][j] = Game::OCCUPIED_O; // Bot's move
                    int moveScore = minimax(0, false, INT_MIN, INT_MAX);
                    _grid_state[i][j] = Game::FREE;

                    if (moveScore > bestScore) {
                        bestScore = moveScore;
                        bestMoveRow = i;
                        bestMoveCol = j;
                    }
                }
            }
        }

        if (bestMoveRow != -1 && bestMoveCol != -1) {
            placeO(bestMoveRow+1, bestMoveCol+1);
            last_bot_row_col.first = bestMoveRow+1;
            last_bot_row_col.second = bestMoveCol+1;
            _game_state = Game::HUMAN_TURN;
        }
    }