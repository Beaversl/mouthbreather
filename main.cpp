/* Logan Beavers 2022/11/16
 * Minesweeper type game
 * Assignment 11
 *

 *
 * Extra Work
 * : User can decide how big grid will be and the frequency of mines
 * : grid is uniformly sized (if I had another 10 hours I would make the cell width grow as needed to make the columns
 * line up) 
 * : Grid is generated after user guesses a square to avoid potentially ending game on first guess moved the game
 * : code to seperate files for readability added constants in the code that can be used to change certain
 * characteristics of the game
 * : if a player uncovers a cell with zero neighboring mouth breathers then it recursively
 * checks and clears all neighboring cells
 */

#include "mouthbreather.hpp"
#include <iostream>

using namespace mouthbreather;

int main(int argc, char** argv)
{
	std::cout << "welcome to mouthbreather, a minesweeper like game, except you are marking out a room full of mouthbreathers" << std::endl;
    // initialize
    Game_Parameters parameters = get_parameters(argc, argv);

    // create the needed space on the stack
    Cell ___space_on_stack___[(parameters.size.x + 1) * (parameters.size.y + 1)];
    Cell* ___cell_ptr_array___[parameters.size.x + 1];
    for(int i = 0; i < (parameters.size.x + 1); ++i) {
        ___cell_ptr_array___[i] = &___space_on_stack___[i * (parameters.size.y + 1)];
    }
    // the nightmare is over
    Grid room(parameters.size, ___cell_ptr_array___);

    room.display();
    Coordinates avoid = user_choice(parameters.size);
    int number_of_mouthbreathers = room.seed(avoid, parameters.frequency);
    room.display();

    Coordinates choice;
    int number_of_selectable_cells = room.size() - number_of_mouthbreathers;
    while(room.number_selected() < number_of_selectable_cells) {
        if(wants_to_flag()) {
            choice = user_choice(parameters.size);
            room.flag(choice);
        } else {
            choice = user_choice(parameters.size);
            if(!room.select(choice)) // gross, shared space with a mouthbreather
            {
                room.display();
                break;
            }
        }
        room.display();
    }
    if(room.number_selected() == number_of_selectable_cells) // won
    {
        std::cout << "the world thanks you!" << std::endl;
    } else { // lost
        std::cout << "their wicked breath haunts you" << std::endl;
    }
    return 0;
}