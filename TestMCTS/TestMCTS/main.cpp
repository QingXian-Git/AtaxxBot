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
const int MAX_NUM = 1e9;
const int MIN_NUM = -1e9;
const int TIME_LIMIT_MS = 700;
int CORNER_WEIGHT = 0;
int STABLE_WEIGHT = 0;
int COUNT_WEIGHT = 0;
int MOBILITY_WEIGHT = 0;
int JUMP_WEIGHT = 0;
int BORDER_WEIGHT = 0;


int bestDepth = 0;
int posCount = 0;
int startX, startY, resultX, resultY;



chrono::time_point<chrono::high_resolution_clock> startTime;



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
bool ProcStep(int gridInfo[7][7], int blackPieceCount, int whitePieceCount,int x0, int y0, int x1, int y1, int color)
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

bool ProcStep(int gridInfo[7][7], int blackPieceCount, int whitePieceCount,int x0, int y0, int x1, int y1, int color, bool& jumpFlag)
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



bool isStable(int gridInfo[7][7],int x, int y) {
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

int GenerateMoves(int gridInfo[7][7], int blackPieceCount, int whitePieceCount,int color) {
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

int GenerateMoves(int gridInfo[7][7], int blackPieceCount, int whitePieceCount,Move moves[], int color) {
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

bool isGameOver(int gridInfo[7][7], int blackPieceCount, int whitePieceCount) {
	int emptyCount = 49 - blackPieceCount - whitePieceCount;
	if (emptyCount == 0)
		return true;
	if (blackPieceCount == 0 || whitePieceCount == 0)
		return true;
	int blackMoves = GenerateMoves(gridInfo, blackPieceCount, whitePieceCount,1);
	int whiteMoves = GenerateMoves(gridInfo,blackPieceCount,whitePieceCount,-1);
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
int Evaluate(int gridInfo[7][7], int blackPieceCount, int whitePieceCount,int currBotColor) {
	int score = 0;
	int stableCount = 0;
	int myCount = (currBotColor == 1 ? blackPieceCount : whitePieceCount);
	int opCount = (currBotColor == 1 ? whitePieceCount : blackPieceCount);
	int emptyCount = 49 - blackPieceCount - whitePieceCount;
	bool flag = true;

	if (emptyCount > 30) {	//开局决策权重
		CORNER_WEIGHT = 0;
		COUNT_WEIGHT = 10;
		STABLE_WEIGHT = 6;
		MOBILITY_WEIGHT = 0;
		JUMP_WEIGHT = 10;
		BORDER_WEIGHT = 0;
	}
	else if (emptyCount > 12) {	//中期决策权重
		CORNER_WEIGHT = 1;
		COUNT_WEIGHT = 10;
		STABLE_WEIGHT = 8;
		MOBILITY_WEIGHT = 0;
		JUMP_WEIGHT = 10;
		BORDER_WEIGHT = 0;
	}
	else {	//末期决策权重
		CORNER_WEIGHT = 2;
		COUNT_WEIGHT = 10;
		STABLE_WEIGHT = 5;
		MOBILITY_WEIGHT = 0;
		JUMP_WEIGHT = 10;
		BORDER_WEIGHT = 0;
	}

	if (MOBILITY_WEIGHT != 0) {
		int opMoveCount = GenerateMoves(gridInfo,blackPieceCount,whitePieceCount,-currBotColor);
		score -= opMoveCount * MOBILITY_WEIGHT;
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
					if (isStable(gridInfo,x, y)) {
						stableCount++;
					}
				}
				else if (gridInfo[x][y] == -currBotColor) {
					if (isStable(gridInfo,x, y)) {
						stableCount--;
					}
				}
			}
		}
	}


	score += (myCount - opCount) * COUNT_WEIGHT;
	score += stableCount * STABLE_WEIGHT;
	return score;
}



int AlphaBeta(int gridInfo[7][7], int blackPieceCount, int whitePieceCount, int depth, int alpha, int beta, bool maximizingPlayer, int color) {
	Move botMoves[500];
	int moveCount = GenerateMoves(gridInfo,blackPieceCount,whitePieceCount,botMoves, color);
	if (depth == 0 || isGameOver(gridInfo,blackPieceCount,whitePieceCount) || timeExceeded())
		return Evaluate(gridInfo,blackPieceCount,whitePieceCount,maximizingPlayer?color:-color);

	if (maximizingPlayer) {
		int value = MIN_NUM;
		for (int i = 0; i < moveCount; i++) {
			Move& m = botMoves[i];
			int backup[7][7];
			memcpy(backup, gridInfo, sizeof(gridInfo));
			int b = blackPieceCount, w = whitePieceCount;
			ProcStep(gridInfo,blackPieceCount,whitePieceCount,m.x0, m.y0, m.x1, m.y1, color);
			value = max(value, AlphaBeta(gridInfo,blackPieceCount,whitePieceCount,depth - 1, alpha, beta, false, -color));
			memcpy(gridInfo, backup, sizeof(gridInfo));
			blackPieceCount = b; whitePieceCount = w;
			alpha = max(alpha, value);
			if (beta <= alpha || timeExceeded()) break;
		}

		return value;
	}
	else {
		int value = MAX_NUM;
		for (int i = 0; i < moveCount; i++) {
			Move& m = botMoves[i];
			int backup[7][7];
			memcpy(backup, gridInfo, sizeof(gridInfo));
			int b = blackPieceCount, w = whitePieceCount;
			ProcStep(gridInfo,blackPieceCount,whitePieceCount,m.x0, m.y0, m.x1, m.y1, color);
			value = min(value, AlphaBeta(gridInfo,blackPieceCount,whitePieceCount,depth - 1, alpha, beta, true, -color));
			memcpy(gridInfo, backup, sizeof(gridInfo));
			blackPieceCount = b; whitePieceCount = w;
			beta = min(beta, value);
			if (beta <= alpha || timeExceeded()) break;
		}

		return value;
	}

}

Move MCTSBestMove(int gridInfo[7][7], int blackPieceCount,int whitePieceCount,int currBotColor) {
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
Move IterativeDeepening(int gridInfo[7][7],int blackPieceCount,int whitePieceCount, int color) {
	startTime = chrono::high_resolution_clock::now();
	Move bestMove = { -1,-1,-1,-1 };		//此处存在错误，当进行一轮迭代之后，后续迭代会沿用上一次的bestScore
	int bestScore = MIN_NUM;				//导致每次比较并不是当前迭代层数内的比较，而是跨层数
	//要想办法在每次迭代前重置这一数值，并且在后一层迭代没有运行完的情况下，优先使用前一层迭代的最终结果（即使用跑完的那个)

	Move moves[500];
	int moveCount = GenerateMoves(gridInfo,blackPieceCount,whitePieceCount,moves, color);
	//bug范围在下方函数中(已修复，bug位于AlphaBeta（）函数中)
	//错误原因：对于每一次的深度，缺乏因深度加深导致无棋可下的局面的判定（moveCount=0）
	for (int depth = 1; depth <= MAX_DEPTH; depth++) {
		bool isJump = false;
		bool completed = true;
		Move currBestMove = { -1,-1,-1,-1 };
		int currBestScore = MIN_NUM;
		for (int i = 0; i < moveCount; i++) {
			if (timeExceeded()) {
				completed = false;
				break;
			}
			Move& m = moves[i];
			int backup[7][7];
			memcpy(backup, gridInfo, sizeof(gridInfo));
			int bCount = blackPieceCount, wCount = whitePieceCount;

			if (!ProcStep(gridInfo,blackPieceCount,whitePieceCount,m.x0, m.y0, m.x1, m.y1, color, isJump))
				continue;
			int score = AlphaBeta(gridInfo,blackPieceCount,whitePieceCount,depth - 1, MIN_NUM, MAX_NUM, false, -color);
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
	return bestMove;
}


int main()
{
	int x0, y0, x1, y1;

	int currBotColor; // 我所执子颜色（1为黑，-1为白，棋盘状态亦同）
	int gridInfo[7][7] = { 0 }; // 先x后y，记录棋盘状态
	int blackPieceCount = 2, whitePieceCount = 2;
	// 初始化棋盘
	gridInfo[0][0] = gridInfo[6][6] = 1;  //|黑|白|
	gridInfo[6][0] = gridInfo[0][6] = -1; //|白|黑|
	// 读入JSON
	string str;
	Json::Reader reader;
	Json::Value input;
	Json::FastWriter writer;
	Json::Value ret;
	reader.parse(str, input);
	bool firstTurn = true;
	currBotColor = -1;
	// 分析自己收到的输入和自己过往的输出，并恢复状态
	while (cin >> str) {
		reader.parse(str, input);
		if (firstTurn) {
			x0 = input["requests"][(Json::Value::UInt)0]["x0"].asInt();
			x1 = input["requests"][(Json::Value::UInt)0]["x1"].asInt();
			y0 = input["requests"][(Json::Value::UInt)0]["y0"].asInt();
			y1 = input["requests"][(Json::Value::UInt)0]["y1"].asInt();
		}
		else {
			x0 = input["x0"].asInt();
			x1 = input["x1"].asInt();
			y0 = input["y0"].asInt();
			y1 = input["y1"].asInt();
		}
		if (firstTurn && x0 == -1) {
			currBotColor = 1;
		}
		if (x0 != -1 && x1 != -1 && y0 != -1 && y1 != -1) {
			ProcStep(gridInfo,blackPieceCount,whitePieceCount,x0, y0, x1, y1, -currBotColor);
		}

		Move botMove = IterativeDeepening(gridInfo,blackPieceCount,whitePieceCount,currBotColor);
		startX = botMove.x0;
		startY = botMove.y0;
		resultX = botMove.x1;
		resultY = botMove.y1;

		firstTurn = false;

		ret["response"]["x0"] = startX;
		ret["response"]["y0"] = startY;
		ret["response"]["x1"] = resultX;
		ret["response"]["y1"] = resultY;

		cout << writer.write(ret) << endl;
		cout << "\n>>>BOTZONE_REQUEST_KEEP_RUNNING<<<\n";
		fflush(stdout);
		
	}
	return 0;
}