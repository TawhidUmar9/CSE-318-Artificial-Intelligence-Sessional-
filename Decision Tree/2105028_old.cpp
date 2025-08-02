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
    unordered_map<string, Node *> children;

    Node() : is_leaf(false), split_feature_index(-1), split_feature_name("") {}

    ~Node()
    {
        for (auto const &pair : children)
        {
            delete pair.second;
        }
        children.clear();
    }
};

string get_majority_class(const DataSet &data)
{
    if (data.data_points.empty())
        return "";

    unordered_map<string, int> class_count;
    for (const auto &point : data.data_points)
    {
        class_count[point.label]++;
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

pair<bool, string> is_homogeneous(const DataSet &data)
{
    if (data.data_points.empty())
    {
        return {false, ""};
    }

    string class_label = data.data_points[0].label;
    for (size_t i = 1; i < data.data_points.size(); ++i)
    {
        if (data.data_points[i].label != class_label)
            return {false, class_label};
    }
    return {true, class_label};
}

double calculate_entropy(const DataSet &data)
{
    if (data.data_points.empty())
        return 0.0;

    unordered_map<string, int> class_count;
    for (const auto &point : data.data_points)
    {
        class_count[point.label]++;
    }

    double entropy = 0.0;
    int total_count = data.data_points.size();
    for (const auto &pair : class_count)
    {
        double probability = (double)(pair.second) / total_count;
        if (probability > 0)
        {
            entropy -= probability * log2(probability);
        }
    }
    return entropy;
}

vector<string> get_unique_feature_values(const DataSet &data, int feature_index)
{
    unordered_set<string> unique_values_set;
    for (const auto &point : data.data_points)
    {
        if (feature_index >= 0 && feature_index < (int)(point.features.size()))
        {
            unique_values_set.insert(point.features[feature_index]);
        }
    }
    return vector<string>(unique_values_set.begin(), unique_values_set.end());
}

DataSet split_data_set(const DataSet &original_data, int feature_index, const string &feature_value)
{
    DataSet new_data;
    new_data.feature_names = original_data.feature_names;
    new_data.dataset_name = original_data.dataset_name;

    for (const auto &point : original_data.data_points)
    {
        if (feature_index >= 0 && feature_index < (int)(point.features.size()) && point.features[feature_index] == feature_value)
        {
            new_data.data_points.push_back(point);
        }
    }
    return new_data;
}

double calculate_information_gain(const DataSet &data, int feature_index)
{
    double parent_entropy = calculate_entropy(data);
    double weighted_child_entropy = 0.0;

    vector<string> unique_values = get_unique_feature_values(data, feature_index);
    int total_count = data.data_points.size();

    if (total_count == 0)
        return 0.0;

    for (const string &value : unique_values)
    {
        DataSet child_data = split_data_set(data, feature_index, value);
        double child_entropy = calculate_entropy(child_data);
        double weight = (double)(child_data.data_points.size()) / total_count;
        weighted_child_entropy += weight * child_entropy;
    }

    return parent_entropy - weighted_child_entropy;
}

double calculate_intrinsic_value(const DataSet &data, int feature_index)
{
    double intrinsic_value = 0.0;
    vector<string> unique_values = get_unique_feature_values(data, feature_index);
    int total_count = data.data_points.size();

    if (total_count == 0)
        return 0.0;

    for (const string &value : unique_values)
    {
        DataSet child_data = split_data_set(data, feature_index, value);
        double proportion = (double)(child_data.data_points.size()) / total_count;
        if (proportion > 0)
        {
            intrinsic_value -= proportion * log2(proportion);
        }
    }
    return (intrinsic_value < numeric_limits<double>::epsilon()) ? 0.0 : intrinsic_value;
}

double calculate_information_gain_ratio(const DataSet &data, int feature_index)
{
    double ig = calculate_information_gain(data, feature_index);
    double iv = calculate_intrinsic_value(data, feature_index);

    if (iv < numeric_limits<double>::epsilon())
    {
        return 0.0;
    }
    return ig / iv;
}

double calculate_normalized_weighted_information_gain(const DataSet &data, int feature_index)
{
    double ig = calculate_information_gain(data, feature_index);
    double k = (double)(get_unique_feature_values(data, feature_index).size());
    double N = (double)(data.data_points.size());

    if (N == 0 || k == 0)
    {
        return 0.0;
    }

    double term1_denominator = log2(k + 1.0);
    double term1 = 0.0;
    if (term1_denominator > numeric_limits<double>::epsilon())
    {
        term1 = ig / term1_denominator;
    }
    else
    {
        term1 = 0.0;
    }

    double term2 = 1.0 - ((double)(k - 1.0) / N);

    return term1 * term2;
}

int global_node_count = 0;
int global_max_depth_reached = 0;

Node *buildTree(DataSet current_data, vector<int> available_feature_indices, int max_depth, int current_depth, double (*criterion_function)(const DataSet &, int))
{
    global_node_count++;
    if (current_depth > global_max_depth_reached)
        global_max_depth_reached = current_depth;

    Node *node = new Node();

    if (current_data.data_points.empty())
    {
        delete node;
        global_node_count--;
        return nullptr;
    }

    auto homogeneous_result = is_homogeneous(current_data);
    if (homogeneous_result.first)
    {
        node->is_leaf = true;
        node->predicted_class = homogeneous_result.second;
        return node;
    }

    if (max_depth != 0 && current_depth > max_depth)
    {
        node->is_leaf = true;
        node->predicted_class = get_majority_class(current_data);
        return node;
    }

    if (available_feature_indices.empty())
    {
        node->is_leaf = true;
        node->predicted_class = get_majority_class(current_data);
        return node;
    }

    int best_feature_index = -1;
    double max_score = -1.0;

    for (int feature_idx : available_feature_indices)
    {
        double current_score = criterion_function(current_data, feature_idx);

        if (isnan(current_score) || current_score < -1e-9)
        {
            current_score = -1.0;
        }

        if (current_score > max_score)
        {
            max_score = current_score;
            best_feature_index = feature_idx;
        }
    }

    if (best_feature_index == -1 || max_score < 1e-9)
    {
        node->is_leaf = true;
        node->predicted_class = get_majority_class(current_data);
        return node;
    }

    node->is_leaf = false;
    node->split_feature_index = best_feature_index;
    node->split_feature_name = current_data.feature_names[best_feature_index];
    node->predicted_class = get_majority_class(current_data);

    vector<string> unique_values = get_unique_feature_values(current_data, best_feature_index);

    vector<int> next_available_feature_indices;
    for (int idx : available_feature_indices)
    {
        if (idx != best_feature_index)
        {
            next_available_feature_indices.push_back(idx);
        }
    }

    for (const string &value : unique_values)
    {
        DataSet subset_data = split_data_set(current_data, best_feature_index, value);
        Node *child_node = buildTree(subset_data, next_available_feature_indices, max_depth, current_depth + 1, criterion_function);

        if (child_node)
        {
            node->children[value] = child_node;
        }
        else
        {
            Node *default_leaf = new Node();
            default_leaf->is_leaf = true;
            default_leaf->predicted_class = get_majority_class(current_data);
            node->children[value] = default_leaf;
        }
    }

    return node;
}

string predict(Node *tree, const DataPoint &data_point)
{
    if (tree == nullptr)
        return "ERROR: Null Tree";

    if (tree->is_leaf)
        return tree->predicted_class;

    int feature_idx = tree->split_feature_index;

    if (feature_idx < 0 || feature_idx >= (int)(data_point.features.size()))
        return tree->predicted_class;

    const string &feature_value = data_point.features[feature_idx];

    auto it = tree->children.find(feature_value);
    if (it != tree->children.end())
    {
        return predict(it->second, data_point);
    }
    else
    {
        return tree->predicted_class;
    }
}

double evaluate(Node *tree, const DataSet &test_data)
{
    if (tree == nullptr || test_data.data_points.empty())
    {
        return 0.0;
    }

    int correct_predictions = 0;
    for (const auto &data_point : test_data.data_points)
    {
        string predicted_label = predict(tree, data_point);
        if (predicted_label == data_point.label)
        {
            correct_predictions++;
        }
    }
    return (double)(correct_predictions) / test_data.data_points.size();
}

void discretize_adult_dataset(DataSet &dataset)
{
    if (dataset.dataset_name != "adult")
        return;

    unordered_map<string, int> feature_indices;
    for (size_t i = 0; i < dataset.feature_names.size(); ++i)
    {
        feature_indices[dataset.feature_names[i]] = (int)(i);
    }

    vector<string> numerical_features = {"age", "finalweight", "education-num", "capital-gain", "capital-loss", "hours-per-week"};

    for (auto &data_point : dataset.data_points)
    {
        for (const auto &feature_name : numerical_features)
        {
            if (feature_indices.count(feature_name))
            {
                int idx = feature_indices[feature_name];
                if (data_point.features[idx] == "?")
                {
                    data_point.features[idx] = "Unknown";
                    continue;
                }
                double value = stod(data_point.features[idx]);
                string new_value;

                if (feature_name == "age")
                {
                    if (value < 30)
                        new_value = "<30";
                    else if (value <= 35)
                        new_value = "30-35";
                    else if (value <= 40)
                        new_value = "35-40";
                    else if (value <= 45)
                        new_value = "40-45";
                    else if (value <= 50)
                        new_value = "45-50";
                    else if (value <= 55)
                        new_value = "50-55";
                    else if (value <= 60)
                        new_value = "55-60";
                    else
                        new_value = ">60";
                }
                else if (feature_name == "hours-per-week")
                {
                    if (value < 45)
                        new_value = "<45";
                    else if (value <= 50)
                        new_value = "45-50";
                    else if (value <= 55)
                        new_value = "50-55";
                    else if (value <= 60)
                        new_value = "55-60";
                    else
                        new_value = ">60";
                }
                else if (feature_name == "education-num")
                {
                    if (value <= 9)
                        new_value = "less";
                    else if (value <= 12)
                        new_value = "moderate";
                    else
                        new_value = "high";
                }
                else if (feature_name == "capital-gain")
                {
                    new_value = (value == 0) ? "Zero" : "NonZero";
                }
                else if (feature_name == "capital-loss")
                {
                    if (value < 30)
                        new_value = "<30";
                    else if (value <= 40)
                        new_value = "30-40";
                    else if (value <= 50)
                        new_value = "40-50";
                    else if (value <= 60)
                        new_value = "50-60";
                    else if (value > 60)
                        new_value = ">60";
                }
                else if (feature_name == "finalweight")
                {
                    if (value < 100000)
                        new_value = "<100000";
                    else if (value <= 200000)
                        new_value = "100000-200000";
                    else if (value <= 300000)
                        new_value = "200000-300000";
                    else if (value <= 400000)
                        new_value = "300000-400000";
                    else if (value <= 500000)
                        new_value = "400000-500000";
                    else
                        new_value = ">500000";
                }
                data_point.features[idx] = new_value;
            }
        }
    }
}

void trim(string &str)
{
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
}

DataSet load_csv(const string &filepath, const string &dataset_name)
{
    DataSet dataset;
    dataset.dataset_name = dataset_name;
    ifstream file(filepath);

    if (!file.is_open())
    {
        cout << "Error: Could not open file " << filepath << endl;
        return dataset;
    }

    string line;
    char delimiter = ',';

    if (getline(file, line))
    {
        stringstream ss(line);
        string cell;
        bool skip_first_col_iris = (dataset_name == "iris");

        while (getline(ss, cell, delimiter))
        {
            if (skip_first_col_iris)
            {
                skip_first_col_iris = false;
                continue;
            }
            trim(cell);
            dataset.feature_names.push_back(cell);
        }
        if (!dataset.feature_names.empty())
        {
            dataset.feature_names.pop_back();
        }
    }
    else
        return dataset;

    while (getline(file, line))
    {
        stringstream ss(line);
        string cell;
        DataPoint data_point;
        int feature_count = 0;
        bool skip_first_col_iris = (dataset_name == "iris");

        while (getline(ss, cell, delimiter))
        {
            if (skip_first_col_iris)
            {
                skip_first_col_iris = false;
                continue;
            }
            trim(cell);

            if (feature_count < (int)(dataset.feature_names.size()))
            {
                data_point.features.push_back(cell);
            }
            else
            {
                data_point.label = cell;
            }
            feature_count++;
        }
        if (!data_point.label.empty() || (data_point.features.size() == dataset.feature_names.size()))
        {
            dataset.data_points.push_back(data_point);
        }
    }

    file.close();
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

    int train_size = (int)(shuffled_data.size() * train_ratio);

    // Creating the 80 20 split.
    for (int i = 0; i < train_size; i++)
        train_data.data_points.push_back(shuffled_data[i]);
    for (int i = train_size; i < (int)(shuffled_data.size()); i++)
        test_data.data_points.push_back(shuffled_data[i]);
    if (train_data.data_points.empty() || test_data.data_points.empty())
        cout << "Warning: One of the datasets is empty after shuffling and splitting." << endl;
    if (train_data.data_points.size() + test_data.data_points.size() != original_data.data_points.size())
        cout << "Warning: The total number of data points after splitting does not match the original dataset." << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <criterion> <maxDepth>" << endl;
        cout << "  <criterion>: IG, IGR, or NWIG" << endl;
        cout << "  <maxDepth>: Integer (0 for no pruning)" << endl;
        return 1;
    }
    string criterion_str = argv[1];
    int max_depth = stoi(argv[2]);
    double (*criterion_function)(const DataSet &, int);
    if (criterion_str == "IG")
        criterion_function = calculate_information_gain;
    else if (criterion_str == "IGR")
        criterion_function = calculate_information_gain_ratio;
    else if (criterion_str == "NWIG")
        criterion_function = calculate_normalized_weighted_information_gain;
    else
    {
        criterion_function = calculate_information_gain;
        criterion_str = "IG (Default)";
    }
    cout << "Selected Criterion: " << criterion_str << ", Max Depth: " << max_depth << endl;

    DataSet iris_original_data = load_csv("Iris.csv", "iris");
    DataSet adult_original_data = load_csv("adult.data", "adult");
    discretize_adult_dataset(adult_original_data);

    vector<DataSet *> datasets_to_process = {&iris_original_data, &adult_original_data};
    random_device rd;
    mt19937 rng(rd());

    for (DataSet *original_data_ptr : datasets_to_process)
    {
        DataSet &original_data = *original_data_ptr;
        cout << "Processing dataset: " << original_data.dataset_name << endl;
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

            double accuracy = evaluate(root, test_data);
            total_accuracy += accuracy;

            delete root;
        }

        double average_accuracy = total_accuracy / num_runs;
        cout << "Average Accuracy over " << num_runs << " runs: " << average_accuracy * 100.0 << "%" << endl;

        if (true)
        {
            double average_nodes = (double)(total_nodes) / num_runs;
            double average_depth = (double)(total_depth) / num_runs;

            cout << "  (Unpruned Tree Mean Stats over " << num_runs << " runs):" << endl;
            cout << "    Average Nodes: " << average_nodes << endl;
            cout << "    Average Depth: " << average_depth << endl;
        }
    }

    return 0;
}