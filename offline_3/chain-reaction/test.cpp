#include <iostream>
#include <vector>
#include <string>
#include <utility> // For std::pair
#include <queue>   // For std::queue
#include <algorithm> // For std::max, std::min
#include <limits> // For std::numeric_limits

// --- Enums and Structs ---
enum OrbColor { EMPTY, RED, BLUE };
struct Cell { int count; OrbColor color; };

// --- Board Class Definition and Implementation ---
// (Copied directly from your consolidated Board class)
class Board {
public:
    int num_rows;
    int num_cols;
    std::vector<std::vector<Cell>> grid;
    char current_player_color;
    std::pair<int, int> last_move_coords;

    Board(int r, int c) : num_rows(r), num_cols(c), last_move_coords({-1,-1}) {
        grid.resize(num_rows, std::vector<Cell>(num_cols, {0, EMPTY}));
        current_player_color = 'R';
    }
    Board(const Board& other) = default; 

    int get_critical_mass(int r, int c) const {
        if((r==0||r==num_rows-1)&&(c==0||c==num_cols-1)) return 2;
        if(r==0||r==num_rows-1||c==0||c==num_cols-1) return 3;
        return 4;
    }
    bool is_valid_move(int r, int c, char player_char) const {
        if(r<0||r>=num_rows||c<0||c>=num_cols) return false;
        OrbColor p_color = char_to_orb_color(player_char);
        return grid[r][c].color==EMPTY || grid[r][c].color==p_color;
    }
    Board apply_move(int r, int c, char player_char) const {
        Board next_state = *this;
        OrbColor p_color = char_to_orb_color(player_char);
        if(next_state.grid[r][c].color==EMPTY) next_state.grid[r][c].color=p_color;
        next_state.grid[r][c].count++;

        std::queue<std::pair<int,int>> explosion_queue; explosion_queue.push({r,c});
        int row_offsets[]={-1,1,0,0}, col_offsets[]={0,0,-1,1};

        while(!explosion_queue.empty()){
            std::pair<int,int> current_cell=explosion_queue.front(); explosion_queue.pop();
            int current_r=current_cell.first, current_c=current_cell.second;
            int critical_mass=next_state.get_critical_mass(current_r,current_c);

            while(next_state.grid[current_r][current_c].count >= critical_mass){
                next_state.grid[current_r][current_c].count -= critical_mass;
                if(next_state.grid[current_r][current_c].count==0) next_state.grid[current_r][current_c].color=EMPTY;

                for(int i=0;i<4;++i){
                    int neighbor_r=current_r+row_offsets[i], neighbor_c=current_c+col_offsets[i];
                    if(neighbor_r>=0&&neighbor_r<next_state.num_rows&&neighbor_c>=0&&neighbor_c<next_state.num_cols){
                        if(next_state.grid[neighbor_r][neighbor_c].color!=EMPTY&&next_state.grid[neighbor_r][neighbor_c].color!=p_color){
                            next_state.grid[neighbor_r][neighbor_c].color=p_color;
                        }else if(next_state.grid[neighbor_r][neighbor_c].color==EMPTY){
                            next_state.grid[neighbor_r][neighbor_c].color=p_color;
                        }
                        next_state.grid[neighbor_r][neighbor_c].count++;
                        if(next_state.grid[neighbor_r][neighbor_c].count>=next_state.get_critical_mass(neighbor_r,neighbor_c)){
                            explosion_queue.push({neighbor_r,neighbor_c});
                        }
                    }
                }
            }
        }
        next_state.current_player_color=(player_char=='R')?'B':'R';
        return next_state;
    }

    bool is_game_over() const {
        int red_orb_count=0, blue_orb_count=0;
        for(int r=0;r<num_rows;++r) for(int c=0;c<num_cols;++c) {
            if(grid[r][c].color==RED) red_orb_count+=grid[r][c].count;
            else if(grid[r][c].color==BLUE) blue_orb_count+=grid[r][c].count;
        }
        if (red_orb_count == 0 && blue_orb_count == 0) return false;
        return (red_orb_count==0&&blue_orb_count>0) || (blue_orb_count==0&&red_orb_count>0);
    }
    char get_winner() const {
        int red_orb_count=0, blue_orb_count=0;
        for(int r=0;r<num_rows;++r) for(int c=0;c<num_cols;++c) {
            if(grid[r][c].color==RED) red_orb_count+=grid[r][c].count;
            else if(grid[r][c].color==BLUE) blue_orb_count+=grid[r][c].count;
        }
        if(red_orb_count==0&&blue_orb_count>0) return 'B';
        if(blue_orb_count==0&&red_orb_count>0) return 'R';
        return 'N';
    }

    void set_current_player_color(char color) { current_player_color=color; }
    char get_current_player_color() const { return current_player_color; }
    std::pair<int, int> get_last_move() const { return last_move_coords; }
    void set_last_move(std::pair<int, int> move) { last_move_coords=move; }

    std::vector<Board> get_children(char player_to_move) const {
        std::vector<Board> children;
        for(const auto& move : get_legal_moves(player_to_move)) {
            Board next_state=apply_move(move.first,move.second,player_to_move);
            next_state.set_last_move(move);
            children.push_back(next_state);
        }
        return children;
    }
    std::vector<std::pair<int, int>> get_legal_moves(char player_char) const {
        std::vector<std::pair<int, int>> legal_moves;
        for(int r=0;r<num_rows;++r) for(int c=0;c<num_cols;++c) {
            if(is_valid_move(r,c,player_char)) legal_moves.push_back({r,c});
        }
        return legal_moves;
    }

    std::string to_string() const {
        std::string s;
        for(int r=0;r<num_rows;++r){
            for(int c=0;c<num_cols;++c){
                const Cell& cell=grid[r][c];
                if(cell.color==EMPTY) s+="0";
                else { s+=std::to_string(cell.count); s+=(cell.color==RED)?'R':'B'; }
                if(c<num_cols-1) s+=" ";
            }
            s+="\n";
        }
        return s;
    }

    static Board from_string_vector(const std::vector<std::vector<std::string>>& str_grid, char player_turn) {
        // This static method is not directly used in this console test, but included for completeness.
        // It would typically involve parsing string like "1R 0 2B".
        // For console testing, we'll manually set up the board.
        throw std::runtime_error("fromStringVector not implemented for this test context.");
    }

    OrbColor char_to_orb_color(char player_char) const {
        if(player_char=='R') return RED;
        if(player_char=='B') return BLUE;
        return EMPTY;
    }
};

// --- Heuristic Function ---
// (Copied directly from your heuristic.cpp)
int evaluate_heuristic(const Board& state, char maximizing_player_color) {
    int player_orbs=0, opponent_orbs=0;
    OrbColor player_enum=state.char_to_orb_color(maximizing_player_color);
    // Directly determine opponent's enum color based on maximizing player's char
    OrbColor opponent_enum=(maximizing_player_color=='R')?BLUE:RED;

    for(int r=0;r<state.num_rows;++r) for(int c=0;c<state.num_cols;++c){
        const Cell& cell=state.grid[r][c];
        if(cell.color==player_enum) player_orbs+=cell.count;
        else if(cell.color==opponent_enum) opponent_orbs+=cell.count;
    }
    return player_orbs-opponent_orbs;
}

// --- Minimax Agent Function ---
// (Copied directly from your minimax.cpp)
const int INF_POS_VAL = std::numeric_limits<int>::max();
const int INF_NEG_VAL = std::numeric_limits<int>::min();

std::pair<std::pair<int, int>, int> minimax(Board state, int depth, char maximizing_player_color, int alpha, int beta, int (*heuristic_func)(const Board&, char)) {
    if (state.is_game_over()) {
        char winner = state.get_winner();
        if (winner == maximizing_player_color) return {{-1, -1}, INF_POS_VAL};
        if (winner != 'N') return {{-1, -1}, INF_NEG_VAL};
        return {{-1, -1}, 0}; // Draw or ambiguous end state
    }
    if (depth == 0) {
        return {{-1, -1}, heuristic_func(state, maximizing_player_color)};
    }

    char current_player_on_turn = state.get_current_player_color();
    if (current_player_on_turn == maximizing_player_color) {
        int max_eval = INF_NEG_VAL;
        std::pair<int, int> best_move_for_this_turn = {-1, -1};
        std::vector<Board> child_states = state.get_children(current_player_on_turn);
        
        // Handle no legal moves for current player (should ideally be covered by is_game_over for eliminating player)
        if (child_states.empty()) return {{ -1, -1}, INF_NEG_VAL }; // Current player has no moves, this path is bad

        for (const auto& child : child_states) {
            int eval = minimax(child, depth - 1, maximizing_player_color, alpha, beta, heuristic_func).second;
            if (eval > max_eval) {
                max_eval = eval;
                best_move_for_this_turn = child.get_last_move();
            }
            alpha = std::max(alpha, max_eval);
            if (beta <= alpha) break; // Beta cut-off
        }
        return {best_move_for_this_turn, max_eval};
    } else { // Minimizing Player's Turn
        int min_eval = INF_POS_VAL;
        std::vector<Board> child_states = state.get_children(current_player_on_turn);

        // Handle no legal moves for opponent (if this path is reached)
        if (child_states.empty()) return {{ -1, -1}, INF_POS_VAL }; // Opponent has no moves, this path is good for us

        for (const auto& child : child_states) {
            int eval = minimax(child, depth - 1, maximizing_player_color, alpha, beta, heuristic_func).second;
            if (eval < min_eval) min_eval = eval;
            beta = std::min(beta, min_eval);
            if (beta <= alpha) break; // Alpha cut-off
        }
        return {{-1, -1}, min_eval}; // No move returned for minimizing nodes
    }
}


// --- Main Function for Console Testing ---
int main() {
    const int TEST_BOARD_ROWS = 9;
    const int TEST_BOARD_COLS = 6;
    const int TEST_AI_DEPTH = 4; // Depth for console test

    // 1. Initialize a test board (empty initially)
    Board test_board(TEST_BOARD_ROWS, TEST_BOARD_COLS);

    // 2. Set up a custom scenario for testing (optional)
    // For example, place some orbs to create a mid-game state:
    
    // Test Scenario 1: AI (Blue) makes the first move on an empty board.
    // Board is already empty, current_player_color is 'R' by default.
    // Set it to 'B' if you want AI to evaluate assuming it's its turn on empty board
    test_board.set_current_player_color('B'); // AI (Blue) is to move

    // Test Scenario 2: Simple situation for AI to react to
    // test_board.grid[0][0] = {1, RED}; // Red has 1 orb at (0,0)
    // test_board.grid[0][1] = {1, BLUE}; // Blue has 1 orb at (0,1)
    // test_board.grid[1][1] = {1, RED}; // Red has 1 orb at (1,1)
    // test_board.set_current_player_color('B'); // AI (Blue) is to move

    // Test Scenario 3: Cell about to explode for AI
    // test_board.grid[0][0] = {1, BLUE}; // Critical mass 2
    // test_board.set_current_player_color('B'); // AI (Blue) is to move


    // 3. Print the initial test board state
    std::cout << "--- Initial Test Board ---\n";
    std::cout << test_board.to_string();
    std::cout << "Current player to move: " << test_board.get_current_player_color() << "\n";
    std::cout << "Is game over? " << (test_board.is_game_over() ? "Yes" : "No") << "\n";
    std::cout << "Legal moves for current player: " << test_board.get_legal_moves(test_board.get_current_player_color()).size() << "\n";

    // 4. Call Minimax for the AI (Blue)
    // Pass the test board, depth, AI player, infinity values, and heuristic function
    std::pair<std::pair<int, int>, int> ai_move_result =
        minimax(test_board, TEST_AI_DEPTH, 'B', INF_NEG_VAL, INF_POS_VAL, evaluate_heuristic);

    // 5. Print the AI's chosen move and its predicted value
    std::cout << "\n--- AI's Decision (Player " << 'B' << ") ---\n";
    std::pair<int, int> chosen_move = ai_move_result.first;
    int predicted_value = ai_move_result.second;

    if (chosen_move.first != -1) {
        std::cout << "Chosen Move: (" << chosen_move.first << ", " << chosen_move.second << ")\n";
        std::cout << "Predicted Value: " << predicted_value << "\n";

        // Optional: Apply the chosen move and print the resulting board
        Board board_after_ai_move = test_board.apply_move(chosen_move.first, chosen_move.second, test_board.get_current_player_color());
        std::cout << "\n--- Board After AI's Chosen Move ---\n";
        std::cout << board_after_ai_move.to_string();
        std::cout << "Next player to move: " << board_after_ai_move.get_current_player_color() << "\n";
    } else {
        std::cout << "AI found no move (or game is over / no legal moves).\n";
        std::cout << "Predicted Value: " << predicted_value << "\n";
    }

    return 0;
}