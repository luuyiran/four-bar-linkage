/////////////////////////////////////////////////////////////
// 程序名称：四杆机构-机械原理
// 编译环境：Visual Studio 2019 (v142)，EasyX_20210730
// 作　　者：luyiran <872289455@qq.com>
// 发布日期：2021-8-11
//
#pragma warning(disable:4244)	// 程序会用到浮点数截断特性
#include <cstdio>				// printf
#include <cmath>				// cosf/sinf/sqrtf
#include <utility>				// std::pair
#include <algorithm>			// std::swap
#include <graphics.h>			// ui

// 屏幕大小
#define	WIDTH			1280	// 宽
#define	HEIGHT			720		// 高

// 显示控制
#define	ALL				0		// 全部显示
#define C1C2			1		// 显示 C1 和 C2 点
#define NOC2			2		// 不绘制 C2 点
#define	NOC1			3		// 不绘制 C1 点

// 四杆机构类型
#define	INVALID			-1		// 无效类型
#define	CRANK_ROCKER	0		// 曲柄摇杆机构
#define	DOUBLE_CRANK	1		// 双曲柄机构
#define	ROCKER			2		// 摇杆机构
#define	ROCKER_CRANK	3		// 曲柄摇杆，摇杆为主动件

// 杆长定义
float AB = 45;
float BC = 100;
float CD = 70;
float AD = 120;

// 运动参数
float t = 0;					// 时间
float w = 0.00314159f;			// 角速度
float angle = w * t;			// 角度

// 显示类型
int draw = ALL;					// 全部显示

// 点向量
struct vec2 {
	float x;					// x 坐标
	float y;					// y 坐标
	vec2(float a = 0, float b = 0) : x(a), y(b) {}

	float length() const {
		return sqrtf(x * x + y * y);
	}

	vec2 operator+(const vec2& b) {
		return vec2(x + b.x, y + b.y);
	}

	vec2 operator-(const vec2& b) {
		return vec2(x - b.x, y - b.y);
	}

	vec2 operator*(const float b) {
		return vec2(x * b, y * b);
	}
};

// 圆
struct Circle {
	union {
		struct { float x, y; };	// 圆心，坐标表示
		vec2 p;					// 圆心，向量表示
	};
	float r;					// 半径
	Circle(float x = 0, float y = 0, float radius = 0) : p(x, y), r(radius) {}
	Circle(vec2 center, float radius) : p(center), r(radius) {}

	// 两圆求交
	std::pair<vec2, vec2> intersections(Circle& rhs) {
		float d, a, h;
		d = (p - rhs.p).length();
		a = (r * r - rhs.r * rhs.r + d * d) / (2 * d);
		h = sqrtf(r * r - a * a);
		vec2 P2 = p + (rhs.p - p) * (a / d);
		float x3, y3, x4, y4;
		x3 = P2.x + h * (rhs.y - y) / d;
		y3 = P2.y - h * (rhs.x - x) / d;
		x4 = P2.x - h * (rhs.y - y) / d;
		y4 = P2.y + h * (rhs.x - x) / d;

		return std::pair<vec2, vec2>(vec2(x3, y3), vec2(x4, y4));
	}
};

// 返回四杆机构类型
int linksType(float AB, float AD, float BC, float CD) {
	float minbar = fmin(AB, fmin(AD, fmin(BC, CD)));
	float maxbar = fmax(AB, fmax(AD, fmax(BC, CD)));

	// 最短杆与最长杆之和小于等于其他两杆之和才有效
	if ((minbar + maxbar) > ((AB + AD + BC + CD) / 2)) return INVALID;

	// AB 最短，曲柄摇杆机构
	if (fabs(AB - minbar) < 1e-6) {
		printf("crank-rocker mechanism.\n");
		return CRANK_ROCKER;
	}

	// AD 最短，双曲柄机构
	if (fabs(AD - minbar) < 1e-6) {
		printf("double-crank mechanism.\n");
		return DOUBLE_CRANK;
	}

	// CD 最短，主动件 AB 为摇杆，被动件 CD 为曲柄
	if (fabs(CD - minbar) < 1e-6) {
		printf("double-crank mechanism.\n");
		return ROCKER_CRANK;
	}

	// 摇杆机构，无曲柄
	return ROCKER;
}

// 指定颜色画圆
inline void drawCircle(const Circle& crcl, COLORREF color) {
	setlinecolor(color);
	circle(crcl.x, crcl.y, crcl.r);
}

// 指定颜色画线
inline void drawLine(const vec2& p1, const vec2& p2, COLORREF color) {
	setlinecolor(color);
	line(p1.x, p1.y, p2.x, p2.y);
}

// 绘制四杆机构，AB 主动件，AD 机架，BC 连杆，CD 被动件
void drawMechanism(vec2 A, float AB, float AD, float BC, float CD) {
	int type = linksType(AB, AD, BC, CD);
	vec2 B;
	vec2 D(A.x + AD, A.y);
	Circle atB;
	Circle atA(A, AB);
	Circle atD(D, CD);

	// 两圆交点可能有两个
	std::pair<vec2, vec2> C;
	vec2& C1 = C.first;
	vec2& C2 = C.second;

	float cur;			// 当前实际角度
	int dir = 0;		// 角度增长方向是否更换

	// 无效类型，无转动副
	if (INVALID == type) {
		printf("invalid four-bar linkage!\n");
		return;
	}
	else if (CRANK_ROCKER == type || DOUBLE_CRANK == type) {
		cur = angle;
	}
	else {
		float maxtheta, mintheta;					// 形成极位夹角
		if (ROCKER_CRANK == type) {					// 摇杆为主动件，曲柄为被动件
			maxtheta = acosf((AB * AB + AD * AD - (BC + CD) * (BC + CD)) / (2 * AB * AD));
			mintheta = acosf((AB * AB + AD * AD - (BC - CD) * (BC - CD)) / (2 * AB * AD));
		}
		else {										// 双摇杆
			maxtheta = acosf((AB * AB + AD * AD - (BC + CD) * (BC + CD)) / (2 * AB * AD));
			mintheta = acosf(((AB + BC) * (AB + BC) + AD * AD - CD * CD) / (2 * (AB + BC) * AD));
		}

		float dis = maxtheta - mintheta;			// 极位夹角，主动件活动范围

		float count = angle / (2 * dis);			// 一个往返为一个循环，求出多少趟
		cur = (count - (int)count) * (2 * dis);		// 浮点数截断取余

		cur += mintheta;							// 加上初始偏移得出当前实际位置
		if (cur > maxtheta) {						// 超出最大值则反向
			cur = maxtheta - (cur - maxtheta);
			if (ROCKER_CRANK == type) dir = 1;
		}
	}


	// 求 B 点
	B = A + vec2(cosf(cur), sinf(cur)) * AB;
	atB = Circle(B, BC);
	// 求 C 点
	C = atB.intersections(atD);
	if (dir) std::swap(C.first, C.second);

	// 画圆
	if (ALL == draw) {
		drawCircle(atA, RED);
		drawCircle(atD, GREEN);
		drawCircle(atB, CYAN);
	}

	// 连线
	drawLine(A, B, RED);

	if (NOC2 != draw) {
		drawLine(B, C2, WHITE);
		drawLine(C2, D, WHITE);
		outtextxy(C2.x, C2.y, _T("C2"));
	}

	if (NOC1 != draw) {
		drawLine(B, C1, MAGENTA);
		drawLine(C1, D, MAGENTA);
		outtextxy(C1.x, C1.y, _T("C1"));
	}

	// 标记
	outtextxy(A.x, A.y, _T("A"));
	outtextxy(B.x, B.y, _T("B"));
	outtextxy(D.x, D.y, _T("D"));
}

int main() {
	initgraph(WIDTH, HEIGHT);
	BeginBatchDraw();

	ExMessage msg;
	vec2 A(WIDTH / 4 - AD / 2, HEIGHT / 4);

	while (1) {
		cleardevice();

		if (peekmessage(&msg, EM_MOUSE)) {
			if (WM_LBUTTONDOWN == msg.message) {
				draw = draw == NOC1 ? ALL : ++draw;
			}
		}

		// 曲柄摇杆 I
		drawMechanism(A, AB, AD, BC, CD);
		// 双曲柄
		drawMechanism(A + vec2(WIDTH / 2, 0), BC, AB, AD, CD);
		// 曲柄摇杆 II
		drawMechanism(A + vec2(0, HEIGHT / 2), CD, AD, BC, AB);
		// 双摇杆
		drawMechanism(A + vec2(WIDTH / 2, HEIGHT / 2), BC, CD, AB, AD);

		t++;
		angle = w * t;

		FlushBatchDraw();
	}

	return 0;
}