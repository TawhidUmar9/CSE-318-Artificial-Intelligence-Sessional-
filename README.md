# CSE 318: Artificial Intelligence Sessional

This repository contains the solutions for the CSE 318 Artificial Intelligence sessional assignments. The projects cover a range of fundamental AI topics, including informed search, local search, adversarial search, and machine learning.

---

## 📂 Repository Structure
```
.
├── N-Puzzle
│   └── 2105028.cpp
├── Max-cut problem by GRASP
│   ├── 2105028.cpp
│   └── 2105028.csv
├── Adversarial Search
│   └── chain-reaction/
└── Decision Tree
├── 2105028.cpp
└── 2105028_report.pdf
```

---

## 🚀 Assignment Overviews

### 1. N-Puzzle Solver using A* Search

* **Problem**: To find the shortest sequence of moves to solve any given `k x k` N-Puzzle, and to identify when a puzzle is unsolvable.
* **Solution**: An **A\* search algorithm** was implemented in [2105028.cpp](./N-Puzzle/2105028.cpp) to find the optimal solution path. The implementation is modular, supporting four different heuristics: **Hamming Distance**, **Manhattan Distance**, **Euclidean Distance**, and **Linear Conflict**. The program first determines if a puzzle is solvable by calculating inversions before beginning the search.

### 2. Max-Cut Problem using GRASP

* **Problem**: For a given weighted graph, find a partition of vertices into two sets that maximizes the sum of the weights of the edges connecting the two sets. This is a well-known NP-hard optimization problem.
* **Solution**: The **GRASP (Greedy Randomized Adaptive Search Procedure)** metaheuristic was implemented in [2105028.cpp](./Max-cut%20problem%20by%20GRASP/2105028.cpp) to find high-quality solutions. This involved building and comparing several algorithms: a **Randomized** heuristic, a **Greedy** heuristic, a **Semi-Greedy** construction using a Restricted Candidate List (RCL), and a **Local Search** phase to refine the generated solutions. Performance was benchmarked on 54 graphs, with results summarized in [2105028.csv](./Max-cut%20problem%20by%20GRASP/2105028.csv).

### 3. Adversarial Search for Chain Reaction

* **Problem**: To create an intelligent agent capable of playing the deterministic, two-player board game "Chain Reaction" against a human opponent.
* **Solution**: A complete game engine was developed inside the [chain-reaction](./Adversarial%20Search/chain-reaction/) directory. The AI agent uses the **Minimax search algorithm with alpha-beta pruning** to determine the best move. The agent's performance relies on a set of five custom-designed **heuristic evaluation functions** that assess the board state's quality, allowing the AI to make strategic decisions. The architecture separates the AI backend from the UI, communicating via a file protocol.

### 4. Decision Tree Learning

* **Problem**: To implement a decision tree learning algorithm from scratch that can classify data based on different attribute selection criteria and handle overfitting through pruning.
* **Solution**: A decision tree classifier was implemented in [2105028.cpp](./Decision%20Tree/2105028.cpp) and analyzed in the accompanying [report](./Decision%20Tree/2105028_report.pdf). The algorithm supports three node-splitting criteria: **Information Gain (IG)**, **Gain Ratio (IGR)**, and a custom **Normalized Weighted Information Gain (NWIG)**. It also implements **depth-based pruning** by accepting a maximum depth as a command-line argument to control the tree's growth and prevent overfitting.

---

### Disclaimer

This work is for educational and reference purposes. Please adhere to your institution's academic integrity policies.
