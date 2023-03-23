#include<iostream>
using namespace std;	//暂时不知道怎么修改
#include<graphics.h>
#include"tools.h"
#include<ctime>
#include<time.h>

#include<mmsystem.h>	//音乐
#pragma comment(lib,"winmm.lib") 



enum {WAN_DAO,XIANG_RI_KUI,SHI_REN_HUA,ZHI_WU_COUNT};

IMAGE imgBg;	//游戏背景图
IMAGE imgBar;	//状态栏，放植物的背景板
IMAGE imgCards[ZHI_WU_COUNT];	//植物卡牌数组
IMAGE* imgZhiWu[ZHI_WU_COUNT][20];	//植物数组 

int curX, curY;	//当前选中植物在移动中的坐标
int curZhiWu;	//当前选中的植物	0-没有选中，1-选中第一种植物

//宏定义游戏窗口大小
#define	WIN_WIDTH 900
#define	WIN_HEIGHT 600

struct zhiWu {	//植物结构体
	int type;		//0-没有植物，1-第一种植物
	int frameIndex;	//序列帧的序号
};

struct sunShineBall {	//阳光球结构体
	int	x, y;//阳光球的x、y坐标
	int	frameIndex;	//阳光球序列帧的序号
	int	destY;	//阳光球停止的y坐标
	bool used;	//阳光球是否在使用
	int timer;	//计时器，用来限制阳光球最后的停留时间
};

struct sunShineBall	balls[10];	//阳光球池，用来事先存储阳光
IMAGE	imgSunShineBall[29];	//阳光序列帧总数	-	可以定义一个宏，方便后期管理

struct zm {		//僵尸结构体	-后期还是需要像植物一样搞个枚举,方便创建不同类型的僵尸
	int x, y;
	int frameIndex;
	bool used;
	int speed;	//僵尸前行速度
};

struct zm zms[10];	//僵尸池,用来事先存储僵尸
IMAGE	imgZm[22];	

int ballMax = sizeof(balls) / sizeof(balls[0]);	//阳光池中阳光的总数	-是不是很奇怪，明明上面已经定义了10，
												//这里还要计算一遍，别问，问就是高可用(个人建议把10换成一个宏定义，更加高可用)
int zmMax = sizeof(zms) / sizeof(zms[0]);	//僵尸池中僵尸的总数

struct zhiWu map[3][9];	//地图数组，方便存储植物

int sunShine;	//阳光值

//声明游戏初始化
void gameInit();

//声明游戏开始界面
void startUI();

//声明渲染游戏窗口(渲染图片到窗口上)
void updateWindow();

//声明用户点击（鼠标操作）
void userClick();

//声明判断文件是否存在
bool fileExist(const char* name);

//声明更新动作
void updateGame();

//声明创建阳光
void createSunShine();

//声明更新阳光动作
void updateSunShine();

//声明收集阳光
void collectSunshine(ExMessage* msg);

int main() {
		
	gameInit();	//游戏初始化

	startUI();	//加载游戏开始界面

	//双缓冲，先将要绘制的内容一次性绘制在图片上，再把图片输出，避免不断从内存读取数据而导致的屏幕闪烁
	//主要由BeginBatchDraw()、EndBatchDraw()、FlushBatchDraw()组成
	BeginBatchDraw();

	int timer=0;
	bool flag = true;

	while (1)
	{
		userClick();	//获取用户点击事件

		timer += getDelay();	//获取间隔时间
		if (timer > 20) {	//用来限制植物渲染时间
			timer = 0;
			flag = true;
		}
		if (flag) {

			flag = false;

			updateWindow();	//更新游戏窗口（渲染）

			updateGame();	//更新动作
		}
		//刷新图片，避免黑屏
		FlushBatchDraw();
	}
	
	EndBatchDraw();

	system("pause");

	return 0;
}

//游戏初始化实现
void gameInit() {
	//加载游戏背景图片
	loadimage(&imgBg, "res/bg.jpg");

	//加载状态栏
	loadimage(&imgBar, "res/bar5.png");

	memset(imgZhiWu, 0, sizeof(imgZhiWu));	//给指针赋空值

	memset(map, 0, sizeof(map));	//初始化地图数组

	memset(balls, 0, sizeof(balls));	//初始化阳光池


	//加载植物卡牌
	char	name[64];
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i+1);
		loadimage(&imgCards[i], name);
		for (int j = 0; j < 20; j++) {	//20是固定值，可以寻求更匹配的方式
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			//判断文件是否存在
			if (fileExist(name)) {
				imgZhiWu[i][j] = new IMAGE;
				loadimage(imgZhiWu[i][j], name);
			}
			else {
				break;
			}
		}		
	}

	//初始化选中植物
	curZhiWu = 0;

	//初始化阳光值
	sunShine = 50;

	//加载阳光
	for (int i = 0; i < 29; i++) {	//29是固定值，可以寻求更匹配的方式
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png",i+1);
		loadimage(&imgSunShineBall[i],name);
	}

	srand(time(NULL));

	//创建游戏窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT,1/*,1*/);	//参数1表示再开一个控制台窗口	

	//设置字体
	LOGFONT	f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");	//设置字体效果
	f.lfQuality = ANTIALIASED_QUALITY;		//抗锯齿
	settextstyle(&f);
	setbkmode(TRANSPARENT);					//字体模式：背景透明
	setcolor(BLACK);						//字体颜色：黑色

}

//游戏开始界面实现
void startUI() {
	IMAGE imgMenu,imgMenu1,imgMenu2;
	int	flag = 0;
	loadimage(&imgMenu, "res/menu.png");	//加载开始背景图
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");

	while (1) {
		BeginBatchDraw();
		putimage(0, 0, &imgMenu);	//渲染开始背景图到窗口上
		putimagePNG(474, 75, flag == 0 ? &imgMenu1 : &imgMenu2);

		ExMessage	msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN &&	//鼠标左键落下		扩展：当鼠标经过时也可以高亮
				msg.x > 474 && msg.x < 774 && msg.y>75 && msg.y < 215) {
				flag = 1;
			}
		}
		else if (msg.message == WM_LBUTTONUP && flag == 1) {	//鼠标左键抬起
			return;
		}
		EndBatchDraw();
	}
}

//把图片加载到窗口上(渲染)
void updateWindow() {
	putimage(0, 0, &imgBg);	//加载(渲染)背景板
	putimagePNG(255, 0, &imgBar);	//加载(渲染)状态栏
	for (int i = 0; i < ZHI_WU_COUNT; i++) {	//加载(渲染)植物卡牌
		int x = 343 + i * 65;
		int y = 6;
		putimagePNG(x, y, &imgCards[i]);
	}

	for (int i = 0; i < 3; i++) {	//在地图上加载(渲染)植物
		for (int j = 0; j < 9; j++){
			if (map[i][j].type > 0) {
				int x = 256 + j * 81;
				int y = 179 + i * 102 + 14;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				putimagePNG(x, y, imgZhiWu[zhiWuType][index]);
			}
		}
	}

	if (curZhiWu > 0) {
		IMAGE* img = imgZhiWu[curZhiWu - 1][0];	//加载(渲染)植物
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}

	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {	//加载(渲染)阳光
			IMAGE* img = &imgSunShineBall[balls[i].frameIndex];
			putimagePNG(balls[i].x, balls[i].y, img);
		}
	}

	//加载(渲染)阳光值
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunShine);	//把阳光值转换成字符类型
	outtextxy(283, 67, scoreText);			//渲染输出	位置可调整成居中,而不使用固定值y		

}

//用户点击实现
void userClick() {
	static	int status = 0;
	ExMessage	msg;
	if (peekmessage(&msg)) {	//判断用户是否有操作
		if (msg.message == WM_LBUTTONDOWN) {	//鼠标左键按下
			if (msg.x > 343 && msg.x < 343 + 65 * ZHI_WU_COUNT && msg.y < 96) {	//点击卡牌的事件
				int index = (msg.x - 343) / 65;
				status = 1;
				curZhiWu = index + 1;
				//使植物显示在点击位置，避免了植物出现在上次消失位置的小bug
				curX = msg.x;
				curY = msg.y;
			}
			else {	//收集阳光事件
				collectSunshine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1) {	//鼠标移动
			curX = msg.x;
			curY = msg.y;
		}else if (msg.message == WM_RBUTTONDOWN) {	//鼠标右键按下
			if (msg.x > 256 && msg.y > 179 && msg.y < 489) {
				int	row = (msg.y - 179) / 102;	//获取行
				int	col = (msg.x - 256) / 81;	//获取列
				if (map[row][col].type == 0) {
					map[row][col].type = curZhiWu;	//给鼠标当前行种下植物
					map[row][col].frameIndex = 0;	//渲染植物第一帧
				}
			}
			//使植物释放消失
			curZhiWu = 0;
			status = 0;
			//重置植物的坐标
			//curX = 1000;
			//curY = 1000;
		}
	}
}

//判断文件是否存在实现
bool fileExist(const char* name) {
	FILE* fp=fopen(name, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

//更新动作实现
void updateGame() {

	//更新植物动作
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				map[i][j].frameIndex++;
				int	zhiWuType = map[i][j].type - 1;
				int	index = map[i][j].frameIndex;
				if (imgZhiWu[zhiWuType][index] == NULL) {
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
	
	//创建阳光
	createSunShine();

	//更新阳光动作
	updateSunShine();
}

//创建阳光实现
void createSunShine() {

	static	int	count = 0;
	static	int	fre = 300;
	count++;
	
	if (count >= fre) {	//限制阳光生成的速度

		fre = 100 + rand() % 200;	//第二次生成阳光的时间随机
		count = 0;

		int i;

		//从阳光池中取出可用的阳光
		for (i = 0; i < ballMax && balls[i].used; i++);	//别问，问就是一种新定义方式，跟{}一个样
		if (i >= ballMax)return;

		balls[i].used = true;
		balls[i].frameIndex = 0;
		balls[i].x = 260 + rand() % (900 - 320);	//随机落点
		balls[i].y = 60;
		balls[i].destY = 200 + (rand() % 4) * 90;	//随机停止位置
	}
}

//更新阳光动作实现
void updateSunShine() {
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;	//更新序列帧
			if (balls[i].y < balls[i].destY) {
				balls[i].y += 2;
			}
			if (balls[i].y >= balls[i].destY) {
				balls[i].timer++;
				//cout << "i=" << i << ":" << balls[i].timer << endl;
				if (balls[i].timer > 100) {
					balls[i].timer = 0;	//重置定时器，避免下一次取出同样的阳光球一到达停止位置就消失
					balls[i].used = false;
					//printf((balls[i].timer));
					//cout << "i=" << i << ":" << balls[i].timer << endl;
				}
			}
		}
	}
}

//收集阳光实现
void collectSunshine(ExMessage* msg) {
	int w = imgSunShineBall[0].getwidth();	//单个阳光球的宽度
	int h = imgSunShineBall[0].getheight();	//单个阳光球的高度
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {	//阳光球被使用了才进行操作
			if (msg->x > balls[i].x && msg->x<balls[i].x + w &&		//只有当光标在阳光范围内才进行操作
				msg->y>balls[i].y && msg->y < balls[i].y + h) {
				balls[i].used = false;	//阳光球消失
				sunShine += 25;			//阳光值加25
				mciSendString("play res/sunshine.mp3", 0, 0, 0);
			}
		}
	}
}

