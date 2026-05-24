#pragma once

#include <Arduino.h>
#include "../display/screens/screens.h"
#include <vector>

class Game {
public:
    // State of the game
    enum GameState {
        IDLE,
        HUMAN_TURN,
        BOT_TURN,
        HUMAN_WIN,
        BOT_WIN,
        DRAW
    };

    // State of each grid
    enum GridState {
        OCCUPIED_O,
        OCCUPIED_X,
        FREE
    };

    // Difficulty level of cpu opponent
    enum Difficulty {
        EASY,
        MEDIUM,
        HARD
    };

    // Player types
    enum Player {
        HUMAN,
        BOT
    };

    // Default constructor
    Game();

    // Constructor with difficulty level
    Game(Difficulty difficulty);

    // Returns the state of the game
    GameState getGameState();

    // Returns the state of a cell
    // Row and col values range from 1 to 3
    GridState getCellState(int row, int col);

    // Returns the difficulty
    Difficulty getDifficulty();

    // Returns the row and column of a cell given the location of a touch
    std::pair<int, int> getRowColTouched(uint16_t tx, uint16_t ty);

    // Returns the row and column of the last move the bot made
    std::pair<int, int> getLastBotMove();

    // Places an 'O' marker in a cell
    // Row and col values range from 1 to 3
    void placeO(int row, int col);

    // Places an 'X' marker in a cell
    // Row and col values range from 1 to 3
    void placeX(int row, int col);

    // Frees all cells in the grid, sets state to idle
    void resetGame();

    bool isBoardFull();

    std::vector<std::pair<int, int>> getFreeCells();

    // Check if a player has won
    bool checkWin(Player player);

    // Sets the difficulty of the game
    void setDifficulty(Difficulty difficulty);

    // Starts the game
    void playGame();

    // Runs minimax algorithm with alpha/beta pruning
    int minimax(int depth, bool isMaximizing, int alpha, int beta);

    // Runs the bot's move, bot plays perfectly when moving
    void runBotMove();

    // Runs the bot's move, bot plays completely randomly
    void runBotMoveEasy();

    // Runs the bot's move, bot plays perfectly only half the time, otherwise it plays randomly
    void runBotMoveMedium();

    // Runs the bot's move, bot plays perfectly almost all the time, with a small chance of playing randomly
    void runBotMoveHard();

    // Ends the game
    void endGame();

    static constexpr int BOT_DELAY_MS = 700;
    
private:
    // State of the game
    GameState _game_state;

    // Individual cell states
    GridState _grid_state[3][3];

    // Difficulty level of the game
    Difficulty _difficulty;

    // Last move the bot made
    std::pair<int, int> last_bot_row_col;

    // Probably of bot making a random move when on medium difficulty
    float p_random_medium = 0.5;

    // Probably of bot making a random move when on hard difficulty
    float p_random_hard = 0.1;
};