#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <limits>
#include <chrono>
#include "minimax.cpp"

const int BOARD_ROWS = 9;
const int BOARD_COLS = 6;
const int AI_SEARCH_DEPTH = 2;
const char AI_PLAYER_CHAR = 'B';
const char HUMAN_PLAYER_CHAR = 'R';
const std::string GAME_STATE_FILE = "input.txt";
const int MINIMAX_TIMEOUT_MS = 8000;

const int INF_POS = std::numeric_limits<int>::max();
const int INF_NEG = std::numeric_limits<int>::min();

std::string parse_header(const std::string &line)
{
  if (line.rfind("Human Move:", 0) == 0)
    return "Human";
  if (line.rfind("AI Move:", 0) == 0)
    return "AI";
  if (line.rfind("Initializing:", 0) == 0)
    return "Initializing";
  return "Unknown";
}

int main()
{
  std::ifstream input_file_stream(GAME_STATE_FILE);
  if (!input_file_stream.is_open())
    return 1;

  std::string header_line;
  if (!std::getline(input_file_stream, header_line))
  {
    input_file_stream.close();
    return 1;
  }
  std::string last_mover_type = parse_header(header_line);

  char current_player_char;
  int human_row = -1, human_col = -1;
  if (last_mover_type == "Human")
  {
    std::istringstream header_stream(header_line);
    std::string token;
    header_stream >> token >> token;
    if (!(header_stream >> human_row >> human_col))
    {
      input_file_stream.close();
      return 1;
    }
    current_player_char = AI_PLAYER_CHAR;
  }
  else if (last_mover_type == "AI")
    current_player_char = HUMAN_PLAYER_CHAR;
  else if (last_mover_type == "Initializing")
    current_player_char = AI_PLAYER_CHAR;
  else
  {
    input_file_stream.close();
    return 1;
  }

  std::vector<std::vector<std::string>> string_grid(BOARD_ROWS, std::vector<std::string>(BOARD_COLS));
  for (int row = 0; row < BOARD_ROWS; ++row)
  {
    std::string row_line;
    if (!std::getline(input_file_stream, row_line))
    {
      input_file_stream.close();
      return 1;
    }
    size_t start_pos = 0;
    size_t space_pos = row_line.find(' ');
    int c = 0;
    while (space_pos != std::string::npos && c < BOARD_COLS)
    {
      string_grid[row][c] = row_line.substr(start_pos, space_pos - start_pos);
      start_pos = space_pos + 1;
      space_pos = row_line.find(' ', start_pos);
      c++;
    }
    if (c < BOARD_COLS)
    {
      string_grid[row][c] = row_line.substr(start_pos);
    }
  }
  input_file_stream.close();

  Board current_game_board(BOARD_ROWS, BOARD_COLS);
  try
  {
    current_game_board = Board::from_string_vector(string_grid, current_player_char);
  }
  catch (const std::exception &e)
  {
    std::cerr << "C++ Agent Error: Board parsing failed: " << e.what() << ".\n";
    return 1;
  }

  if (last_mover_type == "Human" && human_row >= 0 && human_col >= 0)
  {
    if (!current_game_board.is_valid_move(human_row, human_col, HUMAN_PLAYER_CHAR))
    {
      std::cerr << "C++ Agent Error: Invalid human move at (" << human_row << "," << human_col << ").\n";
      return 1;
    }
    current_game_board = current_game_board.apply_move(human_row, human_col, HUMAN_PLAYER_CHAR);
    std::cerr << "C++ Agent: Applied human move\n";
  }

  if (current_game_board.is_game_over())
  {
    std::cerr << "C++ Agent: Game over detected, winner: " << current_game_board.get_winner() << "\n";
    std::ofstream output_file_stream(GAME_STATE_FILE);
    if (!output_file_stream.is_open())
    {
      std::cerr << "C++ Agent Error: Cannot open " << GAME_STATE_FILE << " for writing.\n";
      return 1;
    }
    output_file_stream << "Game Over! Winner: " << current_game_board.get_winner() << "\n"
                       << current_game_board.to_string();
    output_file_stream.close();
    std::cerr << "C++ Agent: Wrote game over to " << GAME_STATE_FILE << "\n";
    return 0;
  }

  if (current_game_board.get_current_player_color() == AI_PLAYER_CHAR)
  {
    bool is_board_empty = true;
    for (int row = 0; row < BOARD_ROWS; ++row)
    {
      for (int c = 0; c < BOARD_COLS; ++c)
      {
        if (current_game_board.grid[row][c].color != EMPTY)
        {
          is_board_empty = false;
          break;
        }
      }
      if (!is_board_empty)
        break;
    }
    std::cerr << "C++ Agent: Board empty: " << (is_board_empty ? "yes" : "no") << "\n";

    std::pair<int, int> chosen_move;
    if (is_board_empty)
    {
      chosen_move = {4, 3};
      std::cerr << "C++ Agent: Empty board detected, selecting default move (4,3).\n";
    }
    else
    {
      std::cerr << "C++ Agent: Starting minimax\n";
      try
      {
        auto start_time = std::chrono::steady_clock::now();
        std::pair<std::pair<int, int>, int> ai_move_result =
            minimax(current_game_board, AI_SEARCH_DEPTH, AI_PLAYER_CHAR, INF_NEG, INF_POS, evaluate_heuristic);
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start_time)
                              .count();
        std::cerr << "C++ Agent: Minimax took " << elapsed_ms << " ms\n";
        chosen_move = ai_move_result.first;
        if (chosen_move.first == -1 && chosen_move.second == -1)
        {
          std::cerr << "C++ Agent Error: No valid move selected by minimax.\n";
          auto legal_moves = current_game_board.get_legal_moves(AI_PLAYER_CHAR);
          if (!legal_moves.empty())
          {
            chosen_move = legal_moves[0];
            std::cerr << "C++ Agent: Fallback to first legal move (" << chosen_move.first << "," << chosen_move.second << ")\n";
          }
          else
          {
            std::cerr << "C++ Agent Error: No legal moves available.\n";
            return 1;
          }
        }
        else
        {
          std::cerr << "C++ Agent: Minimax selected move (" << chosen_move.first << "," << chosen_move.second << ")\n";
        }
        if (elapsed_ms > MINIMAX_TIMEOUT_MS)
        {
          std::cerr << "C++ Agent Warning: Minimax exceeded timeout, using selected move\n";
        }
      }
      catch (const std::exception &e)
      {
        std::cerr << "C++ Agent Error: Minimax failed: " << e.what() << "\n";
        auto legal_moves = current_game_board.get_legal_moves(AI_PLAYER_CHAR);
        if (!legal_moves.empty())
        {
          chosen_move = legal_moves[0];
          std::cerr << "C++ Agent: Fallback to first legal move (" << chosen_move.first << "," << chosen_move.second << ")\n";
        }
        else
        {
          std::cerr << "C++ Agent Error: No legal moves available.\n";
          return 1;
        }
      }
    }

    Board next_game_board = current_game_board.apply_move(chosen_move.first, chosen_move.second, AI_PLAYER_CHAR);
    std::cerr << "C++ Agent: After AI move, is game over: " << (next_game_board.is_game_over() ? "yes" : "no") << "\n";
    std::ofstream output_file_stream(GAME_STATE_FILE);
    if (!output_file_stream.is_open())
    {
      std::cerr << "C++ Agent Error: Cannot open " << GAME_STATE_FILE << " for writing.\n";
      return 1;
    }
    if (next_game_board.is_game_over())
    {
      std::cerr << "C++ Agent: Game over after AI move, winner: " << next_game_board.get_winner() << "\n";
      output_file_stream << "Game Over! Winner: " << next_game_board.get_winner() << "\n"
                         << next_game_board.to_string();
    }
    else
    {
      output_file_stream << "AI Move:\n"
                         << next_game_board.to_string();
      std::cerr << "C++ Agent: Wrote AI move to " << GAME_STATE_FILE << "\n";
    }
    output_file_stream.close();
  }
  else
  {
    std::cerr << "C++ Agent Warning: Agent executed but it is not AI's turn (" << current_game_board.get_current_player_color() << ")\n";
  }

  std::cerr << "C++ Agent: Execution completed\n";
  return 0;
}