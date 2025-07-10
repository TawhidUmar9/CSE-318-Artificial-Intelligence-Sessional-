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
    DataSet(string name) : dataset_name(move(name)) {}
};

class Node
{
public:
    bool is_leaf;
    string predicted_class;
    int split_feature_index;
    string split_feature_name;
    bool is_numerical_split;
    double split_value; // for numerical splits

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

string get_majority_class(const DataSet &original_data, const vector<size_t> &indices)
{
    if (indices.empty())
        return "";
    unordered_map<string, int> class_count;
    for (size_t index : indices)
    {
        class_count[original_data.data_points[index].label]++;
    }

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

pair<bool, string> is_homogeneous(const DataSet &original_data, const vector<size_t> &indices)
{
    if (indices.empty())
        return {false, ""};
    string class_label = original_data.data_points[indices[0]].label;
    for (size_t i = 1; i < indices.size(); ++i)
    {
        if (original_data.data_points[indices[i]].label != class_label)
            return {false, ""};
    }
    return {true, class_label};
}

double calculate_entropy(const DataSet &original_data, const vector<size_t> &indices)
{
    if (indices.empty())
        return 0.0;

    unordered_map<string, int> class_count;
    for (size_t index : indices)
    {
        class_count[original_data.data_points[index].label]++;
    }

    double entropy = 0.0;
    double total_size = indices.size();
    for (const auto &pair : class_count)
    {
        double probability = (double)(pair.second) / total_size;
        if (probability > 0)
            entropy -= probability * log2(probability);
    }
    return entropy;
}

double calculate_information_gain(double entropy_parent, const DataSet &original_data, const vector<vector<size_t>> &children_splits, double total_parent_size)
{
    double weighted_child_entropy = 0.0;
    for (const auto &child_indices : children_splits)
    {
        if (!child_indices.empty())
        {
            weighted_child_entropy += (child_indices.size() / total_parent_size) * calculate_entropy(original_data, child_indices);
        }
    }
    return entropy_parent - weighted_child_entropy;
}

double calculate_intrinsic_value(double total_parent_size, const vector<vector<size_t>> &children_splits)
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

double calculate_information_gain_ratio(double entropy_parent, const DataSet &original_data,
                                        const vector<vector<size_t>> &children_splits, double total_parent_size)
{
    double gain = calculate_information_gain(entropy_parent, original_data, children_splits, total_parent_size);
    double intrinsic_value = calculate_intrinsic_value(total_parent_size, children_splits);
    // if (intrinsic_value < 1e-9 || total_parent_size == 0)
    //     return 0.0;
    return gain / intrinsic_value;
}

double calculate_normalized_weighted_information_gain(double entropy_parent, const DataSet &original_data,
                                                      const vector<vector<size_t>> &children_splits, double total_parent_size)
{
    double gain = calculate_information_gain(entropy_parent, original_data, children_splits, total_parent_size);
    double k = (double)(children_splits.size());
    if (k <= 1 || total_parent_size == 0)
        return 0.0;

    double term1_denominator = log2(k + 1.0);
    // double term1 = (term1_denominator > 1e-9) ? gain / term1_denominator : 0.0;
    double term1 = gain / term1_denominator;
    double term2 = 1.0 - ((double)(k - 1.0) / total_parent_size);
    return term1 * term2;
}

unordered_map<string, vector<bool>> numerical_feature_flags;
int global_node_count = 0;
int global_max_depth_reached = 0;

Node *buildTree(const DataSet &original_data, const vector<size_t> &indices, vector<int> available_feature_indices, int max_depth, int current_depth,
                double (*criterion_function)(double, const DataSet &, const vector<vector<size_t>> &, double))
{
    global_node_count++;
    if (current_depth > global_max_depth_reached)
        global_max_depth_reached = current_depth;

    if (indices.empty())
    {
        global_node_count--;
        return nullptr;
    }
    auto homogeneous_result = is_homogeneous(original_data, indices);
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
        node->predicted_class = get_majority_class(original_data, indices);
        return node;
    }

    int best_feature_index = -1;
    double best_score = -1.0;
    double best_split_value = 0.0;
    bool best_split_is_numerical = false;

    double parent_entropy = calculate_entropy(original_data, indices);
    double total_size = indices.size();

    for (int feature_index : available_feature_indices)
    {
        bool is_numerical = numerical_feature_flags[original_data.dataset_name][feature_index];

        if (is_numerical)
        {
            unordered_set<double> unique_values_set;
            for (size_t index : indices)
            {
                try
                {
                    unique_values_set.insert(stod(original_data.data_points[index].features[feature_index]));
                }
                catch (exception &e)
                {
                }
            }

            if (unique_values_set.size() < 2)
                continue;

            vector<double> unique_values(unique_values_set.begin(), unique_values_set.end());
            sort(unique_values.begin(), unique_values.end());

            for (size_t i = 0; i < unique_values.size() - 1; i++)
            {
                double split_point = (unique_values[i] + unique_values[i + 1]) / 2.0;

                vector<size_t> left_indices, right_indices;
                for (size_t index : indices)
                {
                    double target_index;
                    try
                    {
                        target_index = stod(original_data.data_points[index].features[feature_index]);
                    }
                    catch (exception &e)
                    {
                    }
                    if (target_index <= split_point)
                        left_indices.push_back(index);
                    else
                        right_indices.push_back(index);
                }

                if (left_indices.empty() || right_indices.empty())
                    continue;

                vector<vector<size_t>> children_indices = {left_indices, right_indices};
                double score = criterion_function(parent_entropy, original_data, children_indices, total_size);

                if (score > best_score)
                {
                    best_score = score;
                    best_feature_index = feature_index;
                    best_split_value = split_point;
                    best_split_is_numerical = true;
                }
            }
        }
        else // Categorical
        {
            unordered_map<string, vector<size_t>> subset_indices;
            for (size_t index : indices)
            {
                subset_indices[original_data.data_points[index].features[feature_index]].push_back(index);
            }

            if (subset_indices.size() < 2)
                continue;

            vector<vector<size_t>> children_indices;
            for (const auto &pair : subset_indices)
                children_indices.push_back(pair.second);

            double score = criterion_function(parent_entropy, original_data, children_indices, total_size);
            if (score > best_score)
            {
                best_score = score;
                best_feature_index = feature_index;
                best_split_is_numerical = false;
            }
        }
    }

    Node *node = new Node();
    if (best_score <= 1e-9)
    {
        node->is_leaf = true;
        node->predicted_class = get_majority_class(original_data, indices);
        return node;
    }

    node->split_feature_index = best_feature_index;
    node->split_feature_name = original_data.feature_names[best_feature_index];
    node->predicted_class = get_majority_class(original_data, indices);

    vector<int> next_available_feature_indices;
    if (!best_split_is_numerical)
    {
        for (int index : available_feature_indices)
            if (index != best_feature_index)
                next_available_feature_indices.push_back(index);
    }
    else
    {
        next_available_feature_indices = available_feature_indices;
    }

    if (best_split_is_numerical)
    {
        node->is_numerical_split = true;
        node->split_value = best_split_value;

        vector<size_t> left_indices, right_indices;
        for (size_t index : indices)
        {
            double target_value;
            try
            {
                target_value = stod(original_data.data_points[index].features[best_feature_index]);
            }
            catch (exception &e)
            {
            }
            if (target_value <= best_split_value)
                left_indices.push_back(index);
            else
                right_indices.push_back(index);
        }

        node->children["<="] = buildTree(original_data, left_indices, next_available_feature_indices, max_depth, current_depth + 1, criterion_function);
        node->children[">"] = buildTree(original_data, right_indices, next_available_feature_indices, max_depth, current_depth + 1, criterion_function);
    }
    else // Categorical
    {
        node->is_numerical_split = false;
        unordered_map<string, vector<size_t>> subset_indices;
        for (size_t index : indices)
        {
            subset_indices[original_data.data_points[index].features[best_feature_index]].push_back(index);
        }
        for (auto &pair : subset_indices)
        {
            node->children[pair.first] = buildTree(original_data, pair.second, next_available_feature_indices, max_depth, current_depth + 1, criterion_function);
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
        {
            double target_value = stod(data_point.features[tree->split_feature_index]);
            const string &branch = (target_value <= tree->split_value) ? "<=" : ">";
            if (tree->children.count(branch))
                return predict(tree->children.at(branch), data_point, tree->predicted_class);
        }
        catch (const invalid_argument &ia)
        {
            return tree->predicted_class;
        }
    }
    else
    {
        const string &feature_value = data_point.features[tree->split_feature_index];
        if (tree->children.count(feature_value))
            return predict(tree->children.at(feature_value), data_point, tree->predicted_class);
    }
    return tree->predicted_class;
}

double evaluate(Node *tree, const DataSet &test_data, const DataSet &train_data)
{
    if (tree == nullptr || test_data.data_points.empty())
        return 0.0;

    int correct_predictions = 0;
    vector<size_t> train_indices(train_data.data_points.size());
    iota(train_indices.begin(), train_indices.end(), 0);
    string default_class = get_majority_class(train_data, train_indices);

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
        int col_index = 0;

        while (getline(ss, cell, ','))
        {
            if (dataset_name == "iris" && col_index == 0)
            {
                col_index++;
                continue;
            }
            if (dataset_name == "adult" && (col_index == 2 || col_index == 3 || col_index == 7))
            {
                col_index++;
                continue;
            }

            trim(cell);
            dataset.feature_names.push_back(cell);
            col_index++;
        }
        if (!dataset.feature_names.empty())
        {
            dataset.feature_names.pop_back();
        }
    }

    while (getline(file, line))
    {
        stringstream ss(line);
        string cell;
        vector<string> row_values;
        int col_index = 0;

        while (getline(ss, cell, ','))
        {
            if (dataset_name == "iris" && col_index == 0)
            {
                col_index++;
                continue;
            }
            else if (dataset_name == "adult" && (col_index == 2 || col_index == 3 || col_index == 7))
            {
                col_index++;
                continue;
            }

            trim(cell);
            row_values.push_back(cell);
            col_index++;
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
    for (size_t i = 0; i < shuffled_data.size(); ++i)
    {
        if (i < train_size)
        {
            train_data.data_points.push_back(shuffled_data[i]);
        }
        else
        {
            test_data.data_points.push_back(shuffled_data[i]);
        }
    }
    if (train_data.data_points.empty() || test_data.data_points.empty())
    {
        cout << "Not enough data to split into train and test sets." << endl;
        return;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <criterion> <maxDepth>" << endl;
        return 1;
    }
    string criterion_str = argv[1];
    int max_depth = stoi(argv[2]);

    double (*criterion_function)(double, const DataSet &, const vector<vector<size_t>> &, double) = nullptr;
    if (criterion_str == "IG")
        criterion_function = calculate_information_gain;
    else if (criterion_str == "IGR")
        criterion_function = calculate_information_gain_ratio;
    else if (criterion_str == "NWIG")
        criterion_function = calculate_normalized_weighted_information_gain;
    else
    {
        cout << "Invalid criterion. Defaulting to IG." << endl;
        criterion_str = "IG";
        criterion_function = calculate_information_gain;
    }

    numerical_feature_flags["adult"] = {true, false, true, false, true, false, false, false, false, false, true, true, true, false};
    numerical_feature_flags["iris"] = {true, true, true, true};

    cout << "Loading datasets..." << endl;
    DataSet iris_original_data = load_csv("Iris.csv", "iris");
    // age, workclass, finalweight, education, education_num, marital_status, occupation, relationship,
    // race, sex, capital_gain, capital_loss, hours_per_week, native_country, income

    DataSet adult_original_data = load_csv("adult.data", "adult");
    cout << "Datasets loaded." << endl;

    vector<DataSet> datasets_to_process = {iris_original_data, adult_original_data};
    random_device rd;
    mt19937 rng(rd());

    for (const auto &original_data : datasets_to_process)
    {
        if (original_data.data_points.empty())
            continue;

        cout << "Processing dataset: " << original_data.dataset_name << endl;
        cout << "Selected Criterion: " << criterion_str << ", Max Depth: " << max_depth << endl;

        double total_accuracy = 0.0;
        long total_nodes = 0;
        int total_depth = 0;
        const int num_runs = 20;

        for (int i = 0; i < num_runs; i++)
        {
            DataSet train_data, test_data;
            shuffle_and_split(original_data, train_data, test_data, 0.8, rng);

            global_node_count = 0;
            global_max_depth_reached = 0;

            vector<size_t> train_indices(train_data.data_points.size());
            iota(train_indices.begin(), train_indices.end(), 0);

            vector<int> all_feature_indices(original_data.feature_names.size());
            iota(all_feature_indices.begin(), all_feature_indices.end(), 0);

            Node *root = buildTree(train_data, train_indices, all_feature_indices, max_depth, 0, criterion_function);

            total_nodes += global_node_count;
            total_depth += global_max_depth_reached;
            total_accuracy += evaluate(root, test_data, train_data);

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