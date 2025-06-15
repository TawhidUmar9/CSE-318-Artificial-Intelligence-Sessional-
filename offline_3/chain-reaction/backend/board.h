#include <vector>
#include <string>
#include <utility>
#include <queue>
#include <stdexcept>

// --- Enums and Structs ---

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

// --- Board Class Definition and Implementation ---

class Board
{
public:
    int rows;
    int cols;
    std::vector<std::vector<Cell>> grid;
    char current_player_color;
    std::pair<int, int> last_move_made;

    Board(int m, int n)
        : rows(m), cols(n), last_move_made({-1, -1})
    {
        grid.resize(rows, std::vector<Cell>(cols, {0, EMPTY}));
        current_player_color = 'R';
        last_move_made = {-1, -1};
    }

    Board(const Board &other)
        : rows(other.rows),
          cols(other.cols),
          grid(other.grid),
          current_player_color(other.current_player_color),
          last_move_made(other.last_move_made) {}

    int get_critical_mass(int row, int col) const
    {
        if ((row == 0 && col == 0) ||
            (row == 0 && col == cols - 1) ||
            (row == rows - 1 && col == 0) ||
            (row == rows - 1 && col == cols - 1))
        {
            return 2;
        }
        if (row == 0 || row == rows - 1 ||
            col == 0 || col == cols - 1)
        {
            return 3;
        }
        return 4;
    }

    bool is_valid_move(int row, int col, char current_player_color) const
    {
        if (row < 0 || row >= rows || col < 0 || col >= cols)
        {
            return false;
        }

        OrbColor player_orb_color = char_to_orbcolor(current_player_color);
        if (player_orb_color == EMPTY && current_player_color != '0')
        {
            return false;
        }

        if (grid[row][col].color == EMPTY || grid[row][col].color == player_orb_color)
        {
            return true;
        }
        return false;
    }

    Board apply_move(int row, int col, char current_player_color) const
    {
        Board next_state = *this;

        OrbColor player_orb_color = char_to_orbcolor(current_player_color);

        if (next_state.grid[row][col].color == EMPTY)
        {
            next_state.grid[row][col].color = player_orb_color;
        }
        next_state.grid[row][col].count++;

        std::queue<std::pair<int, int>> cells_to_process_queue;
        cells_to_process_queue.push({row, col});

        int offset_row[] = {-1, 1, 0, 0};
        int offset_col[] = {0, 0, -1, 1};

        while (!cells_to_process_queue.empty())
        {
            std::pair<int, int> current_cell_coordinates = cells_to_process_queue.front();
            cells_to_process_queue.pop();

            int current_row = current_cell_coordinates.first;
            int current_col = current_cell_coordinates.second;

            int critical_mass = next_state.get_critical_mass(current_row, current_col);

            while (next_state.grid[current_row][current_col].count >= critical_mass)
            {
                next_state.grid[current_row][current_col].count -= critical_mass;

                if (next_state.grid[current_row][current_col].count == 0)
                    next_state.grid[current_row][current_col].color = EMPTY;

                for (int i = 0; i < 4; i++)
                {
                    int neighbor_row = current_row + offset_row[i];
                    int neighbor_col = current_col + offset_col[i];

                    if (neighbor_row >= 0 && neighbor_row < next_state.rows &&
                        neighbor_col >= 0 && neighbor_col < next_state.cols)
                    {

                        if (next_state.grid[neighbor_row][neighbor_col].color != EMPTY &&
                            next_state.grid[neighbor_row][neighbor_col].color != player_orb_color)
                        {
                            next_state.grid[neighbor_row][neighbor_col].color = player_orb_color;
                        }
                        else if (next_state.grid[neighbor_row][neighbor_col].color == EMPTY)
                        {
                            next_state.grid[neighbor_row][neighbor_col].color = player_orb_color;
                        }

                        next_state.grid[neighbor_row][neighbor_col].count++;

                        if (next_state.grid[neighbor_row][neighbor_col].count >=
                            next_state.get_critical_mass(neighbor_row, neighbor_col))
                        {
                            cells_to_process_queue.push({neighbor_row, neighbor_col});
                        }
                    }
                }
            }
        }

        next_state.current_player_color = (current_player_color == 'R') ? 'B' : 'R';
        return next_state;
    }

    bool is_game_over() const {
    int red_orb_count = 0;
    int blue_orb_count = 0;

    // Count orbs for both players
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (grid[r][c].color == RED) {
                red_orb_count += grid[r][c].count;
            } else if (grid[r][c].color == BLUE) {
                blue_orb_count += grid[r][c].count;
            }
        }
    }

    if (red_orb_count == 0 && blue_orb_count == 0) {
        return false;
    }

    return (red_orb_count == 0 && blue_orb_count > 0) || (blue_orb_count == 0 && red_orb_count > 0);
}
    char get_winner() const
    {
        int red_orb_count = 0;
        int blue_orb_count = 0;
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                if (grid[row][col].color == RED)
                {
                    red_orb_count += grid[row][col].count;
                }
                else if (grid[row][col].color == BLUE)
                {
                    blue_orb_count += grid[row][col].count;
                }
            }
        }
        if (red_orb_count == 0 && blue_orb_count > 0)
            return 'B';
        else if (blue_orb_count == 0 && red_orb_count > 0)
            return 'R';
        return 'N';
    }

    void set_current_player_color(char color)
    {
        current_player_color = color;
    }

    char get_current_player_color() const
    {
        return current_player_color;
    }

    std::pair<int, int> get_last_move() const
    {
        return last_move_made;
    }

    void set_last_move(std::pair<int, int> move)
    {
        last_move_made = move;
    }

    std::vector<Board> get_children(char player_to_move) const
    {
        std::vector<Board> children_states;
        std::vector<std::pair<int, int>> legal_moves = get_legal_moves(player_to_move);

        for (const auto &move : legal_moves)
        {
            Board next_board_state = apply_move(move.first, move.second, player_to_move);
            next_board_state.set_last_move(move);
            children_states.push_back(next_board_state);
        }
        return children_states;
    }

    std::vector<std::pair<int, int>> get_legal_moves(char player_color_char) const
    {
        std::vector<std::pair<int, int>> legal_moves;
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                if (is_valid_move(row, col, player_color_char))
                {
                    legal_moves.push_back({row, col});
                }
            }
        }
        return legal_moves;
    }

    std::string to_string() const
    {
        std::string result;
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                const Cell &cell = grid[r][c];
                if (cell.color == EMPTY)
                {
                    result += "0";
                }
                else
                {
                    result += std::to_string(cell.count);
                    result += (cell.color == RED) ? 'R' : 'B';
                }
                if (c < cols - 1)
                {
                    result += " ";
                }
            }
            result += "\n";
        }
        return result;
    }

    static Board from_string_vector(const std::vector<std::vector<std::string>> &str_grid, char player_turn)
    {
        if (str_grid.empty() || str_grid[0].empty())
        {
            throw std::invalid_argument("Empty string grid provided for Board::from_string_vector.");
        }

        Board new_board(str_grid.size(), str_grid[0].size());
        new_board.set_current_player_color(player_turn);

        for (int i = 0; i < str_grid.size(); i++)
        {
            for (int j = 0; j < str_grid[i].size(); j++)
            {
                const std::string &cell_str = str_grid[i][j];
                if (cell_str == "0")
                {
                    new_board.grid[i][j] = {0, EMPTY};
                }
                else
                {
                    int count = 0;
                    try
                    {
                        count = std::stoi(cell_str.substr(0, cell_str.length() - 1));
                    }
                    catch (const std::invalid_argument &e)
                    {
                        throw std::invalid_argument("Invalid orb count format in cell string: " + cell_str + ". " + e.what());
                    }
                    catch (const std::out_of_range &e)
                    {
                        throw std::out_of_range("Orb count out of range in cell string: " + cell_str + ". " + e.what());
                    }

                    char color_char = cell_str.back();

                    new_board.grid[i][j].count = count;
                    new_board.grid[i][j].color = new_board.char_to_orbcolor(color_char);
                    if (new_board.grid[i][j].color == EMPTY && color_char != '0')
                    {
                        throw std::invalid_argument("Invalid color character in cell string: " + cell_str);
                    }
                }
            }
        }
        return new_board;
    }

private:
    OrbColor char_to_orbcolor(char player_color_char) const
    {
        if (player_color_char == 'R')
        {
            return RED;
        }
        else if (player_color_char == 'B')
        {
            return BLUE;
        }
        return EMPTY;
    }
};