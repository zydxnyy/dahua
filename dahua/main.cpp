#include "process.h"
using namespace std;

int TotalFrm = 250;

extern int height, width; 

const char* BASEDIR = "../";

string file_name = "2";
string colorPath = BASEDIR + file_name + ".yuv";
string paramPath = BASEDIR + file_name + ".txt";
string resultPath = BASEDIR + string("result") + file_name + ".yuv";

int main(int argcnt, char* arg[])
{
	BYTE resultMatrix[128][128]; //检测区域最大128x128
	bool alarmResult;
	FILE *InputFile;
	
	ifstream ins(paramPath.c_str());
	
	if (!ins) {
		printf("Please add the param file!\n");
		exit(-1);
	} 
	
	int w, h, px1, py1, px2, py2, r, c, s, t;
	ins >> w >> h >> px1 >> py1 >> px2 >> py2 >> r >> c >> t >> s;
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
	remove("result.yuv");
	//打开要读取的文件
	if ((f1 = fopen(colorPath.c_str(), "rb")) == NULL)
	{
		printf("Can't open file %s!\n", colorPath.c_str());
		exit(-1);
	}
	
	for (int i=0;i<TotalFrm;++i) {
		fread(yuvData,sizeof(char), sizeof(BYTE)*frame_size, f1);
		yuv_process(yuvData, resultMatrix, alarmResult);
	}
	
	//是否能写到该地址
	fflush(f1);
	fclose(f1);
	
	delete [] yuvData;
	long long time2 = clock();
	cout << time2-time1 << endl; 
}
