class Game {
public:
    // State of each grid
    enum GridState {
        OCCUPIED_O,
        OCCUPIED_X,
        FREE
    };

    // Returns true if a cell is occupied, false if it is free
    bool getOccupied(int row, int col);

    // Places an 'O' marker in a cell
    void placeO(int row, int col);

    // Places an 'X' marker in a cell
    void placeX(int row, int col);
    
private:
    GridState game_state[3][3];
};