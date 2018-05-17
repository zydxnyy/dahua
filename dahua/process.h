#include<iostream>
#include<cstdlib>
#include<cstring>
#include <cmath>
#include <time.h>
#include <fstream>
using namespace std;
typedef unsigned char BYTE;
#define conv(r, c) (r*width+c)
#define convs(r, c) ((r)*swidth+c)
#define cw(i) (i>=xx1 && i<xx2)
#define ch(i) (i>=yy1 && i<yy2)
#define dist(p1, p2) abs(p1-p2)
#define random(n) (rand() % n)
#define random_1 ((double)rand() / (double)RAND_MAX)

void yuv_process(BYTE* yuvData, BYTE resultMatrix[128][128], bool& alarmResult);

void set_resolution(int width,int height);

void set_cell_split(int row , int col);

void set_detection_region(int x1, int y1, int x2, int y2);

void set_threshold_sensity(int t, int s);

void gauss_filter(double win[3][3], BYTE* yuvData);

void generateGaussianTemplate(double win[][3], int ksize, double sigma);

void write_yuv(BYTE* yuvData, char* name);

void set_background(BYTE* yuvData);

void predict(BYTE* yuvData, BYTE resultMatrix[128][128], int bg_flush);

