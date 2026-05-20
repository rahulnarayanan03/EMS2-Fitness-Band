#pragma once

#include <Arduino.h>

class Game {
public:
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

    // Default constructor
    Game();

    // Constructor with difficulty level
    Game(Difficulty difficulty);

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

    // Sets the difficulty of the game
    void setDifficulty(Difficulty difficulty);

    // Finds the most optimal move at the game state
    std::pair<int, int> getOptimalMove();

    // Runs the bot's move
    void runBotMove();
    
private:
    // Individual cell states
    GridState _game_state[3][3];

    // Difficulty level of the game
    Difficulty _difficulty;
};