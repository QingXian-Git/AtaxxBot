#include <iostream>
#include <cstring>
#include <chrono>
#include <limits>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <cstdint>
#include <unordered_map>
#include "jsoncpp/json.h" // C++编译时默认包含此库

using namespace std;

const int MAX_DEPTH = 6;
const double MAX_NUM = 1e9;
const double MIN_NUM = -1e9;
const int TIME_LIMIT_MS = 700;
double CORNER_WEIGHT = 0;
double STABLE_WEIGHT = 0;
double COUNT_WEIGHT = 0;
double MOBILITY_WEIGHT = 0;
double BORDER_WEIGHT = 0;
double JUMP_WEIGHT = 0;

Json::Value ret;
int bestDepth = 0;
int posCount = 0;
int startX, startY, resultX, resultY;



chrono::time_point<chrono::high_resolution_clock> startTime;


int currBotColor; // 我所执子颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[7][7] = { 0 }; // 先x后y，记录棋盘状态
int blackPieceCount = 2, whitePieceCount = 2;
static int delta[24][2] = { { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },
{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
{ 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },
{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
{ -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },
{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 } };

struct Move {
	int x0, y0, x1, y1;
	bool operator==(const Move& other) const {
		return x0 == other.x0 && y0 == other.y0 &&
			x1 == other.x1 && y1 == other.y1;
	}
};

struct GameState {
	int board[7][7] = { 0 };
	int blackCount;
	int whiteCount;
};


struct MCTSNode {
	MCTSNode* parent;
	vector<MCTSNode*> children;
	GameState state;
	int visits;
	int wins;
	Move move;
	bool fullyExpanded;

	MCTSNode(GameState state, MCTSNode* parent = nullptr, Move move = {})
		:parent(parent), move(move), state(state), visits(0), wins(0), fullyExpanded(false) {}

	~MCTSNode() {
		for (MCTSNode* child : children) {
			delete child;
		}
	}
};

namespace std {
	template<>
	struct hash<Move> {
		std::size_t operator()(const Move& m) const {
			return ((m.x0 * 31 + m.y0) * 31 + m.x1) * 31 + m.y1;
		}
	};
}

// 判断是否在地图内
inline bool inMap(int x, int y)
{
	if (x < 0 || x > 6 || y < 0 || y > 6)
		return false;
	return true;
}

// 向Direction方向改动坐标，并返回是否越界
inline bool MoveStep(int& x, int& y, int Direction)
{
	x = x + delta[Direction][0];
	y = y + delta[Direction][1];
	return inMap(x, y);
}

// 在坐标处落子，检查是否合法或模拟落子
bool ProcStep(int x0, int y0, int x1, int y1, int color)
{
	if (color == 0)
		return false;
	if (x1 == -1) // 无路可走，跳过此回合
		return true;
	if (!inMap(x0, y0) || !inMap(x1, y1)) // 超出边界
		return false;
	if (gridInfo[x0][y0] != color)
		return false;
	int dx, dy, x, y, currCount = 0, dir;
	int effectivePoints[8][2];
	dx = abs((x0 - x1)), dy = abs((y0 - y1));
	if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) // 保证不会移动到原来位置，而且移动始终在5×5区域内
		return false;
	if (gridInfo[x1][y1] != 0) // 保证移动到的位置为空
		return false;
	if (dx == 2 || dy == 2) // 如果走的是5×5的外围，则不是复制粘贴
		gridInfo[x0][y0] = 0;
	else
	{
		if (color == 1)
			blackPieceCount++;
		else
			whitePieceCount++;
	}

	gridInfo[x1][y1] = color;
	for (dir = 0; dir < 8; dir++) // 影响邻近8个位置
	{
		x = x1 + delta[dir][0];
		y = y1 + delta[dir][1];
		if (!inMap(x, y))
			continue;
		if (gridInfo[x][y] == -color)
		{
			effectivePoints[currCount][0] = x;
			effectivePoints[currCount][1] = y;
			currCount++;
			gridInfo[x][y] = color;
		}
	}
	if (currCount != 0)
	{
		if (color == 1)
		{
			blackPieceCount += currCount;
			whitePieceCount -= currCount;
		}
		else
		{
			whitePieceCount += currCount;
			blackPieceCount -= currCount;
		}
	}
	return true;
}

bool ProcStep(int x0, int y0, int x1, int y1, int color, bool& jumpFlag)
{
	if (color == 0)
		return false;
	if (x1 == -1) // 无路可走，跳过此回合
		return true;
	if (!inMap(x0, y0) || !inMap(x1, y1)) // 超出边界
		return false;
	if (gridInfo[x0][y0] != color)
		return false;
	int dx, dy, x, y, currCount = 0, dir;
	int effectivePoints[8][2];
	dx = abs((x0 - x1)), dy = abs((y0 - y1));
	if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) // 保证不会移动到原来位置，而且移动始终在5×5区域内
		return false;
	if (gridInfo[x1][y1] != 0) // 保证移动到的位置为空
		return false;
	if (dx == 2 || dy == 2) { // 如果走的是5×5的外围，则不是复制粘贴
		jumpFlag = true;
		gridInfo[x0][y0] = 0;
	}
	else
	{
		if (color == 1)
			blackPieceCount++;
		else
			whitePieceCount++;
	}

	gridInfo[x1][y1] = color;
	for (dir = 0; dir < 8; dir++) // 影响邻近8个位置
	{
		x = x1 + delta[dir][0];
		y = y1 + delta[dir][1];
		if (!inMap(x, y))
			continue;
		if (gridInfo[x][y] == -color)
		{
			effectivePoints[currCount][0] = x;
			effectivePoints[currCount][1] = y;
			currCount++;
			gridInfo[x][y] = color;
		}
	}
	if (currCount != 0)
	{
		if (color == 1)
		{
			blackPieceCount += currCount;
			whitePieceCount -= currCount;
		}
		else
		{
			whitePieceCount += currCount;
			blackPieceCount -= currCount;
		}
	}
	return true;
}

bool timeExceeded() {
	auto now = chrono::high_resolution_clock::now();
	int elapsed = chrono::duration_cast<chrono::milliseconds>(now - startTime).count();
	return elapsed >= TIME_LIMIT_MS;
}

bool timeExceeded(const int& limit) {
	auto now = chrono::high_resolution_clock::now();
	int elapsed = chrono::duration_cast<chrono::milliseconds>(now - startTime).count();
	return elapsed >= limit;
}

bool applyMove(GameState& state, const Move& move) {
	int color = state.board[move.x0][move.y0];
	int dx, dy, dir;
	int currCount = 0;
	dx = abs((move.x0 - move.x1)), dy = abs((move.y0 - move.y1));
	if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) // 保证不会移动到原来位置，而且移动始终在5×5区域内
		return false;
	if (state.board[move.x1][move.y1] != 0) // 保证移动到的位置为空
		return false;
	if (dx == 2 || dy == 2) { // 如果走的是5×5的外围，则不是复制粘贴
		state.board[move.x0][move.y0] = 0;
	}
	else
	{
		if (color == 1)
			state.blackCount++;
		else
			state.whiteCount++;
	}
	state.board[move.x1][move.y1] = color;
	for (dir = 0; dir < 8; dir++) // 影响邻近8个位置
	{
		int nx = move.x1 + delta[dir][0];
		int ny = move.y1 + delta[dir][1];
		if (!inMap(nx, ny))
			continue;
		if (state.board[nx][ny] == -color)
		{
			currCount++;
			state.board[nx][ny] = color;

		}
	}
	if (currCount != 0)
	{
		if (color == 1)
		{
			state.blackCount += currCount;
			state.whiteCount -= currCount;
		}
		else
		{
			state.whiteCount += currCount;
			state.blackCount -= currCount;
		}
	}
	return true;
}



bool isStable(int x, int y) {
	for (int d = 0; d < 8; d++) {
		int nx = x + delta[d][0];
		int ny = y + delta[d][1];
		if (!inMap(nx, ny)) {
			continue;
		}
		if (gridInfo[nx][ny] == 0) {
			return false;
		}
	}
	return true;
}

int MCTSMoves(GameState& state, Move moves[], int color) {
	int count = 0;
	bool visited[7][7] = { false };
	for (int y0 = 0; y0 < 7; ++y0) {
		for (int x0 = 0; x0 < 7; ++x0) {
			if (state.board[x0][y0] != color) continue;

			for (int dir = 0; dir < 24; ++dir) {
				int dx = delta[dir][0], dy = delta[dir][1];
				int x1 = x0 + dx;
				int y1 = y0 + dy;

				if (!inMap(x1, y1)) continue;
				if (state.board[x1][y1] != 0) continue;
				if (abs(dx) <= 1 && abs(dy) <= 1) {
					if (!visited[x1][y1]) {
						visited[x1][y1] = true;
						moves[count++] = { x0, y0, x1, y1 };
					}
				}
				// 跳跃步直接记录，不去重
				else {
					moves[count++] = { x0, y0, x1, y1 };
				}

			}
		}
	}
	return count;
}

int MCTSMoves(const GameState& state, int color) {
	int count = 0;
	bool visited[7][7] = { false };
	for (int y0 = 0; y0 < 7; ++y0) {
		for (int x0 = 0; x0 < 7; ++x0) {
			if (state.board[x0][y0] != color) continue;

			for (int dir = 0; dir < 24; ++dir) {
				int dx = delta[dir][0], dy = delta[dir][1];
				int x1 = x0 + dx;
				int y1 = y0 + dy;

				if (!inMap(x1, y1)) continue;
				if (state.board[x1][y1] != 0) continue;
				if (abs(dx) <= 1 && abs(dy) <= 1) {
					if (!visited[x1][y1]) {
						visited[x1][y1] = true;
						count++;
					}
				}
				// 跳跃步直接记录，不去重
				else {
					count++;
				}

			}
		}
	}
	return count;
}

int GenerateMoves(int color) {
	int count = 0;
	bool visited[7][7] = { false };
	for (int y0 = 0; y0 < 7; ++y0) {
		for (int x0 = 0; x0 < 7; ++x0) {
			if (gridInfo[x0][y0] != color) continue;

			for (int dir = 0; dir < 24; ++dir) {
				int dx = delta[dir][0], dy = delta[dir][1];
				int x1 = x0 + dx;
				int y1 = y0 + dy;

				if (!inMap(x1, y1)) continue;
				if (gridInfo[x1][y1] != 0) continue;

				if (abs(dx) <= 1 && abs(dy) <= 1) {
					if (!visited[x1][y1]) {
						visited[x1][y1] = true;
						count++;
					}
				}
				// 跳跃步直接记录，不去重
				else {
					count++;
				}
			}
		}
	}
	return count;
}

int GenerateMoves(Move moves[], int color) {
	int count = 0;
	bool visited[7][7] = { false };
	for (int y0 = 0; y0 < 7; ++y0) {
		for (int x0 = 0; x0 < 7; ++x0) {
			if (gridInfo[x0][y0] != color) continue;

			for (int dir = 0; dir < 24; ++dir) {
				int dx = delta[dir][0], dy = delta[dir][1];
				int x1 = x0 + dx;
				int y1 = y0 + dy;

				if (!inMap(x1, y1)) continue;
				if (gridInfo[x1][y1] != 0) continue;
				if (abs(dx) <= 1 && abs(dy) <= 1) {
					if (!visited[x1][y1]) {
						visited[x1][y1] = true;
						moves[count++] = { x0, y0, x1, y1 };
					}
				}
				// 跳跃步直接记录，不去重
				else {
					moves[count++] = { x0, y0, x1, y1 };
				}

			}
		}
	}
	return count;
}

bool isGameOver(int& blackMoves, int& whiteMoves) {
	int emptyCount = 49 - blackPieceCount - whitePieceCount;
	if (emptyCount == 0)
		return true;
	if (blackPieceCount == 0 || whitePieceCount == 0)
		return true;
	if (blackMoves == 0 || whiteMoves == 0)
		return true;
	return false;
}

bool isGameOver(const GameState& state) {
	// 1. 棋盘已满
	int emptyCount = 49 - state.blackCount - state.whiteCount;

	if (emptyCount == 0)
		return true;

	// 2. 某一方无棋子
	if (state.blackCount == 0 || state.whiteCount == 0)
		return true;

	// 3. 一方没有合法走法

	int blackMoves = MCTSMoves(state, 1);
	int whiteMoves = MCTSMoves(state, -1);
	if (blackMoves == 0 || whiteMoves == 0)
		return true;

	// 否则游戏尚未结束
	return false;
}

int getWinner(const GameState& state) {

	int blackMoves = MCTSMoves(state, 1);
	int whiteMoves = MCTSMoves(state, -1);
	int bCount = state.blackCount;
	int wCount = state.whiteCount;
	int emptyCount = 49 - bCount - wCount;
	if (blackMoves > 0 && whiteMoves == 0) {
		bCount += emptyCount;
	}
	else if (blackMoves == 0 && whiteMoves > 0) {
		wCount += emptyCount;
	}
	if (bCount > wCount)
		return 1;      // 黑棋胜
	else if (wCount > bCount)
		return -1;     // 白棋胜
	else
		return 0;      // 平局
}

//MCTS
MCTSNode* selectPromisingNode(MCTSNode* node) {
	while (!node->children.empty()) {
		node = *max_element(node->children.begin(), node->children.end(),
			[](MCTSNode* a, MCTSNode* b) {
				//加上一个极小数字防止visit=0时报错
				double ucb_a = a->wins / (a->visits + 1e-6) + sqrt(2 * log(a->parent->visits + 1) / (a->visits + 1e-6));
				double ucb_b = b->wins / (b->visits + 1e-6) + sqrt(2 * log(b->parent->visits + 1) / (b->visits + 1e-6));
				return ucb_a < ucb_b;
			});
	}
	return node;
}

MCTSNode* expandNode(MCTSNode* node, int color) {
	Move legalMoves[500];
	int moveCount = MCTSMoves(node->state, legalMoves, color);
	unordered_set<Move> tried;
	for (auto* child : node->children) {
		tried.insert(child->move);
	}
	for (int i = 0; i < moveCount; i++) {
		if (tried.count(legalMoves[i]) == 0) {
			GameState newState = node->state;
			if (!applyMove(newState, legalMoves[i]))
				continue;
			MCTSNode* child = new MCTSNode(newState, node, legalMoves[i]);
			node->children.push_back(child);
			return child;
		}
	}
	node->fullyExpanded = true;
	return node;
}

int simulateRandom(GameState state, int currPlayer) {
	while (!isGameOver(state)) {
		Move legalMoves[500];
		int moveCount = MCTSMoves(state, legalMoves, currPlayer);
		if (moveCount == 0) {
			currPlayer = -currPlayer;
			continue;
		}
		int index = rand() % moveCount;
		applyMove(state, legalMoves[index]);
		currPlayer = -currPlayer;
	}
	int winner = getWinner(state);
	return winner;
}

void backPropagate(MCTSNode* node, int result, int botColor) {
	while (node != nullptr) {
		node->visits++;
		if (result == botColor) {
			node->wins++;
		}
		node = node->parent;
	}
}

//待办：空位被对方使用后的目数
double Evaluate(bool jumpTag) {
	double score = 0;
	int stableCount = 0;
	int myCount = (currBotColor == 1 ? blackPieceCount : whitePieceCount);
	int opCount = (currBotColor == 1 ? whitePieceCount : blackPieceCount);
	int emptyCount = 49 - blackPieceCount - whitePieceCount;
	int myMoveCount = 0;
	int opMoveCount = 0;
	int mobility = 0;
	bool flag = true;

	if (emptyCount > 32) {	//开局决策权重
		CORNER_WEIGHT = 0.05;
		COUNT_WEIGHT = 0.55;
		STABLE_WEIGHT = 0.25;
		MOBILITY_WEIGHT = 0;
		BORDER_WEIGHT = 0;
		JUMP_WEIGHT = 0.15;
	}
	else if (emptyCount > 15) {	//中期决策权重
		CORNER_WEIGHT = 0.05;
		COUNT_WEIGHT = 0.6;
		STABLE_WEIGHT = 0.25;
		MOBILITY_WEIGHT = 0.05;
		BORDER_WEIGHT = 0;
		JUMP_WEIGHT = 0.15;
	}
	else {	//末期决策权重
		CORNER_WEIGHT = 0;
		COUNT_WEIGHT = 0.6;
		STABLE_WEIGHT = 0.1;
		MOBILITY_WEIGHT = 0;
		BORDER_WEIGHT = 0;
		JUMP_WEIGHT = 0.3;
	}

	if (MOBILITY_WEIGHT != 0) {
		myMoveCount = GenerateMoves(currBotColor);
		opMoveCount = GenerateMoves(-currBotColor);
		mobility = myMoveCount - opMoveCount;
	}



	// 角落
	if (CORNER_WEIGHT != 0) {
		int corners[4][2] = { {0, 0}, {0, 6}, {6, 0}, {6, 6} };
		for (int i = 0; i < 4; i++) {
			int x = corners[i][0], y = corners[i][1];
			if (gridInfo[x][y] == currBotColor)
				score += CORNER_WEIGHT;
			else if (gridInfo[x][y] == -currBotColor)
				score -= CORNER_WEIGHT;
		}
	}


	//边界
	if (BORDER_WEIGHT != 0) {
		for (int i = 1; i < 6; i++) {
			if (gridInfo[0][i] == currBotColor) score += BORDER_WEIGHT;
			if (gridInfo[6][i] == currBotColor) score += BORDER_WEIGHT;
			if (gridInfo[i][0] == currBotColor) score += BORDER_WEIGHT;
			if (gridInfo[i][6] == currBotColor) score += BORDER_WEIGHT;
			if (gridInfo[0][i] == -currBotColor) score -= BORDER_WEIGHT;
			if (gridInfo[6][i] == -currBotColor) score -= BORDER_WEIGHT;
			if (gridInfo[i][0] == -currBotColor) score -= BORDER_WEIGHT;
			if (gridInfo[i][6] == -currBotColor) score -= BORDER_WEIGHT;
		}
	}

	// 稳定性
	if (STABLE_WEIGHT != 0) {
		for (int y = 0; y < 7; y++) {
			for (int x = 0; x < 7; x++) {
				if (gridInfo[x][y] == currBotColor) {
					if (isStable(x, y)) {
						stableCount++;
					}
				}
			}
		}
	}

	if (jumpTag && mobility >= 0) {
		score -= 3 * JUMP_WEIGHT;
	}


	score += (myCount - opCount) * COUNT_WEIGHT;
	score += stableCount * STABLE_WEIGHT;
	score += (mobility * MOBILITY_WEIGHT);
	return score;
}

double EasyEvaluate() {
	int myCount = (currBotColor == 1 ? blackPieceCount : whitePieceCount);
	int opCount = (currBotColor == 1 ? whitePieceCount : blackPieceCount);
	int score = 0.6 * (myCount - opCount);
	return score;
}

double EvaluateEnd(int& blackMoves, int& whiteMoves) {
	int myCount = (currBotColor == 1 ? blackPieceCount : whitePieceCount);
	int opCount = (currBotColor == 1 ? whitePieceCount : blackPieceCount);
	int emptyCount = 49 - myCount - opCount;
	if (blackMoves > 0 && whiteMoves == 0) {
		myCount += currBotColor == 1 ? emptyCount : 0;
		opCount += currBotColor == 1 ? 0 : emptyCount;
	}
	else if (blackMoves == 0 && whiteMoves > 0) {
		myCount += currBotColor == 1 ? 0 : emptyCount;
		opCount += currBotColor == 1 ? emptyCount : 0;
	}
	if (myCount >= opCount) {
		return MAX_NUM;
	}
	else {
		return MIN_NUM;
	}

	return 0;
}



double AlphaBeta(int depth, double alpha, double beta, bool maximizingPlayer, int color, bool jumpTag) {
	Move botMoves[500];
	int moveCount = GenerateMoves(botMoves, color);
	int blackMoves = GenerateMoves(1);
	int whiteMoves = GenerateMoves(-1);
	if (depth == 0 || isGameOver(blackMoves, whiteMoves))
		return Evaluate(jumpTag);

	if (maximizingPlayer) {
		double value = MIN_NUM;
		for (int i = 0; i < moveCount; i++) {
			Move& m = botMoves[i];
			int backup[7][7];
			memcpy(backup, gridInfo, sizeof(gridInfo));
			int b = blackPieceCount, w = whitePieceCount;
			ProcStep(m.x0, m.y0, m.x1, m.y1, color);
			value = max(value, AlphaBeta(depth - 1, alpha, beta, false, -color, jumpTag));
			memcpy(gridInfo, backup, sizeof(gridInfo));
			blackPieceCount = b; whitePieceCount = w;
			alpha = max(alpha, value);
			if (beta <= alpha || timeExceeded()) break;
		}

		return value;
	}
	else {
		double value = MAX_NUM;
		for (int i = 0; i < moveCount; i++) {
			Move& m = botMoves[i];
			int backup[7][7];
			memcpy(backup, gridInfo, sizeof(gridInfo));
			int b = blackPieceCount, w = whitePieceCount;
			ProcStep(m.x0, m.y0, m.x1, m.y1, color);
			value = min(value, AlphaBeta(depth - 1, alpha, beta, true, -color, jumpTag));
			memcpy(gridInfo, backup, sizeof(gridInfo));
			blackPieceCount = b; whitePieceCount = w;
			beta = min(beta, value);
			if (beta <= alpha || timeExceeded()) break;
		}

		return value;
	}

}

Move MCTSBestMove() {
	auto startTime = chrono::high_resolution_clock::now();  // 记录开始时间
	::startTime = startTime;  // 设置全局 startTime，供 timeExceeded 使用
	GameState rootState;
	memcpy(rootState.board, gridInfo, sizeof(gridInfo));
	rootState.blackCount = blackPieceCount;
	rootState.whiteCount = whitePieceCount;

	MCTSNode* root = new MCTSNode(rootState);

	int simulations = 0;

	while (!timeExceeded()) {  // 时间控制
		MCTSNode* node = selectPromisingNode(root);

		int currPlayer = (node->state.blackCount + node->state.whiteCount) % 2 == 0 ? 1 : -1;
		if (!node->fullyExpanded) {
			node = expandNode(node, currPlayer);
		}

		int result = simulateRandom(node->state, currPlayer);
		backPropagate(node, result, currBotColor);

		simulations++;
	}

	// 选择访问次数最多的子节点作为最佳落子
	MCTSNode* bestChild = nullptr;
	int maxVisits = -1;
	for (auto child : root->children) {
		if (child->visits > maxVisits) {
			maxVisits = child->visits;
			bestChild = child;
		}
	}

	Move bestMove = bestChild ? bestChild->move : Move{ -1, -1, -1, -1 };

	// 清理内存
	delete root;

	// 可选：输出调试信息
	// std::cerr << "Simulations: " << simulations << std::endl;

	return bestMove;
}

//提高剪枝效率
Move IterativeDeepening(int color) {
	startTime = chrono::high_resolution_clock::now();
	Move bestMove = { -1,-1,-1,-1 };		//此处存在错误，当进行一轮迭代之后，后续迭代会沿用上一次的bestScore
	double bestScore = MIN_NUM;				//导致每次比较并不是当前迭代层数内的比较，而是跨层数
	//要想办法在每次迭代前重置这一数值，并且在后一层迭代没有运行完的情况下，优先使用前一层迭代的最终结果（即使用跑完的那个)

	Move moves[500];
	int moveCount = GenerateMoves(moves, color);
	bestMove = moves[0];
	//bug范围在下方函数中(已修复，bug位于AlphaBeta（）函数中)
	//错误原因：对于每一次的深度，缺乏因深度加深导致无棋可下的局面的判定（moveCount=0）
	for (int depth = 1; depth <= MAX_DEPTH; depth++) {
		bool completed = true;
		Move currBestMove = { -1,-1,-1,-1 };
		double currBestScore = MIN_NUM;
		for (int i = 0; i < moveCount; i++) {
			bool isJump = false;;
			if (timeExceeded()) {
				completed = false;
				break;
			}
			Move& m = moves[i];
			int backup[7][7];
			memcpy(backup, gridInfo, sizeof(gridInfo));
			int bCount = blackPieceCount, wCount = whitePieceCount;

			if (!ProcStep(m.x0, m.y0, m.x1, m.y1, color, isJump))
				continue;
			double score = AlphaBeta(depth - 1, MIN_NUM, MAX_NUM, false, -color, isJump);
			memcpy(gridInfo, backup, sizeof(gridInfo));
			blackPieceCount = bCount;
			whitePieceCount = wCount;
			if (score > currBestScore) {
				currBestScore = score;
				currBestMove = m;
			}
		}
		if (completed) {
			bestScore = currBestScore;
			bestMove = currBestMove;
			bestDepth = depth;
		}
		else {
			/*
			if (!timeExceeded(900) && bestMove.x0 != -1) {
				int backup[7][7];
				memcpy(backup, gridInfo, sizeof(gridInfo));
				int bCount = blackPieceCount, wCount = whitePieceCount;

				if (ProcStep(bestMove.x0, bestMove.y0, bestMove.x1, bestMove.y1, color)) {
					int fallbackScore = AlphaBeta(depth - 1, MIN_NUM, MAX_NUM, false, -color);
					memcpy(gridInfo, backup, sizeof(gridInfo));
					blackPieceCount = bCount;
					whitePieceCount = wCount;

					if (fallbackScore > currBestScore) {
						// 补偿之后还是浅层更好
						bestMove = bestMove;
						bestScore = fallbackScore;
					}
					else {
						// 当前深度找到的 currBest 更好
						bestMove = currBestMove;
						bestScore = currBestScore;
					}
				}
			}
			else if (bestMove.x0 == -1 && GenerateMoves(color) != 0) {
				bestMove = currBestMove;
				bestScore = currBestScore;
			}
			*/

			break;
		}
	}
	Json::Value depthInfo;
	depthInfo["CurrentDepth"] = bestDepth;
	ret["debug"] = depthInfo;
	return bestMove;
}


int main()
{
	int x0, y0, x1, y1;

	// 初始化棋盘
	gridInfo[0][0] = gridInfo[6][6] = 1;  //|黑|白|
	gridInfo[6][0] = gridInfo[0][6] = -1; //|白|黑|
	// 读入JSON
	string str;
	getline(cin, str);
	Json::Reader reader;
	Json::Value input;
	reader.parse(str, input);

	// 分析自己收到的输入和自己过往的输出，并恢复状态
	int turnID = input["responses"].size();
	currBotColor = input["requests"][(Json::Value::UInt)0]["x0"].asInt() < 0 ? 1 : -1; // 第一回合收到坐标是-1, -1，说明我是黑方
	for (int i = 0; i < turnID; i++)
	{
		// 根据这些输入输出逐渐恢复状态到当前回合
		x0 = input["requests"][i]["x0"].asInt();
		y0 = input["requests"][i]["y0"].asInt();
		x1 = input["requests"][i]["x1"].asInt();
		y1 = input["requests"][i]["y1"].asInt();
		if (x1 >= 0)
			ProcStep(x0, y0, x1, y1, -currBotColor); // 模拟对方落子
		x0 = input["responses"][i]["x0"].asInt();
		y0 = input["responses"][i]["y0"].asInt();
		x1 = input["responses"][i]["x1"].asInt();
		y1 = input["responses"][i]["y1"].asInt();
		if (x1 >= 0)
			ProcStep(x0, y0, x1, y1, currBotColor); // 模拟己方落子
	}

	// 看看自己本回合输入
	x0 = input["requests"][turnID]["x0"].asInt();
	y0 = input["requests"][turnID]["y0"].asInt();
	x1 = input["requests"][turnID]["x1"].asInt();
	y1 = input["requests"][turnID]["y1"].asInt();
	if (x1 >= 0)
		ProcStep(x0, y0, x1, y1, -currBotColor); // 模拟对方落子


	// 做出决策（你只需修改以下部分）

	Move botMove = IterativeDeepening(currBotColor);
	startX = botMove.x0;
	startY = botMove.y0;
	resultX = botMove.x1;
	resultY = botMove.y1;

	// 决策结束，输出结果（你只需修改以上部分）
	ret["response"]["x0"] = startX;
	ret["response"]["y0"] = startY;
	ret["response"]["x1"] = resultX;
	ret["response"]["y1"] = resultY;
	Json::FastWriter writer;
	cout << writer.write(ret) << endl;
	return 0;
}