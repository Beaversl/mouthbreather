#include "mouthbreather.hpp"

using namespace mouthbreather;

// convert command line arguments into game parameters
Game_Parameters mouthbreather::get_parameters(int& number_of_arguments, char** arguments)
{
    Game_Parameters parameters;

    if(number_of_arguments != 3 &&
       number_of_arguments != 4) // improper cl arguments, or none specified, either way: defaults
    {
        parameters.Default();
        return parameters;
    }

    // check the arguments
    std::string convert{ arguments[1] }; // x
    try {
        parameters.size.x = std::stoi(convert, nullptr);
    } catch(const std::invalid_argument& ia) {
        std::cerr << ia.what() << ": cannot convert command line argument to integer" << std::endl;
        parameters.Default();
        return parameters;
    }
    convert = arguments[2]; // y
    try {
        parameters.size.y = std::stoi(convert, nullptr);
    } catch(const std::invalid_argument& ia) {
        std::cerr << ia.what() << ": cannot convert command line argument to integer" << std::endl;
        parameters.Default();
        return parameters;
    }
    if(parameters.size.x > SIZE_LIMIT_ || parameters.size.y > SIZE_LIMIT_) {
        std::cerr << "parameters passed are too large" << std::endl;
        parameters.Default();
        return parameters;
    }
    if(parameters.size.x < SIZE_MININUM_ || parameters.size.y < SIZE_MININUM_) {
        std::cerr << "size cannot be smaller than: " << SIZE_MININUM_ << std::endl;
        parameters.Default();
        return parameters;
    }
    if(number_of_arguments == 4) {
        convert = arguments[3]; // frequency
        try {
            parameters.frequency = std::stof(convert, nullptr);
        } catch(const std::invalid_argument& ia) {
            std::cerr << ia.what() << ": cannot convert command line argument to float" << std::endl;
            parameters.Default();
            return parameters;
        }
        if(parameters.frequency > 1 || parameters.frequency <= 0) {
            std::cerr << "frequency parameter is unacceptable, apply yourself" << std::endl;
            parameters.frequency = DEFAULT_FREQUENCY_;
            return parameters;
        }
    } else {
        parameters.frequency = DEFAULT_FREQUENCY_;
    }
    return parameters;
}

// create an indexed uniform grid of cells
mouthbreather::Grid::Grid(Coordinates size, Cell** grid)
{
    // set coordinates to real size of the Grid
    Grid::_size = size;
    ++_size.x;
    ++_size.y;
    Grid::_contents = grid;

    // how many characters wide the cell will be
    int cell_width = 1;
    int power_of_ten = 10;
    while(power_of_ten <= Grid::_size.x || power_of_ten <= Grid::_size.y) {
        power_of_ten = power_of_ten * 10;
        ++cell_width;
    }
    std::string display = MOUTHBREATHER_CELL_SYMBOL_;
    if(display.size() > cell_width)
        cell_width = display.size();
    display = UNKNOWN_CELL_SYMBOL_;
    if(display.size() > cell_width)
        cell_width = display.size();
    Grid::_cell_size = cell_width;

    // fill left column with letters
    for(int i = 1; i < _size.y; ++i) {
        _contents[0][_size.y - i].display = number_to_letter(i);
        _contents[0][_size.y - i].display.insert(0, _cell_size - _contents[0][_size.y - i].display.size(),
                                                 ' '); // pad left side of cell with whitespace
    }

    // fill origin of cell
    _contents[0][0].display.insert(0, Grid::_cell_size, ' ');

    // create display-cell contents
    int padding_right = (_cell_size - display.size()) / 2;
    int padding_left = ((_cell_size - display.size()) + 1) / 2;
    display.append(padding_right, ' ');
    display.insert(0, padding_left, ' ');

    power_of_ten = 10;
    cell_width = 1;

    for(int x = 1; x < _size.x; ++x) {
        // fill cells
        for(int y = 1; y < _size.y; ++y) {
            _contents[x][y].display = display;
            _contents[x][y].actual = 0;
        }

        // add the numbers to bottom row for index
        while(power_of_ten <= x) {
            power_of_ten = power_of_ten * 10;
            ++cell_width;
        }
        _contents[x][0].display = std::to_string(x);
        _contents[x][0].display.insert(0, Grid::_cell_size - cell_width, ' '); // pad left side of cell with whitespace
        _contents[x][0].actual = 0;
    }
}

int mouthbreather::Grid::seed(Coordinates& avoid, float& frequency)
{
    // init
    srand(time(nullptr));
    int size_x = Grid::_size.x - 1;
    int size_y = Grid::_size.y - 1;
    int mouthbreather_count = round(double(size_x * size_y) * frequency);
    Coordinates mouthbreather_locations[mouthbreather_count];
    // seed grid without duplicates, and not in the squares surrounding the cell the user first selected
    std::vector<Coordinates> cells_to_avoid = bordering_cells_coordinates(avoid);
    cells_to_avoid.push_back(avoid);

    for(int i = 0; i < mouthbreather_count; ++i) {
    try_again:
        mouthbreather_locations[i].x = (rand() % size_x) + 1;
        mouthbreather_locations[i].y = (rand() % size_x) + 1;
        for(int d = i - 1; d >= 0; --d) { // check for duplicates
            if(mouthbreather_locations[i] == mouthbreather_locations[d]) {
                goto try_again;
            }
        }
        for(std::vector<mouthbreather::Coordinates>::size_type a = 0; a < cells_to_avoid.size();
            ++a) { // make sure it isn't in the  starting area
            if(mouthbreather_locations[i] == cells_to_avoid[a]) {
                goto try_again;
            }
        }
    }
    for(int i = 0; i < mouthbreather_count; ++i) {
        _contents[mouthbreather_locations[i].x][mouthbreather_locations[i].y].actual = -1;
        std::vector<Cell*> bordering_cells = border_cells(mouthbreather_locations[i]);
        for(auto r : bordering_cells) {
            if(r->actual != -1)
                ++r->actual;
        }
    }

    for(std::vector<mouthbreather::Coordinates>::size_type i = 0; i < cells_to_avoid.size();
        ++i) // clear the starting place for the player
    {
        select(cells_to_avoid[i]);
    }
    return mouthbreather_count;
}

// returns all the cells surrounding a cell at specified coordinates
std::vector<Cell*> mouthbreather::Grid::border_cells(Coordinates& cell_coordinates)
{
    std::vector<Cell*> bordering_cells;
    if(cell_coordinates.x == 1) // left edge
    {
        if(cell_coordinates.y == Grid::_size.y - 1) // top left corner
        {
            bordering_cells.reserve(3);
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y - 1]); // below
            bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y]); // to the right
            bordering_cells.push_back(
                &_contents[cell_coordinates.x + 1][cell_coordinates.y - 1]); // to the right and below
        } else if(cell_coordinates.y == 1)                                   // bottom left corner
        {
            bordering_cells.reserve(3);
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y + 1]); // above
            bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y]); // to the right
            bordering_cells.push_back(
                &_contents[cell_coordinates.x + 1][cell_coordinates.y + 1]); // to the right and above
        } else {
            bordering_cells.reserve(5);
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y + 1]); // above
            bordering_cells.push_back(
                &_contents[cell_coordinates.x + 1][cell_coordinates.y + 1]);                   // to the right and above
            bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y]); // to the right
            bordering_cells.push_back(
                &_contents[cell_coordinates.x + 1][cell_coordinates.y - 1]);                   // to the right and below
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y - 1]); // below
        }
    } else if(cell_coordinates.x == Grid::_size.x - 1) // right edge
    {
        if(cell_coordinates.y == Grid::_size.y - 1) // top right corner
        {
            bordering_cells.reserve(3);
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y - 1]); // below
            bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y]); // to the left
            bordering_cells.push_back(
                &_contents[cell_coordinates.x - 1][cell_coordinates.y - 1]); // to the left and below
        } else if(cell_coordinates.y == 1)                                   // bottom right corner
        {
            bordering_cells.reserve(3);
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y + 1]); // above
            bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y]); // to the left
            bordering_cells.push_back(
                &_contents[cell_coordinates.x - 1][cell_coordinates.y + 1]); // to the left and above
        } else {
            bordering_cells.reserve(5);
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y + 1]); // above
            bordering_cells.push_back(
                &_contents[cell_coordinates.x - 1][cell_coordinates.y + 1]);                   // to the left and above
            bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y]); // to the left
            bordering_cells.push_back(
                &_contents[cell_coordinates.x - 1][cell_coordinates.y - 1]);                   // to the left and below
            bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y - 1]); // below
        }
    } else if(cell_coordinates.y == 1) // bottom edge
    {
        bordering_cells.reserve(5);
        bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y]);     // to the left
        bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y + 1]); // to the left and above
        bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y + 1]);     // above
        bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y + 1]); // to the right and above
        bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y]);     // to the right

    } else if(cell_coordinates.y == _size.y - 1) // top edge
    {
        bordering_cells.reserve(5);
        bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y]);     // to the left
        bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y - 1]); // to the left and below
        bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y - 1]);     // below
        bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y - 1]); // to the right and below
        bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y]);     // to the right

    } else if(cell_coordinates.x < _size.x - 1 && cell_coordinates.x >= 1 && cell_coordinates.y < _size.y - 1 &&
              cell_coordinates.y >= 1) // all around + check to make sure the coordinate is even in bounds of the array
    {
        bordering_cells.reserve(8);
        bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y]);     // to the left
        bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y + 1]); // to the left and above
        bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y + 1]);     // above
        bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y + 1]); // to the right and above
        bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y]);     // to the right
        bordering_cells.push_back(&_contents[cell_coordinates.x + 1][cell_coordinates.y - 1]); // to the right and below
        bordering_cells.push_back(&_contents[cell_coordinates.x][cell_coordinates.y - 1]);     // below
        bordering_cells.push_back(&_contents[cell_coordinates.x - 1][cell_coordinates.y - 1]); // to the left and below
    }
    return bordering_cells;
}

std::vector<Coordinates> mouthbreather::Grid::bordering_cells_coordinates(Coordinates& cell_coordinates)
{
    std::vector<Coordinates> bordering_coordinates;
    if(cell_coordinates.x == 1) // left edge
    {
        if(cell_coordinates.y == Grid::_size.y - 1) // top left corner
        {
            bordering_coordinates.reserve(3);
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y - 1)); // below
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x + 1, cell_coordinates.y)); // to the right
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x + 1, cell_coordinates.y - 1)); // to the right and below
        }
        if(cell_coordinates.y == 1) // bottom left corner
        {
            bordering_coordinates.reserve(3);
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y + 1)); // above
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x + 1, cell_coordinates.y)); // to the right
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x + 1, cell_coordinates.y + 1)); // to the right and above
        } else {
            bordering_coordinates.reserve(5);
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y + 1)); // above
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x + 1, cell_coordinates.y + 1)); // to the right and above
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x + 1, cell_coordinates.y)); // to the right
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x + 1, cell_coordinates.y - 1)); // to the right and below
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y - 1)); // below
        }
    } else if(cell_coordinates.x == Grid::_size.x - 1) // right edge
    {
        if(cell_coordinates.y == Grid::_size.y - 1) // top right corner
        {
            bordering_coordinates.reserve(3);
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y - 1)); // below
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x - 1, cell_coordinates.y)); // to the left
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x - 1, cell_coordinates.y - 1)); // to the left and below
        }
        if(cell_coordinates.y == 1) // bottom right corner
        {
            bordering_coordinates.reserve(3);
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y + 1)); // above
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x - 1, cell_coordinates.y)); // to the left
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x - 1, cell_coordinates.y + 1)); // to the left and above
        } else {
            bordering_coordinates.reserve(5);
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y + 1)); // above
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x - 1, cell_coordinates.y + 1)); // to the left and above
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x - 1, cell_coordinates.y)); // to the left
            bordering_coordinates.push_back(
                Coordinates(cell_coordinates.x - 1, cell_coordinates.y - 1)); // to the left and below
            bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y - 1)); // below
        }
    } else if(cell_coordinates.y == 1) // bottom edge
    {
        bordering_coordinates.reserve(5);
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x - 1, cell_coordinates.y)); // to the left
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x - 1, cell_coordinates.y + 1)); // to the left and above
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y + 1)); // above
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x + 1, cell_coordinates.y + 1)); // to the right and above
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x + 1, cell_coordinates.y)); // to the right

    } else if(cell_coordinates.y == _size.y - 1) // top edge
    {
        bordering_coordinates.reserve(5);
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x - 1, cell_coordinates.y)); // to the left
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x - 1, cell_coordinates.y - 1)); // to the left and below
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y - 1)); // below
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x + 1, cell_coordinates.y - 1)); // to the right and below
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x + 1, cell_coordinates.y)); // to the right

    } else if(in_bounds(cell_coordinates)) // all around + check to make sure the coordinate is even in bounds of
                                           // the array
    {
        bordering_coordinates.reserve(8);
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x - 1, cell_coordinates.y)); // to the left
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x - 1, cell_coordinates.y + 1)); // to the left and above
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y + 1)); // above
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x + 1, cell_coordinates.y + 1)); // to the right and above
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x + 1, cell_coordinates.y)); // to the right
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x + 1, cell_coordinates.y - 1)); // to the right and below
        bordering_coordinates.push_back(Coordinates(cell_coordinates.x, cell_coordinates.y - 1)); // below
        bordering_coordinates.push_back(
            Coordinates(cell_coordinates.x - 1, cell_coordinates.y - 1)); // to the left and below
    }
    return bordering_coordinates;
}

// print the grid to console
void mouthbreather::Grid::display()
{
    for(int y = 1; y <= _size.y; ++y) {
        for(int x = 0; x < _size.x; ++x) {
            std::cout << _contents[x][_size.y - y].display << "|";
            //<< _contents[x][_size.y - y].actual
        }
        std::cout << std::endl;
    }
}

// convert a number to a Spreadsheet like index
std::string mouthbreather::number_to_letter(int number)
{
    std::string letters;
    int power_of_26 = 26;
    while(power_of_26 < number) {
        power_of_26 = power_of_26 * 26;
    }
    power_of_26 = power_of_26 / 26;
    int digit = number / power_of_26;    // base 26 digit
    while(digit * power_of_26 != number) // there is a remainder
    {
        letters += char(digit + 64); // ascii transform base 26 to letters
        number = number - (digit * power_of_26);
        power_of_26 = power_of_26 / 26;
        digit = number / power_of_26;
    }
    letters += char(digit + 64);
    return letters;
}

// convert a Spreadsheet like index to a number
int mouthbreather::letter_to_number(std::string letter)
{
    int number = 0;
    int size = letter.size();
    for(int check = 0; check < size; ++check) {
        if(letter[check] < 65 || letter[check] > 90) {
            return -1;
        }
        number += pow((letter[check] - 65), size - check);
    }
    return number;
}

Coordinates mouthbreather::user_choice(Coordinates& size)
{
    std::string buffer;
    Coordinates choice;
try_again:
    std::cout << "(x y): " << std::flush;
    // x
    std::cin >> buffer;
    try {
        choice.x = std::stoi(buffer, nullptr);
    } catch(const std::invalid_argument& ia) {
        std::cerr << ia.what() << ": cannot convert x coordinate to integer" << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear y out so it can run again neatly
        goto try_again;
    }
    if(choice.x > size.x || choice.x == 0) {
        std::cerr << "invalid x coordinate" << std::endl;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear y out so it can run again neatly
        goto try_again;
    }
    // y
    std::cin >> buffer;
    choice.y = letter_to_number(buffer);
    if(choice.y < 0 || choice.y >= size.y) {
        std::cerr << "invalid y coordinate" << std::endl;
        goto try_again;
    }
    choice.y = size.y - choice.y;
    return choice;
}

// if user wants to select a cell or flag it as containing a gross mouthbreather
bool mouthbreather::wants_to_flag()
{
    std::string buffer;
try_again:
    std::cout << "flag (f) or select (s)? " << std::flush;
    std::cin >> buffer;
    if(buffer == "f") {
        return true;
    } else if(buffer == "s") {
        return false;
    }
    std::cout << "\ninvalid choice, try again" << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
                    '\n'); // i usually accidently type in the coordinates, TODO make it automatically select the
                           // coordinates, unless prefixed with an f ( f 5 6, for flag, else 5 6)
    goto try_again;
}

bool mouthbreather::Grid::select(Coordinates& cell_coordinates)
{
    if(in_bounds(cell_coordinates)) {
        Cell* selection = get_cell(cell_coordinates);
        if(!selection->selected) {
            ++total_cells_selected;
            selection->selected = true;
            if(selection->actual == -1) {
                selection->display = MOUTHBREATHER_CELL_SYMBOL_;
                selection->display.append((_cell_size - selection->display.size()) / 2, ' ');          // padding right
                selection->display.insert(0, ((_cell_size - selection->display.size()) + 1) / 2, ' '); // padding left
                return false;
            } else if(selection->actual == 0) {
                auto_clear(cell_coordinates, selection);
            } else {
                selection->display = char(selection->actual + 48);
                // int padding_right = (Grid::_cell_size - 1) / 2;
                // int padding_left = ((Grid::_cell_size - 1) + 1) / 2;
                selection->display.append((_cell_size - 1) / 2, ' ');          // padding right
                selection->display.insert(0, ((_cell_size - 1) + 1) / 2, ' '); // padding left
            }
        }
    }
    return true;
}

// mark a cell as a mouthbreather
void mouthbreather::Grid::flag(Coordinates& cell_coordinates)
{
    // TODO if you unflag a cell that has already been selected, it will show it as a mystery, and you can't unselect it
    if(in_bounds(cell_coordinates)) {
        Cell* selection = get_cell(cell_coordinates);
        if(selection->display.find(WARNING_CELL_SYMBOL_) != std::string::npos) // unflag
        {
            selection->display = UNKNOWN_CELL_SYMBOL_;
            // int padding_right = (Grid::_cell_size - size of unknown cell symbol) / 2;
            // int padding_left = ((Grid::_cell_size - size of unknown cell symbol) + 1) / 2;
            selection->display.append((_cell_size - selection->display.size()) / 2, ' ');          // padding right
            selection->display.insert(0, ((_cell_size - selection->display.size()) + 1) / 2, ' '); // padding left

        } else { // flag
            selection->display = WARNING_CELL_SYMBOL_;
            // int padding_right = (Grid::_cell_size - size of warning_cell_symbol) / 2;
            // int padding_left = ((Grid::_cell_size - size of warning_cell_symbol) + 1) / 2;
            selection->display.append((_cell_size - selection->display.size()) / 2, ' ');          // padding right
            selection->display.insert(0, ((_cell_size - selection->display.size()) + 1) / 2, ' '); // padding left
        }
    }
}

Cell* mouthbreather::Grid::get_cell(Coordinates& cell_coordinates)
{
    if(in_bounds(cell_coordinates))
        return &_contents[cell_coordinates.x][cell_coordinates.y];
    else
        return nullptr;
}

void mouthbreather::Grid::auto_clear(Coordinates& cell_coordinates, Cell* selection)
{
    if(in_bounds(cell_coordinates)) {
        selection->display = " ";
        selection->display.insert(0, _cell_size - 1, ' '); // pad cells
        std::vector<Coordinates> surrounding_cells = bordering_cells_coordinates(cell_coordinates);
        for(std::vector<mouthbreather::Coordinates>::size_type i = 0; i < surrounding_cells.size(); ++i) {
            select(surrounding_cells[i]);
        }
    }
}

inline bool mouthbreather::Grid::in_bounds(Coordinates& cell_coordinates)
{
    return cell_coordinates.x <= Grid::_size.x - 1 && cell_coordinates.x >= 1 &&
           cell_coordinates.y <= Grid::_size.y - 1 && cell_coordinates.y >= 1;
}