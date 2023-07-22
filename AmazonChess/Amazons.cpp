#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <math.h>


#define GRIDSIZE 8
#define OBSTACLE 2
#define judge_black 0
#define judge_white 1
#define grid_black 1
#define grid_white -1

using namespace std;



double start_time, current_time; double threshold = 0.975 * (double)CLOCKS_PER_SEC;

int currBotColor; // 本方所执子颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[GRIDSIZE][GRIDSIZE] = { 0 }; // 先x后y，记录棋盘状态
int dx[] = { -1,-1,-1,0,0,1,1,1 };
int dy[] = { -1,0,1,-1,1,-1,0,1 };



struct player
{
    int x;
    int y;
};
// 判断是否在棋盘内
inline bool inMap(int x, int y)
{
    if (x < 0 || x >= GRIDSIZE || y < 0 || y >= GRIDSIZE)
        return false;
    return true;
}


// 在坐标处落子，检查是否合法或模拟落子
bool ProcStep(int x0, int y0, int x1, int y1, int x2, int y2, int color, bool check_only)
{
    if ((!inMap(x0, y0)) || (!inMap(x1, y1)) || (!inMap(x2, y2)))
        return false;
    if (gridInfo[x0][y0] != color || gridInfo[x1][y1] != 0)
        return false;
    if ((gridInfo[x2][y2] != 0) && !(x2 == x0 && y2 == y0))
        return false;
    if (!check_only)
    {
        gridInfo[x0][y0] = 0;
        gridInfo[x1][y1] = color;
        gridInfo[x2][y2] = OBSTACLE;
    }
    return true;
}
//第一部分灵活度的估值
int Kingmove[GRIDSIZE][GRIDSIZE] = { 0 };//保存每个空格的灵活度
void getKingmove(int grid[GRIDSIZE][GRIDSIZE]) {
    for (int i = 0; i < GRIDSIZE; i++) {
        for (int j = 0; j < GRIDSIZE; j++) {
            if (grid[i][j] != 0) {
                if (grid[i][j] > 0)Kingmove[i][j] = -grid[i][j];//将障碍和黑白棋的位置复制到Kingmove
                //黑棋：-1；障碍：-2
                else if (grid[i][j] == -1) {
                    Kingmove[i][j] = -3;//白棋：-3 
                }
                continue;
            }
            int cnt = 0;//记录空格
            for (int k = 0; k < 8; k++) {
                int xx = i + dx[k];
                int yy = j + dy[k];
                if (grid[xx][yy] == 0)cnt++;
            }
            Kingmove[i][j] = cnt;
        }
    }
}

double Mobility(int grid[GRIDSIZE][GRIDSIZE], struct player my[4], struct player you[4])
{
    getKingmove(grid);
    //八个棋盘，分别记录
    double my_mobility = 0.0;//己方所有棋子灵活度
    double your_mobility = 0.0;//对方所有棋子灵活度
    for (int i = 0; i < 4; i++) {
        //选择己方的一个棋子
        int x = my[i].x;
        int y = my[i].y;
        for (int delta = 1; delta < GRIDSIZE; delta++) {
            for (int k = 0; k < 8; k++) {
                int xx = x + dx[k] * delta;
                int yy = y + dy[k] * delta;
                if (!inMap(xx, yy))continue;
                if (Kingmove[xx][yy] < 0) {//是障碍或者棋子
                    continue;
                }
                my_mobility += (double)Kingmove[xx][yy] / delta;
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        int x = you[i].x;
        int y = you[i].y;
        for (int delta = 1; delta < GRIDSIZE; delta++) {
            for (int k = 0; k < 8; k++) {
                int xx = x + dx[k] * delta;
                int yy = y + dy[k] * delta;
                if (!inMap(xx, yy))continue;
                if (Kingmove[xx][yy] < 0) {//是障碍或者棋子
                    continue;
                }
                your_mobility += (double)Kingmove[xx][yy] / delta;
            }
        }
    }
    return my_mobility - your_mobility;
}


//第二部分领地的估值
const int MAX = 9999;
const double deltak = 0.25;



int player1foot[4][64];
int player2foot[4][64];
int player1dist[4][64];
int player2dist[4][64];

struct queue {
    int num[520]{ 0 };
    int tail = 0;
    int head = 0;
};
void enqueue(queue& q, int v) {
    q.num[(q.tail++) % 520] = v;
}

int dequeue(queue& q) {
    int i = q.num[(q.head++) % 520];
    return i;
}

int vis[64]; queue Q;

void BFS_dist(int chessboard[8][8], int row, int col, int vis[64], int dist[64]) {
    Q.tail = 0; Q.head = 0;
    for (int i = 0; i < 64; i++) {
        vis[i] = 0;
        dist[i] = MAX;

    }
    int v = row * 8 + col;
    vis[v] = 1; dist[v] = 0;
    enqueue(Q, v);
    while (Q.head != Q.tail) {
        v = dequeue(Q);
        int x = v / 8;
        int y = v % 8;
        int xx, yy;
        for (int i = 0; i < 8; i++) {
            xx = x + dx[i];
            yy = y + dy[i];
            int v0 = xx * 8 + yy;
            if (inMap(xx, yy) && chessboard[xx][yy] == 0 && vis[v0] == 0) {
                dist[v0] = dist[v] + 1;
                vis[v0] = 1;
                enqueue(Q, v0);

            }
        }

    }
    dist[row * 8 + col] = MAX;
}



void BFS_foot(int chessboard[8][8], int row, int col, int vis[64], int foot[64]) {
    Q.tail = 0; Q.head = 0;
    for (int i = 0; i < 64; i++) {
        vis[i] = 0;

        foot[i] = MAX;


    }
    int v = row * 8 + col;
    vis[v] = 1;  foot[v] = 0;
    enqueue(Q, v);
    while (Q.head != Q.tail) {
        v = dequeue(Q);
        int x = v / 8;
        int y = v % 8;
        int xx, yy;
        for (int i = 0; i < 8; i++) {


            for (int p = 1;; p++) {
                xx = x + dx[i] * p;
                yy = y + dy[i] * p;
                int v0 = xx * 8 + yy;
                if (!inMap(xx, yy))break;
                if (chessboard[xx][yy] != 0)break;
                if (vis[v0] == 0) {
                    vis[v0] = 1;
                    foot[v0] = foot[v] + 1;

                    enqueue(Q, v0);
                }

            }

        }

    }
    foot[row * 8 + col] = MAX;
}


//将想要生成的局面传进来

int player1d1[64];
int player1d2[64];
int player2d1[64];
int player2d2[64];




void c1t1w(float& c1, float& t1, float& w, float& c2, float& t2) {

    c1 = 0; t1 = 0; w = 0;
    int foot1, foot2;

    c2 = 0; t2 = 0;
    int dist1, dist2;

    for (int i = 0; i < 64; i++) {
        foot1 = std::min(player1foot[0][i], std::min(player1foot[1][i], std::min(player1foot[2][i], player1foot[3][i])));
        foot2 = std::min(player2foot[0][i], std::min(player2foot[1][i], std::min(player2foot[2][i], player2foot[3][i])));
        c1 = c1 + pow(0.5, foot1) - pow(0.5, foot2);

        if (foot1 == foot2 && foot1 == MAX);
        else if (foot1 == foot2 && foot2 < MAX)t1 = t1 + deltak;
        else if (foot1 < foot2)t1 = t1 + 1;
        else if (foot1 > foot2)t1 = t1 - 1;

        if (foot1 < MAX && foot2 < MAX) {
            int k = foot1 - foot2;
            if (k < 0)k = -k;
            w = w + pow(0.5, k);
        }


        dist1 = std::min(player1dist[0][i], std::min(player1dist[1][i], std::min(player1dist[2][i], player1dist[3][i])));
        dist2 = std::min(player2dist[0][i], std::min(player2dist[1][i], std::min(player2dist[2][i], player2dist[3][i])));
        float max = (dist2 - dist1) / 6, min = 1;

        if (-1 > max)max = -1;
        if (min > max)min = max;
        c2 = c2 + min;


        if (dist1 == MAX && dist2 == MAX)continue;
        else if (dist1 == dist2 && dist2 < MAX)t2 = t2 + deltak;
        else if (dist1 < dist2)t2 = t2 + 1;
        else if (dist1 > dist2)t2 = t2 - 1;






    }
    c1 = 2 * c1;
}




//将想要生成的局面传进来
float t(int chessboard[8][8], struct player my[4], struct player you[4], int turnID) {
    float t = 0;
    for (int i = 0; i < 4; i++) {

        BFS_dist(chessboard, my[i].x, my[i].y, vis, player1dist[i]);
        BFS_dist(chessboard, you[i].x, you[i].y, vis, player2dist[i]);
        BFS_foot(chessboard, my[i].x, my[i].y, vis, player1foot[i]);
        BFS_foot(chessboard, you[i].x, you[i].y, vis, player2foot[i]);
    }


    float c1 = 0, t1 = 0, w = 0, c2 = 0, t2 = 0;
    c1t1w(c1, t1, w, c2, t2);

    if (turnID <= 10) {
        t = 0.14 * t1 + 0.37 * t2 + 0.13 * c1 + 0.13 * c2;

    }
    else if (turnID <= 25) {
        t = 0.3 * t1 + 0.25 * t2 + 0.2 * c1 + 0.2 * c2;
    }
    else {
        t = 0.8 * t1 + 0.1 * t2 + 0.05 * c1 + 0.05 * c2;
    }
    return t;
}


float totalvalue(int grid[GRIDSIZE][GRIDSIZE], struct player my[4], struct player you[4], int turnID)
{
    float x = 0, y = 0, k2 = 0;
    x = t(grid, my, you, turnID);
    y = Mobility(grid, my, you);

    if (turnID <= 10)k2 = 0.2;
    else if (turnID <= 25)k2 = 0.05;
    else k2 = 0;
    return x + k2 * y;
}






//总的估值函数


int chessboard[8][8];
int deepest = 1;
const float INF = 0x3f3f3f;

int sx[8] = { 1,0,1,1,0,-1,-1,-1 };
int sy[8] = { 1,1,-1,0,-1,1,0,-1 };
struct player aftermy[4];
struct player afteryou[4];

int ansx1, ansx2, ansx3, ansy1, ansy2, ansy3;




int time_flag = 0;

float alpha_beta(int h, float alpha, float beta, int turnID)      //h搜索深度
{


    current_time = clock();

    if (time_flag == 1 || current_time - start_time >= threshold) {
        time_flag = 1;
        return totalvalue(chessboard, aftermy, afteryou, turnID);
    }


    if (h == deepest) {

        return totalvalue(chessboard, aftermy, afteryou, turnID);

    }


    if (h % 2 == 0) {//自己


        for (int i = 0; i < 4; i++) {
            int x = aftermy[i].x, y = aftermy[i].y;
            for (int j = 0; j <= 7; j++) {

                for (int k = 1;; k++) {

                    int xx = x + sx[j] * k, yy = y + sy[j] * k;
                    if (!inMap(xx, yy)) {
                        break;
                    }
                    if (chessboard[xx][yy])break;

                    chessboard[x][y] = 0;
                    aftermy[i].x = xx; aftermy[i].y = yy;
                    chessboard[xx][yy] = currBotColor;


                    for (int q = 0; q <= 7; q++) {
                        for (int t = 1;; t++) {

                            int bx = xx + sx[q] * t, by = yy + sy[q] * t;
                            if (!inMap(bx, by)) {
                                break;
                            }
                            if (chessboard[bx][by])break;

                            chessboard[bx][by] = 2;


                            float ans = alpha_beta(h + 1, alpha, beta, turnID);
                            chessboard[bx][by] = 0;
                            if (h != 0)alpha = std::max(ans, alpha);
                            else if (ans > alpha) {    //通过向上传递的子节点beta值修正alpha值 
                                alpha = ans;
                                ansx1 = x;
                                ansy1 = y;
                                ansx2 = xx;
                                ansy2 = yy;
                                ansx3 = bx;
                                ansy3 = by;    //记录位置 


                            }

                            if (time_flag == 1) {

                                chessboard[x][y] = currBotColor;
                                aftermy[i].x = x; aftermy[i].y = y;
                                chessboard[xx][yy] = 0;



                                return alpha;


                            }



                            if (alpha >= beta)   //发生 beta剪枝 
                            {
                                chessboard[x][y] = currBotColor;
                                aftermy[i].x = x; aftermy[i].y = y;
                                chessboard[xx][yy] = 0;



                                return beta;
                            }



                        }


                    }


                    chessboard[x][y] = currBotColor;
                    aftermy[i].x = x; aftermy[i].y = y;
                    chessboard[xx][yy] = 0;



                }
            }
        }
        if (h != 0 && alpha == -INF)return totalvalue(chessboard, aftermy, afteryou, turnID);
        return alpha;

    }

    else {


        for (int i = 0; i < 4; i++) {
            int x = afteryou[i].x, y = afteryou[i].y;
            for (int j = 0; j <= 7; j++) {

                for (int k = 1;; k++) {

                    int xx = x + sx[j] * k, yy = y + sy[j] * k;
                    if (!inMap(xx, yy)) {
                        break;
                    }
                    if (chessboard[xx][yy])break;

                    chessboard[x][y] = 0;
                    afteryou[i].x = xx; afteryou[i].y = yy;
                    chessboard[xx][yy] = -currBotColor;


                    for (int q = 0; q <= 7; q++) {
                        for (int t = 1;; t++) {

                            int bx = xx + sx[q] * t, by = yy + sy[q] * t;
                            if (!inMap(bx, by)) {
                                break;
                            }
                            if (chessboard[bx][by])break;
                            chessboard[bx][by] = 2;
                            float ans = alpha_beta(h + 1, alpha, beta, turnID + 1);
                            chessboard[bx][by] = 0;
                            if (ans < beta) {     //通过向上传递的子节点alpha值修正beta值 
                                beta = ans;
                            }

                            if (time_flag == 1) {

                                chessboard[x][y] = currBotColor;
                                aftermy[i].x = x; aftermy[i].y = y;
                                chessboard[xx][yy] = 0;



                                return beta;


                            }





                            if (alpha >= beta)   //发生 alpha剪枝
                            {
                                chessboard[x][y] = -currBotColor;
                                afteryou[i].x = x; afteryou[i].y = y;
                                chessboard[xx][yy] = 0;
                                return alpha;
                            }

                        }


                    }
                    chessboard[x][y] = -currBotColor;
                    afteryou[i].x = x; afteryou[i].y = y;
                    chessboard[xx][yy] = 0;



                }
            }
        }
        if (beta == INF)return totalvalue(chessboard, aftermy, afteryou, turnID);
        return beta;


    }

}



int main()
{
    int x0, y0, x1, y1, x2, y2;
    for (int i = 0; i < GRIDSIZE; i++)
        for (int j = 0; j < GRIDSIZE; j++)
            gridInfo[i][j] = 0;

    // 初始化棋盘
    gridInfo[0][2] = grid_black;
    gridInfo[2][0] = grid_black;
    gridInfo[5][0] = grid_black;
    gridInfo[7][2] = grid_black;

    gridInfo[0][5] = grid_white;
    gridInfo[2][7] = grid_white;
    gridInfo[5][7] = grid_white;
    gridInfo[7][5] = grid_white;


    // 分析自己收到的输入和自己过往的输出，并恢复棋盘状态
    int turnID;
    cin >> turnID;

    currBotColor = grid_white; // 先假设自己是白方
    for (int i = 0; i < turnID; i++)
    {
        // 根据这些输入输出逐渐恢复状态到当前回合

        // 首先是对手行动
        cin >> x0 >> y0 >> x1 >> y1 >> x2 >> y2;
        if (x0 == -1)
            currBotColor = grid_black; // 第一回合收到坐标是-1, -1，说明我方是黑方
        else
            ProcStep(x0, y0, x1, y1, x2, y2, -currBotColor, false); // 模拟对方落子

        // 然后是本方当时的行动
        // 对手行动总比自己行动多一个
        if (i < turnID - 1)
        {
            cin >> x0 >> y0 >> x1 >> y1 >> x2 >> y2;
            if (x0 >= 0)
                ProcStep(x0, y0, x1, y1, x2, y2, currBotColor, false); // 模拟本方落子
        }
    }



    /*********************************************************************************************************/
    /***在下面填充你的代码，决策结果（本方将落子的位置）存入startX、startY、resultX、resultY、obstacleX、obstacleY中*****/
    //下面仅为随机策略的示例代码，且效率低，可删除
    int  startX, startY, resultX, resultY, obstacleX, obstacleY;
    /*struct player
    {
        int x;
        int y;
    };*/
    struct player my[4];//表示本方
    struct player you[4];//表示敌方
    int k = 0;
    int k1 = 0;
    //  currBotColor = grid_white;
    for (int i = 0; i < GRIDSIZE; i++)
        for (int j = 0; j < GRIDSIZE; j++)
        {
            if (gridInfo[i][j] == currBotColor)
            {
                my[k].x = i;
                my[k++].y = j;
            }
            else if (gridInfo[i][j] == -currBotColor)
            {
                you[k1].x = i;
                you[k1++].y = j;
            }
        }


    /****在上方填充你的代码，决策结果（本方将落子的位置）存入startX、startY、resultX、resultY、obstacleX、obstacleY中****/


    if (turnID >= 20)deepest = 3;
    else if (turnID >= 10)deepest = 2;
    else if (turnID >= 1)deepest = 1;

    start_time = clock();

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            chessboard[i][j] = gridInfo[i][j];
        }
    }
    for (int i = 0; i < 4; i++) {
        aftermy[i] = my[i];
    }

    for (int i = 0; i < 4; i++) {
        afteryou[i] = you[i];
    }


    alpha_beta(0, -INF, INF, turnID);

    startX = ansx1; startY = ansy1; resultX = ansx2; resultY = ansy2; obstacleX = ansx3; obstacleY = ansy3;

    /*********************************************************************************************************/



    cout << startX << ' ' << startY << ' ' << resultX << ' ' << resultY << ' ' << obstacleX << ' ' << obstacleY << endl;


    return 0;
}