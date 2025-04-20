#include "AlphaBeta.h"

extern int gridInfo[7][7]; // 共享棋盘
extern int currBotColor;   // 机器人执子颜色
extern int blackPieceCount, whitePieceCount; // 统计棋子数量

// 棋盘方向增量（8 方向 + 远跳 16 方向）
static int delta[24][2] = {
    { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
    { 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
    { -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 }
};

// 判断坐标是否在棋盘内
bool inMap(int x, int y) {
    return x >= 0 && x < 7 && y >= 0 && y < 7;
}

// 模拟执行落子并计算翻转棋子数
int SimulateMove(int x0, int y0, int x1, int y1, int color) {
    int flipped = 0;
    gridInfo[x1][y1] = color; // 放置棋子
    if (abs(x1 - x0) <= 1 && abs(y1 - y0) <= 1) {
        // 复制（增长）
    }
    else {
        // 跳跃（移动）
        gridInfo[x0][y0] = 0;
    }

    // 影响周围 8 个方向
    for (int d = 0; d < 8; d++) {
        int nx = x1 + delta[d][0];
        int ny = y1 + delta[d][1];
        if (inMap(nx, ny) && gridInfo[nx][ny] == -color) {
            gridInfo[nx][ny] = color;
            flipped++;
        }
    }
    return flipped;
}

// 还原棋盘
void UndoMove(int x0, int y0, int x1, int y1, int color, int flipped) {
    gridInfo[x1][y1] = 0;
    if (abs(x1 - x0) > 1 || abs(y1 - y0) > 1) {
        gridInfo[x0][y0] = color; // 跳跃回原位
    }
    for (int d = 0; d < 8 && flipped > 0; d++) {
        int nx = x1 + delta[d][0];
        int ny = y1 + delta[d][1];
        if (inMap(nx, ny) && gridInfo[nx][ny] == color) {
            gridInfo[nx][ny] = -color;
            flipped--;
        }
    }
}

// 评估函数
int Evaluate() {
    int pieceScore = blackPieceCount - whitePieceCount;
    int mobilityScore = 0, flippingScore = 0, emptySpaces = 0;

    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 7; x++) {
            if (gridInfo[x][y] == 0) {
                emptySpaces++;
                continue;
            }

            if (gridInfo[x][y] == currBotColor) {
                for (int d = 0; d < 24; d++) {
                    int nx = x + delta[d][0], ny = y + delta[d][1];
                    if (inMap(nx, ny) && gridInfo[nx][ny] == 0)
                        mobilityScore++;
                }
            }
        }
    }

    return (pieceScore * 5) + (mobilityScore * 3) + (flippingScore * 2) - (emptySpaces * 1);
}

// AlphaBeta 剪枝
int AlphaBeta(int depth, int alpha, int beta, bool isMaximizingPlayer) {
    if (depth == 0) return Evaluate();

    int bestValue = isMaximizingPlayer ? -INF : INF;

    for (int i = 0; i < posCount; i++) {
        int x0 = beginPos[i][0], y0 = beginPos[i][1];
        int x1 = possiblePos[i][0], y1 = possiblePos[i][1];

        int flipped = SimulateMove(x0, y0, x1, y1, isMaximizingPlayer ? currBotColor : -currBotColor);
        int value = AlphaBeta(depth - 1, alpha, beta, !isMaximizingPlayer);
        UndoMove(x0, y0, x1, y1, isMaximizingPlayer ? currBotColor : -currBotColor, flipped);

        if (isMaximizingPlayer) {
            bestValue = max(bestValue, value);
            alpha = max(alpha, bestValue);
        }
        else {
            bestValue = min(bestValue, value);
            beta = min(beta, bestValue);
        }

        if (beta <= alpha) break;
    }

    return bestValue;
}
