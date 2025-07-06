#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <random>
#include <limits>
#include <numeric>
#include <functional>

using namespace std;

class DataPoint
{
public:
    vector<string> features;
    string label;
};

class DataSet
{
public:
    vector<DataPoint> data_points;
    vector<string> feature_names;
    string dataset_name;

    DataSet() = default;
    DataSet(string name) : dataset_name(std::move(name)) {}
};

class Node
{
public:
    bool is_leaf;
    string predicted_class;
    int split_feature_index;
    string split_feature_name;
    bool is_numerical_split;
    double split_value;

    unordered_map<string, Node *> children;

    Node() : is_leaf(false), split_feature_index(-1), split_feature_name(""), is_numerical_split(false), split_value(0.0) {}

    ~Node()
    {
        for (auto const &pair : children)
        {
            delete pair.second;
        }
        children.clear();
    }
};

void trim(string &str)
{
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
}

string get_majority_class(const DataSet &data)
{
    if (data.data_points.empty())
        return "";
    unordered_map<string, int> class_count;
    for (const auto &point : data.data_points)
        class_count[point.label]++;

    string majority_class;
    int max_count = -1;
    for (const auto &pair : class_count)
    {
        if (pair.second > max_count)
        {
            max_count = pair.second;
            majority_class = pair.first;
        }
    }
    return majority_class;
}

pair<bool, string> is_homogeneous(const DataSet &data)
{
    if (data.data_points.empty())
        return {false, ""};

    string class_label = data.data_points[0].label;
    for (size_t i = 1; i < data.data_points.size(); ++i)
    {
        if (data.data_points[i].label != class_label)
            return {false, ""};
    }
    return {true, class_label};
}

double calculate_entropy(const vector<DataPoint> &data_points)
{
    if (data_points.empty())
        return 0.0;

    unordered_map<string, int> class_count;
    for (const auto &point : data_points)
        class_count[point.label]++;

    double entropy = 0.0;
    double total_size = data_points.size();
    for (const auto &pair : class_count)
    {
        double probability = (double)(pair.second) / total_size;
        if (probability > 0)
            entropy -= probability * log2(probability);
    }
    return entropy;
}

double calculate_intrinsic_value(double total_parent_size, const vector<vector<DataPoint>> &children_splits)
{
    double intrinsic_value = 0.0;
    for (const auto &child : children_splits)
    {
        double proportion = child.size() / total_parent_size;
        if (proportion > 0)
        {
            intrinsic_value -= proportion * log2(proportion);
        }
    }
    return intrinsic_value;
}

double information_gain(double entropy_parent, const vector<vector<DataPoint>> &children_splits, double total_parent_size)
{
    double weighted_child_entropy = 0.0;
    for (const auto &child_split : children_splits)
    {
        if (!child_split.empty())
        {
            weighted_child_entropy += (child_split.size() / total_parent_size) * calculate_entropy(child_split);
        }
    }
    return entropy_parent - weighted_child_entropy;
}

double information_gain_ratio(double entropy_parent, const vector<vector<DataPoint>> &children_splits, double total_parent_size)
{
    double gain = information_gain(entropy_parent, children_splits, total_parent_size);
    double intrinsic_value = calculate_intrinsic_value(total_parent_size, children_splits);
    if (intrinsic_value < numeric_limits<double>::epsilon())
    {
        return 0.0;
    }
    return gain / intrinsic_value;
}

double normalized_weighted_information_gain(double entropy_parent, const vector<vector<DataPoint>> &children_splits, double total_parent_size)
{
    double gain = information_gain(entropy_parent, children_splits, total_parent_size);
    double k = (double)(children_splits.size());
    if (k == 0 || total_parent_size == 0)
    {
        return 0.0;
    }
    double term1_denominator = log2(k + 1.0);
    double term1 = (term1_denominator > numeric_limits<double>::epsilon()) ? gain / term1_denominator : 0.0;
    double term2 = 1.0 - ((double)(k - 1.0) / total_parent_size);
    return term1 * term2;
}

unordered_map<string, vector<bool>> numerical_feature_flags;
int global_node_count = 0;
int global_max_depth_reached = 0;

Node *buildTree(DataSet &current_data, vector<int> available_feature_indices, int max_depth, int current_depth,
                double (*criterion_function)(double, const vector<vector<DataPoint>> &, double))
{
    // cout << "Building tree at depth: " << current_depth << ", available features: " << available_feature_indices.size() << endl;
    // cout << "Dataset: " << current_data.dataset_name << ", Data Points: " << current_data.data_points.size() << endl;
    global_node_count++;
    if (current_depth > global_max_depth_reached)
        global_max_depth_reached = current_depth;

    if (current_data.data_points.empty())
    {
        global_node_count--;
        return nullptr;
    }
    auto homogeneous_result = is_homogeneous(current_data);
    if (homogeneous_result.first)
    {
        Node *node = new Node();
        node->is_leaf = true;
        node->predicted_class = homogeneous_result.second;
        return node;
    }
    if ((max_depth != 0 && current_depth >= max_depth) || available_feature_indices.empty())
    {
        Node *node = new Node();
        node->is_leaf = true;
        node->predicted_class = get_majority_class(current_data);
        return node;
    }

    int best_feature_index = -1;
    double best_score = -1.0;
    double best_split_value = 0.0;
    bool best_split_is_numerical = false;

    double parent_entropy = calculate_entropy(current_data.data_points);
    double total_size = current_data.data_points.size();

    for (int feature_idx : available_feature_indices)
    {
        bool is_numerical = numerical_feature_flags[current_data.dataset_name][feature_idx];

        if (is_numerical)
        {
            // cout << "Evaluating numerical feature: " << current_data.feature_names[feature_idx] << endl;
            unordered_set<double> unique_values_set;
            for (const auto &dp : current_data.data_points)
                unique_values_set.insert(stod(dp.features[feature_idx]));

            if (unique_values_set.size() < 2)
                continue;

            vector<double> unique_values(unique_values_set.begin(), unique_values_set.end());
            sort(unique_values.begin(), unique_values.end());

            for (size_t i = 0; i < unique_values.size() - 1; i++)
            {
                // cout<<"Evaluating split point between: " << unique_values[i] << " and " << unique_values[i + 1] << endl;
                double split_point = (unique_values[i] + unique_values[i + 1]) / 2.0;

                vector<DataPoint> left_split, right_split;
                for (const auto &dp : current_data.data_points)
                {
                    if (stod(dp.features[feature_idx]) <= split_point)
                        left_split.push_back(dp);
                    else
                        right_split.push_back(dp);
                }

                if (left_split.empty() || right_split.empty())
                    continue;

                vector<vector<DataPoint>> children = {left_split, right_split};
                double score = criterion_function(parent_entropy, children, total_size);
                if (score > best_score)
                {
                    best_score = score;
                    best_feature_index = feature_idx;
                    best_split_value = split_point;
                    best_split_is_numerical = true;
                }
            }
        }
        else
        {
            unordered_map<string, vector<DataPoint>> subsets;
            for (const auto &dp : current_data.data_points)
            {
                subsets[dp.features[feature_idx]].push_back(dp);
            }

            if (subsets.size() < 2)
                continue;

            vector<vector<DataPoint>> children;
            for (const auto &pair : subsets)
                children.push_back(pair.second);

            double score = criterion_function(parent_entropy, children, total_size);
            if (score > best_score)
            {
                best_score = score;
                best_feature_index = feature_idx;
                best_split_is_numerical = false;
            }
        }
    }

    Node *node = new Node();
    if (best_score <= 0)
    {
        node->is_leaf = true;
        node->predicted_class = get_majority_class(current_data);
        return node;
    }

    node->split_feature_index = best_feature_index;
    node->split_feature_name = current_data.feature_names[best_feature_index];
    node->predicted_class = get_majority_class(current_data);

    vector<int> next_available_feature_indices;
    if (!best_split_is_numerical)
    {
        for (int idx : available_feature_indices)
        {
            if (idx != best_feature_index)
                next_available_feature_indices.push_back(idx);
        }
    }
    else
    {
        next_available_feature_indices = available_feature_indices;
    }

    if (best_split_is_numerical)
    {
        node->is_numerical_split = true;
        node->split_value = best_split_value;

        DataSet left_data(current_data.dataset_name), right_data(current_data.dataset_name);
        left_data.feature_names = right_data.feature_names = current_data.feature_names;

        for (const auto &dp : current_data.data_points)
        {
            if (stod(dp.features[best_feature_index]) <= best_split_value)
                left_data.data_points.push_back(dp);
            else
                right_data.data_points.push_back(dp);
        }

        node->children["<="] = buildTree(left_data, next_available_feature_indices, max_depth, current_depth + 1, criterion_function);
        node->children[">"] = buildTree(right_data, next_available_feature_indices, max_depth, current_depth + 1, criterion_function);
    }
    else
    {
        node->is_numerical_split = false;
        unordered_map<string, DataSet> subsets;
        for (const auto &dp : current_data.data_points)
        {
            string val = dp.features[best_feature_index];
            if (subsets.find(val) == subsets.end())
            {
                subsets[val] = DataSet(current_data.dataset_name);
                subsets[val].feature_names = current_data.feature_names;
            }
            subsets[val].data_points.push_back(dp);
        }
        for (auto &pair : subsets)
        {
            node->children[pair.first] = buildTree(pair.second, next_available_feature_indices, max_depth, current_depth + 1, criterion_function);
        }
    }
    for (auto &pair : node->children)
    {
        if (pair.second == nullptr)
        {
            Node *leaf = new Node();
            leaf->is_leaf = true;
            leaf->predicted_class = node->predicted_class;
            pair.second = leaf;
        }
    }
    return node;
}

string predict(Node *tree, const DataPoint &data_point, const string &default_class)
{
    if (tree == nullptr)
        return default_class;
    if (tree->is_leaf)
        return tree->predicted_class;

    if (tree->is_numerical_split)
    {
        try
        { // To handle '?'
            double val = stod(data_point.features[tree->split_feature_index]);
            const string &branch = (val <= tree->split_value) ? "<=" : ">";
            if (tree->children.count(branch))
            {
                return predict(tree->children.at(branch), data_point, tree->predicted_class);
            }
        }
        catch (const std::invalid_argument &ia)
        {
            return tree->predicted_class;
        }
    }
    else
    {
        const string &feature_value = data_point.features[tree->split_feature_index];
        if (tree->children.count(feature_value))
        {
            return predict(tree->children.at(feature_value), data_point, tree->predicted_class);
        }
    }
    return tree->predicted_class;
}

double evaluate(Node *tree, const DataSet &test_data)
{
    if (tree == nullptr || test_data.data_points.empty())
        return 0.0;

    int correct_predictions = 0;
    string default_class = get_majority_class(test_data);
    for (const auto &data_point : test_data.data_points)
    {
        string predicted_label = predict(tree, data_point, default_class);
        if (predicted_label == data_point.label)
        {
            correct_predictions++;
        }
    }
    return (double)(correct_predictions) / test_data.data_points.size();
}

DataSet load_csv(const string &filepath, const string &dataset_name)
{
    DataSet dataset(dataset_name);
    ifstream file(filepath);
    if (!file.is_open())
    {
        cout << "Error: Could not open file " << filepath << endl;
        return dataset;
    }
    string line;
    if (getline(file, line))
    {
        stringstream ss(line);
        string cell;
        if (dataset_name == "iris")
            getline(ss, cell, ',');

        while (getline(ss, cell, ','))
        {
            trim(cell);
            dataset.feature_names.push_back(cell);
        }
        if (!dataset.feature_names.empty())
            dataset.feature_names.pop_back();
    }

    while (getline(file, line))
    {
        stringstream ss(line);
        string cell;
        if (dataset_name == "iris")
            getline(ss, cell, ',');

        vector<string> row_values;
        while (getline(ss, cell, ','))
        {
            trim(cell);
            row_values.push_back(cell);
        }

        if (row_values.size() == dataset.feature_names.size() + 1)
        {
            DataPoint dp;
            dp.features.assign(row_values.begin(), row_values.end() - 1);
            dp.label = row_values.back();
            dataset.data_points.push_back(dp);
        }
    }
    return dataset;
}

void shuffle_and_split(const DataSet &original_data, DataSet &train_data, DataSet &test_data, double train_ratio, mt19937 &rng)
{
    train_data.feature_names = original_data.feature_names;
    train_data.dataset_name = original_data.dataset_name;
    test_data.feature_names = original_data.feature_names;
    test_data.dataset_name = original_data.dataset_name;
    train_data.data_points.clear();
    test_data.data_points.clear();

    vector<DataPoint> shuffled_data = original_data.data_points;
    shuffle(shuffled_data.begin(), shuffled_data.end(), rng);

    size_t train_size = static_cast<size_t>(shuffled_data.size() * train_ratio);
    for (size_t i = 0; i < train_size && i < shuffled_data.size(); ++i)
        train_data.data_points.push_back(shuffled_data[i]);

    for (size_t i = train_size; i < shuffled_data.size(); ++i)
        test_data.data_points.push_back(shuffled_data[i]);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <criterion> <maxDepth>" << endl;
        cout << "  <criterion>: IG, IGR, or NWIG" << endl;
        return 1;
    }
    string criterion_str = argv[1];
    int max_depth = stoi(argv[2]);

    double (*criterion_function)(double, const vector<vector<DataPoint>> &, double) = nullptr;
    if (criterion_str == "IG")
        criterion_function = information_gain;
    else if (criterion_str == "IGR")
        criterion_function = information_gain_ratio;
    else if (criterion_str == "NWIG")
        criterion_function = normalized_weighted_information_gain;
    else
    {
        cout << "Invalid criterion. Defaulting to IG." << endl;
        criterion_str = "IG";
        criterion_function = information_gain;
    }

    numerical_feature_flags["adult"] = {true, false, true, false, true, false, false, false, false, false, true, true, true, false};
    numerical_feature_flags["iris"] = {true, true, true, true};

    DataSet iris_original_data = load_csv("Iris.csv", "iris");
    DataSet adult_original_data = load_csv("adult.data", "adult");

    vector<DataSet *> datasets_to_process = {&iris_original_data, &adult_original_data};
    random_device rd;
    mt19937 rng(rd());

    for (DataSet *original_data_ptr : datasets_to_process)
    {
        DataSet &original_data = *original_data_ptr;
        if (original_data.data_points.empty())
            continue;

        cout << "Selected Criterion: " << criterion_str << ", Max Depth: " << max_depth << endl;

        double total_accuracy = 0.0;
        long total_nodes = 0;
        int total_depth = 0;
        const int num_runs = 20;

        for (int i = 0; i < num_runs; ++i)
        {
            DataSet train_data, test_data;
            shuffle_and_split(original_data, train_data, test_data, 0.8, rng);

            global_node_count = 0;
            global_max_depth_reached = 0;

            vector<int> all_feature_indices(original_data.feature_names.size());
            iota(all_feature_indices.begin(), all_feature_indices.end(), 0);

            Node *root = buildTree(train_data, all_feature_indices, max_depth, 0, criterion_function);

            total_nodes += global_node_count;
            total_depth += global_max_depth_reached;
            total_accuracy += evaluate(root, test_data);

            delete root;
        }

        double average_accuracy = total_accuracy / num_runs;
        cout << "Average Accuracy over " << num_runs << " runs: " << average_accuracy * 100.0 << "%" << endl;

        double average_nodes = (double)(total_nodes) / num_runs;
        double average_depth = (double)(total_depth) / num_runs;
        cout << "Mean Stats over " << num_runs << " runs:" << endl;
        cout << "    Average Nodes: " << average_nodes << endl;
        cout << "    Average Depth: " << average_depth << endl;
    }

    return 0;
}