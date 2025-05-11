#include <iostream>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <math.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <chrono>
#include <vector>
#include <utility>
using namespace std;

unordered_map<string, int> best_value = {
    {"G1", 12078},
    {"G2", 12084},
    {"G3", 12077},
    {"G11", 627},
    {"G12", 621},
    {"G13", 645},
    {"G14", 3187},
    {"G15", 3169},
    {"G22", 14123},
    {"G23", 14129},
    {"G24", 14131},
    {"G32", 1560},
    {"G33", 1537},
    {"G34", 1541},
    {"G35", 8000},
    {"G36", 7996},
    {"G37", 8009},
    {"G43", 7027},
    {"G44", 7022},
    {"G45", 7020},
    {"G48", 6000},
    {"G49", 6000},
    {"G50", 5988}};

long long get_cut_weight(unordered_set<int> &X, unordered_set<int> &Y, const vector<vector<int>> &edges)
{
    long long cut_weight = 0;
    for (auto x : X)
    {
        for (auto y : Y)
        {
            cut_weight += edges[x][y];
        }
    }
    return cut_weight;
}

double randomized_max_cut(const unordered_set<int> &vertices, const vector<vector<int>> &edges, int n)
{
    double total_weight = 0.0;
    for (int i = 0; i < n; i++)
    {
        unordered_set<int> X, Y;
        for (auto e : vertices)
        {
            if (rand() % 2 == 0)
                X.insert(e);
            else
                Y.insert(e);
        }
        double cut_weight = get_cut_weight(X, Y, edges);
        total_weight += cut_weight;
    }
    return (total_weight / n);
}

pair<unordered_set<int>, unordered_set<int>> greedy_max_cut(const unordered_set<int> &vertices, const vector<vector<int>> &edges, const pair<int, int> &heaviest_edge)

{
    int u = -1, v = -1;

    // 1) Find the heaviest edge
    u = heaviest_edge.first;
    v = heaviest_edge.second;

    unordered_set<int> X{u}, Y{v};

    // 2) Build list of unassigned vertices
    vector<int> unassigned;
    for (int z : vertices)
    {
        if (z != u && z != v)
            unassigned.push_back(z);
    }

    // 3) Greedy assignment
    for (int z : unassigned)
    {
        long long w_x = 0, w_y = 0;
        for (int y : Y)
            w_x += edges[z][y];
        for (int x : X)
            w_y += edges[z][x];

        if (w_x > w_y)
        {
            X.insert(z);
        }
        else
        {
            Y.insert(z);
        }
    }

    return {X, Y};
}

pair<unordered_set<int>, unordered_set<int>> semi_greedy_max_cut(const unordered_set<int> &vertices,
                                                                 const vector<vector<int>> &edges, double alpha, const pair<int, int> &heaviest_edge)
{
    srand(time(0));

    // 1) find the heaviest edge
    int u = -1, v = -1;
    u = heaviest_edge.first;
    v = heaviest_edge.second;

    unordered_set<int> X{u}, Y{v};
    // 2) build list of unassigned vertices
    unordered_set<int> v_prime;
    for (int z : vertices)
    {
        if (z != u && z != v)
            v_prime.insert(z);
    }
    while (!v_prime.empty())
    {
        unordered_map<int, long long> sigma_x, sigma_y;

        for (auto v : v_prime) // calculatin sigma_x and sigma_y
        {
            sigma_x[v] = 0;
            sigma_y[v] = 0;
            for (auto x : X)
            {
                sigma_x[v] += edges[v][x];
            }
            for (auto y : Y)
            {
                sigma_y[v] += edges[v][y];
            }
        }
        // calculating w_min and w_max

        long long w_min = numeric_limits<long long>::max();
        long long w_max = numeric_limits<long long>::min();

        for (auto v : v_prime)
        {
            w_min = min(min(sigma_y[v], sigma_x[v]), w_min);
            w_max = max(max(sigma_y[v], sigma_x[v]), w_max);
        }
        double mu = (double)w_min + alpha * (double)(w_max - w_min);

        // form the RCL
        vector<int> RCL;
        for (auto v : v_prime)
        {
            long long f_v = max(sigma_x[v], sigma_y[v]);
            if ((double)f_v >= mu)
            {
                RCL.push_back(v);
            }
        }

        if (RCL.empty())
        {
            break;
        }
        // randomly select a vertex from RCL
        int index = rand() % RCL.size();
        int v_star = RCL[index];
        if (sigma_x[v_star] > sigma_y[v_star])
        {
            Y.insert(v_star);
        }
        else
        {
            X.insert(v_star);
        }
        v_prime.erase(v_star);
    }
    return {X, Y};
}

pair<unordered_set<int>, unordered_set<int>> local_search(const unordered_set<int> &S, const unordered_set<int> &S_bar, const vector<vector<int>> &edges, const unordered_set<int> &vertices)
{
    unordered_set<int> set_S(S), set_S_bar(S_bar);
    bool improved = true;
    while (improved)
    {
        improved = false;
        long long best_delta = 0;
        long long best_vertex = -1;
        long long delta = 0;

        for (auto v : vertices)
        {
            long long sigma_S = 0, sigma_S_bar = 0;
            bool in_S = set_S.count(v) > 0;

            for (auto u : set_S)
            {
                if (u == v)
                    continue;
                sigma_S += edges[u][v];
            }

            for (auto u : set_S_bar)
            {
                if (u == v)
                    continue;
                sigma_S_bar += edges[u][v];
            }

            if (in_S)
                delta = sigma_S - sigma_S_bar;
            else
                delta = sigma_S_bar - sigma_S;

            if (delta > best_delta)
            {
                best_delta = delta;
                best_vertex = v;
            }
        }

        if (best_delta > 0)
        {
            improved = true;
            if (set_S.count(best_vertex))
            {
                set_S.erase(best_vertex);
                set_S_bar.insert(best_vertex);
            }
            else
            {
                set_S_bar.erase(best_vertex);
                set_S.insert(best_vertex);
            }
        }
    }

    // Return the improved cut:
    return {set_S, set_S_bar};
}

pair<unordered_set<int>, unordered_set<int>> grasp(const unordered_set<int> &vertices, const vector<vector<int>> &edges, const pair<int, int> &heaviest_edge,

                                                   int maxIterations, double alpha = 0.5)
{
    unordered_set<int> best_X, best_Y;
    long long best_weight = 0;

    for (int i = 0; i < maxIterations; ++i)
    {
        // cout << "Iteration: " << i << endl;
        auto partition = semi_greedy_max_cut(vertices, edges, alpha, heaviest_edge);
        // cout << "Semi-Greedy Partition: " << endl;
        unordered_set<int> X = partition.first;
        unordered_set<int> Y = partition.second;

        // cout << "Local Search Partition: " << endl;
        auto improvedPartition = local_search(X, Y, edges, vertices);
        X = improvedPartition.first;
        Y = improvedPartition.second;

        long long current_weight = 0;
        current_weight = get_cut_weight(X, Y, edges);

        if (i == 0 || (current_weight > best_weight))
        {
            best_X = X;
            best_Y = Y;
            best_weight = current_weight;
        }
    }
    return {best_X, best_Y};
}

void test(string file_name, ofstream &csv, int max_iterations, double alpha)

{
    ifstream file(file_name);

    if (!file.is_open())
    {
        cerr << "Error opening file: " << file_name << endl;
        csv << file_name << ",ERROR,ERROR,ERROR,ERROR,ERROR,ERROR,ERROR,N/A\n";
        csv.flush();
        return;
    }

    string name = file_name.substr(file_name.find_last_of("/\\") + 1);
    name = name.substr(0, name.find_last_of('.'));
    name[0] = toupper(name[0]);

    unordered_set<int> vertices;
    int n, m;
    file >> n >> m;

    // Initialize edges with size (n+1) x (n+1) for 1-based indexing
    vector<vector<int>> edges(n + 1, vector<int>(n + 1, 0));

    // heaviest edge
    int heaviest_edge_u = -1, heaviest_edge_v = -1;
    int heaviest_edge_weight = 0;
    // Read edges
    for (int i = 0; i < m; i++)
    {
        int u, v, w;
        file >> u >> v >> w;
        edges[u][v] = w;
        edges[v][u] = w;
        // add the vertices to the set if not already present
        vertices.insert(u);
        vertices.insert(v);
        // find the heaviest edge
        if (w > heaviest_edge_weight)
        {
            heaviest_edge_weight = w;
            heaviest_edge_u = u;
            heaviest_edge_v = v;
        }
    }

    pair<int, int> heaviest_edge = {heaviest_edge_u, heaviest_edge_v};
    file.close();
    // Run the algorithms

    auto start = chrono::high_resolution_clock::now();
    double rand_cut = randomized_max_cut(vertices, edges, max_iterations);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Randomized Max Cut: " << rand_cut << " (Time: " << duration.count() << " ms)" << endl;

    start = chrono::high_resolution_clock::now();
    auto greedy_cut = greedy_max_cut(vertices, edges, heaviest_edge);
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Greedy Max Cut: " << get_cut_weight(greedy_cut.first, greedy_cut.second, edges) << " (Time: " << duration.count() << " ms)" << endl;

    start = chrono::high_resolution_clock::now();
    auto semi_greedy_cut = semi_greedy_max_cut(vertices, edges, alpha, heaviest_edge);
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Semi-Greedy Max Cut: " << get_cut_weight(semi_greedy_cut.first, semi_greedy_cut.second, edges) << " (Time: " << duration.count() << " ms)" << endl;

    start = chrono::high_resolution_clock::now();
    auto local_cut = local_search(semi_greedy_cut.first, semi_greedy_cut.second, edges, vertices);
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Local Search Max Cut: " << get_cut_weight(local_cut.first, local_cut.second, edges) << " (Time: " << duration.count() << " ms)" << endl;

    start = chrono::high_resolution_clock::now();
    auto grasp_cut = grasp(vertices, edges, heaviest_edge, max_iterations, alpha);
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "GRASP Max Cut: " << get_cut_weight(grasp_cut.first, grasp_cut.second, edges) << " (Time: " << duration.count() << " ms)" << endl;
    cout << "--------------------------------------------------------\n";

    // Write results to CSV
    csv << name << ","
        << n << ","
        << m << ","
        << rand_cut << ","
        << get_cut_weight(greedy_cut.first, greedy_cut.second, edges) << ","
        << get_cut_weight(semi_greedy_cut.first, semi_greedy_cut.second, edges) << ","
        << get_cut_weight(local_cut.first, local_cut.second, edges) << ","
        << get_cut_weight(grasp_cut.first, grasp_cut.second, edges) << ",";

    auto it = best_value.find(name);
    if (it != best_value.end())
        csv << it->second << "\n";
    else
        csv << "N/A\n";
    csv.flush();
}
int main()
{
    try
    {
        string inputDir = "set1";
        ofstream csv("2105028.csv", ios::app);
        if (!csv.is_open())
        {
            cerr << "ERROR: could not open 2105028.csv for writing\n";
            return 1;
        }
        int max_iterations = 50;
        double alpha = 0.5;
        csv << "Name,|V|,|M|,Randomized,Greedy,Semi-Greedy,Local-Search,GRASP,KnownBest\n";
        for (const auto &entry : filesystem::directory_iterator(inputDir))
        {
            if (entry.path().extension() == ".rud")
            {
                string filename = entry.path().string();
                cout << "Processing file: " << filename << endl;
                test(filename, csv, max_iterations, alpha);
                csv.flush(); // Flush after each file
            }
        }
        csv.close();
        cout << "CSV file created successfully." << endl;
    }
    catch (const std::exception &e)
    {
        cerr << "ERROR: An exception occurred: " << e.what() << endl;
        return 1;
    }
    return 0;
}