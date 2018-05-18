#include "process.h"
using namespace std;

int TotalFrm = 250;

extern int height, width; 

const char* BASEDIR = "../";

string file_name = "室内人物1_1920x1080";
string inputPath = BASEDIR + file_name + ".yuv";
string paramPath = BASEDIR + file_name + ".txt";
string resultPath = BASEDIR + string("result_") + file_name + ".yuv";

int main(int argcnt, char* arg[])
{
	BYTE resultMatrix[128][128] = {0}; //检测区域最大128x128
	bool alarmResult;
	FILE *InputFile;
	
	int w, h, px1, py1, px2, py2, r, c, s, t;
	
	ifstream ins(paramPath.c_str());
	
	if (!ins) {
//		printf("Please add the param file!\n");
//		exit(-1);
		printf("请依次输入：width height x1 y1 x2 y2 row col sensity threshold\n");
		scanf("%d%d%d%d%d%d%d%d%d%d", &w, &h, &px1, &py1, &px2, &py2, &r, &c, &s, &t); 
	} 
	else {
		ins >> w >> h >> px1 >> py1 >> px2 >> py2 >> r >> c >> t >> s;
	}
	
	ins.close();
	srand((unsigned)time(NULL));
	long long time1 = clock();
	set_resolution(w, h);
	set_cell_split(r, c);
	set_detection_region(px1, py1, px2, py2);
	set_threshold_sensity(t,s);
	int frame_size = width*height*3/2;
	BYTE* yuvData = new BYTE [frame_size];
	
	//获取缓存空间
	
	FILE *f1;
	ofstream rrr("result.txt");
	remove(resultPath.c_str());
	//打开要读取的文件
	if ((f1 = fopen(inputPath.c_str(), "rb")) == NULL)
	{
		printf("Can't open file %s!\n", resultPath.c_str());
		exit(-1);
	}
	
	for (int i=0;i<TotalFrm;++i) {
		fread(yuvData,sizeof(char), sizeof(BYTE)*frame_size, f1);
		yuv_process(yuvData, resultMatrix, alarmResult);
		rrr << "第" << i+1 << "帧检测动检结果为<" << (alarmResult ? "触发动检" : "相安无事") << ">，检测矩阵：" << endl;
		for (int row=0;row<128;++row) {
			for (int col=0;col<128;++col) rrr << (int)resultMatrix[row][col] << " ";
			rrr << endl;
		}
	}
	
	rrr.close();
	
	//是否能写到该地址
	fflush(f1);
	fclose(f1);
	
	delete [] yuvData;
	long long time2 = clock();
	printf("所用时间：%dms\n", time2-time1);
}
