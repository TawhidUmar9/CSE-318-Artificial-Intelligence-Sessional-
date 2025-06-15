#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <limits>
#include "minimax.cpp"

const int BOARD_ROWS = 9;
const int BOARD_COLS = 6;
const int AI_SEARCH_DEPTH = 4;
const char AI_PLAYER_CHAR = 'B';
const char HUMAN_PLAYER_CHAR = 'R';
const std::string GAME_STATE_FILE = "input.txt";

const int INF_POS = std::numeric_limits<int>::max();
const int INF_NEG = std::numeric_limits<int>::min();

std::string parse_header(const std::string &line) {
  if (line.rfind("Human Move:", 0) == 0) return "Human";
  if (line.rfind("AI Move:", 0) == 0) return "AI";
  return "Unknown";
}

int main() {
  std::ifstream input_file_stream(GAME_STATE_FILE);
  if (!input_file_stream.is_open()) {
    std::cerr << "C++ Agent Error: Cannot open " << GAME_STATE_FILE << " for reading.\n";
    return 1;
  }

  std::string header_line;
  if (!std::getline(input_file_stream, header_line)) {
    std::cerr << "C++ Agent Error: Cannot read header from " << GAME_STATE_FILE << ".\n";
    input_file_stream.close();
    return 1;
  }
  std::string last_mover_type = parse_header(header_line);

  char current_player_char;
  if (last_mover_type == "Human") {
    current_player_char = AI_PLAYER_CHAR;
  } else if (last_mover_type == "AI") {
    current_player_char = HUMAN_PLAYER_CHAR;
  } else {
    std::cerr << "C++ Agent Error: Unknown header '" << header_line << "' in " << GAME_STATE_FILE << ".\n";
    input_file_stream.close();
    return 1;
  }

  std::vector<std::vector<std::string>> string_grid(BOARD_ROWS, std::vector<std::string>(BOARD_COLS));
  for (int r = 0; r < BOARD_ROWS; ++r) {
    std::string row_line;
    if (!std::getline(input_file_stream, row_line)) {
      std::cerr << "C++ Agent Error: Incomplete board data in " << GAME_STATE_FILE << " at row " << r << ".\n";
      input_file_stream.close();
      return 1;
    }
    size_t start_pos = 0;
    size_t space_pos = row_line.find(' ');
    int c = 0;
    while (space_pos != std::string::npos && c < BOARD_COLS) {
      string_grid[r][c] = row_line.substr(start_pos, space_pos - start_pos);
      start_pos = space_pos + 1;
      space_pos = row_line.find(' ', start_pos);
      c++;
    }
    if (c < BOARD_COLS) {
      string_grid[r][c] = row_line.substr(start_pos);
    }
  }
  input_file_stream.close();

  Board current_game_board(BOARD_ROWS, BOARD_COLS);
  try {
    current_game_board = Board::from_string_vector(string_grid, current_player_char);
  } catch (const std::exception &e) {
    std::cerr << "C++ Agent Error: Board parsing failed: " << e.what() << ".\n";
    return 1;
  }

  // Check if board is empty (no orbs)
  bool is_board_empty = true;
  for (int r = 0; r < BOARD_ROWS; ++r) {
    for (int c = 0; c < BOARD_COLS; ++c) {
      if (current_game_board.grid[r][c].color != EMPTY) {
        is_board_empty = false;
        break;
      }
    }
    if (!is_board_empty) break;
  }

  if (is_board_empty && last_mover_type == "Human") {
    std::cerr << "C++ Agent Warning: Empty board with 'Human Move:' header. Waiting for human's first move.\n";
    return 0; // Exit without making a move
  }

  if (current_game_board.is_game_over()) {
    std::ofstream output_file_stream(GAME_STATE_FILE);
    if (!output_file_stream.is_open()) {
      std::cerr << "C++ Agent Error: Cannot open " << GAME_STATE_FILE << " for writing.\n";
      return 1;
    }
    output_file_stream << "Game Over! Winner: " << current_game_board.get_winner() << "\n"
                       << current_game_board.to_string();
    output_file_stream.close();
    return 0;
  }

  if (current_game_board.get_current_player_color() == AI_PLAYER_CHAR) {
    std::pair<std::pair<int, int>, int> ai_move_result =
        minimax(current_game_board, AI_SEARCH_DEPTH, AI_PLAYER_CHAR, INF_NEG, INF_POS, evaluate_heuristic);
    std::pair<int, int> chosen_move = ai_move_result.first;
    if (chosen_move.first == -1 && chosen_move.second == -1) {
      std::cerr << "C++ Agent Error: No valid move selected by minimax.\n";
      return 1;
    }
    Board next_game_board = current_game_board.apply_move(chosen_move.first, chosen_move.second, AI_PLAYER_CHAR);
    std::ofstream output_file_stream(GAME_STATE_FILE);
    if (!output_file_stream.is_open()) {
      std::cerr << "C++ Agent Error: Cannot open " << GAME_STATE_FILE << " for writing.\n";
      return 1;
    }
    output_file_stream << "AI Move:\n" << next_game_board.to_string();
    output_file_stream.close();
  } else {
    std::cerr << "C++ Agent Warning: Agent executed but it is not AI's turn (" << current_game_board.get_current_player_color() << ")\n";
  }

  return 0;
}