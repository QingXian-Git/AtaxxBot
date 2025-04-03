// ͬ���壨Ataxx�����׽�����������
// �������
// ���ߣ�zhouhy zys
// ��Ϸ��Ϣ��http://www.botzone.org/games#Ataxx


#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

int currBotColor; // ����ִ����ɫ��1Ϊ�ڣ�-1Ϊ�ף�����״̬��ͬ��
int gridInfo[7][7] = { 0 }; // ��x��y����¼����״̬
int blackPieceCount = 2, whitePieceCount = 2;
int startX, startY, resultX, resultY;
static int delta[24][2] = { { 1,1 },{ 0,1 },{ -1,1 },{ -1,0 },
{ -1,-1 },{ 0,-1 },{ 1,-1 },{ 1,0 },
{ 2,0 },{ 2,1 },{ 2,2 },{ 1,2 },
{ 0,2 },{ -1,2 },{ -2,2 },{ -2,1 },
{ -2,0 },{ -2,-1 },{ -2,-2 },{ -1,-2 },
{ 0,-2 },{ 1,-2 },{ 2,-2 },{ 2,-1 } };

// �ж��Ƿ��ڵ�ͼ��
inline bool inMap(int x, int y)
{
	if (x < 0 || x > 6 || y < 0 || y > 6)
		return false;
	return true;
}

// ��Direction����Ķ����꣬�������Ƿ�Խ��
inline bool MoveStep(int& x, int& y, int Direction)
{
	x = x + delta[Direction][0];
	y = y + delta[Direction][1];
	return inMap(x, y);
}

//(������)���������ǰ����״̬
void printBoard() {
	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 7; j++) {
			cout << gridInfo[i][j] << "\t";
		}
		cout << endl;
		cout << endl;
	}
}

// �����괦���ӣ�����Ƿ�Ϸ���ģ������
bool ProcStep(int x0, int y0, int x1, int y1, int color)
{
	if (color == 0)
		return false;
	if (x1 == -1) // ��·���ߣ������˻غ�
		return true;
	if (!inMap(x0, y0) || !inMap(x1, y1)) // �����߽�
		return false;
	if (gridInfo[x0][y0] != color)
		return false;
	int dx, dy, x, y, currCount = 0, dir;
	int effectivePoints[8][2];
	dx = abs((x0 - x1)), dy = abs((y0 - y1));
	if ((dx == 0 && dy == 0) || dx > 2 || dy > 2) // ��֤�����ƶ���ԭ��λ�ã������ƶ�ʼ����5��5������
		return false;
	if (gridInfo[x1][y1] != 0) // ��֤�ƶ�����λ��Ϊ��
		return false;
	if (dx == 2 || dy == 2) // ����ߵ���5��5����Χ�����Ǹ���ճ��
		gridInfo[x0][y0] = 0;
	else
	{
		if (color == 1)
			blackPieceCount++;
		else
			whitePieceCount++;
	}

	gridInfo[x1][y1] = color;
	for (dir = 0; dir < 8; dir++) // Ӱ���ڽ�8��λ��
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

int main()
{
	istream::sync_with_stdio(false);

	int x0, y0, x1, y1;

	// ��ʼ������
	gridInfo[0][0] = gridInfo[6][6] = 1;  //|��|��|
	gridInfo[6][0] = gridInfo[0][6] = -1; //|��|��|

	// �����Լ��յ���������Լ���������������ָ�״̬
	int turnID;
	currBotColor = -1; // ��һ�غ��յ�������-1, -1��˵�����Ǻڷ�
	cin >> turnID;
	for (int i = 0; i < turnID - 1; i++)
	{
		// ������Щ��������𽥻ָ�״̬����ǰ�غ�
		cin >> x0 >> y0 >> x1 >> y1;
		if (x1 >= 0)
			ProcStep(x0, y0, x1, y1, -currBotColor); // ģ��Է�����
		else
			currBotColor = 1;
		cin >> x0 >> y0 >> x1 >> y1;
		if (x1 >= 0)
			ProcStep(x0, y0, x1, y1, currBotColor); // ģ�⼺������
	}

	// �����Լ����غ�����
	cin >> x0 >> y0 >> x1 >> y1;
	if (x1 >= 0)
		ProcStep(x0, y0, x1, y1, -currBotColor); // ģ��Է�����
	else
		currBotColor = 1;

	// �ҳ��Ϸ����ӵ�
	int beginPos[1000][2], possiblePos[1000][2], posCount = 0, choice, dir;

	for (y0 = 0; y0 < 7; y0++)
		for (x0 = 0; x0 < 7; x0++)
		{
			if (gridInfo[x0][y0] != currBotColor)
				continue;
			for (dir = 0; dir < 24; dir++)
			{
				x1 = x0 + delta[dir][0];
				y1 = y0 + delta[dir][1];
				if (!inMap(x1, y1))
					continue;
				if (gridInfo[x1][y1] != 0)
					continue;
				beginPos[posCount][0] = x0;
				beginPos[posCount][1] = y0;
				possiblePos[posCount][0] = x1;
				possiblePos[posCount][1] = y1;
				posCount++;
			}
		}

	// �������ߣ���ֻ���޸����²��֣�
	int startX, startY, resultX, resultY;
	if (posCount > 0)
	{
		srand(time(0));
		choice = rand() % posCount;
		startX = beginPos[choice][0];
		startY = beginPos[choice][1];
		resultX = possiblePos[choice][0];
		resultY = possiblePos[choice][1];
	}
	else
	{
		startX = -1;
		startY = -1;
		resultX = -1;
		resultY = -1;
	}



	// ���߽���������������ֻ���޸����ϲ��֣�
	printBoard();
	cout << startX << " " << startY << " " << resultX << " " << resultY;
	return 0;
}