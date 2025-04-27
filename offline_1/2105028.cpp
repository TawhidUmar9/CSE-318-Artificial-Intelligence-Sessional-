#include <iostream>
#include <queue>
#include <vector>
#include <unordered_set>
#include <stack>
using namespace std;

#define SIZE 3

int explored_node = 0;
int expanded_node = 0;

class search_node
{
public:
    int size;
    vector<vector<int>> current_board_configuration = vector<vector<int>>(SIZE, vector<int>(SIZE));
    int priority_value;
    search_node *parent_node;
    pair<int, int> empty_tile_position;
    int (*heuristic_function)(const search_node &);
    int g_n = 0;
    int h_n = 0;

public:
    search_node(int size_,
                int (*h_fn)(const search_node &) = nullptr)
        : size(size_),
          current_board_configuration(size_, vector<int>(size_)),
          priority_value(0),
          parent_node(nullptr),
          heuristic_function(h_fn),
          g_n(0),
          h_n(0)
    {
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                int input;
                cin >> input;
                current_board_configuration[i][j] = input;
                if (input == 0)
                    empty_tile_position = {i, j};
            }
        }
        h_n = heuristic_function
                  ? heuristic_function(*this)
                  : 0;
        priority_value = g_n + h_n;
    }
    search_node(const search_node &node)
        : size(node.size),
          current_board_configuration(node.current_board_configuration),
          priority_value(node.priority_value),
          parent_node(node.parent_node),
          empty_tile_position(node.empty_tile_position),
          heuristic_function(node.heuristic_function),
          g_n(node.g_n),
          h_n(node.h_n)
    {
    }
    void print_current_configuration()
    {
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                cout << current_board_configuration[i][j] << " ";
            }
            cout << endl;
        }
    }
    string get_board_string()
    {
        string config = "";
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                config += to_string(current_board_configuration[i][j]) + ",";
            }
        }
        return config;
    }

    void set_heuristic_function(int (*heuristic_function)(const search_node &))
    {
        this->heuristic_function = heuristic_function;
    }
};

struct ComparePriority
{
    bool operator()(search_node *const &n1,
                    search_node *const &n2) const
    {
        if (n1->priority_value == n2->priority_value)
        {
            return n1->h_n > n2->h_n;
        }
        return n1->priority_value > n2->priority_value;
    }
};

unordered_set<string> closed_list;
priority_queue<search_node *, vector<search_node *>, ComparePriority> open_list;
vector<search_node *> all_allocated_nodes;

bool solvable(search_node &node, int size = 3)
{
    vector<int> config;
    int inversions = 0;

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            int val = node.current_board_configuration[i][j];
            if (val != 0)
                config.push_back(val);
        }
    }

    for (int i = 0; i < config.size() - 1; i++)
    {
        for (int j = i + 1; j < config.size(); j++)
        {
            if (config[i] > config[j])
                inversions++;
        }
    }

    cout << "inversions: " << inversions << endl;

    // Solvability condition
    if (size % 2 != 0)
        return inversions % 2 == 0;
    else
    {
        int row_from_bottom = size - node.empty_tile_position.first ; 
        if (row_from_bottom % 2 == 0)                                    // empty tile is on an even row from the bottom
        {
            return inversions % 2 != 0;
        }
        else // empty tile is on an odd row from the bottom
        {
            return inversions % 2 == 0;
        }
    }
}

void generate_children(search_node &node)
{
    int row = node.empty_tile_position.first;
    int col = node.empty_tile_position.second;
    // move up
    if (row > 0)
    {
        search_node *child = new search_node(node);
        child->parent_node = &node;
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row - 1][col]);
        child->empty_tile_position = {row - 1, col};
        child->g_n = node.g_n + 1;
        child->h_n = child->heuristic_function(*child);
        child->priority_value = child->g_n + child->h_n;
        string board_string = child->get_board_string();
        if (closed_list.find(board_string) == closed_list.end())
        {
            open_list.push(child);
            explored_node++;
            all_allocated_nodes.push_back(child);
            closed_list.insert(board_string);
        }
    }
    // move down
    if (row < node.size - 1)
    {
        search_node *child = new search_node(node);
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row + 1][col]);
        child->parent_node = &node;
        child->empty_tile_position = make_pair(row + 1, col);
        child->g_n = node.g_n + 1;
        child->h_n = child->heuristic_function(*child);
        child->priority_value = child->g_n + child->h_n;
        string board_string = child->get_board_string();
        if (closed_list.find(board_string) == closed_list.end())
        {
            open_list.push(child);
            explored_node++;
            all_allocated_nodes.push_back(child);
            closed_list.insert(board_string);
        }
    }
    // move left
    if (col > 0)
    {
        search_node *child = new search_node(node);
        child->parent_node = &node;
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row][col - 1]);
        child->empty_tile_position = make_pair(row, col - 1);
        child->g_n = node.g_n + 1;
        child->h_n = child->heuristic_function(*child);
        child->priority_value = child->g_n + child->h_n;
        string board_string = child->get_board_string();
        if (closed_list.find(board_string) == closed_list.end())
        {
            open_list.push(child);
            explored_node++;
            all_allocated_nodes.push_back(child);
            closed_list.insert(board_string);
        }
    }
    // move right
    if (col < node.size - 1)
    {
        search_node *child = new search_node(node);
        child->parent_node = &node;
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row][col + 1]);
        child->empty_tile_position = make_pair(row, col + 1);
        child->g_n = node.g_n + 1;
        child->h_n = child->heuristic_function(*child);
        child->priority_value = child->g_n + child->h_n;
        string board_string = child->get_board_string();
        if (closed_list.find(board_string) == closed_list.end())
        {
            open_list.push(child);
            explored_node++;
            all_allocated_nodes.push_back(child);
            closed_list.insert(board_string);
        }
    }
}

int hamming_distance(const search_node &node)
{
    int distance = 0;
    for (int i = 0; i < node.size; i++)
    {
        for (int j = 0; j < node.size; j++)
        {
            int expected_value = (i * node.size + j + 1) % (node.size * node.size);
            if (node.current_board_configuration[i][j] != expected_value)
            {
                distance++;
            }
        }
    }
    return distance;
}

int manhattan_distane(const search_node &node)
{
    int distance = 0;

    return distance;
}

int euclidean_distance(const search_node &node)
{
    int distance = 0;

    return distance;
}

int linear_conflict(const search_node &node)
{
    int conflict = 0;

    return conflict;
}

search_node *puzzle_solver(const string &correct_configuration)
{
    while (!open_list.empty())
    {
        search_node *promising_node = open_list.top();
        open_list.pop();
        expanded_node++;
        if (promising_node->get_board_string() == correct_configuration)
        {
            cout << "solved\n";
            cout << "solved\n";
            return promising_node;
        }
        generate_children(*promising_node);
    }
    cout << "no solution\n";
    return nullptr;
}

int main(int argc, char *argv[])
{
    int n;
    cin >> n;
    search_node *node = new search_node(n, hamming_distance);
    cout << node->size << endl;

    node->g_n = 0;
    node->h_n = hamming_distance(*node);
    node->priority_value = node->g_n + node->h_n;
    bool solvable_flag = solvable(*node, n);

    if (!solvable_flag)
    {
        cout << "not solvable" << endl;
        delete node;
        return 0;
    }

    open_list.push(node);
    all_allocated_nodes.push_back(node);

    explored_node++;

    closed_list.insert(node->get_board_string());

    string correct_configuration = "";

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            int reference_value = (i * n + j + 1) % (n * n);
            correct_configuration += to_string(reference_value) + ",";
        }
    }

    search_node *correct_config = puzzle_solver(correct_configuration);

    stack<search_node *> path;
    while (correct_config != nullptr)
    {
        path.push(correct_config);
        correct_config = correct_config->parent_node;
    }

    while (!path.empty())
    {
        search_node *step = path.top();
        path.pop();
        step->print_current_configuration();
        cout << "----------------" << endl;
    }
    cout << "explored node: " << explored_node << endl;
    cout << "expanded node: " << expanded_node << endl;
    while (!all_allocated_nodes.empty())
    {
        search_node *temp = all_allocated_nodes.back();
        all_allocated_nodes.pop_back();
        delete temp;
    }
    return 0;
}
