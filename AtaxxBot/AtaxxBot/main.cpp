#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
using namespace std;

int gridInfo[7][7] = { 0 };
int blackPieceCount = 2, whitePieceCount = 2;
int currBotColor = 1;
bool isBlackBot = false;
bool isWhiteBot = false;

static int delta[24][2] = {
    { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
    { 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
    { -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 }
};

inline bool inMap(int x, int y) {
    return x >= 0 && x < 7 && y >= 0 && y < 7;
}

bool ProcStep(int x0, int y0, int x1, int y1, int color) {
    if (color == 0) return false;
    if (x1 == -1) return true;
    if (!inMap(x0, y0) || !inMap(x1, y1)) return false;
    if (gridInfo[x0][y0] != color) return false;

    int dx = abs(x0 - x1), dy = abs(y0 - y1);
    if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) return false;
    if (gridInfo[x1][y1] != 0) return false;

    if (dx == 2 || dy == 2)
        gridInfo[x0][y0] = 0;
    else {
        if (color == 1) blackPieceCount++;
        else whitePieceCount++;
    }

    gridInfo[x1][y1] = color;

    int currCount = 0;
    int effectivePoints[8][2];
    for (int dir = 0; dir < 8; dir++) {
        int x = x1 + delta[dir][0];
        int y = y1 + delta[dir][1];
        if (!inMap(x, y)) continue;
        if (gridInfo[x][y] == -color) {
            effectivePoints[currCount][0] = x;
            effectivePoints[currCount][1] = y;
            currCount++;
            gridInfo[x][y] = color;
        }
    }
    if (currCount > 0) {
        if (color == 1) {
            blackPieceCount += currCount;
            whitePieceCount -= currCount;
        }
        else {
            whitePieceCount += currCount;
            blackPieceCount -= currCount;
        }
    }
    return true;
}

void PrintBoard() {
    cout << "\n当前棋盘状态：\n";
    cout << "  0 1 2 3 4 5 6\n";
    for (int y = 0; y < 7; y++) {
        cout << y << " ";
        for (int x = 0; x < 7; x++) {
            if (gridInfo[x][y] == 1) cout << "○ ";
            else if (gridInfo[x][y] == -1) cout << "● ";
            else cout << "· ";
        }
        cout << "\n";
    }
    cout << "黑子数(○): " << blackPieceCount << "  白子数(●): " << whitePieceCount << "\n\n";
}

void GameOver() {
    cout << "\n===== 对局结束 =====\n";
    PrintBoard();
    exit(0);
}

bool BotMove(int color, int& x0, int& y0, int& x1, int& y1) {
    // 简单随机合法落子
    for (int tx = 0; tx < 7; tx++) {
        for (int ty = 0; ty < 7; ty++) {
            if (gridInfo[tx][ty] != color) continue;
            for (int d = 0; d < 24; d++) {
                int nx = tx + delta[d][0];
                int ny = ty + delta[d][1];
                if (inMap(nx, ny) && gridInfo[nx][ny] == 0) {
                    x0 = tx; y0 = ty;
                    x1 = nx; y1 = ny;
                    return true;
                }
            }
        }
    }
    return false;
}

bool PlayerMove(int color) {
    string inputLine;
    string sideStr = (color == 1 ? "黑方(○)" : "白方(●)");
    cout << sideStr << " 请输入落子坐标 (x0 y0 x1 y1)，跳过输入 -1 -1 -1 -1，退出输入 q：";
    getline(cin, inputLine);

    if (inputLine == "q" || inputLine == "Q") {
        cout << "玩家主动退出。\n";
        GameOver();
    }

    int x0, y0, x1, y1;
    sscanf(inputLine.c_str(), "%d %d %d %d", &x0, &y0, &x1, &y1);

    if (x0 == -1 && y0 == -1 && x1 == -1 && y1 == -1) {
        cout << sideStr << " 选择跳过本回合。\n";
        return false;
    }

    if (!ProcStep(x0, y0, x1, y1, color)) {
        cout << "无效落子，请重新输入。\n";
        return PlayerMove(color); // 递归重新输入
    }

    return true;
}

int main() {
    srand(time(0));
    int mode;
    cout << "请选择模式 (0: 复盘操作, 1: 逐步输入): ";
    cin >> mode;

    gridInfo[0][6] = gridInfo[6][0] = -1;   // 黑白
    gridInfo[0][0] = gridInfo[6][6] = 1;    // 白黑

    PrintBoard();

    if (mode == 0) {
        int turnID;
        cout << "请输入复盘步数（turnID）: ";
        cin >> turnID;

        for (int i = 0; i < turnID; i++) {
            int x0, y0, x1, y1;
            cout << "请输入第 " << i + 1 << " 回合对方落子 (x0 y0 x1 y1): ";
            cin >> x0 >> y0 >> x1 >> y1;
            ProcStep(x0, y0, x1, y1, -currBotColor);
            PrintBoard();

            cout << "请输入我方落子 (x0 y0 x1 y1): ";
            cin >> x0 >> y0 >> x1 >> y1;
            ProcStep(x0, y0, x1, y1, currBotColor);
            PrintBoard();
        }
    }
    else {
        cout << "是否将黑方设置为 Bot？(1: 是, 0: 否): ";
        cin >> isBlackBot;
        cout << "是否将白方设置为 Bot？(1: 是, 0: 否): ";
        cin >> isWhiteBot;
        cin.ignore(); // 清除输入缓冲

        int turn = 0;
        int passCount = 0;

        while (true) {
            turn++;
            string sideStr = (currBotColor == 1 ? "黑方(○)" : "白方(●)");
            cout << "\n[第 " << turn << " 回合] 当前轮到 " << sideStr << " 落子\n";

            bool moved = false;
            if ((currBotColor == 1 && isBlackBot) || (currBotColor == -1 && isWhiteBot)) {
                int x0, y0, x1, y1;
                if (BotMove(currBotColor, x0, y0, x1, y1)) {
                    cout << sideStr << "（Bot）落子：" << x0 << " " << y0 << " -> " << x1 << " " << y1 << "\n";
                    ProcStep(x0, y0, x1, y1, currBotColor);
                    moved = true;
                }
                else {
                    cout << sideStr << "（Bot）跳过本回合。\n";
                }
            }
            else {
                moved = PlayerMove(currBotColor);
            }

            if (moved) {
                passCount = 0;
            }
            else {
                passCount++;
                if (passCount >= 2) {
                    cout << "双方连续跳过，对局结束。\n";
                    GameOver();
                }
            }

            PrintBoard();
            currBotColor *= -1;
        }
    }

    return 0;
}
