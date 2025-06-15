#include <vector>
#include <string>
#include <utility>
#include <queue>
#include <stdexcept>

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
    int num_rows;
    int num_cols;
    std::vector<std::vector<Cell>> grid;
    char current_player_color;
    std::pair<int, int> last_move_coords;

    Board(int r, int c) : num_rows(r), num_cols(c), last_move_coords({-1, -1})
    {
        grid.resize(num_rows, std::vector<Cell>(num_cols, {0, EMPTY}));
        current_player_color = 'R';
    }
    Board(const Board &other) = default;

    int get_critical_mass(int r, int c) const
    {
        if ((r == 0 || r == num_rows - 1) && (c == 0 || c == num_cols - 1))
            return 2;
        if (r == 0 || r == num_rows - 1 || c == 0 || c == num_cols - 1)
            return 3;
        return 4;
    }
    bool is_valid_move(int r, int c, char player_char) const
    {
        if (r < 0 || r >= num_rows || c < 0 || c >= num_cols)
            return false;
        OrbColor p_color = char_to_orb_color(player_char);
        return grid[r][c].color == EMPTY || grid[r][c].color == p_color;
    }
    Board apply_move(int r, int c, char player_char) const
    {
        Board next_state = *this;
        OrbColor p_color = char_to_orb_color(player_char);
        if (next_state.grid[r][c].color == EMPTY)
            next_state.grid[r][c].color = p_color;
        next_state.grid[r][c].count++;

        std::queue<std::pair<int, int>> explosion_queue;
        explosion_queue.push({r, c});
        int row_offsets[] = {-1, 1, 0, 0}, col_offsets[] = {0, 0, -1, 1};

        while (!explosion_queue.empty())
        {
            std::pair<int, int> current_cell = explosion_queue.front();
            explosion_queue.pop();
            int current_r = current_cell.first, current_c = current_cell.second;
            int critical_mass = next_state.get_critical_mass(current_r, current_c);

            while (next_state.grid[current_r][current_c].count >= critical_mass)
            {
                next_state.grid[current_r][current_c].count -= critical_mass;
                if (next_state.grid[current_r][current_c].count == 0)
                    next_state.grid[current_r][current_c].color = EMPTY;

                for (int i = 0; i < 4; ++i)
                {
                    int neighbor_r = current_r + row_offsets[i], neighbor_c = current_c + col_offsets[i];
                    if (neighbor_r >= 0 && neighbor_r < next_state.num_rows && neighbor_c >= 0 && neighbor_c < next_state.num_cols)
                    {
                        if (next_state.grid[neighbor_r][neighbor_c].color != EMPTY && next_state.grid[neighbor_r][neighbor_c].color != p_color)
                        {
                            next_state.grid[neighbor_r][neighbor_c].color = p_color;
                        }
                        else if (next_state.grid[neighbor_r][neighbor_c].color == EMPTY)
                        {
                            next_state.grid[neighbor_r][neighbor_c].color = p_color;
                        }
                        next_state.grid[neighbor_r][neighbor_c].count++;
                        if (next_state.grid[neighbor_r][neighbor_c].count >= next_state.get_critical_mass(neighbor_r, neighbor_c))
                        {
                            explosion_queue.push({neighbor_r, neighbor_c});
                        }
                    }
                }
            }
        }
        next_state.current_player_color = (player_char == 'R') ? 'B' : 'R';
        return next_state;
    }

    bool is_game_over() const
    {
        int red_orb_count = 0, blue_orb_count = 0, empty_count = 0;
        for (int r = 0; r < num_rows; ++r)
            for (int c = 0; c < num_cols; ++c)
            {
                if (grid[r][c].color == RED)
                    red_orb_count += grid[r][c].count;
                else if (grid[r][c].color == BLUE)
                    blue_orb_count += grid[r][c].count;
            }
        if (red_orb_count == 0 && blue_orb_count == 0)
            return false;
        return (red_orb_count == 0 && blue_orb_count > 0) || (blue_orb_count == 0 && red_orb_count > 0);
    }
    char get_winner() const
    {
        int red_orb_count = 0, blue_orb_count = 0;
        for (int r = 0; r < num_rows; ++r)
            for (int c = 0; c < num_cols; ++c)
            {
                if (grid[r][c].color == RED)
                    red_orb_count += grid[r][c].count;
                else if (grid[r][c].color == BLUE)
                    blue_orb_count += grid[r][c].count;
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
        for (int r = 0; r < num_rows; ++r)
            for (int c = 0; c < num_cols; ++c)
            {
                if (is_valid_move(r, c, player_char))
                    legal_moves.push_back({r, c});
            }
        return legal_moves;
    }

    std::string to_string() const
    {
        std::string s;
        for (int r = 0; r < num_rows; ++r)
        {
            for (int c = 0; c < num_cols; ++c)
            {
                const Cell &cell = grid[r][c];
                if (cell.color == EMPTY)
                    s += "0";
                else
                {
                    s += std::to_string(cell.count);
                    s += (cell.color == RED) ? 'R' : 'B';
                }
                if (c < num_cols - 1)
                    s += " ";
            }
            s += "\n";
        }
        return s;
    }

    static Board from_string_vector(const std::vector<std::vector<std::string>> &str_grid, char player_turn)
    {
        if (str_grid.empty() || str_grid[0].empty())
            throw std::invalid_argument("Empty grid.");
        Board new_board(str_grid.size(), str_grid[0].size());
        new_board.set_current_player_color(player_turn);
        for (int r = 0; r < str_grid.size(); ++r)
            for (int c = 0; c < str_grid[r].size(); ++c)
            {
                const std::string &cell_str = str_grid[r][c];
                if (cell_str == "0")
                    new_board.grid[r][c] = {0, EMPTY};
                else
                {
                    int count = std::stoi(cell_str.substr(0, cell_str.length() - 1));
                    char color_char = cell_str.back();
                    new_board.grid[r][c] = {count, new_board.char_to_orb_color(color_char)};
                    if (new_board.grid[r][c].color == EMPTY && color_char != '0')
                        throw std::invalid_argument("Invalid color.");
                }
            }
        return new_board;
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