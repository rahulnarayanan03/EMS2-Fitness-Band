#pragma once

class Game {
public:
    // State of each grid
    enum GridState {
        OCCUPIED_O,
        OCCUPIED_X,
        FREE
    };

    // Returns the state of a cell
    // Row and col values range from 1 to 3
    GridState getCellState(int row, int col);

    // Places an 'O' marker in a cell
    // Row and col values range from 1 to 3
    void placeO(int row, int col);

    // Places an 'X' marker in a cell
    // Row and col values range from 1 to 3
    void placeX(int row, int col);
    
private:
    GridState game_state[3][3];
};