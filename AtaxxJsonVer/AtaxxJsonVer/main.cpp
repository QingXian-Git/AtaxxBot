#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "jsoncpp/json.h" // C++编译时默认包含此库

using namespace std;

const int MAX_DEPTH = 6; // 设定搜索深度
int currBotColor;
int gridInfo[7][7] = { 0 };
int blackPieceCount = 2, whitePieceCount = 2;
static int delta[24][2] = { { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
                            { 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
                            { -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 } };

inline bool inMap(int x, int y) {
    return x >= 0 && x < 7 && y >= 0 && y < 7;
}

bool ProcStep(int x0, int y0, int x1, int y1, int color) {
    if (!inMap(x0, y0) || !inMap(x1, y1) || gridInfo[x0][y0] != color || gridInfo[x1][y1] != 0) return false;
    int dx = abs(x0 - x1), dy = abs(y0 - y1);
    if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) return false;
    if (dx == 2 || dy == 2) gridInfo[x0][y0] = 0;
    gridInfo[x1][y1] = color;
    for (int dir = 0; dir < 8; dir++) {
        int x = x1 + delta[dir][0], y = y1 + delta[dir][1];
        if (inMap(x, y) && gridInfo[x][y] == -color) gridInfo[x][y] = color;
    }
    return true;
}

int Evaluate() {
    return (currBotColor == 1) ? blackPieceCount - whitePieceCount : whitePieceCount - blackPieceCount;
}

int AlphaBeta(int depth, int alpha, int beta, bool maximizing) {
    if (depth == 0) return Evaluate();
    int bestValue = maximizing ? -1000 : 1000;
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 7; x++) {
            if (gridInfo[x][y] != (maximizing ? currBotColor : -currBotColor)) continue;
            for (int d = 0; d < 24; d++) {
                int nx = x + delta[d][0], ny = y + delta[d][1];
                if (!inMap(nx, ny) || gridInfo[nx][ny] != 0) continue;
                int prevGrid[7][7];
                memcpy(prevGrid, gridInfo, sizeof(gridInfo));
                ProcStep(x, y, nx, ny, maximizing ? currBotColor : -currBotColor);
                int value = AlphaBeta(depth - 1, alpha, beta, !maximizing);
                memcpy(gridInfo, prevGrid, sizeof(gridInfo));
                if (maximizing) {
                    bestValue = max(bestValue, value);
                    alpha = max(alpha, bestValue);
                }
                else {
                    bestValue = min(bestValue, value);
                    beta = min(beta, bestValue);
                }
                if (beta <= alpha) return bestValue;
            }
        }
    }
    return bestValue;
}

void FindBestMove(int& startX, int& startY, int& resultX, int& resultY) {
    int bestValue = -1000;
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 7; x++) {
            if (gridInfo[x][y] != currBotColor) continue;
            for (int d = 0; d < 24; d++) {
                int nx = x + delta[d][0], ny = y + delta[d][1];
                if (!inMap(nx, ny) || gridInfo[nx][ny] != 0) continue;
                int prevGrid[7][7];
                memcpy(prevGrid, gridInfo, sizeof(gridInfo));
                ProcStep(x, y, nx, ny, currBotColor);
                int value = AlphaBeta(MAX_DEPTH - 1, -1000, 1000, false);
                memcpy(gridInfo, prevGrid, sizeof(gridInfo));
                if (value > bestValue) {
                    bestValue = value;
                    startX = x;
                    startY = y;
                    resultX = nx;
                    resultY = ny;
                }
            }
        }
    }
}

int main() {
    string str;
    getline(cin, str);
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);
    int turnID = input["responses"].size();
    currBotColor = input["requests"][(Json::Value::UInt)0]["x0"].asInt() < 0 ? 1 : -1;
    for (int i = 0; i < turnID; i++) {
        int x0 = input["requests"][i]["x0"].asInt();
        int y0 = input["requests"][i]["y0"].asInt();
        int x1 = input["requests"][i]["x1"].asInt();
        int y1 = input["requests"][i]["y1"].asInt();
        if (x1 >= 0) ProcStep(x0, y0, x1, y1, -currBotColor);
        x0 = input["responses"][i]["x0"].asInt();
        y0 = input["responses"][i]["y0"].asInt();
        x1 = input["responses"][i]["x1"].asInt();
        y1 = input["responses"][i]["y1"].asInt();
        if (x1 >= 0) ProcStep(x0, y0, x1, y1, currBotColor);
    }
    int startX = -1, startY = -1, resultX = -1, resultY = -1;
    FindBestMove(startX, startY, resultX, resultY);
    Json::Value ret;
    ret["response"]["x0"] = startX;
    ret["response"]["y0"] = startY;
    ret["response"]["x1"] = resultX;
    ret["response"]["y1"] = resultY;
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
    return 0;
}
