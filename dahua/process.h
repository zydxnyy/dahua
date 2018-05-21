#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <time.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <algorithm>
using namespace std;
typedef unsigned char BYTE;
#define conv(r, c) ((r)*width+c)
#define convs(r, c) ((r)*swidth+c)
#define cw(i) (i>=xx1 && i<xx2)
#define ch(i) (i>=yy1 && i<yy2)
#define dist(p1, p2) abs(p1-p2)
#define random(n) (rand() % n)
#define random_1 ((double)rand() / (double)RAND_MAX)


//#define proga_fg
//#define _bg_flush

//定义动检成立时是否将颜色反转 
#define reverse_bg
//多帧背景建模
//#define multi_bg_build
//定义是否将结果视频输出，输出路径为当前目录的result.yuv
#define output_yuv
//是否将满足动检的单元格上色，满足的单元格将会变成灰色 
//#define paint_cell
//是否进行鬼影消除 
//#define ghost_clear 

void yuv_process(BYTE* yuvData, BYTE resultMatrix[128][128], bool* alarmResult);

void set_resolution(int width,int height);

void set_cell_split(int row , int col);

void set_detection_region(int x1, int y1, int x2, int y2);

void set_threshold_sensity(int t, int s);

void gauss_filter(float win[3][3], BYTE* yuvData);

void generateGaussianTemplate(float win[][3], int ksize, float sigma);

void write_yuv(BYTE* yuvData, string name);

void set_background(BYTE* yuvData);

void predict(BYTE* yuvData, BYTE resultMatrix[128][128], int bg_flush);

void corrode(BYTE* yuvData);

void swell(BYTE* yuvData);

void check(BYTE* yuvData, BYTE resultMatrix[128][128], bool* alarm); 

void filter();

void update_bg(BYTE* yuvData, BYTE resultMatrix[128][128], int bg_flush);

void set_background_mul(BYTE* yuvData); 

void fill_in();
