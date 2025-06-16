#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <fstream>
#include <future> // Required for std::async and std::future

#include "ai_minimax.cpp"

#define BOARD_ROWS 9
#define BOARD_COLS 6
#define MAX_TURNS 2000
#define MAX_HEURISTIC_TIME 5000 // Timeout for the AI in milliseconds
#define INF_NEG_VAL -10000
#define INF_POS_VAL 10000

// Forward declare heuristic functions
double simple_heuristic(const Board &, char);
double corner_heuristic(const Board &, char);
double critical_heuristic(const Board &, char);
double amalgam_heuristic(const Board &, char);
double board_control_heuristic(const Board &, char);
double near_critical_heuristic(const Board &, char);

// A function pointer for our heuristic
using HeuristicFunction = double (*)(const Board &, char);

HeuristicFunction select_heuristic(const std::string &name)
{
    if (name == "s")
        return simple_heuristic;
    if (name == "c")
        return corner_heuristic;
    if (name == "cr")
        return critical_heuristic;
    if (name == "a")
        return amalgam_heuristic;
    if (name == "b")
        return board_control_heuristic;
    if (name == "n")
        return near_critical_heuristic;
    if (name == "r")
        return random_heuristic;
    return simple_heuristic;
}

int main(int argc, char *argv[])
{
    // Use append mode for the log file
    std::ofstream log_file("game_log.txt", std::ios::app);
    if (!log_file.is_open())
    {
        std::cerr << "Error: Could not open log file for writing.\n";
        return 1;
    }
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " <heuristic_P1> <depth_P1> <heuristic_P2> <depth_P2>\n";
        std::cerr << "Example: " << argv[0] << " a 4 b 4\n";
        return 1;
    }

    srand(static_cast<unsigned int>(time(nullptr)));

    std::string p1_heuristic_name = argv[1];
    int p1_depth = std::stoi(argv[2]);
    HeuristicFunction p1_heuristic = select_heuristic(p1_heuristic_name);

    std::string p2_heuristic_name = argv[3];
    int p2_depth = std::stoi(argv[4]);
    HeuristicFunction p2_heuristic = select_heuristic(p2_heuristic_name);

    Board game_board(BOARD_ROWS, BOARD_COLS);
    char current_player_char = 'R';
    int turn_count = 0;

    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << "--- Game Start! ---\n";
    std::cout << game_board.to_string() << "\n";

    while (turn_count < MAX_TURNS)
    {
        if (game_board.is_game_over())
        {
            break;
        }

        HeuristicFunction current_heuristic = (current_player_char == 'R') ? p1_heuristic : p2_heuristic;
        HeuristicFunction opponent_heuristic = (current_player_char == 'R') ? p2_heuristic : p1_heuristic;
        int current_depth = (current_player_char == 'R') ? p1_depth : p2_depth;
        std::string current_heuristic_name = (current_player_char == 'R') ? p1_heuristic_name : p2_heuristic_name;

        std::cout << "\n--- Turn " << turn_count + 1 << ": Player " << current_player_char << " (" << current_heuristic_name << ") is thinking... ---\n";

        std::pair<int, int> chosen_move = {-1, -1};

        std::future<std::pair<std::pair<int, int>, double>> minimax_future = std::async(std::launch::async, [&]()
                                                                                        { return minimax(game_board, current_depth, current_player_char, INF_NEG_VAL, INF_POS_VAL, current_heuristic, opponent_heuristic); });

        std::future_status status = minimax_future.wait_for(std::chrono::milliseconds(MAX_HEURISTIC_TIME));

        if (status == std::future_status::ready)
        {
            auto ai_move_result = minimax_future.get();
            chosen_move = ai_move_result.first;
        }
        else
        {
            auto legal_moves = game_board.get_legal_moves(current_player_char);
            if (!legal_moves.empty())
            {
                chosen_move = legal_moves[0];
            }
            else
            {
                std::cerr << "Error: No legal moves available on timeout. Ending game.\n";
                break;
            }
        }

        if (chosen_move.first == -1)
        {
            auto legal_moves = game_board.get_legal_moves(current_player_char);
            if (!legal_moves.empty())
            {
                chosen_move = legal_moves[0];
                std::cerr << "Warning: Minimax returned no move. Falling back to first legal move.\n";
            }
            else
            {
                std::cerr << "Error: No legal moves for Player " << current_player_char << ". Ending game.\n";
                break;
            }
        }

        std::cout << "Player " << current_player_char << " places an orb at (" << chosen_move.first << ", " << chosen_move.second << ")\n";
        game_board = game_board.apply_move(chosen_move.first, chosen_move.second, current_player_char);
        std::cout << game_board.to_string() << "\n";

        current_player_char = (current_player_char == 'R') ? 'B' : 'R';
        turn_count++;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::cout << "\n--- GAME OVER ---\n";

    std::string winner_str = "Draw";
    if (game_board.is_game_over())
    {
        winner_str = std::string(1, game_board.get_winner());
        std::cout << "Winner: Player " << winner_str << "!\n";
    }
    else if (turn_count >= MAX_TURNS)
    {
        std::cout << "Result: Draw (Maximum turns reached).\n";
    }
    else
    {
        winner_str = "Unexpected";
        std::cout << "Result: Game ended unexpectedly.\n";
    }

    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    log_file << "Game Result: " << winner_str << "\n";
    log_file << "Total Turns: " << turn_count << "\n";
    log_file << "Elapsed Time: " << elapsed_seconds.count() << " seconds\n";
    log_file.close();

    return 0;
}