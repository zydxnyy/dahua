#include "process.h"
using namespace std;

//#define proga_fg
//#define _bg_flush

const float SIGMA = 0.8;
const int WIN_SIZE = 3;
const float pi = 3.1415926535;
const int N = 20;
const int D_MIN = 2;
const float R_RATE = 1.5f;
int norm_R = 20;
int R = 10;	//
const float RATE = 1;
const float PROGA_RATE = 1/3;
const int BLINK_FRAME = 10;

int swidth, sheight;

static int count = 1;
static bool first_set = true;

/**需要开数组或者矩阵的变量**/
BYTE** bg_set;
bool* is_fg;
bool* is_fg2;
/****/

float gauss_win[3][3];

//二值形态格式刷 
const bool corrode_flush[3][3] = { {0,1,0}, {1,1,1}, {0,1,0} };

int cx1, cy1, cx2, cy2;
int width, height;
int row, col;
int xx1, yy1, xx2, yy2;
int threshold, sensity;
int cell_width, cell_height;

float cell_dect;

extern string resultPath;

void set_resolution(int _width,int _height) {
	width = _width;
	height = _height;
}

void set_cell_split(int _row, int _col) {
	row = _row;
	col = _col;
	cell_width = width / col;
	cell_height = height / row; 
}

void set_detection_region(int _x1, int _y1, int _x2, int _y2) {
	xx1 = _x1 / cell_width * cell_width;
	yy1 = _y1 / cell_height * cell_height;
	xx2 = (( (_x2-1)/cell_width)+ 1) * cell_width;
	yy2 = (( (_y2-1)/cell_height)+ 1) * cell_height;
	cx1 = xx1 / cell_width, cy1 = yy1 / cell_height;
	cx2 = xx2 / cell_width, cy2 = yy2 / cell_height;

//	xx1 = _x1*cell_width, yy1 = _y1*cell_height;
//	xx2 = _x2*cell_width, yy2 = _y2*cell_height;
//	cx1 = _x1, cy1 = _y1;
//	cx2 = _x2, cy2 = _y2;
//	cout << xx1 << " " << yy1 << " " << xx2 << " " << yy2 << "\n" << cx1 << " " << cy1 << " " << cx2 << " " << cy2 << endl;
}

void set_threshold_sensity(int t, int s) {
	threshold = t;
	sensity = s;
	cell_dect = -0.07*(float)s+0.8;
	
//	printf("cell_dect = %f\n", cell_dect);
}

void yuv_process(BYTE* yuvData, BYTE resultMatrix[128][128], bool& alarmResult) {
//	for (int i=0;i<width*height;++i) cout << (int)yuvData[i] << " ";
//	cout << endl;
//	system("pause");
	if (first_set) {
		swidth = xx2-xx1;
		sheight = yy2-yy1;
//		printf("swidth = %d sheight = %d\n", swidth, sheight);
		bg_set = new BYTE* [swidth*sheight];
		for (int i=0;i<swidth*sheight;++i) bg_set[i] = new BYTE [N];
		is_fg = new bool [swidth*sheight]; 
		is_fg2 = new bool [swidth*sheight]; 
		//生成高斯离散核函数
		generateGaussianTemplate(gauss_win, WIN_SIZE, SIGMA);
		set_background(yuvData);
		first_set = false;
	}
	
	printf("%dth frame\n", count); 
	gauss_filter(gauss_win, yuvData);
	//初始帧不稳定，增大判为背景的概率 
	if (count++ < BLINK_FRAME) {
		R = R_RATE * norm_R;
		predict(yuvData, resultMatrix, 0);
	}
	else {
		R = norm_R;
		predict(yuvData, resultMatrix, 0);
	}
	corrode(yuvData);
	swell(yuvData);
	
//	filter();
	
	check(yuvData, resultMatrix, alarmResult); 
	
	write_yuv(yuvData, resultPath);
	
}

//产生高斯离散核函数 
void generateGaussianTemplate(float win[][3], int ksize, float sigma) {
    int center = ksize / 2; // 模板的中心位置，也就是坐标的原点
    double ax2, ay2;
    double sum = 0;
    for (int i = 0; i < ksize; i++)
    {
        ax2 = pow(i - center, 2);
        for (int j = 0; j < ksize; j++)
        {
            ay2 = pow(j - center, 2);
            double g = exp(-(ax2 + ay2) / (2 * sigma * sigma));
            g /= 2 * pi * sigma;
            sum += g;
            win[i][j] = g;
        }
    }
    //double k = 1 / window[0][0]; // 将左上角的系数归一化为1
    for (int i = 0; i < ksize; i++)
    {
        for (int j = 0; j < ksize; j++)
        {
            win[i][j] /= sum;
        }
    }
}

//将图像数据用高斯离散核函数过滤 
void gauss_filter(float win[3][3], BYTE* yuvData) {
	double weight_sum, y_sum;
	for (int i=yy1;i<yy2;++i) {
		for (int j=xx1;j<xx2;++j) {
			
			weight_sum = y_sum = 0;
			
			for (int k=i-1;k<=i+1;++k) 
				for (int l=j-1;l<=j+1;++l) {
					//如果坐标合法 
					if (ch(k) && cw(l)) {
						weight_sum += win[k-i+1][l-j+1];
						y_sum += yuvData[conv(k, l)] * win[k-i+1][l-j+1];
					}
				}
			yuvData[conv(i, j)] = y_sum / weight_sum;
		}
	}
}

//将检测结果输出到文件 
void write_yuv(BYTE* yuvData, string name) {
	FILE* wfile;
	if ((wfile = fopen(name.c_str(), "ab")) == NULL) { 
		exit(-1);
	}
	fwrite(yuvData, sizeof(char),sizeof(BYTE)*width*height,wfile);
	fclose(wfile);
}

//建立背景模型 
void set_background(BYTE* yuvData) {
	int choose, chx, chy;
	//遍历所有的像素，为每一个像素点设置背景颜色集合 
	for (int i=yy1;i<yy2;++i) for (int j=xx1;j<xx2;++j) {
		for (int m=0;m<N;++m) {
			choose = random(9);
			switch(choose) {
				case 0: chy = i-1, chx = j-1;break;
				case 1: chy = i-1, chx = j;break;
				case 2: chy = i-1, chx = j+1;break;
				case 3: chy = i, chx = j-1;break;
				case 4: chy = i, chx = j+1;break;
				case 5: chy = i+1, chx = j-1;break;
				case 6: chy = i+1, chx = j;break;
				case 7: chy = i+1, chx = j+1;break;
				case 8: chy = i, chx = j;break;
			}
			if (ch(chy) && cw(chx)) {
				bg_set[convs(i-yy1, j-xx1)][m] = yuvData[conv(chy, chx)];
			}
			else {
				--m;
			}
		}
	}
	
//	for (int i=0;i<height*width;++i) {
//		for (int j=0;j<N;++j) printf("%d ", bg_set[i][j]);
//		printf("\n");
//	}
}

void predict(BYTE* yuvData, BYTE resultMatrix[128][128], int bg_flush) {
	int bg_cnt;
	int choose, chx, chy;
	//遍历每一个单元格 
	for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
		
			//遍历单元格里面的每一个像素 
			for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
				//记录这一帧的像素，供差分使用 
//				yuvBackup[convs(i-yy1, j-xx1)] = yuvData[conv(i, j)];
				bg_cnt = 0;
				//遍历每一个背景样本集合 
				for (int m=0;m<N;++m) {
					//判断背景样本重叠数 
					if (dist(yuvData[conv(i, j)], bg_set[convs(i-yy1, j-xx1)][m]) < R) if (++bg_cnt > D_MIN) break;
				}
				//当背景样本重叠数大于阈值，则为背景
				if (bg_cnt > D_MIN) {
					//以一定概率更新背景
					if (random_1 < RATE) {
						bg_set[convs(i-yy1, j-xx1)][random(N)] = yuvData[conv(i, j)];
						//传播更新 
						if (random_1 < RATE) {
							choose = random(8);
							switch(choose) {
								case 0: chy = i-1, chx = j-1;break;
								case 1: chy = i-1, chx = j;break;
								case 2: chy = i-1, chx = j+1;break;
								case 3: chy = i, chx = j-1;break;
								case 4: chy = i, chx = j+1;break;
								case 5: chy = i+1, chx = j-1;break;
								case 6: chy = i+1, chx = j;break;
								case 7: chy = i+1, chx = j+1;break;
							}
							if (ch(chy) && cw(chx)) {
								bg_set[convs(chy-yy1, chx-xx1)][random(N)] = yuvData[conv(i, j)];
							}
						}
						
					}
					is_fg[convs(i-yy1, j-xx1)] = false;
				}
				//前景 
				else {
					is_fg[convs(i-yy1, j-xx1)] = true;
					//以一定概率传播前景，填补空白
#ifdef proga_fg					
					if (random_1 > PROGA_RATE) {
						choose = random(8);
						switch(choose) {
							case 0: chy = i-1, chx = j-1;break;
							case 1: chy = i-1, chx = j;break;
							case 2: chy = i-1, chx = j+1;break;
							case 3: chy = i, chx = j-1;break;
							case 4: chy = i, chx = j+1;break;
							case 5: chy = i+1, chx = j-1;break;
							case 6: chy = i+1, chx = j;break;
							case 7: chy = i+1, chx = j+1;break;
							case 8: chy = i, chx = j;break;
						}
						if (ch(chy) && cw(chx)) {
							is_fg[convs(chy-yy1, chx-xx1)] = true;
						}
					}
#endif
					
				}
				
#ifdef _bg_flush
				//刷新背景 
				for (int bg=0;bg<bg_flush;++bg) {
					choose = random(8);
					switch(choose) {
						case 0: chy = i-1, chx = j-1;break;
						case 1: chy = i-1, chx = j;break;
						case 2: chy = i-1, chx = j+1;break;
						case 3: chy = i, chx = j-1;break;
						case 4: chy = i, chx = j+1;break;
						case 5: chy = i+1, chx = j-1;break;
						case 6: chy = i+1, chx = j;break;
						case 7: chy = i+1, chx = j+1;break;
					}
					if (ch(chy) && cw(chx)) {
						bg_set[convs(chy-yy1, chx-xx1)][random(N)] = yuvData[conv(chy, chx)];
					}
				}
#endif
				
					
			}
			
	}
	
}

//二值形态腐蚀 
void corrode(BYTE* yuvData) {
	for (int i=yy1;i<yy2;++i) for (int j=xx1;j<xx2;++j) {
		
		for (int k=i-1;k<=i+1;++k) for (int l=j-1;l<=j+1;++l) {
			if (ch(k) && cw(l)) {
				if (corrode_flush[k-i+1][l-j+1] && !is_fg[convs(k-yy1, l-xx1)]) {
					is_fg2[convs(i-yy1, j-xx1)] = 0;
					goto out1;
				}
			}
		}
		is_fg2[convs(i-yy1, j-xx1)] = 1;
		out1:;
	}
	swap(is_fg, is_fg2);
}

//二值形态膨胀 
void swell(BYTE* yuvData) {
	for (int i=yy1;i<yy2;++i) for (int j=xx1;j<xx2;++j) {
		
		for (int k=i-1;k<=i+1;++k) for (int l=j-1;l<=j+1;++l) {
			if (ch(k) && cw(l)) {
				if (corrode_flush[k-i+1][l-j+1] && is_fg[convs(k-yy1, l-xx1)]) {
					is_fg2[convs(i-yy1, j-xx1)] = 1;
					goto out2;
				}
			}
		}
		is_fg2[convs(i-yy1, j-xx1)] = 0;
		out2:;
	}
	swap(is_fg, is_fg2);
}

//邹氏过滤器 
//void filter() {
//	int cnt = 0;
//	for (int i=yy1;i<yy2;++i) for (int j=xx1;j<xx2;++j) {
//		cnt = 0;
//		for (int k=i-1;k<=i+1;++k) for (int l=j-1;l<=j+1;++l) {
//			if (ch(k) && cw(l)) {
//				if (is_fg[convs(k-yy1, l-xx1)]) {
//					cnt += 1;
//					if (cnt>=4) {
//						is_fg2[convs(i-yy1, j-xx1)] = 1;
//						goto out2;
//					}
//				}
//				
//			}
//		}
//		is_fg2[convs(i-yy1, j-xx1)] = 0;
//		out2:;
//	}
//	swap(is_fg, is_fg2);
//}

//检查当前帧是否满足动检条件 
void check(BYTE* yuvData, BYTE resultMatrix[128][128], bool& alarm) {
	//cell_count单元格里面多少个像素点是前景，block_count检测区域有多少个单元格判为动态 
	int cell_count = 0, block_count = 0;

	//遍历每一个单元格 
	for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
		
		cell_count = 0;
		
		//遍历单元格里面的每一个像素 
		for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
			if (is_fg[convs(i-yy1, j-xx1)]) {
				cell_count += 1;
				yuvData[conv(i, j)] = 255;
			}
			else {
				yuvData[conv(i, j)] = 0;
			}
		}
		
//		if (cell_count != 0) printf("cell_count=%d\n", cell_count);

//		if (cell_count >= cell_height*cell_width*cell_dect) {
//			block_count += 1;
//			resultMatrix[p-cy1][q-cx1] = 1;
//		}
//		else {
//			resultMatrix[p-cy1][q-cx1] = 0;
//		}
	}
	
//	printf("block_cnt=%d<-->%d\n", block_count, (cx2-cx1)*(cy2-cy1));
	
	//如果大于阈值，检测当前帧为动检帧 
	if (block_count >= (cx2-cx1)*(cy2-cy1)*threshold/100) {
		alarm = true;
		
//		printf("*********alarm at frame %d*********\n", count);
				//遍历每一个单元格 
//		for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
//		//遍历单元格里面的每一个像素 
//			for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
//				if (yuvData[conv(i, j)] == 255) yuvData[conv(i, j)] = 0;
//				else yuvData[conv(i, j)] = 255;
//			}
//		}

	}
	else {
		alarm = false;
	}
}
