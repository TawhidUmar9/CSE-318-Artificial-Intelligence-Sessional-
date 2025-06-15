//some heuristic functions for the chain reaction problem
#include "board.h"

int evaluate_heuristic(const Board& state, char maximizing_player_color) {
    int player_orbs = 0;
    int opponent_orbs = 0;

    // Convert char player color to enum OrbColor for comparison
    OrbColor player_enum_color = (maximizing_player_color == 'R') ? RED : BLUE;
    OrbColor opponent_enum_color = (maximizing_player_color == 'R') ? BLUE : RED;

    // Iterate through all cells on the board
    for (int r = 0; r < state.rows; ++r) {
        for (int c = 0; c < state.cols; ++c) {
            // Get the cell details from the state's grid
            const Cell& cell = state.grid[r][c]; // Use const reference for efficiency

            if (cell.color == player_enum_color) {
                player_orbs += cell.count; // Add count of player's orbs
            } else if (cell.color == opponent_enum_color) {
                opponent_orbs += cell.count; // Add count of opponent's orbs
            }
            // Empty cells don't contribute to orb counts for either player
        }
    }

    // Simple heuristic: difference between player's orbs and opponent's orbs.
    // The maximizing player wants to maximize this difference.
    return player_orbs - opponent_orbs;
}