#include <graphics.h>
#include <easyx.h>
#include <conio.h>
#include<cstdlib>
#include<ctime>
#include<cmath>
#include<cstring>
#include<fstream>
#include <windows.h>
#include<iostream>
#define BLOCK_SIZE 20 // 每个小格子的长宽大小
#define HEIGHT 29 // 高度上一共30个小格子
#define WIDTH 39 // 宽度上一共40个小格子
#pragma comment(lib,"Winmm.lib")

using namespace std;

typedef struct snake //定义蛇身链表
{
    int x, y;
    struct snake* next;
}SList;

int Blocks[WIDTH+1][HEIGHT+1] = {0}; //     二维数组，用于记录所有的游戏地图数据
int game_mode = 3; //   用于选择游戏难度,3简单，2中等，1困难
int moveDirection;  //  小蛇移动方向
bool skill; //本次更新是否使用技能
bool turnable;
int food_i, food_j; //  食物的位置
int isFailure = 0; //  是否游戏失败
int isClose = 0;//  判断是否退出游戏
int MaxScore = 0;//     记录当前最大分数
double score = 0; //    记录得分
int alpha_step = 0;//    记录画面透明度
int paly_genshin_music_time = 1;
SList* G_head; //   用于储存蛇头信息
IMAGE img_start,img_login,img_select_diff,genshin,white;//    需要用到的五张图片
int which_image = 1; //播放哪一张图片作为遮罩

void StrToCh(char *ch , string str)//用于将字符串(string)转换成字符(char)
{
    int i;
    for (i = 0; i < str.length() ; i++) {
        *(ch + i) = str[i];
    }
    *(ch+i) = '\0';
}
// 绘图函数，补充透明度 AA
void drawAlpha(IMAGE* image, int x, int y, int width, int height, int pic_x, int pic_y, double AA = 1)//AA从0到1
{
    // 变量初始化
    DWORD* dst = GetImageBuffer();			// GetImageBuffer() 函数，用于获取绘图设备的显存指针
    DWORD* draw = GetImageBuffer();
    DWORD* src = GetImageBuffer(image);		// 获取 picture 的显存指针
    int imageWidth = image->getwidth();		// 获取图片宽度
    int imageHeight = image->getheight();	// 获取图片宽度
    int dstX = 0;							// 在 绘图区域 显存里像素的角标

    // 实现透明贴图 公式： Cp=αp*FP+(1-αp)*BP ， 贝叶斯定理来进行点颜色的概率计算
    for (int iy = 0; iy < height; iy++)
    {
        for (int ix = 0; ix < width; ix++)
        {
            // 防止越界
            if (ix + pic_x >= 0 && ix + pic_x < imageWidth && iy + pic_y >= 0 && iy + pic_y < imageHeight &&
                ix + x >= 0 && ix + x < (WIDTH+1)*BLOCK_SIZE && iy + y >= 0 && iy + y < (HEIGHT+1)*BLOCK_SIZE)
            {
                // 获取像素角标
                int srcX = (ix + pic_x) + (iy + pic_y) * imageWidth;
                dstX = (ix + x) + (iy + y) * (WIDTH+1)*BLOCK_SIZE;

                int sa = ((src[srcX] & 0xff000000) >> 24) * AA;			// 0xAArrggbb; AA 是透明度
                int sr = ((src[srcX] & 0xff0000) >> 16);				// 获取 RGB 里的 R
                int sg = ((src[srcX] & 0xff00) >> 8);					// G
                int sb = src[srcX] & 0xff;								// B

                // 设置对应的绘图区域像素信息
                int dr = ((dst[dstX] & 0xff0000) >> 16);
                int dg = ((dst[dstX] & 0xff00) >> 8);
                int db = dst[dstX] & 0xff;
                draw[dstX] = ((sr * sa / 255 + dr * (255 - sa) / 255) << 16)  //公式： Cp=αp*FP+(1-αp)*BP  ； αp=sa/255 , FP=sr , BP=dr
                             | ((sg * sa / 255 + dg * (255 - sa) / 255) << 8)         //αp=sa/255 , FP=sg , BP=dg
                             | (sb * sa / 255 + db * (255 - sa) / 255);              //αp=sa/255 , FP=sb , BP=db
            }
        }
    }
}

void play_music()//播放音乐
{
    mciSendString(_T("open bgm.mp3 alias bkmusic"), NULL, 0, NULL);//加载音乐
    mciSendString(_T("open out.mp3 alias outmusic"), NULL, 0, NULL);
    mciSendString(_T("open genshin.mp3 alias gsmusic"), NULL, 0, NULL);
    mciSendString("setaudio bkmusic volume to 400", NULL, 0, NULL);
    mciSendString("setaudio outmusic volume to 1000", NULL, 0, NULL);
    mciSendString("setaudio gsmusic volume to 400", NULL, 0, NULL);
}

void image_loaded()//加载需要用的图片
{
    loadimage(&img_start,_T("game_start.png"));
    loadimage(&img_login,_T("game_login.png"));
    loadimage(&img_select_diff,_T("game_select.jpg"));
    loadimage(&genshin,_T("genshin.png"));
    loadimage(&white,_T("white.png"));
}

void save_info(const string& username,const string& password)//注册时,将信息输入至txt文本文档中
{
    ofstream psFile(R"(login_information.txt)",ios::app);
    if(psFile.is_open() && (!username.empty()) && (!password.empty())){
        psFile << username << "\n";
        psFile << password << "\n";
        psFile.close();
    }
}

bool read_info(const string& username,const string& password)//从txt文本文档中，验证登录信息
{
    string read_username , read_password;
    ifstream prFile(R"(login_information.txt)");

    while(!(prFile.eof())) {
        prFile >> read_username;
        prFile >> read_password;
        if(read_username == username && read_password == password){
            return true;
        }
    }
    return false;
}

void save_MaxScore()//一次游戏后，写入最大分数
{
    char abuf[8] = {0};
    int ibuf = 0;
    FILE* pFile;
    pFile = fopen(R"(MaxScore_information.txt)","r");
    if(pFile == NULL){
        perror("open file failed:");
        return;
    }
    fgets(abuf, sizeof(abuf), pFile);
    ibuf = atoi(abuf);
    fclose(pFile);
    if(MaxScore > ibuf){
        pFile = fopen(R"(MaxScore_information.txt)","w");
        fprintf(pFile,"%d\n",MaxScore);
        fclose(pFile);
    }
    return;
}

int read_MaxScore()//从文件中读取最大分数
{
    FILE* pFile;
    char bufS[32] = {0};
    int int_bufS = 0;
    int MaxS = 0;
    pFile = fopen(R"(MaxScore_information.txt)","r");
    if(pFile == NULL){
        perror("open file failed:");
    }
    while(!feof(pFile)){
        fgets(bufS,sizeof(bufS),pFile);
        int_bufS = atoi(bufS);
        if(int_bufS > MaxS){
            MaxS = int_bufS;
        }
    }

    fclose(pFile);
    return MaxS;
}

void login_window_show_ch(const string& password, const string& username, int situation)//更新登陆界面，并循环打印
{
    cleardevice();
    setfillcolor(BLACK);
    settextstyle(35, 0, _T("宋体"));
    putimage(0,0,&img_login);
    char c_usern[8] = {'\0'};
    StrToCh(c_usern,username);

    if(situation == 1){
        fillrectangle(320,380,520,430);
        if(!password.empty()){
            settextcolor(BLACK);
            for(int i = 0; i < password.length() ; i++){
                outtextxy(325 + i*20, 462, _T('*'));
            }
        }
        if(!username.empty()){
            settextcolor(WHITE);
            outtextxy(325, 382, _T(c_usern));
        }
    }
    else if(situation == 2){
        fillrectangle(320,460,520,510);
        if(!password.empty()){
            settextcolor(WHITE);
            for(int i = 0; i < password.length() ; i++){
                outtextxy(325 + i*20, 462, _T('*'));
            }
        }
        if(!username.empty()){
            settextcolor(BLACK);
            outtextxy(325, 382, _T(c_usern));
        }
    }
    else if(situation == 0)
    {
        if(!password.empty()){
            settextcolor(BLACK);
            for(int i = 0; i < password.length() ; i++){
                outtextxy(325 + i*20, 462, _T('*'));
            }
        }
        if(!username.empty()){
            settextcolor(BLACK);
            outtextxy(325, 382, _T(c_usern));
        }
    }
    FlushBatchDraw();
}

void login_window()//显示可交互界面——注册与登陆界面
{
    string show_USERNAME;
    string show_PASSWORD;

    char get_from_kb = '0';
    bool login_re;
    int situ = 0;
    ExMessage msg{};
    setfillcolor(BLACK);
    BeginBatchDraw();
    login_window_show_ch(show_PASSWORD,show_USERNAME,situ);

    while(true){
        msg = getmessage(EX_MOUSE);
        login_window_show_ch(show_PASSWORD,show_USERNAME,situ);
        if(msg.message == WM_LBUTTONDOWN && (msg.x >= 320) && (msg.x <= 520) && (msg.y >= 380) && (msg.y <= 420)){
            situ = 1;
            while(1){
                login_window_show_ch(show_PASSWORD,show_USERNAME,situ);
                get_from_kb = _getch();
                if(get_from_kb == 8){
                    show_USERNAME = show_USERNAME.substr(0, show_USERNAME.length() - 1);
                }
                else if(get_from_kb == 13){
                    break;
                }
                else {
                    show_USERNAME += get_from_kb;
                }
            }
            situ = 0;
        }

        if(msg.message == WM_LBUTTONDOWN && (msg.x >= 320) && (msg.x <= 520) && (msg.y >= 455) && (msg.y <= 500)){
            situ = 2;
            while(1){
                login_window_show_ch(show_PASSWORD,show_USERNAME,situ);
                get_from_kb = _getch();
                if(get_from_kb == 8){
                    show_PASSWORD = show_PASSWORD.substr(0, show_PASSWORD.length() - 1);
                }
                else if(get_from_kb == 13){
                    break;
                }
                else
                    show_PASSWORD += get_from_kb;
            }
            situ = 0;
        }

        if(msg.message == WM_LBUTTONDOWN && (msg.x >= 200) && (msg.x <= 280) && (msg.y >= 420) && (msg.y <= 460)){
            settextcolor(RGB(0, 0, 0));
            settextstyle(50, 0, _T("宋体"));
            if(show_USERNAME.empty()){
                outtextxy(140, 220, _T("请输入用户名"));
            }
            else if(show_PASSWORD.empty()){
                outtextxy(140, 220, _T("请输入密码"));
            }
            else{
                login_re = read_info(show_USERNAME,show_PASSWORD);
                if(!login_re){
                    outtextxy(140, 220, _T("无账号信息，请注册！"));
                }
                else {
                    outtextxy(140, 220, _T("登录成功！3秒后开始游戏"));
                    FlushBatchDraw();
                    Sleep(2000);
                    break;
                }
            }
            FlushBatchDraw();
            Sleep(2000);
            cleardevice();
            putimage(0,0,&img_login);
        }

        if(msg.message == WM_LBUTTONDOWN && (msg.x >= 560) && (msg.x <= 640) && (msg.y >= 420) && (msg.y <= 460)){
            settextcolor(RGB(0, 0, 0));
            settextstyle(50, 0, _T("宋体"));
            if(show_USERNAME.empty()){
                outtextxy(140, 220, _T("请输入用户名"));
            }
            else if(show_PASSWORD.empty()){
                outtextxy(140, 220, _T("请输入密码"));
            }
            else{
                save_info(show_USERNAME, show_PASSWORD);
                outtextxy(140, 220, _T("注册成功！请登录！"));
            }
            FlushBatchDraw();
            Sleep(2000);
            cleardevice();
            putimage(0,0,&img_login);
        }

    }
    EndBatchDraw();
    return;
}

void select_diff_windows()//显示可交互界面——选择难度界面
{
    cleardevice();
    ExMessage msg{};
    putimage(0,0,&img_select_diff);
    while(true){
        msg = getmessage(EX_MOUSE);
        if(msg.message == WM_LBUTTONDOWN){
            if((msg.x >= 360) && (msg.x <= 510) && (msg.y >= 350) && (msg.y <= 400)){
                game_mode = 3;
                break;
            }
            else if((msg.x >= 360) && (msg.x <= 510) && (msg.y >= 430) && (msg.y <= 480)){
                game_mode = 2;
                break;
            }
            else if((msg.x >= 360) && (msg.x <= 510) && (msg.y >= 510) && (msg.y <= 560)){
                game_mode = 1;
                break;
            }
        }
    }
    login_window();
    return ;
}

void hello_window()//显示可交互界面——欢迎界面
{
    mciSendString(_T("resume bkmusic"), NULL, 0, NULL);//继续播放背景音乐
    ExMessage msg{};
    char mMaxScore[20] = {0};
    itoa(MaxScore,mMaxScore,10);
    settextcolor(RGB(255, 0, 0));
    settextstyle(40, 0, _T("宋体"));
    putimage(0,0,&img_start);
    setbkmode(TRANSPARENT);
    outtextxy(0, 0, _T("Max Score:"));
    outtextxy(200, 2, _T(mMaxScore));
    while(true){
        msg = getmessage(EX_MOUSE);
        if(msg.message == WM_LBUTTONDOWN){
            if((msg.x >= 340) && (msg.x <= 510) && (msg.y >= 380) && (msg.y <= 420)){
                break;
            }
            if((msg.x >= 350) && (msg.x <= 500) && (msg.y >= 520) && (msg.y <= 570)){
                isClose = 1;
                return;
            }
        }
    }
    select_diff_windows();
    return;
}

SList* first_create(int n) //新建蛇身，初次建立n个像素长的蛇身
{
    SList* Sr, *Sp;
    SList* head = (SList*)malloc(sizeof(SList));
    head->x = WIDTH/2;
    head->y = HEIGHT/2;
    head->next = NULL;
    Sr = head;

    for (int i = 1; i <= n; i++) {
        Sp = (SList*)malloc(sizeof(SList));
        Sp->x = Sr->x + 1;
        Sp->y = Sr->y;
        Sr->next = Sp;
        Sr = Sp;
    }
    Sr->next = NULL;
    return head;
}

void create_head(int newHead_i,int newHead_j) //在小蛇蛇头碰到果实的情况下，更新蛇身
{
    SList* Np = (SList*)malloc(sizeof(SList));
    Np->x = newHead_i;
    Np->y = newHead_j;
    Np->next = G_head;
    G_head = Np;
}

void refresh(int newHead_i, int newHead_j) //在小蛇蛇头没有碰到果实的情况下，更新蛇身
{
    SList* p = G_head;
    int number = 0;
    while (p->next != nullptr && p->next->next) {
        number++;
        p = p->next;
    }
    free(p->next);
    p->next = NULL;
    create_head(newHead_i, newHead_j);
}

void free_list(SList *p) //游戏结束后，清除小蛇链表数据
{
    SList* temp;
    while (p) {
        temp = p;
        p = p->next;
        free(temp);
    }
}

void move_snake() //  移动小蛇及相关处理函数
{
    SList* p = G_head;
    int i = 0;
    int oldHead_i=-1, oldHead_j=-1; // 定义变量，存储旧蛇头坐标
    oldHead_i = p->x;
    oldHead_j = p->y;

    for(int j = 0 ; j <= HEIGHT ; j++){
        for(int k = 0; k <= WIDTH ; k++){
            Blocks[k][j] = 0 ;
        }
    }

    while (p != nullptr) {
        i++;
        Blocks[p->x][p->y] = i;
        p = p->next;
    }

    int newHead_i = oldHead_i; //  设定变量存储新蛇头的位置
    int newHead_j = oldHead_j;

    //根据用户按键，设定新蛇头的位置
    switch(moveDirection){
        case 'w':// 向上移动
            newHead_j = oldHead_j - 1;
            break;
        case 's':// 向下移动
            newHead_j = oldHead_j + 1;
            break;
        case 'a':// 向左移动
            newHead_i = oldHead_i - 1;
            break;
        case 'd':// 向右移动
            newHead_i = oldHead_i + 1;
            break;
    }
    if(skill == true){
        if(score < 5){
            isFailure = 1; //  游戏失败
            return; // 函数返回
        }
        score = score - 1;
    }
    //  如果蛇头超出边界，或者蛇头碰到蛇身，游戏失败
    if (newHead_i > WIDTH || newHead_i < 0 || newHead_j > HEIGHT || newHead_j < 0 ||
        Blocks[newHead_i][newHead_j] > 0 || (alpha_step == 10 && which_image == 0)){
        if (!skill) {
            isFailure = 1; //  游戏失败
            return; // 函数返回
        }

    }
    if (newHead_i == food_i && newHead_j == food_j && !skill) //  如果新蛇头正好碰到食物
    {
        score = score + round(exp(4 - game_mode) * i);
        create_head(newHead_i, newHead_j);
        do {
            food_i = rand() % (WIDTH - 1) + 2; //  食物重新随机位置
            food_j = rand() % (HEIGHT - 1) + 2;
        } while ((Blocks[food_i][food_j] != 0) && (food_i != newHead_i) && (food_j != newHead_i));
    }
    else {
        refresh(newHead_i, newHead_j);
    }
}

void startup_play()  //  游戏界面初始化函数
{
    G_head = first_create(3);
    moveDirection = 's';	 //  初始向左移动
    food_i = rand() % (WIDTH - 5) + 2; //  初始化随机食物位置
    food_j = rand() % (HEIGHT - 5) + 2; //
}

void show()  // 绘制游戏界面
{
    string score_str = to_string(int(score));
    BeginBatchDraw();
    setfillcolor(BLACK);
    cleardevice(); // 清屏
    int i, j;
    for (i = 0; i < WIDTH+1; i++) //  对二维数组所有元素遍历
    {
        for (j = 0; j < HEIGHT+1; j++)
        {
            if((Blocks[i][j] != 0) && (skill == true))// 元素不为0且使用无敌技能，可以蛇身变为黑色
                setfillcolor(RGB(0,0,0));
            else if (Blocks[i][j] != 0) // 元素不为0表示是蛇，这里让蛇的身体颜色色调渐变
                setfillcolor(HSVtoRGB((Blocks[i][j]) * 15, exp(-0.1 * Blocks[i][j]), 1));
            else
                setfillcolor(RGB(150, 150, 150)); // 元素为0表示为空，颜色为灰色
            fillrectangle(i * BLOCK_SIZE, j * BLOCK_SIZE,(i + 1) * BLOCK_SIZE, (j + 1) * BLOCK_SIZE);// 在对应位置处，以对应颜色绘制小方格
        }
    }
    setfillcolor(RGB(0, 255, 0)); //  食物为绿色
    fillrectangle(food_i * BLOCK_SIZE, food_j * BLOCK_SIZE,(food_i + 1) * BLOCK_SIZE, (food_j + 1) * BLOCK_SIZE);//  绘制食物小方块
    settextcolor(RGB(255, 0, 0));// 设定文字颜色
    settextstyle(30, 0, _T("宋体")); //  设定文字大小、样式
    outtextxy(0, 0, _T(("your score:" + score_str).c_str()));
    if(which_image == 0) {
        if(paly_genshin_music_time == 1){
            mciSendString("seek outmusic to start",NULL,0,NULL);
            mciSendString("play gsmusic",NULL,0,NULL);
        }
        paly_genshin_music_time++;
        drawAlpha(&genshin, 0, 0, (WIDTH + 1) * BLOCK_SIZE, (HEIGHT + 1) * BLOCK_SIZE, 0, 0, 0.1 * alpha_step);
    }
    else {
        paly_genshin_music_time = 1;
        mciSendString("pause gsmusic",NULL,0,NULL);
        drawAlpha(&white, 0, 0, (WIDTH + 1) * BLOCK_SIZE, (HEIGHT + 1) * BLOCK_SIZE, 0, 0, 0.1 * alpha_step);
    }
    if (isFailure) //  如果游戏失败
    {
        setbkmode(TRANSPARENT); // 文字字体透明
        settextcolor(RGB(255, 0, 0));// 设定文字颜色
        mciSendString("seek outmusic to start",NULL,0,NULL);
        mciSendString(_T("play outmusic"), NULL, 0, NULL);
        settextstyle(80, 0, _T("宋体")); //  设定文字大小、样式
        outtextxy(140, 220, _T("GAME OVER!!"));
        settextstyle(35, 0, _T("宋体"));
        outtextxy(140, 300, _T("点击任意位置回到开始界面")); //  输出文字内容
    }
    FlushBatchDraw(); // 批量绘制
}

void update_WithoutInput()// 与输入无关的更新函数
{
    if (isFailure == 1) //  如果游戏失败，函数返回
        return;
    static int waitIndex = 1; // 静态局部变量，初始化时为1
    static int waitImg = 1; // 静态局部变量，初始化时为1
    waitIndex++; // 每一帧+1
    waitImg++;// 每一帧+1
    if (waitIndex == game_mode * 5) // 这样小蛇每隔几帧移动一次，移动速度与游戏难度有关
    {
        move_snake(); //  调用小蛇移动函数
        waitIndex = 1; // 再变成1
        turnable = true;
    }
    if(waitImg == game_mode * 10){
        waitImg = 1;
        if (alpha_step == 10) {
            alpha_step = 0;
            which_image = rand() % 3;
        }
        else
            alpha_step++;
    }
}

void update_WithInput()  // 和输入有关的更新函数
{
    if (_kbhit() && (isFailure == 0))  //  如果有按键输入，并且不失败
    {
        if (!turnable) return;
        int input = getch(); //  获得按键输入
        turnable = false;
        if (!((input == 'a' && moveDirection == 'd') || (input == 'd' && moveDirection == 'a') || (input == 'w' && moveDirection == 's') || (input == 's' && moveDirection == 'w') )) // 如果是asdw且与当前方向不是相反
        {
            if (input != 'e')
                moveDirection = input;  // 设定移动方向
        }
        if(input == 'e'){
            skill = !skill;
        }
    }
}

void program_init()//程序开始前的数据初始化
{
    play_music();
    image_loaded();
    mciSendString(_T("play bkmusic repeat"), NULL, 0, NULL);//循环播放背景音乐
}

void game_init()//游戏开始前的数据初始化
{
    game_mode = 3;
    skill = false;
    turnable = true;
    moveDirection = 'a';
    isFailure = 0;
    isClose = 0;
    score = 0;
    MaxScore = read_MaxScore();
    alpha_step = 0;
    paly_genshin_music_time = 1;
    which_image = 1;
    initgraph((WIDTH + 1)* BLOCK_SIZE, (HEIGHT + 1)* BLOCK_SIZE); //  新开画面
    setlinecolor(RGB(200, 200, 200)); // 设置线条颜色
    mciSendString("pause gsmusic",NULL,0,NULL);
    mciSendString("seek gsmusic to start",NULL,0,NULL);

}

void play()//游戏过程中的函数调用
{
    if(isClose){
        return;
    }
    startup_play();  // 初始化函数，仅执行一次
    mciSendString(_T("pause bkmusic"), NULL, 0, NULL);//停止播放背景音乐
    while (isFailure == 0)   // 循环
    {
        srand((unsigned)time(NULL));
        update_WithInput();    // 和输入有关的更新
        update_WithoutInput(); // 和输入无关的更新
        show();  // 进行绘制
    }
    ExMessage msg{};
    while(true){
        msg = getmessage(EX_MOUSE);
        if(msg.message == WM_LBUTTONDOWN){
            break;
        }
    }
    EndBatchDraw();
    free_list(G_head);
    if(score > MaxScore) {
        MaxScore = score;
    }
    save_MaxScore();
    closegraph();
}

int main() //  主函数
{
    program_init();
    while(isClose == 0) {
        srand((unsigned) time(NULL));
        game_init();
        hello_window();
        play();
    }
    return 0;
}