#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace mouthbreather
{
constexpr int SIZE_LIMIT_ = 511; // don't want to put a lot on the stack, plus I hear it's good general practice to have
                                 // hard limits in all of your programs
constexpr int SIZE_MININUM_ = 5;
constexpr int DEFAULT_SIZE_ = 5;
constexpr float DEFAULT_FREQUENCY_ = .2;
constexpr auto UNKNOWN_CELL_SYMBOL_ = "."; // "\\_(\\\")_/" = \_(\")_/
constexpr auto MOUTHBREATHER_CELL_SYMBOL_ = ":O";
constexpr auto WARNING_CELL_SYMBOL_ = "+";

struct Coordinates {
    Coordinates(){};
    Coordinates(int ex, int why)
        : x(ex)
        , y(why){};

    bool operator==(Coordinates const& c) const // don't know what this extra const does, but i'm keeping it
    {
        if(this->x == c.x && this->y == c.y)
            return true;
        else
            return false;
    }
    int x;
    int y;
};

struct Game_Parameters {
    Coordinates size;
    float frequency;
    void Default()
    {
        frequency = DEFAULT_FREQUENCY_;
        size.x = DEFAULT_SIZE_;
        size.y = DEFAULT_SIZE_;
    }
};

struct Cell {
    int actual;          // TODO make this an enum (-1 = uh_oh_mouthbreather, or something)
    std::string display; // this would be more efficient if it was a string pointer, but i don't want to mess around
                         // with all that
    bool selected = false;
};

class Grid
{
    Cell** _contents;
    /*
     * y
     * |
     * |
     * |
     * 0______x
     */
    Coordinates _size;
    int _cell_size;

    // returns all the cells surrounding a cell at specified coordinates
    std::vector<Cell*> border_cells(Coordinates& location);
    std::vector<Coordinates> bordering_cells_coordinates(Coordinates& location);
    void auto_clear(Coordinates& location, Cell* selection);

    // return cell at location
    Cell* get_cell(Coordinates& location);
    inline bool in_bounds(Coordinates& location);
    int total_cells_selected = 0;

public:
    // create an indexed uniform grid of cells
    Grid(Coordinates size, Cell** grid); // TODO make this a singleton, and call the constructor from the seed function
    int seed(Coordinates& avoid, float& frequency);
    // print the grid to console
    void display();
    bool select(Coordinates& location);
    // mark a cell as a mouthbreather
    void flag(Coordinates& location);
    int size() { return (_size.x - 1) * (_size.y - 1); }
    int number_selected() { return total_cells_selected; }
};

// convert command line arguments into game parameters
Game_Parameters get_parameters(int& number_of_arguments, char** arguments);

// convert a number to a Spreadsheet like index
std::string number_to_letter(int number);

// convert a Spreadsheet like index to a number
int letter_to_number(std::string letter);

Coordinates user_choice(Coordinates& size);

// if the user wants to
bool wants_to_flag();

inline void increment(Cell& c);

} // namespace mouthbreather