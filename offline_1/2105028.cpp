#include <iostream>
#include <queue>
#include <vector>
#include <unordered_set>
#include <stack>
#include <cmath>
using namespace std;

#define SIZE 3

int explored_node = 0;
int expanded_node = 0;

class search_node
{
public:
    int size;
    vector<vector<int>> current_board_configuration = vector<vector<int>>(SIZE, vector<int>(SIZE));
    double priority_value;
    search_node *parent_node;
    pair<int, int> empty_tile_position;
    double (*heuristic_function)(const search_node &);
    double g_n = 0;
    double h_n = 0;

public:
    search_node(int size_,
                double (*h_fn)(const search_node &) = nullptr)
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

    void set_heuristic_function(double (*heuristic_function)(const search_node &))
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
        int row_from_bottom = size - node.empty_tile_position.first;
        if (row_from_bottom % 2 == 0) // empty tile is on an even row from the bottom
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
        }else{
            delete child; 
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
        }else{
            delete child;
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
        }else{
            delete child; 
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
        }else{
            delete child; // Avoid memory leak if the node is already in closed_list
        }
    }
}

double hamming_distance(const search_node &node)
{
    double distance = 0;
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

double manhattan_distane(const search_node &node)
{
    double distance = 0;
    for (int i = 0; i < node.size; i++)
    {
        for (int j = 0; j < node.size; j++)
        {
            int value = node.current_board_configuration[i][j];
            if (value != 0)
            {
                int expected_row = (value - 1) / node.size;
                int expected_col = (value - 1) % node.size;
                distance += abs(expected_row - i) + abs(expected_col - j);
            }
        }
    }

    return distance;
}

double euclidean_distance(const search_node &node)
{
    double distance = 0;
    for (int i = 0; i < node.size; i++)
    {
        for (int j = 0; j < node.size; j++)
        {
            int value = node.current_board_configuration[i][j];
            if (value != 0)
            {
                int expected_row = (value - 1) / node.size;
                int expected_col = (value - 1) % node.size;
                distance += sqrt(pow(expected_row - i, 2) + pow(expected_col - j, 2));
            }
        }
    }

    return distance;
}

double linear_conflict(const search_node &node)
{
    int size = node.size;
    const auto &grid = node.current_board_configuration;
    int conflicts = 0;

    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            int value1 = grid[i][j];
            if (value1 == 0)
                continue;
            int correct_row_value1 = (value1 - 1) / size;
            int correct_col_value2 = (value1 - 1) % size;
            if (correct_row_value1 != i)
                continue; // Row te belong na korle conitnue

            for (int k = j + 1; k < size; ++k)
            {
                int value2 = grid[i][k];

                if (value2 == 0)
                    continue;
                int correct_row_value2 = (value2 - 1) / size;
                int correct_col_value1 = (value2 - 1) % size;
                if (correct_col_value2 != i) // Correct row te belong kortese na
                    continue;
                if (correct_col_value1 > correct_col_value2) // Order is wrong
                    conflicts++;
            }
        }
    }

    // Column conflicts
    for (int j = 0; j < size; ++j)
    {
        for (int i = 0; i < size; ++i)
        {
            int value1 = grid[i][j];
            if (value1 == 0)
                continue;
            int correct_row_value1 = (value1 - 1) / size;
            int correct_col_value1 = (value1 - 1) % size;
            if (correct_col_value1 != j)
                continue; // correct column e belong kortese na

            for (int k = i + 1; k < size; ++k)
            {
                int value2 = grid[k][j];
                if (value2 == 0)
                    continue;
                int correct_row_value2 = (value2 - 1) / size;
                int correct_col_value2 = (value2 - 1) % size;
                if (correct_col_value2 != j) // correct column e belong kortese na
                    continue;

                if (correct_row_value1 > correct_row_value2)
                    conflicts++;
            }
        }
    }
    double total_distance = manhattan_distane(node) + 2 * conflicts;
    return total_distance;
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
    int heuristic_choice;
    search_node *node = new search_node(n, hamming_distance);
    if (argc < 2)
    {
        cout << "Missing arguement" << endl;
        return 0;
    }
    switch (argv[1][0])
    {
    case '1':
        node->set_heuristic_function(hamming_distance);
        cout << "Heuristic function: Hamming distance" << endl;
        break;
    case '2':
        node->set_heuristic_function(manhattan_distane);
        cout << "Heuristic function: Manhattan distance" << endl;
        break;
    case '3':
        node->set_heuristic_function(euclidean_distance);
        cout << "Heuristic function: Euclidean distance" << endl;
        break;
    case '4':
        node->set_heuristic_function(linear_conflict);
        cout << "Heuristic function: Linear conflict" << endl;
        break;
    default:
        break;
    }

    node->g_n = 0;
    node->h_n = node->heuristic_function(*node);
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
    int path_length = 0;
    while (correct_config != nullptr)
    {
        path.push(correct_config);
        path_length++;
        correct_config = correct_config->parent_node;
    }

    cout << "Minimum number of moves = " << --path_length << endl;
    while (!path.empty())
    {
        search_node *step = path.top();
        path.pop();
        step->print_current_configuration();
        cout << endl;
    }
    cout << "Explored node: " << explored_node << endl;
    cout << "Expanded node: " << expanded_node << endl;
    while (!all_allocated_nodes.empty())
    {
        search_node *temp = all_allocated_nodes.back();
        all_allocated_nodes.pop_back();
        delete temp;
    }
    closed_list.clear();


    return 0;
}
