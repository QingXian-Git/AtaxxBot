#pragma once
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>

const int INF = 10000;

// AlphaBeta ¼ôÖ¦ËÑË÷
int AlphaBeta(int depth, int alpha, int beta, bool isMaximizingPlayer);

// ÆÀ¹Àº¯Êý
int Evaluate();

