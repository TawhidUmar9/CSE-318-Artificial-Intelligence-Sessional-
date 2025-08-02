#include "board.h"
#include <iostream>

const int CORNER_BENEFIT_VALUE = 25;
const double W_ORB = 1.0;
const double W_EDGE = 2.5;
const double W_CORNER = 3.5;
const double W_CRITICAL = 3.0;
const double W_BLOCK = 2.5;
const double W_VULNERABLE = 1.5;
const double W_ROW = 1.0;
const double W_COL = 0.8;
const double W_DOMINANCE = 0.5;

double simple_heuristic(const Board &board, char player_color)
{
    int player_progress = 0;
    int opponent_progress = 0;
    char opponent_color = (player_color == 'R') ? 'B' : 'R';

    for (int row = 0; row < board.rows; ++row)
    {
        for (int col = 0; col < board.cols; ++col)
        {
            if (board.grid[row][col].color == board.char_to_orb_color(player_color))
            {
                int critical_mass = board.get_critical_mass(row, col);
                if (critical_mass > 0)
                {
                    player_progress += static_cast<double>(board.grid[row][col].count) / critical_mass;
                }
            }
            else if (board.grid[row][col].color == board.char_to_orb_color(opponent_color))
            {
                int critical_mass = board.get_critical_mass(row, col);
                if (critical_mass > 0)
                {
                    opponent_progress += static_cast<double>(board.grid[row][col].count) / critical_mass;
                }
            }
        }
    }

    int eval = player_progress - opponent_progress;
    return eval;
}

double corner_heuristic(const Board &board, char player_color)
{
    int score = 0;
    char opponent_color = (player_color == 'R') ? 'B' : 'R';

    std::vector<std::pair<int, int>> corners = {
        {0, 0},
        {0, board.cols - 1},
        {board.rows - 1, 0},
        {board.rows - 1, board.cols - 1}};

    for (const auto &corner : corners)
    {
        int row = corner.first;
        int col = corner.second;

        if (row >= 0 && row < board.rows && col >= 0 && col < board.cols)
        {
            if (board.grid[row][col].color == board.char_to_orb_color(player_color))
            {
                score += CORNER_BENEFIT_VALUE;
            }
            else if (board.grid[row][col].color == board.char_to_orb_color(opponent_color))
            {
                score -= CORNER_BENEFIT_VALUE;
            }
        }
    }
    return score;
}

// source : https://brilliant.org/wiki/chain-reaction-game/
double count_contiguous_critical_cells(int row, int col, const Board &board, char player_color, std::vector<std::vector<bool>> &visited)
{
    if (row < 0 || row >= board.rows || col < 0 || col >= board.cols || visited[row][col] ||
        board.grid[row][col].color != board.char_to_orb_color(player_color) || !board.is_critical(row, col))
    {
        return 0;
    }

    visited[row][col] = true;
    int count = 1;

    count += count_contiguous_critical_cells(row + 1, col, board, player_color, visited);
    count += count_contiguous_critical_cells(row - 1, col, board, player_color, visited);
    count += count_contiguous_critical_cells(row, col + 1, board, player_color, visited);
    count += count_contiguous_critical_cells(row, col - 1, board, player_color, visited);

    return count;
}

double critical_heuristic(const Board &board, char player_color)
{
    double value = 0.0;
    char opponent_color = (player_color == 'R') ? 'B' : 'R';
    auto player_orb_color = board.char_to_orb_color(player_color);
    auto opponent_orb_color = board.char_to_orb_color(opponent_color);

    // Neighbor offsets (Up, Down, Left, Right)
    const int row_offset[] = {-1, 1, 0, 0};
    const int col_offset[] = {0, 0, -1, 1};

    // Track critical cells for block counting
    std::vector<std::vector<bool>> visited(board.rows, std::vector<bool>(board.cols, false));

    for (int row = 0; row < board.rows; ++row)
    {
        for (int col = 0; col < board.cols; ++col)
        {
            if (board.grid[row][col].color != player_orb_color)
                continue;

            // Orb count
            value += W_ORB * board.grid[row][col].count;

            // Check vulnerability
            bool is_vulnerable = false;
            for (int i = 0; i < 4; ++i)
            {
                int nr = row + row_offset[i], nc = col + col_offset[i];
                if (nr >= 0 && nr < board.rows && nc >= 0 && nc < board.cols &&
                    board.grid[nr][nc].color == opponent_orb_color && board.is_critical(nr, nc))
                {
                    value -= W_VULNERABLE * (5 - board.get_critical_mass(nr, nc));
                    is_vulnerable = true;
                }
            }

            if (!is_vulnerable)
            {
                int critical_mass = board.get_critical_mass(row, col);
                if (critical_mass == 2)
                    value += W_CORNER;
                else if (critical_mass == 3)
                    value += W_EDGE;

                if (board.is_critical(row, col))
                {
                    value += W_CRITICAL;
                }
            }

            visited[row][col] = board.is_critical(row, col);
        }
    }
    // Count contiguous critical blocks
    for (int row = 0; row < board.rows; ++row)
    {
        for (int col = 0; col < board.cols; ++col)
        {
            if (visited[row][col] && board.grid[row][col].color == player_orb_color)
            {
                int block_size = count_contiguous_critical_cells(row, col, board, player_color, visited);
                if (block_size > 1)
                {
                    value += W_BLOCK * block_size * block_size; // Quadratic scaling
                }
            }
        }
    }
    return value;
}

double near_critical_heuristic(const Board &board, char player_color)
{
    int player_near_critical_count = 0;
    int opponent_near_critical_count = 0;

    char opponent_color_char = (player_color == 'R') ? 'B' : 'R';

    // Iterate over every cell on the board.
    for (int row = 0; row < board.rows; row++)
    {
        for (int col = 0; col < board.cols; col++)
        {
            // Check only non-empty cells.
            if (board.grid[row][col].count > 0)
            {
                int critical_mass = board.get_critical_mass(row, col);
                bool is_near_critical = (board.grid[row][col].count == critical_mass - 1);
                if (is_near_critical)
                {
                    if (board.grid[row][col].color == player_color)
                    {
                        player_near_critical_count++;
                    }
                    else if (board.grid[row][col].color == opponent_color_char)
                    {
                        opponent_near_critical_count++;
                    }
                }
            }
        }
    }
    int score = player_near_critical_count - opponent_near_critical_count;
    return score;
}

double board_control_heuristic(const Board &board, char player_color)
{
    double total_score = 0.0;
    char opponent_color = (player_color == 'R') ? 'B' : 'R';
    auto player_orb_color = board.char_to_orb_color(player_color);
    auto opponent_orb_color = board.char_to_orb_color(opponent_color);

    // Vectors to store row and column orb counts and critical cell counts
    std::vector<int> player_row_orbs(board.rows, 0), opponent_row_orbs(board.rows, 0);
    std::vector<int> player_col_orbs(board.cols, 0), opponent_col_orbs(board.cols, 0);
    std::vector<int> row_critical(board.rows, 0), col_critical(board.cols, 0);

    // Single pass over the board
    for (int row = 0; row < board.rows; row++)
    {
        for (int col = 0; col < board.cols; col++)
        {
            if (board.grid[row][col].color == player_orb_color)
            {
                player_row_orbs[row] += board.grid[row][col].count;
                player_col_orbs[col] += board.grid[row][col].count;
                if (board.is_critical(row, col))
                {
                    row_critical[row]++;
                    col_critical[col]++;
                }
            }
            else if (board.grid[row][col].color == opponent_orb_color)
            {
                opponent_row_orbs[row] += board.grid[row][col].count;
                opponent_col_orbs[col] += board.grid[row][col].count;
            }
        }
    }

    // Calculate row contributions
    for (int row = 0; row < board.rows; ++row)
    {
        int row_diff = player_row_orbs[row] - opponent_row_orbs[row];
        total_score += W_ROW * row_diff;
        // Bonus for critical cells in the row
        total_score += W_CRITICAL * row_critical[row];
    }

    // Calculate column contributions
    for (int col = 0; col < board.cols; ++col)
    {
        int col_diff = player_col_orbs[col] - opponent_col_orbs[col];
        total_score += W_COL * col_diff;
        // Bonus for critical cells in the column
        total_score += W_CRITICAL * col_critical[col];
    }

    return total_score;
}

double amalgam_heuristic(const Board &board, char player_color)
{
    double score = 0.0;

    // Apply the corner heuristic
    score += corner_heuristic(board, player_color) * 1.0;
    // Apply the critical heuristic
    score += critical_heuristic(board, player_color) * 2.5;
    // Apply the near-critical heuristic
    score += near_critical_heuristic(board, player_color) * 1.5;

    score += board_control_heuristic(board, player_color) * 0.8;

    return score;
}

double random_heuristic(const Board &board, char player_color)
{
    //seed the random number generator
    srand(static_cast<unsigned int>(time(nullptr)));
    return static_cast<double>(rand() % 201 - 100);
}