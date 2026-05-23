#pragma once

#include <Arduino.h>

class Game {
public:
    // State of the game
    enum GameState {
        IDLE,
        HUMAN_TURN,
        BOT_TURN,
        HUMAN_WIN,
        BOT_WIN
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

    // Places an 'O' marker in a cell
    // Row and col values range from 1 to 3
    void placeO(int row, int col);

    // Places an 'X' marker in a cell
    // Row and col values range from 1 to 3
    void placeX(int row, int col);

    // Frees all cells in the grid
    void resetGrid();

    bool isBoardFull();

    // Check if a player has won
    bool checkWin(Player player);

    // Sets the difficulty of the game
    void setDifficulty(Difficulty difficulty);

    // Runs minimax algorithm with alpha/beta pruning
    int minimax(int depth, bool isMaximizing, int alpha, int beta);

    // Runs the bot's move
    void runBotMove();
    
private:
    // State of the game
    GameState _game_state;

    // Individual cell states
    GridState _grid_state[3][3];

    // Difficulty level of the game
    Difficulty _difficulty;
};