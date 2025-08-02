#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <string>
#include <utility>
#include <queue>
#include <stdexcept>
#include <set>
enum OrbColor
{
    EMPTY,
    RED,
    BLUE
};

struct Cell
{
    int count;
    OrbColor color;
};

class Board
{
public:
    int rows;
    int cols;
    std::vector<std::vector<Cell>> grid;
    char current_player_color;
    std::pair<int, int> last_move_coords;
    std::set<char> active_players;
    int total_moves_made;
    Board(int rows, int cols) : rows(rows), cols(cols), last_move_coords({-1, -1}), total_moves_made(0)
    {
        grid.resize(rows, std::vector<Cell>(cols, {0, EMPTY}));
        current_player_color = 'R';
    }

    Board(const Board &other)
    {
        rows = other.rows;
        cols = other.cols;
        grid = other.grid;
        current_player_color = other.current_player_color;
        last_move_coords = other.last_move_coords;
        active_players = other.active_players;
        total_moves_made = other.total_moves_made;
    }

    int get_critical_mass(int row, int col) const
    {
        if ((row == 0 || row == rows - 1) && (col == 0 || col == cols - 1))
            return 2;
        if (row == 0 || row == rows - 1 || col == 0 || col == cols - 1)
            return 3;
        return 4;
    }

    bool is_valid_move(int row, int col, char player_char) const
    {
        if (row < 0 || row >= rows || col < 0 || col >= cols)
            return false;
        OrbColor p_color = char_to_orb_color(player_char);
        return grid[row][col].color == EMPTY || grid[row][col].color == p_color;
    }

    Board apply_move(int row, int col, char player_char) const

    {
        Board next_state = *this;
        next_state.total_moves_made++;
        OrbColor p_color = char_to_orb_color(player_char);

        Cell &target_cell = next_state.grid[row][col];
        target_cell.color = p_color;
        target_cell.count++;
        std::queue<std::pair<int, int>> explosion_queue;
        std::vector<std::vector<bool>> in_queue(next_state.rows, std::vector<bool>(next_state.cols, false));

        if (target_cell.count >= next_state.get_critical_mass(row, col))
        {
            explosion_queue.push({row, col});
            in_queue[row][col] = true;
        }
        int row_offsets[] = {-1, 1, 0, 0};
        int col_offsets[] = {0, 0, -1, 1};
        while (!explosion_queue.empty())
        {
            std::pair<int, int> current_pos = explosion_queue.front();
            explosion_queue.pop();
            in_queue[current_pos.first][current_pos.second] = false;
            next_state.last_move_coords = current_pos;
            next_state.active_players.insert(player_char);
            int row = current_pos.first;
            int col = current_pos.second;
            OrbColor explosion_color = next_state.grid[row][col].color;
            int critical_mass = next_state.get_critical_mass(row, col);
            next_state.grid[row][col].count -= critical_mass;
            if (next_state.grid[row][col].count == 0)
                next_state.grid[row][col].color = EMPTY;

            for (int i = 0; i < 4; ++i)
            {
                int neighbour_row = row + row_offsets[i];
                int neighbour_col = col + col_offsets[i];
                if (neighbour_row >= 0 && neighbour_row < next_state.rows && neighbour_col >= 0 && neighbour_col < next_state.cols)
                {
                    Cell &neighbor_cell = next_state.grid[neighbour_row][neighbour_col];
                    neighbor_cell.color = explosion_color;
                    neighbor_cell.count++;
                    if (neighbor_cell.count >= next_state.get_critical_mass(neighbour_row, neighbour_col) && !in_queue[neighbour_row][neighbour_col])
                    {
                        explosion_queue.push({neighbour_row, neighbour_col});
                        in_queue[neighbour_row][neighbour_col] = true;
                    }
                }
            }
        }
        char opponent_char = (player_char == 'R') ? 'B' : 'R';
        bool opponent_found = false;
        for (int row = 0; row < next_state.rows; ++row)
        {
            for (int col = 0; col < next_state.cols; ++col)
            {
                if (next_state.grid[row][col].color == char_to_orb_color(opponent_char))
                {
                    opponent_found = true;
                    break;
                }
            }
            if (next_state.grid[row][col].color == char_to_orb_color(opponent_char))
            {
                opponent_found = true;
                break;
            }

            if (opponent_found)
            {
                break;
            }
        }
        next_state.active_players.clear();
        next_state.active_players.insert(player_char);
        if (opponent_found)
        {
            next_state.active_players.insert(opponent_char);
        }
        next_state.current_player_color = (player_char == 'R') ? 'B' : 'R';
        return next_state;
    }

    bool is_game_over() const
    {
        int red_orb_count = 0;
        int blue_orb_count = 0;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                if (grid[row][col].color == RED)
                    red_orb_count++;
                else if (grid[row][col].color == BLUE)
                    blue_orb_count++;
            }
        }
        if (red_orb_count == 0 && blue_orb_count == 0)
            return false;

        bool is_red_eliminated = (red_orb_count == 0 && blue_orb_count > 0);
        bool is_blue_eliminated = (blue_orb_count == 0 && red_orb_count > 0);

        if (active_players.size() < 2 && total_moves_made < 2)
        {
            return false;
        }

        return is_red_eliminated || is_blue_eliminated;
    }

    char get_winner() const
    {
        int red_orb_count = 0, blue_orb_count = 0;
        for (int row = 0; row < rows; ++row)
            for (int col = 0; col < cols; ++col)
            {
                if (grid[row][col].color == RED)
                    red_orb_count++;
                else if (grid[row][col].color == BLUE)
                    blue_orb_count++;
            }
        if (red_orb_count == 0 && blue_orb_count > 0)
            return 'B';
        if (blue_orb_count == 0 && red_orb_count > 0)
            return 'R';
        return 'N';
    }

    void set_current_player_color(char color) { current_player_color = color; }
    char get_current_player_color() const { return current_player_color; }
    std::pair<int, int> get_last_move() const { return last_move_coords; }
    void set_last_move(std::pair<int, int> move) { last_move_coords = move; }

    std::vector<Board> get_children(char player_to_move) const
    {
        std::vector<Board> children;
        for (const auto &move : get_legal_moves(player_to_move))
        {
            Board next_state = apply_move(move.first, move.second, player_to_move);
            next_state.set_last_move(move);
            children.push_back(next_state);
        }
        return children;
    }

    std::vector<std::pair<int, int>> get_legal_moves(char player_char) const
    {
        std::vector<std::pair<int, int>> legal_moves;
        for (int row = 0; row < rows; ++row)
            for (int col = 0; col < cols; ++col)
            {
                if (is_valid_move(row, col, player_char))
                    legal_moves.push_back({row, col});
            }
        return legal_moves;
    }

    std::string to_string() const
    {
        std::string s;
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                const Cell &cell = grid[row][col];
                if (cell.color == EMPTY)
                    s += "0";
                else
                {
                    s += std::to_string(cell.count);
                    s += (cell.color == RED) ? 'R' : 'B';
                }
                if (col < cols - 1)
                    s += " ";
            }
            s += "\n";
        }
        return s;
    }
    bool is_critical(int row, int col) const
    {
        if (row < 0 || row >= rows || col < 0 || col >= cols)
            return false;
        return grid[row][col].count >= get_critical_mass(row, col);
    }

    static Board from_string_vector(const std::vector<std::vector<std::string>> &str_grid, char player_turn)
    {
        if (str_grid.empty() || str_grid[0].empty())
            throw std::invalid_argument("Empty grid.");
        Board new_board(str_grid.size(), str_grid[0].size());
        new_board.set_current_player_color(player_turn);

        for (int row = 0; row < str_grid.size(); ++row)
        {
            for (int col = 0; col < str_grid[row].size(); ++col)
            {
                const std::string &cell_str = str_grid[row][col];
                if (cell_str == "0")
                {
                    new_board.grid[row][col] = {0, EMPTY};
                }
                else
                {
                    int count = std::stoi(cell_str.substr(0, cell_str.length() - 1));
                    char color_char = cell_str.back();
                    OrbColor cell_color = new_board.char_to_orb_color(color_char);
                    new_board.grid[row][col] = {count, cell_color};

                    if (cell_color == RED)
                    {
                        new_board.active_players.insert('R');
                        new_board.total_moves_made++;
                    }
                    else if (cell_color == BLUE)
                    {
                        new_board.active_players.insert('B');
                        new_board.total_moves_made++;
                    }
                }
            }
        }
        return new_board;
    }
    bool is_empty() const
    {
        for (int row = 0; row < rows; ++row)
            for (int col = 0; col < cols; ++col)
            {
                if (grid[row][col].count > 0)
                    return false;
            }
        return true;
    }

    OrbColor char_to_orb_color(char player_char) const
    {
        if (player_char == 'R')
            return RED;
        if (player_char == 'B')
            return BLUE;
        return EMPTY;
    }
};

#endif // BOARD_H