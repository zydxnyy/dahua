#include "process.h"
using namespace std;

const int K = 5;
const int Q = 2; 
const float SIGMA = 0.8;
const int WIN_SIZE = 3;
const float pi = 3.1415926535;

//样本集大小 
const int N = 8;
//最小背景匹配样本数 
const int D_MIN = 2;
//定义基准背景划分阈值的倍率 
const float R_RATE = 1.2f;
//定义基准背景划分阈值 
int norm_R = 10;
//定义背景划分阈值 
int R = 10;
//定义背景更新率 
const float RATE = 1.0;
//定义背景传播更新率 
const int PROGA_RATE = 3;
//定义闪烁帧 
const int BLINK_FRAME = 10;
//定义多帧建模的背景学习率 
const float alpha = 0.005;
//定义鬼影消除的阈值帧数 
const BYTE FG_CNT_T = 50; 

int count_frame = 1;
bool first_set = true;

/**需要开数组或者矩阵的变量**/
BYTE** bg_set;
bool* is_fg;
bool* is_fg2;

BYTE*** bg_set2;
float* weight;

BYTE* fg_cnt;
/****/

float gauss_win[3][3];

//二值形态格式刷 
const bool corrode_flush[3][3] = { {0,1,0}, {1,1,1}, {0,1,0} };

//检测区域单元格的坐标 
int cx1=0, cy1=0, cx2=0, cy2=0;
//视频的宽与高 
int width, height;
//将视频分割为多少行多少列 
int row, col;
//检测区域的左上角与右下角坐标（已经经过转换取整） 
int xx1, yy1, xx2, yy2;
//灵敏度与阈值 
int threshold, sensity;
//单元格尺寸 
int cell_width, cell_height;
//检测窗口大小 
int swidth, sheight;

//float cell_dect;

//extern string resultPath;

//设置分辨率 
void set_resolution(int _width,int _height) {
	width = _width;
	height = _height;
}

//设置视频的划分 
void set_cell_split(int _row, int _col) {
	row = _row;
	col = _col;
	cell_width = width / col;
	cell_height = height / row; 
}

//设置检测的矩形区域 
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

//设置灵敏度与阈值 
void set_threshold_sensity(int t, int s) {
//	printf("sen = %d, thres = %d\n", t, s);
	threshold = t;
	sensity = s;
//	cell_dect = -0.07*(float)s+0.8;
	norm_R = -2*s+31;
//	printf("cell_dect = %f\n", cell_dect);
}

//视频处理主函数 
void yuv_process(BYTE* yuvData, BYTE resultMatrix[128][128], bool* alarmResult) {
//第一帧的时候动态开内存并建立背景模型 
	if (first_set) {
		swidth = xx2-xx1;
		sheight = yy2-yy1;
//		printf("swidth = %d sheight = %d\n", swidth, sheight);
		bg_set = new BYTE* [swidth*sheight];
		for (int i=0;i<swidth*sheight;++i) bg_set[i] = new BYTE [N];
		
#ifdef multi_bg_build
		bg_set2 = new BYTE** [K];
		for (int i=0;i<K;++i) {
			bg_set2[i] = new BYTE* [swidth*sheight];
			for (int j=0;j<swidth*sheight;++j) bg_set2[i][j] = new BYTE[Q]; 
		}
		weight = new float [K];
		for (int i=0;i<K;++i) {
			if (i==0) {
				weight[i] = 0.3;
			}
			else if (i==K-1) {
				weight[i] = 0.1;
			}
			else {
				weight[i] = 0.2;
			}
		}
#endif
		
#ifdef ghost_clear
		fg_cnt = new BYTE [swidth*sheight];
#endif	
	
		is_fg = new bool [swidth*sheight];
		is_fg2 = new bool [swidth*sheight];
		//生成高斯离散核函数
		generateGaussianTemplate(gauss_win, WIN_SIZE, SIGMA);
		set_background(yuvData);

#ifdef multi_bg_build
		set_background_mul(yuvData);
#endif
		first_set = false;
	}
	
//	printf("%dth frame\n", count_frame); 
//用生成的高斯核函数来进行去噪平滑 
	gauss_filter(gauss_win, yuvData);

#ifndef multi_bg_build
	//初始帧不稳定，增大判为背景的概率 
	if (count_frame++ < BLINK_FRAME) {
		R = R_RATE * norm_R;
		//进行动检 
		predict(yuvData);
	}
	else {
		R = norm_R;
		predict(yuvData);
	}
#endif

#ifdef multi_bg_build
	if (count_frame++ < BLINK_FRAME) {
		R = R_RATE * norm_R;
		update_bg(yuvData);
	}
	else {
		R = norm_R;
		predict(yuvData);
	}
	//背景建模完成，将建模的背景集填充 
	if (count_frame == BLINK_FRAME) {
		fill_in();
	}
#endif

//二值形态操作，这里是开操作 
	corrode(yuvData);
	swell(yuvData);
	
//	filter();

//将结果写入 
	check(yuvData, resultMatrix, alarmResult); 
	
//将检测视频输出	
#ifdef output_yuv
	write_yuv(yuvData);	
#endif
	
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
				bg_set[convs(i-yy1, j-xx1)][m] = yuvData[conv(chy, chx)];
			}
			else {
				--m;
			}
		}
#ifdef ghost_clear
		fg_cnt[convs(i-yy1, j-xx1)] = 0;
#endif
	}
	
}

//建立背景模型 
void set_background_mul(BYTE* yuvData) {
	int choose, chx, chy;
	//遍历所有的像素，将背景集合2全部初始化为第一帧的图像 
	for (int i=yy1;i<yy2;++i) for (int j=xx1;j<xx2;++j) {
		for (int ccc=0;ccc<K;++ccc) {
			for (int m=0;m<Q;++m) {
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
						bg_set2[ccc][convs(i-yy1, j-xx1)][m] = yuvData[conv(chy, chx)];
				}
				else {
					--m;
				}
			}
		}
	}
}

void predict(BYTE* yuvData) {
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
						//传播更新，这里重复PROGA_RATE次，加快鬼影消除速率 
						for (int z=0;z<PROGA_RATE;++z) if (random_1 < RATE) {
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
							else {
								--z;
							}
						}
						
					}
					is_fg[convs(i-yy1, j-xx1)] = false;
#ifdef ghost_clear
					fg_cnt[convs(i-yy1, j-xx1)] = 0;					
#endif					
					
				}
				//前景 
				else {
					is_fg[convs(i-yy1, j-xx1)] = true;
					
#ifdef ghost_clear
					fg_cnt[convs(i-yy1, j-xx1)] +=1;
					//如果判为前景超过一定次数，用当前的像素点更新背景集合，并传播更新 
					if (fg_cnt[convs(i-yy1, j-xx1)] > FG_CNT_T) {
							//以一定概率更新背景
						if (random_1 < 1.0/8.0) {
							bg_set[convs(i-yy1, j-xx1)][random(N)] = yuvData[conv(i, j)];
							//传播更新 
							if (random_1 < 1.0/8.0) {
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
					}
#endif					
				}
					
			}
			
	}
	
}

void update_bg(BYTE* yuvData) {
	int bg_cnt;
	int choose, chx, chy;
	int min_set;
	float min_w;
	int matched;
	float w_sum;
	//遍历每一个单元格 
	for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
			
			//遍历单元格里面的每一个像素 
			for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
				//记录这一帧的像素，供差分使用 
//				yuvBackup[convs(i-yy1, j-xx1)] = yuvData[conv(i, j)];
				bg_cnt = 0;
				matched = -1;
				min_w = 10;
				w_sum = 0;
				//遍历每一个背景样本集合 
				for (int bg_set_cnt=0;bg_set_cnt<K;++bg_set_cnt) {
					for (int m=0;m<Q;++m) {
						//判断背景样本重叠数 
						if (dist(yuvData[conv(i, j)], bg_set2[bg_set_cnt][convs(i-yy1, j-xx1)][m]) > R) {
							break;
						}
						else if (m == Q-1) {
							matched = bg_set_cnt;
						}
					}
					//已经找到匹配的背景集合 
					if (matched != -1) break;
				}
				//如果没有找到匹配的背景集合，替代最小的权重的背景集合 
				if (matched == -1) {
					for (int m=0;m<K;++m) {
						if (min_w > weight[m]) {
							min_w = weight[m];
							min_set = m;
						}
					}
					//替换z最小权重集所有元素 
					for (int m=0;m<Q;++m) {
//						bg_set2[min_set][convs(i-yy1, j-xx1)][m] = yuvData[conv(i, j)];
						
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
							bg_set2[min_set][convs(i-yy1, j-xx1)][m] = yuvData[conv(chy, chx)];
						}
						else {
							--m;
						} 
					}
					//将图像设置为前景 
					is_fg[convs(i-yy1, j-xx1)] = true;
					
//					printf("Update all\n");
				}
				//如果有匹配的样本集，更新 
				else {
					//以一定概率更新匹配背景样本集所有的元素 
					if (random_1 < 1.0/8.0) {
						for (int m=0;m<Q;++m) {
//							bg_set2[matched][convs(i-yy1, j-xx1)][m] = yuvData[conv(i, j)];

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
								bg_set2[matched][convs(i-yy1, j-xx1)][m] = yuvData[conv(chy, chx)];
							}
							else {
								--m;
							} 
						}
						//权值更新归一化 
						for (int m=0;m<K;++m) {
							if (m == matched) weight[m] += alpha * (1.0-weight[m]);
							else weight[m] -= alpha * weight[m];
							w_sum += weight[m];
						}
						for (int m=0;m<K;++m) {
							weight[m] = weight[m] / w_sum;
//							printf("w%d update %f ", m+1, weight[m]);
						}
					}
//					printf("matched with set %d\n", matched);
//					printf("\n");
					//将图像设置为背景 
					is_fg[convs(i-yy1, j-xx1)] = false;
				}
			}
			
	}
	
}

bool cmp(const pair<float, BYTE**> &p1, const pair<float, BYTE**> &p2) {
	return p1.first > p2.first;
}

const float TTT = 0.8;
void fill_in() {
	//排序找出权重和大于阈值的前B个样本集 
	vector<pair<float, BYTE**> > vec;
	for (int i=0;i<K;++i) {
		vec.push_back(make_pair(weight[i], bg_set2[i]));
	}
	sort(vec.begin(), vec.end(), cmp);
	
	for (int i=0;i<K;++i) {
		printf("w%d=%f ", i+1, vec[i].first);
	}
	printf("\n");
	
	float weight_sum = 0;
	int set_end = 0;
	for (int i=0;i<K;++i) {
		weight_sum += vec[i].first;
		if (weight_sum >= TTT) {
			set_end = i;
		}
	}
	
	//将样本集循环填充到背景里面 
	int nn = 0, set_p=0;
	while (nn < N) {
		
		for (int i=0;i<Q;++i) {
			for (int k=yy1;k<yy2;++k) for (int l=xx1;l<xx2;++l) {
				bg_set[convs(k-yy1, l-xx1)][nn] = vec[set_p].second[convs(k-yy1, l-xx1)][i];
			}
			++nn;
		}
		
		if (++set_p > set_end) set_p = 0;
	}
	
	for (int i=0;i<K;++i) {
		for (int j=0;j<swidth*sheight;++j) {
			delete [] bg_set2[i][j]; 
		}
		delete [] bg_set2[i];
	} 
	delete [] bg_set2;
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


void filter() {
	int cnt = 0;
	for (int i=yy1;i<yy2;++i) for (int j=xx1;j<xx2;++j) {
		cnt = 0;
		for (int k=i-1;k<=i+1;++k) for (int l=j-1;l<=j+1;++l) {
			if (ch(k) && cw(l)) {
				if (is_fg[convs(k-yy1, l-xx1)]) {
					cnt += 1;
					if (cnt>=4) {
						is_fg2[convs(i-yy1, j-xx1)] = 1;
						goto out2;
					}
				}
				
			}
		}
		is_fg2[convs(i-yy1, j-xx1)] = 0;
		out2:;
	}
	swap(is_fg, is_fg2);
}

//检查当前帧是否满足动检条件 
void check(BYTE* yuvData, BYTE resultMatrix[128][128], bool* alarm) {
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
		//如果单元格内前景元素超过1半则判断单元格发生动检
		if (cell_count >= cell_height*cell_width/10) {
			block_count += 1;
			resultMatrix[p-cy1][q-cx1] = 1;

#ifdef paint_cell
			for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
				
				yuvData[conv(i, j)] *= 0.5;
			}
#endif 

		}
		else {
			resultMatrix[p-cy1][q-cx1] = 0;
		}
	}
	
	//如果大于阈值，检测当前帧为动检帧 
	if (block_count > (cx2-cx1)*(cy2-cy1)*threshold/100) {
		*alarm = true;
		
//		printf("*********alarm at frame %d*********\n", count_frame);
				//遍历每一个单元格 

#ifdef reverse_bg
		for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
		//遍历单元格里面的每一个像素 
			for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
//				if (yuvData[conv(i, j)] == 255) yuvData[conv(i, j)] = 0;
//				else yuvData[conv(i, j)] = 255;
				yuvData[conv(i, j)] = ~yuvData[conv(i, j)];
			}
		}
#endif

	}
	else {
		*alarm = false;
	}
}
