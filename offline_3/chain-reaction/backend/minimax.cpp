#include "heuristic.cpp"
#include <algorithm>
#include <utility>
#include <limits> 


const int INF = std::numeric_limits<int>::max();
const int NEG_INF = std::numeric_limits<int>::min();


// The function returns a pair: {{row, col}, score}
std::pair<std::pair<int, int>, int> minimax(Board state, int depth, char maximizing_player_color, int alpha, int beta, int (*heuristic_function)(const Board &, char))
{

    if (state.is_game_over())
    {
        char winner = state.get_winner();
        if (winner == maximizing_player_color)
        {
            return {{-1, -1}, INF};
        }
        else if (winner != 'N')
        {
            return {{-1, -1}, -INF};
        }
        return {{-1, -1}, 0};
    }
    if (depth == 0)
    {
        return {{-1, -1}, heuristic_function(state, maximizing_player_color)};
    }

    char current_player_on_turn = state.get_current_player_color();

    if (current_player_on_turn == maximizing_player_color)
    {
        int max_eval = -INF;
        std::pair<int, int> best_move_at_this_level = {-1, -1};

        std::vector<Board> children_states = state.get_children(current_player_on_turn);

        for (const auto &child : children_states)
        {
            int eval = minimax(child, depth - 1, maximizing_player_color, alpha, beta, heuristic_function).second;

            if (eval > max_eval)
            {
                max_eval = eval;
                best_move_at_this_level = child.get_last_move();
            }

            alpha = std::max(alpha, max_eval);
            if (beta <= alpha)
            {
                break;
            }
        }
        return {best_move_at_this_level, max_eval};
    }

    else
    {
        int min_eval = INF;
        std::vector<Board> children_states = state.get_children(current_player_on_turn);
        for (const auto &child : children_states)
        {
            int eval = minimax(child, depth - 1, maximizing_player_color, alpha, beta, heuristic_function).second;
            if (eval < min_eval)
            {
                min_eval = eval;
            }
            beta = std::min(beta, min_eval);
            if (beta <= alpha)
            {
                break;
            }
        }
        return {{-1, -1}, min_eval};
    }
}