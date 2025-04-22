#include <iostream>
#include <queue>
#include <vector>
#include <unordered_set>
using namespace std;

#define SIZE 3

string correct_configuration = "1,2,3,4,5,6,7,8,0,";

class search_node
{
public:
    int current_board_configuration[SIZE][SIZE];
    int priority_value;
    search_node *parent_node;
    pair<int, int> empty_tile_position;

public:
    search_node()
    {
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                int value;
                cin >> value;
                if (value == 0)
                {
                    empty_tile_position.first = i;
                    empty_tile_position.second = j;
                }
                current_board_configuration[i][j] = value;
            }
        }
        priority_value = 0;
        parent_node = nullptr;
    }
    search_node(const search_node &node)
    {
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                current_board_configuration[i][j] = node.current_board_configuration[i][j];
            }
        }
        empty_tile_position = node.empty_tile_position;
        priority_value = node.priority_value;
        parent_node = node.parent_node;
    }
    void print_current_configuration()
    {
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                cout << current_board_configuration[i][j] << " ";
            }
            cout << endl;
        }
    }
};

struct ComparePriority
{
    bool operator()(search_node *const &node_1, search_node *const &node_2) const
    {
        return node_1->priority_value > node_2->priority_value;
    }
};

priority_queue<search_node *, vector<search_node *>, ComparePriority> pq;
unordered_set<string> visited_configurations;

string get_board_string(const search_node &node)
{
    string config = "";
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            config += to_string(node.current_board_configuration[i][j]) + ",";
        }
    }
    return config;
}

int hamming_distance(const search_node &node)
{
    int distance = 0;
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            int reference_value = (i * SIZE + j + 1) % (SIZE * SIZE);
            if (node.current_board_configuration[i][j] != reference_value)
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

void generate_children(search_node &node, int (*heuristic_function)(const search_node &), int g_n)
{
    int row = node.empty_tile_position.first;
    int col = node.empty_tile_position.second;

    // Move Up
    if (row > 0)
    {
        search_node *child = new search_node(node);
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row - 1][col]);
        child->empty_tile_position = {row - 1, col};
        child->priority_value = heuristic_function(*child) + g_n + 1;
        child->parent_node = &node;
        string board_string = get_board_string(*child);
        if (visited_configurations.find(board_string) == visited_configurations.end())
        {
            pq.push(child);
            visited_configurations.insert(board_string);
        }
    }

    // Move Down
    if (row < SIZE - 1)
    {
        search_node *child = new search_node(node);
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row + 1][col]);
        child->empty_tile_position = {row + 1, col};
        child->priority_value = heuristic_function(*child) + g_n + 1;
        child->parent_node = &node;
        string board_string = get_board_string(*child);
        if (visited_configurations.find(board_string) == visited_configurations.end())
        {
            pq.push(child);
            visited_configurations.insert(board_string);
        }
    }

    // Move Left
    if (col > 0)
    {
        search_node *child = new search_node(node);
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row][col - 1]);
        child->empty_tile_position = {row, col - 1};
        child->priority_value = heuristic_function(*child) + g_n + 1;
        child->parent_node = &node;
        string board_string = get_board_string(*child);
        if (visited_configurations.find(board_string) == visited_configurations.end())
        {
            pq.push(child);
            visited_configurations.insert(board_string);
        }
    }

    // Move Right
    if (col < SIZE - 1)
    {
        search_node *child = new search_node(node);
        swap(child->current_board_configuration[row][col], child->current_board_configuration[row][col + 1]);
        child->empty_tile_position = {row, col + 1};
        child->priority_value = heuristic_function(*child) + g_n + 1;
        child->parent_node = &node;
        string board_string = get_board_string(*child);
        if (visited_configurations.find(board_string) == visited_configurations.end())
        {
            pq.push(child);
            visited_configurations.insert(board_string);
        }
    }
}

void puzze_solver(search_node &node, int (*heuristic_function)(const search_node &), int g_n)
{
    if (pq.empty())
    {
        cout << "no solution" << endl;
        return;
    }
    node = *pq.top();
    pq.pop();
    node.print_current_configuration();
    cout << "\n"
         << "------------------\n";
    if (get_board_string(node) == correct_configuration)
    {
        cout << "solved" << endl;
        return;
    }
    generate_children(node, heuristic_function, g_n + 1);
    puzze_solver(node, heuristic_function, g_n + 1);
}

int main(int argc, char *argv[])
{
    search_node node;
    node.priority_value = hamming_distance(node) + 0;
    pq.push(&node);
    visited_configurations.insert(get_board_string(node));
    puzze_solver(node, hamming_distance, 0);

    return 0;
}