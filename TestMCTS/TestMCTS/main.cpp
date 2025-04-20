#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace std::chrono;

const int SIZE = 7;
const int DX[8] = { -1, -1, -1, 0, 1, 1, 1, 0 };
const int DY[8] = { -1, 0, 1, 1, 1, 0, -1, -1 };

struct Move {
    int x0, y0, x1, y1;
    Move(int a = -1, int b = -1, int c = -1, int d = -1) : x0(a), y0(b), x1(c), y1(d) {}
};

struct GameState {
    int board[SIZE][SIZE];
    int turn;

    GameState() {
        memset(board, 0, sizeof(board));
        board[0][0] = board[SIZE - 1][SIZE - 1] = 1;
        board[0][SIZE - 1] = board[SIZE - 1][0] = -1;
        turn = 0;
    }

    vector<Move> getLegalMoves(int player) const {
        vector<Move> moves;
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                if (board[x][y] == player) {
                    for (int dx = -2; dx <= 2; dx++) {
                        for (int dy = -2; dy <= 2; dy++) {
                            int nx = x + dx, ny = y + dy;
                            if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && board[nx][ny] == 0)
                                moves.emplace_back(x, y, nx, ny);
                        }
                    }
                }
            }
        }
        return moves;
    }

    void doMove(int x0, int y0, int x1, int y1, int player) {
        if (abs(x1 - x0) <= 1 && abs(y1 - y0) <= 1)
            board[x1][y1] = player; // clone
        else {
            board[x0][y0] = 0;
            board[x1][y1] = player; // jump
        }

        for (int i = 0; i < 8; i++) {
            int nx = x1 + DX[i], ny = y1 + DY[i];
            if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && board[nx][ny] == -player)
                board[nx][ny] = player;
        }
    }

    bool isGameOver() const {
        return getLegalMoves(1).empty() && getLegalMoves(-1).empty();
    }

    int getResult() const {
        int score = 0;
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                score += board[i][j];
        return score;
    }
};

// --- MCTS 节点结构 ---
struct MCTSNode {
    GameState state;
    Move move;
    MCTSNode* parent;
    vector<MCTSNode*> children;
    int visits;
    double wins;

    MCTSNode(GameState s, MCTSNode* p = nullptr, Move m = Move()) :
        state(s), parent(p), move(m), visits(0), wins(0) {}

    ~MCTSNode() {
        for (auto child : children)
            delete child;
    }

    MCTSNode* selectChild(int player) {
        MCTSNode* best = nullptr;
        double bestUCB1 = -1e9;
        for (auto child : children) {
            double ucb1 = child->wins / (child->visits + 1e-6) +
                sqrt(2.0 * log(visits + 1) / (child->visits + 1e-6));
            if (ucb1 > bestUCB1) {
                bestUCB1 = ucb1;
                best = child;
            }
        }
        return best;
    }
};

void expandNode(MCTSNode* node, int player) {
    vector<Move> moves = node->state.getLegalMoves(player);
    for (const auto& m : moves) {
        GameState newState = node->state;
        newState.doMove(m.x0, m.y0, m.x1, m.y1, player);
        node->children.push_back(new MCTSNode(newState, node, m));
    }
}

int simulate(GameState state, int player) {
    int currentPlayer = player;
    int rounds = 0;
    while (!state.isGameOver() && rounds < 100) {
        vector<Move> moves = state.getLegalMoves(currentPlayer);
        if (!moves.empty()) {
            Move m = moves[rand() % moves.size()];
            state.doMove(m.x0, m.y0, m.x1, m.y1, currentPlayer);
        }
        currentPlayer = -currentPlayer;
        rounds++;
    }
    return state.getResult();
}

// --- MCTS 主逻辑 ---
const int TIME_LIMIT_MS = 850;
const int MAX_SIMULATIONS = 2000;

Move selectBestMove_MCTS(GameState& state, int player) {
    MCTSNode* root = new MCTSNode(state);
    int simulations = 0;
    auto startTime = high_resolution_clock::now();

    while (simulations < MAX_SIMULATIONS &&
        duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count() < TIME_LIMIT_MS) {
        MCTSNode* node = root;
        int currentPlayer = player;

        // Selection
        while (!node->children.empty()) {
            node = node->selectChild(currentPlayer);
            currentPlayer = -currentPlayer;
        }

        // Expansion
        if (node->visits > 0 && !node->state.isGameOver()) {
            expandNode(node, currentPlayer);
            if (!node->children.empty()) {
                node = node->children[rand() % node->children.size()];
                currentPlayer = -currentPlayer;
            }
        }

        // Simulation
        int result = simulate(node->state, currentPlayer);

        // Backpropagation
        while (node != nullptr) {
            node->visits++;
            if ((player == 1 && result > 0) || (player == -1 && result < 0))
                node->wins++;
            else if (result == 0)
                node->wins += 0.5;
            node = node->parent;
        }

        simulations++;
    }

    // 最终选择胜率最高的动作
    MCTSNode* bestChild = nullptr;
    double bestRate = -1.0;
    for (auto child : root->children) {
        if (child->visits > 0) {
            double winRate = child->wins / child->visits;
            if (winRate > bestRate) {
                bestRate = winRate;
                bestChild = child;
            }
        }
    }

    Move bestMove = (bestChild ? bestChild->move : Move());
    delete root;
    return bestMove;
}

// --- 主函数入口 ---
int main() {
    srand(time(0));
    GameState state;
    int currBotColor;
    cin >> currBotColor;

    // 读取历史落子
    int steps;
    cin >> steps;
    for (int i = 0; i < steps; i++) {
        int x0, y0, x1, y1;
        cin >> x0 >> y0 >> x1 >> y1;
        int color = (i % 2 == 0 ? -currBotColor : currBotColor);
        state.doMove(x0, y0, x1, y1, color);
    }

    // 决策
    Move bestMove = selectBestMove_MCTS(state, currBotColor);
    cout << bestMove.x0 << " " << bestMove.y0 << " "
        << bestMove.x1 << " " << bestMove.y1 << endl;
    return 0;
}
