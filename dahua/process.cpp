#include "process.h"
using namespace std;

const int K = 5;
const int Q = 2; 
const float SIGMA = 0.8;
const int WIN_SIZE = 3;
const float pi = 3.1415926535;
const int N = 8;
const int D_MIN = 2;
const float R_RATE = 1.2f;
int norm_R = 10;
int R = 10;	//
const float RATE = 1.0;
const int PROGA_RATE = 3;
const int BLINK_FRAME = 10;
const float alpha = 0.00001;
const BYTE FG_CNT_T = 40; 

int swidth, sheight;

int count_frame = 1;
static bool first_set = true;

/**��Ҫ��������߾���ı���**/
BYTE** bg_set;
bool* is_fg;
bool* is_fg2;

BYTE*** bg_set2;
float* weight;

BYTE* fg_cnt;
/****/

float gauss_win[3][3];

//��ֵ��̬��ʽˢ 
const bool corrode_flush[3][3] = { {0,1,0}, {1,1,1}, {0,1,0} };

int cx1=0, cy1=0, cx2=0, cy2=0;
int width, height;
int row, col;
int xx1, yy1, xx2, yy2;
int threshold, sensity;
int cell_width, cell_height;

//float cell_dect;

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
	cout << xx1 << " " << yy1 << " " << xx2 << " " << yy2 << "\n" << cx1 << " " << cy1 << " " << cx2 << " " << cy2 << endl;
}

void set_threshold_sensity(int t, int s) {
	threshold = t;
	sensity = s;
//	cell_dect = -0.07*(float)s+0.8;
	norm_R = -2*s+31;
//	printf("cell_dect = %f\n", cell_dect);
}

void yuv_process(BYTE* yuvData, BYTE resultMatrix[128][128], bool* alarmResult) {
//	for (int i=0;i<width*height;++i) cout << (int)yuvData[i] << " ";
//	cout << endl;
//	system("pause");
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
		//���ɸ�˹��ɢ�˺���
		generateGaussianTemplate(gauss_win, WIN_SIZE, SIGMA);
//#ifndef multi_bg_build
		set_background(yuvData);
//#endif

#ifdef multi_bg_build
		set_background_mul(yuvData);
#endif
		first_set = false;
	}
	
	printf("%dth frame\n", count_frame); 
	gauss_filter(gauss_win, yuvData);

#ifndef multi_bg_build
	//��ʼ֡���ȶ���������Ϊ�����ĸ��� 
	if (count_frame++ < BLINK_FRAME) {
		R = R_RATE * norm_R;
		predict(yuvData, resultMatrix, 0);
	}
	else {
		R = norm_R;
		predict(yuvData, resultMatrix, 0);
	}
#endif

#ifdef multi_bg_build
	if (count_frame++ < BLINK_FRAME) {
		R = R_RATE * norm_R;
		update_bg(yuvData, resultMatrix, 0);
	}
	else {
		R = norm_R;
		predict(yuvData, resultMatrix, 0);
	}
	//������ģ��ɣ�����ģ�ı�������� 
	if (count_frame == BLINK_FRAME) {
		fill_in();
	}
#endif
	
	corrode(yuvData);
	swell(yuvData);
	
//	filter();
	
	check(yuvData, resultMatrix, alarmResult); 
	
#ifdef output_yuv
	write_yuv(yuvData, resultPath);	
#endif
	
}

//������˹��ɢ�˺��� 
void generateGaussianTemplate(float win[][3], int ksize, float sigma) {
    int center = ksize / 2; // ģ�������λ�ã�Ҳ���������ԭ��
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
    //double k = 1 / window[0][0]; // �����Ͻǵ�ϵ����һ��Ϊ1
    for (int i = 0; i < ksize; i++)
    {
        for (int j = 0; j < ksize; j++)
        {
            win[i][j] /= sum;
        }
    }
}

//��ͼ�������ø�˹��ɢ�˺������� 
void gauss_filter(float win[3][3], BYTE* yuvData) {
	double weight_sum, y_sum;
	for (int i=yy1;i<yy2;++i) {
		for (int j=xx1;j<xx2;++j) {
			
			weight_sum = y_sum = 0;
			
			for (int k=i-1;k<=i+1;++k) 
				for (int l=j-1;l<=j+1;++l) {
					//�������Ϸ� 
					if (ch(k) && cw(l)) {
						weight_sum += win[k-i+1][l-j+1];
						y_sum += yuvData[conv(k, l)] * win[k-i+1][l-j+1];
					}
				}
			yuvData[conv(i, j)] = y_sum / weight_sum;
		}
	}
}

//�������������ļ� 
void write_yuv(BYTE* yuvData, string name) {
	FILE* wfile;
	if ((wfile = fopen(name.c_str(), "ab")) == NULL) { 
		exit(-1);
	}
	fwrite(yuvData, sizeof(char),sizeof(BYTE)*width*height,wfile);
	fclose(wfile);
}

//��������ģ�� 
void set_background(BYTE* yuvData) {
	int choose, chx, chy;
	//�������е����أ�Ϊÿһ�����ص����ñ�����ɫ���� 
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

//��������ģ�� 
void set_background_mul(BYTE* yuvData) {
	int choose, chx, chy;
	//�������е����أ�����������2ȫ����ʼ��Ϊ��һ֡��ͼ�� 
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

void predict(BYTE* yuvData, BYTE resultMatrix[128][128], int bg_flush) {
	int bg_cnt;
	int choose, chx, chy;
	//����ÿһ����Ԫ�� 
	for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
		
			//������Ԫ�������ÿһ������ 
			for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
				//��¼��һ֡�����أ������ʹ�� 
//				yuvBackup[convs(i-yy1, j-xx1)] = yuvData[conv(i, j)];
				bg_cnt = 0;
				//����ÿһ�������������� 
				for (int m=0;m<N;++m) {
					//�жϱ��������ص��� 
					if (dist(yuvData[conv(i, j)], bg_set[convs(i-yy1, j-xx1)][m]) < R) if (++bg_cnt > D_MIN) break;
				}
				//�����������ص���������ֵ����Ϊ����
				if (bg_cnt > D_MIN) {
					//��һ�����ʸ��±���
					if (random_1 < RATE) {
						bg_set[convs(i-yy1, j-xx1)][random(N)] = yuvData[conv(i, j)];
						//�������� 
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
				//ǰ�� 
				else {
					is_fg[convs(i-yy1, j-xx1)] = true;
					
#ifdef ghost_clear
					fg_cnt[convs(i-yy1, j-xx1)] +=1;
					//�����Ϊǰ������һ���������õ�ǰ�����ص���±������ϣ����������� 
					if (fg_cnt[convs(i-yy1, j-xx1)] > FG_CNT_T) {
							//��һ�����ʸ��±���
						if (random_1 < 1.0/8.0) {
							bg_set[convs(i-yy1, j-xx1)][random(N)] = yuvData[conv(i, j)];
							//�������� 
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
					
#ifdef proga_fg					
					//��һ�����ʴ���ǰ������հ�
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
				//ˢ�±��� 
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

void update_bg(BYTE* yuvData, BYTE resultMatrix[128][128], int bg_flush) {
	int bg_cnt;
	int choose, chx, chy;
	int min_set;
	float min_w;
	int matched;
	float w_sum;
	//����ÿһ����Ԫ�� 
	for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
			
			//������Ԫ�������ÿһ������ 
			for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
				//��¼��һ֡�����أ������ʹ�� 
//				yuvBackup[convs(i-yy1, j-xx1)] = yuvData[conv(i, j)];
				bg_cnt = 0;
				matched = -1;
				min_w = 10;
				w_sum = 0;
				//����ÿһ�������������� 
				for (int bg_set_cnt=0;bg_set_cnt<K;++bg_set_cnt) {
					for (int m=0;m<Q;++m) {
						//�жϱ��������ص��� 
						if (dist(yuvData[conv(i, j)], bg_set2[bg_set_cnt][convs(i-yy1, j-xx1)][m]) > R) {
							break;
						}
						else if (m == Q-1) {
							matched = bg_set_cnt;
						}
					}
					//�Ѿ��ҵ�ƥ��ı������� 
					if (matched != -1) break;
				}
				//���û���ҵ�ƥ��ı������ϣ������С��Ȩ�صı������� 
				if (matched == -1) {
					for (int m=0;m<K;++m) {
						if (min_w > weight[m]) {
							min_w = weight[m];
							min_set = m;
						}
					}
					//�滻z��СȨ�ؼ�����Ԫ�� 
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
					//��ͼ������Ϊǰ�� 
					is_fg[convs(i-yy1, j-xx1)] = true;
					
//					printf("Update all\n");
				}
				//�����ƥ��������������� 
				else {
					//��һ�����ʸ���ƥ�䱳�����������е�Ԫ�� 
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
						//Ȩֵ���¹�һ�� 
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
					//��ͼ������Ϊ���� 
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
	//�����ҳ�Ȩ�غʹ�����ֵ��ǰB�������� 
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
	
	//��������ѭ����䵽�������� 
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

//��ֵ��̬��ʴ 
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

//��ֵ��̬���� 
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

//��鵱ǰ֡�Ƿ����㶯������ 
void check(BYTE* yuvData, BYTE resultMatrix[128][128], bool* alarm) {
	//cell_count��Ԫ��������ٸ����ص���ǰ����block_count��������ж��ٸ���Ԫ����Ϊ��̬ 
	int cell_count = 0, block_count = 0;

	//����ÿһ����Ԫ�� 
	for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
		
		cell_count = 0;
		
		//������Ԫ�������ÿһ������ 
		for (int i=p*cell_height;i<(p+1)*cell_height;++i) for (int j=q*cell_width;j<(q+1)*cell_width;++j) {
			if (is_fg[convs(i-yy1, j-xx1)]) {
				cell_count += 1;
				yuvData[conv(i, j)] = 255;
			}
			else {
				yuvData[conv(i, j)] = 0;
			}
		}
		//�����Ԫ����ǰ��Ԫ�س���1�����жϵ�Ԫ��������
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
	
	printf("block_cnt=%d<-->%d\n", block_count, (cx2-cx1)*(cy2-cy1));
	
	//���������ֵ����⵱ǰ֡Ϊ����֡ 
	if (block_count > (cx2-cx1)*(cy2-cy1)*threshold/100) {
		*alarm = true;
		
		printf("*********alarm at frame %d*********\n", count_frame);
				//����ÿһ����Ԫ�� 

#ifdef reverse_bg
		for (int p=cy1;p<cy2;++p) for (int q=cx1;q<cx2;++q) {
		//������Ԫ�������ÿһ������ 
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
