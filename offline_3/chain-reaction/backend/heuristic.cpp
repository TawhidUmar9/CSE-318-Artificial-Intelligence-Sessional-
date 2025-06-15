#include "board.h"
#include <iostream>

int evaluate_heuristic(const Board &board, char player_color)
{
    int player_progress = 0;
    int opponent_progress = 0;
    char opponent_color = (player_color == 'R') ? 'B' : 'R';

    for (int r = 0; r < board.rows; ++r)
    {
        for (int c = 0; c < board.cols; ++c)
        {
            if (board.grid[r][c].color == board.char_to_orb_color(player_color))
            {
                int critical_mass = board.get_critical_mass(r, c);
                if (critical_mass > 0)
                {
                    player_progress += static_cast<double>(board.grid[r][c].count) / critical_mass;
                }
            }
            else if (board.grid[r][c].color == board.char_to_orb_color(opponent_color))
            {
                int critical_mass = board.get_critical_mass(r, c);
                if (critical_mass > 0)
                {
                    opponent_progress += static_cast<double>(board.grid[r][c].count) / critical_mass;
                }
            }
        }
    }

    int eval = player_progress - opponent_progress;
    return eval;
}