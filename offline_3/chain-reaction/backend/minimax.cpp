#include "heuristic.cpp"
#include <algorithm>
#include <utility>
#include <limits>
#include <iostream>

const int INF_POS_VAL = std::numeric_limits<int>::max();
const int INF_NEG_VAL = std::numeric_limits<int>::min();

std::pair<std::pair<int, int>, int> minimax(Board state, int depth, char maximizing_player_color, int alpha, int beta, int (*heuristic_func)(const Board &, char))
{
    if (state.is_game_over())
    {
        char winner = state.get_winner();
        if (winner == maximizing_player_color)
            return {{-1, -1}, INF_POS_VAL};
        if (winner != 'N')
            return {{-1, -1}, INF_NEG_VAL};
        return {{-1, -1}, 0};
    }
    if (depth == 0)
    {
        int eval = heuristic_func(state, maximizing_player_color);
        return {{-1, -1}, eval};
    }

    char current_player_on_turn = state.get_current_player_color();
    std::vector<Board> child_states = state.get_children(current_player_on_turn);

    if (child_states.empty())
    {
        return {{-1, -1}, heuristic_func(state, maximizing_player_color)};
    }

    if (current_player_on_turn == maximizing_player_color)
    {
        int max_eval = INF_NEG_VAL;
        std::pair<int, int> best_move_for_this_turn = {-1, -1};
        for (size_t i = 0; i < child_states.size(); ++i)
        {
            const auto &child = child_states[i];
            int eval = minimax(child, depth - 1, maximizing_player_color, alpha, beta, heuristic_func).second;
            if (eval > max_eval)
            {
                max_eval = eval;
                best_move_for_this_turn = child.get_last_move();
            }
            alpha = std::max(alpha, max_eval);
            if (beta <= alpha)
            {
                break;
            }
        }
        return {best_move_for_this_turn, max_eval};
    }
    else
    {
        int min_eval = INF_POS_VAL;
        for (size_t i = 0; i < child_states.size(); ++i)
        {
            const auto &child = child_states[i];
            int eval = minimax(child, depth - 1, maximizing_player_color, alpha, beta, heuristic_func).second;
            if (eval < min_eval)
                min_eval = eval;
            beta = std::min(beta, min_eval);
            if (beta <= alpha)
            {
                break;
            }
        }
        return {{-1, -1}, min_eval};
    }
}