// Catch The Eggs - Compact Edition (<=1500 lines)
// Build: g++ "Catch The Eggs Game Compact.cpp" -o "Catch The Eggs Game Compact.exe" -lfreeglut -lopengl32 -lglu32 -lwinmm
#include <windows.h>
#include <mmsystem.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
using namespace std;

#define PI 3.14159265358979f
#define NUM_LEVELS 6
#define V2(x,y) glVertex2f(x,y)
#define C3(r,g,b) glColor3f(r,g,b)
#define C4(r,g,b,a) glColor4f(r,g,b,a)
#define F18 GLUT_BITMAP_HELVETICA_18
#define F12 GLUT_BITMAP_HELVETICA_12
#define FTR GLUT_BITMAP_TIMES_ROMAN_24

// === 2. Enums + structs ===
enum GameState { MENU, LEVEL_SELECT, PLAYING, PAUSED, HELP, GAMEOVER };
enum ObjType {
    EGG_NORMAL=0, EGG_BLUE, EGG_GOLD, POOP, BOMB,
    PERK_LARGE, PERK_SLOW, PERK_EXTRA, PERK_DOUBLE, PERK_MAGNET, PERK_SHIELD,
    DEB_SMALL, DEB_REVERSE, DEB_FREEZE, OT_COUNT
};
enum { EFX_LARGE=0, EFX_SMALL, EFX_SLOW, EFX_DOUBLE, EFX_MAGNET, EFX_REVERSE, EFX_FREEZE, EFX_COUNT };

struct FObj  { float x,y,dx,dy,r,spin; ObjType t; bool act; };
struct Chick { float x,y,sp,dt0,di,bp; int dir,si; };
struct Cloud { float x,y,sp,sc; };
struct Part  { float x,y,dx,dy,life,maxlife,r,g,b,sz; };
struct FText { float x,y,vy,life,maxlife,r,g,b; char s[32]; void* fn; };
struct Basket{ float x,y,w,h,bw; };
struct Lvl   { const char* name; int sticks,chicks; float speed,spawn,wind; int poop,bomb; float time,mult; int theme; };

// === 3. Globals ===
int winW=1280, winH=720;
GameState gs=MENU;
int curLvl=0;
int score=0, hi[NUM_LEVELS]={0,0,0,0,0,0}, combo=0, maxCombo=0;
float timeLeft=60, totalTime=60;
int lastT=0;
bool keys[256]={0}, sk[256]={0};
float wind=0, windT=0;
float effT[EFX_COUNT]={0,0,0,0,0,0,0};
int shield=0;
float shake=0, flash=0;
bool newHi=false;
Basket bk;
vector<FObj> objs;
vector<float> sticks;
vector<Chick> chicks;
vector<Cloud> clouds;
vector<Part> parts;
vector<FText> ftxts;
int hov=-1;
bool bgmOn=false;

void resetGame();
void saveHiScore();
void loadHiScore();
void spawnClouds();
inline bool effOn(int i){ return effT[i] > 0; }

// === 4. Level table ===
Lvl levels[NUM_LEVELS] = {
    {"Lost Tourist",          1,1,120,2.0f,  0,  10, 0,60,1.00f,0},
    {"Casual Farmer",         2,2,180,1.5f, 25,  15, 0,60,1.25f,1},
    {"Skilled Harvester",     2,3,240,1.3f, 45,  20, 5,75,1.50f,2},
    {"Sleep-Deprived Genius", 3,4,280,1.1f, 75,  25,10,75,1.75f,3},
    {"Mad Farmer",            4,6,330,0.9f,110,  30,15,90,2.00f,4},
    {"Ruthless Farmer",       5,8,420,0.5f,160,  35,25,90,3.00f,5}
};

// === 5. Drawing primitives ===
void drawEllipse(float cx,float cy,float rx,float ry,int seg){
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++){ float t=2*PI*i/seg; V2(cx+rx*cosf(t),cy+ry*sinf(t)); }
    glEnd();
}
#define drawCircle(cx,cy,r,seg) drawEllipse(cx,cy,r,r,seg)

void drawRing(float cx,float cy,float rIn,float rOut,int seg){
    glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=seg;i++){ float t=2*PI*i/seg, c=cosf(t), s=sinf(t); V2(cx+rIn*c,cy+rIn*s); V2(cx+rOut*c,cy+rOut*s); }
    glEnd();
}
void drawArcWedge(float cx,float cy,float r,float a0,float a1,int seg){
    glBegin(GL_TRIANGLE_FAN); V2(cx,cy);
    for(int i=0;i<=seg;i++){ float t=a0+(a1-a0)*i/seg; V2(cx+r*cosf(t),cy+r*sinf(t)); }
    glEnd();
}
void drawRect(float x,float y,float w,float h){
    glBegin(GL_POLYGON); V2(x,y); V2(x+w,y); V2(x+w,y+h); V2(x,y+h); glEnd();
}
void drawGrad(float x,float y,float w,float h,float r1,float g1,float b1,float r2,float g2,float b2){
    glBegin(GL_QUADS);
    C3(r1,g1,b1); V2(x,y); V2(x+w,y);
    C3(r2,g2,b2); V2(x+w,y+h); V2(x,y+h);
    glEnd();
}
void drawRR(float x,float y,float w,float h,float r){
    if(r*2>w) r=w*0.5f; if(r*2>h) r=h*0.5f;
    drawRect(x+r,y,w-2*r,h);
    drawRect(x,y+r,w,h-2*r);
    drawCircle(x+r,y+r,r,16);
    drawCircle(x+w-r,y+r,r,16);
    drawCircle(x+r,y+h-r,r,16);
    drawCircle(x+w-r,y+h-r,r,16);
}
int textW(const char* s,void* fn){ int w=0; for(const char*p=s;*p;p++) w+=glutBitmapWidth(fn,*p); return w; }
void drawText(const char* s,float x,float y,void* fn,float r,float g,float b){
    C3(r,g,b); glRasterPos2f(x,y);
    for(const char*p=s;*p;p++) glutBitmapCharacter(fn,*p);
}
void drawTextC(const char* s,float cx,float y,void* fn,float r,float g,float b){
    drawText(s,cx-textW(s,fn)/2.0f,y,fn,r,g,b);
}

// === 6. Audio (per-level BGM + Thunder SFX) ===
struct BgmConf { float nd, dist, noise, sub; int mel8[8]; int chord[4]; int bassOff; };
static const BgmConf bgmConfs[NUM_LEVELS] = {
    {0.28f, 0.00f, 0.00f, 0.0f, {0,4,7,12, 7,4,0,4},  {60,65,67,60}, -12},
    {0.25f, 0.05f, 0.00f, 0.0f, {0,4,7,11, 7,4,2,0},  {65,70,67,72}, -12},
    {0.22f, 0.10f, 0.00f, 0.1f, {0,3,7,10, 5,7,3,0},  {62,67,65,70}, -12},
    {0.20f, 0.22f, 0.02f, 0.2f, {0,3,7,12, 3,10,7,3}, {57,62,65,60}, -12},
    {0.18f, 0.42f, 0.05f, 0.4f, {0,1,5,7,  1,8,5,3},  {52,55,53,50}, -12},
    {0.14f, 0.70f, 0.14f, 0.7f, {0,3,6,9,  6,1,3,6},  {46,49,52,55}, -12}
};
inline float midiHz(int m){ return 440.0f*powf(2.0f,(m-69)/12.0f); }
void writeWavHdr(FILE* f,int sr,int ns){
    int byteRate=sr*2, dataSize=ns*2, fileSize=36+dataSize, fmtSize=16;
    short fmt=1, chans=1, bps=16, ba=2;
    fwrite("RIFF",1,4,f); fwrite(&fileSize,4,1,f); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); fwrite(&fmtSize,4,1,f); fwrite(&fmt,2,1,f); fwrite(&chans,2,1,f);
    fwrite(&sr,4,1,f); fwrite(&byteRate,4,1,f); fwrite(&ba,2,1,f); fwrite(&bps,2,1,f);
    fwrite("data",1,4,f); fwrite(&dataSize,4,1,f);
}
void generateBGM(const char* path, const BgmConf& c){
    FILE* f=fopen(path,"rb");
    if(f){ fseek(f,0,SEEK_END); long sz=ftell(f); fclose(f); if(sz>1000) return; }
    f=fopen(path,"wb"); if(!f){ fprintf(stderr,"BGM write fail %s\n",path); return; }
    const int sr=22050;
    const int spn=(int)(sr*c.nd), nn=32, ns=spn*nn;
    writeWavHdr(f,sr,ns);
    for(int i=0;i<ns;i++){
        int ni=i/spn; float tl=(float)(i%spn)/sr, t=(float)i/sr;
        float att=0.012f, rel=0.05f, env;
        if(tl<att) env=tl/att;
        else if(tl>c.nd-rel) env=(c.nd-tl)/rel;
        else env=1.0f;
        int chordRoot=c.chord[ni/8];
        int melNote=c.mel8[ni%8]+chordRoot;
        int bassNote=chordRoot+c.bassOff;
        float mf=midiHz(melNote), bf=midiHz(bassNote);
        float mw=0.32f*sinf(2*PI*mf*t)+0.14f*sinf(4*PI*mf*t);
        if(c.dist>0) mw += c.dist*(0.14f*sinf(6*PI*mf*t)+0.09f*sinf(8*PI*mf*t));
        float bw=0.30f*sinf(2*PI*bf*t);
        float sw=c.sub*0.35f*sinf(PI*bf*t);
        float nz=c.noise*(((float)rand()/RAND_MAX)*2.0f-1.0f);
        float s=(mw*env+bw*env*0.7f+sw*env*0.8f+nz*env)*0.55f;
        if(c.dist>0) s=tanhf(s*(1.0f+c.dist*2.0f));
        if(s>1) s=1; if(s<-1) s=-1;
        short samp=(short)(s*22000); fwrite(&samp,2,1,f);
    }
    fclose(f);
}
void generateThunder(const char* path){
    FILE* f=fopen(path,"rb");
    if(f){ fseek(f,0,SEEK_END); long sz=ftell(f); fclose(f); if(sz>1000) return; }
    f=fopen(path,"wb"); if(!f) return;
    const int sr=22050; const float dur=1.8f; const int ns=(int)(sr*dur);
    writeWavHdr(f,sr,ns);
    float lp=0;
    for(int i=0;i<ns;i++){
        float t=(float)i/sr;
        float wn=((float)rand()/RAND_MAX)*2.0f-1.0f;
        float crack=(t<0.14f)? wn*(1.0f-t/0.14f)*0.9f : 0.0f;
        lp = lp*0.96f + wn*0.04f;
        float rumbleEnv=expf(-t*1.2f);
        float rumble=lp*rumbleEnv*1.6f;
        float subSweep=0.55f*sinf(2*PI*(70.0f-30.0f*t/dur)*t)*rumbleEnv;
        float s=(crack+rumble+subSweep)*0.7f;
        if(s>1) s=1; if(s<-1) s=-1;
        short samp=(short)(s*28000); fwrite(&samp,2,1,f);
    }
    fclose(f);
}
int curBgmLvl=-1;
void startBGM(int lvl){
    if(lvl<0||lvl>=NUM_LEVELS) return;
    if(curBgmLvl==lvl && bgmOn) return;
    mciSendStringA("close bgm",NULL,0,NULL);
    bgmOn=false;
    char fname[64]; sprintf(fname,"catcheggs_bgm_%d.wav",lvl);
    char full[MAX_PATH]={0};
    GetFullPathNameA(fname,MAX_PATH,full,NULL);
    char cmd[MAX_PATH+64];
    sprintf(cmd,"open \"%s\" type waveaudio alias bgm",full);
    MCIERROR e=mciSendStringA(cmd,NULL,0,NULL);
    if(e!=0){
        char buf[256]={0}; mciGetErrorStringA(e,buf,sizeof(buf));
        fprintf(stderr,"BGM open fail (%lu): %s | %s\n",(unsigned long)e,buf,full);
        return;
    }
    mciSendStringA("setaudio bgm volume to 1000",NULL,0,NULL);
    e=mciSendStringA("play bgm",NULL,0,NULL);
    if(e!=0){ fprintf(stderr,"BGM play fail %lu\n",(unsigned long)e); return; }
    bgmOn=true; curBgmLvl=lvl;
}
void updateBGM(){
    if(!bgmOn) return;
    char st[64]={0};
    mciSendStringA("status bgm mode",st,sizeof(st),NULL);
    if(st[0]==0 || strstr(st,"stopped") || strstr(st,"not ready")){
        mciSendStringA("seek bgm to start",NULL,0,NULL);
        mciSendStringA("play bgm",NULL,0,NULL);
    }
}
void playSfx(int k){
    static const char* names[5]={"SystemDefault","SystemAsterisk","SystemHand","SystemExclamation","SystemQuestion"};
    if(k<0||k>4) return;
    PlaySoundA(names[k],NULL,SND_ALIAS|SND_ASYNC);
}
void playThunder(){
    PlaySoundA("catcheggs_thunder.wav",NULL,SND_FILENAME|SND_ASYNC);
}

// === 7. Scenery ===
static const float skyTheme[6][6] = {
    {0.55f,0.80f,0.95f, 0.90f,0.97f,1.00f},
    {0.45f,0.70f,0.92f, 0.98f,0.88f,0.72f},
    {0.52f,0.58f,0.66f, 0.82f,0.85f,0.88f},
    {0.14f,0.16f,0.26f, 0.40f,0.42f,0.52f},
    {0.04f,0.04f,0.13f, 0.22f,0.17f,0.38f},
    {0.14f,0.02f,0.05f, 0.42f,0.08f,0.10f}
};
void drawSkyTheme(int t){
    if(t<0||t>5) t=0;
    const float* p=skyTheme[t];
    drawGrad(0,0,(float)winW,(float)winH,p[0],p[1],p[2],p[3],p[4],p[5]);
}
void drawSun(float cx,float cy,float r){
    float t=glutGet(GLUT_ELAPSED_TIME)*0.0004f;
    C3(1.0f,0.92f,0.5f); glLineWidth(3.0f);
    glBegin(GL_LINES);
    for(int i=0;i<12;i++){
        float a=i*(PI/6.0f)+t;
        V2(cx+(r+8)*cosf(a),cy+(r+8)*sinf(a));
        V2(cx+(r+30)*cosf(a),cy+(r+30)*sinf(a));
    }
    glEnd(); glLineWidth(1.0f);
    C3(1.0f,0.82f,0.15f); drawCircle(cx,cy,r,30);
    C3(1.0f,0.95f,0.45f); drawCircle(cx,cy,r*0.72f,24);
}
// kind: 0=moon, 1=blood
void drawMoonLike(float cx,float cy,float r,int kind){
    if(kind==0){ C3(0.95f,0.95f,0.85f); drawCircle(cx,cy,r,30); C3(0.80f,0.80f,0.70f); }
    else       { C3(0.85f,0.15f,0.15f); drawCircle(cx,cy,r,32); C3(0.58f,0.08f,0.10f); }
    drawCircle(cx-r*0.28f,cy+r*0.22f,r*(kind?0.18f:0.15f),14);
    drawCircle(cx+r*0.30f,cy-r*0.18f,r*(kind?0.12f:0.10f),12);
    drawCircle(cx-r*0.05f,cy-r*0.32f,r*(kind?0.10f:0.08f),10);
    glEnable(GL_BLEND);
    if(kind==0){ C4(1,1,0.85f,0.18f); drawCircle(cx,cy,r*1.5f,30); }
    else       { C4(1.0f,0.22f,0.15f,0.22f); drawCircle(cx,cy,r*1.7f,30);
                 C4(1.0f,0.10f,0.05f,0.14f); drawCircle(cx,cy,r*2.2f,30); }
    glDisable(GL_BLEND);
}
void drawStars(){
    unsigned saved=(unsigned)rand(); srand(42);
    int n=90; float t=glutGet(GLUT_ELAPSED_TIME)*0.003f;
    for(int i=0;i<n;i++){
        float x=(float)(rand()%winW);
        float y=(float)(winH*0.4f+(rand()%(int)(winH*0.55f)));
        float s=1.0f+(float)(rand()%3);
        float tw=0.55f+0.45f*sinf(t+i*0.37f);
        C3(tw,tw,0.85f+0.15f*tw); drawCircle(x,y,s,8);
    }
    srand(saved);
}
void drawCloud(float x,float y,float sc){
    glEnable(GL_BLEND);
    C4(1,1,1,0.95f);
    drawCircle(x,           y,           20*sc,20);
    drawCircle(x+18*sc,     y+6*sc,      22*sc,20);
    drawCircle(x+35*sc,     y,           18*sc,20);
    drawCircle(x+12*sc,     y-8*sc,      16*sc,16);
    C4(0.85f,0.88f,0.95f,0.7f); drawCircle(x+8*sc,y-4*sc,10*sc,14);
    glDisable(GL_BLEND);
}
void drawClouds(){ for(size_t i=0;i<clouds.size();i++) drawCloud(clouds[i].x,clouds[i].y,clouds[i].sc); }
void drawBarn(float x,float y,float sc){
    float w=180*sc, h=120*sc;
    C3(0.75f,0.16f,0.16f); drawRect(x,y,w,h);
    C3(0.62f,0.12f,0.12f); glLineWidth(1.0f);
    glBegin(GL_LINES);
    for(float xx=x+14*sc; xx<x+w; xx+=22*sc){ V2(xx,y); V2(xx,y+h); }
    glEnd();
    C3(0.50f,0.10f,0.10f);
    glBegin(GL_TRIANGLES);
    V2(x-10*sc,y+h); V2(x+w+10*sc,y+h); V2(x+w*0.5f,y+h+55*sc);
    glEnd();
    C3(0.35f,0.22f,0.10f); drawRect(x+70*sc,y,40*sc,72*sc);
    C3(0.85f,0.70f,0.45f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    V2(x+70*sc,y); V2(x+110*sc,y+72*sc);
    V2(x+110*sc,y); V2(x+70*sc,y+72*sc);
    glEnd();
    C3(0.95f,0.88f,0.40f);
    drawRect(x+18*sc,y+82*sc,32*sc,24*sc);
    drawRect(x+130*sc,y+82*sc,32*sc,24*sc);
    C3(0.15f,0.08f,0.05f);
    glBegin(GL_LINES);
    V2(x+34*sc,y+82*sc);  V2(x+34*sc,y+106*sc);
    V2(x+18*sc,y+94*sc);  V2(x+50*sc,y+94*sc);
    V2(x+146*sc,y+82*sc); V2(x+146*sc,y+106*sc);
    V2(x+130*sc,y+94*sc); V2(x+162*sc,y+94*sc);
    glEnd(); glLineWidth(1.0f);
}
void drawGrass(float br){
    float gh=60.0f;
    drawGrad(0,0,(float)winW,gh, 0.18f*br,0.42f*br,0.14f*br, 0.33f*br,0.62f*br,0.20f*br);
    C3(0.20f*br,0.55f*br,0.18f*br); glLineWidth(1.5f);
    glBegin(GL_LINES);
    for(int x=0;x<winW;x+=14){
        V2((float)x,5);    V2((float)x,18);
        V2((float)x+5,5);  V2((float)x+5,22);
        V2((float)x+10,5); V2((float)x+10,16);
    }
    glEnd();
    for(int x=30;x<winW;x+=85){
        int off=(x*13)%22; float fy=22.0f+off;
        C3(0.10f*br,0.55f*br,0.10f*br); glLineWidth(2.0f);
        glBegin(GL_LINES); V2((float)x,8); V2((float)x,fy-4); glEnd();
        int hue=(x*7)%3;
        if(hue==0)      C3(1.00f,0.42f,0.55f);
        else if(hue==1) C3(1.00f,0.88f,0.22f);
        else            C3(0.90f,0.40f,0.92f);
        drawCircle((float)x,fy,4.0f,10);
        C3(1,1,0.35f); drawCircle((float)x,fy,1.4f,8);
    }
    glLineWidth(1.0f);
}
void drawMountains(float br){
    static const float peaks[3][3]={{0.0f,0.32f,0.16f},{0.22f,0.55f,0.36f},{0.58f,1.0f,0.82f}};
    static const float caps[3][2] ={{0.30f,0.38f},{0.34f,0.44f},{0.38f,0.48f}};
    C3(0.24f*br,0.22f*br,0.32f*br);
    glBegin(GL_TRIANGLES);
    for(int i=0;i<3;i++){
        V2(winW*peaks[i][0],60); V2(winW*peaks[i][1],60);
        V2(winW*peaks[i][2],winH*caps[i][1]);
    }
    glEnd();
    C3(0.85f*br,0.88f*br,0.95f*br);
    glBegin(GL_TRIANGLES);
    static const float capX[3][3]={{0.12f,0.20f,0.16f},{0.32f,0.40f,0.36f},{0.78f,0.86f,0.82f}};
    for(int i=0;i<3;i++){
        V2(winW*capX[i][0],winH*caps[i][0]);
        V2(winW*capX[i][1],winH*caps[i][0]);
        V2(winW*capX[i][2],winH*caps[i][1]);
    }
    glEnd();
}
void drawFireflies(){
    float t=glutGet(GLUT_ELAPSED_TIME)*0.001f;
    glEnable(GL_BLEND);
    for(int i=0;i<28;i++){
        float x=80.0f+(float)((i*137)%(winW-100));
        float y=90.0f+90.0f*sinf(t+i*0.7f)+(i*11%140);
        x+=30.0f*cosf(t*0.7f+i);
        float gl=0.55f+0.45f*sinf(t*3.0f+i);
        C4(0.85f,1.0f,0.40f,gl); drawCircle(x,y,3.0f,10);
        C4(1.0f,1.0f,0.70f,gl*0.5f); drawCircle(x,y,6.0f,12);
    }
    glDisable(GL_BLEND);
}
void drawLightning(){
    int seed=(int)(glutGet(GLUT_ELAPSED_TIME)/4500);
    int phase=glutGet(GLUT_ELAPSED_TIME)%4500;
    if(phase>120) return;
    srand(seed+77);
    float br=1.0f-phase/120.0f;
    glEnable(GL_BLEND); C4(1,1,0.9f,0.35f*br); drawRect(0,0,(float)winW,(float)winH); glDisable(GL_BLEND);
    C3(1,1,0.9f); glLineWidth(3.0f);
    float sx=100.0f+rand()%(winW-200), sy=winH-40.0f;
    glBegin(GL_LINE_STRIP); V2(sx,sy);
    float cx=sx, cy=sy;
    for(int i=0;i<10;i++){
        cx+=-30+rand()%60; cy-=40+rand()%40;
        if(cy<100) break;
        V2(cx,cy);
    }
    glEnd(); glLineWidth(1.0f);
}
void drawScenery(int theme){
    drawSkyTheme(theme);
    float br = (theme>=5)?0.30f : (theme==4)?0.35f : (theme==3)?0.55f : (theme==2)?0.80f : 1.00f;
    if(theme==5){ drawStars(); drawMoonLike(winW-140.0f,winH-140.0f,62.0f,1); drawMountains(br); drawLightning(); }
    else if(theme==4){ drawStars(); drawMoonLike(winW-140.0f,winH-140.0f,55.0f,0); drawMountains(br); }
    else if(theme==3){ drawLightning(); }
    else { drawSun(winW-130.0f,winH-140.0f,50.0f); }
    if(theme<=3) drawClouds();
    drawBarn(60.0f,60.0f,0.9f);
    drawGrass(br);
    if(theme==4) drawFireflies();
}

// === 8. Chicken + stick ===
void drawStick(float y){
    C3(0.56f,0.40f,0.15f); drawRect(0,y-7,(float)winW,14);
    C3(0.35f,0.22f,0.08f); glLineWidth(1.5f);
    glBegin(GL_LINES);
    for(int x=45;x<winW;x+=80){ V2((float)x,y-7); V2((float)x,y+7); }
    glEnd();
    C3(0.78f,0.58f,0.22f); drawRect(0,y+4,(float)winW,2);
    glLineWidth(1.0f);
}
void drawChicken(float x,float y,float bp){
    float bob=2.2f*sinf(glutGet(GLUT_ELAPSED_TIME)*0.006f+bp);
    y+=bob;
    C3(1.0f,0.55f,0.05f); glLineWidth(3.0f);
    glBegin(GL_LINES);
    V2(x-5,y); V2(x-5,y-10); V2(x+7,y); V2(x+7,y-10);
    V2(x-9,y-10); V2(x-2,y-10); V2(x+3,y-10); V2(x+11,y-10);
    glEnd(); glLineWidth(1.0f);
    C3(1.0f,0.98f,0.95f); drawEllipse(x,y+14,24,18,22);
    C3(0.90f,0.90f,0.92f);
    glBegin(GL_TRIANGLES);
    V2(x-22,y+16); V2(x-34,y+28); V2(x-20,y+26);
    V2(x-20,y+20); V2(x-32,y+32); V2(x-18,y+30);
    glEnd();
    C3(0.85f,0.85f,0.87f); drawEllipse(x-4,y+14,13,9,18);
    C3(0.70f,0.70f,0.74f);
    glBegin(GL_LINE_STRIP); V2(x-14,y+14); V2(x-8,y+8); V2(x+2,y+10); glEnd();
    C3(1.0f,0.98f,0.95f); drawCircle(x+16,y+26,12,18);
    C3(0.92f,0.10f,0.10f);
    for(int k=-4;k<=4;k+=4) drawCircle(x+16+k,y+(k==0?38:36),4,10);
    C3(1.0f,0.58f,0); glBegin(GL_TRIANGLES); V2(x+25,y+27); V2(x+33,y+25); V2(x+25,y+22); glEnd();
    C3(0.88f,0.10f,0.10f); drawCircle(x+23,y+20,2.6f,10);
    C3(1,1,1); drawCircle(x+19,y+29,2.6f,10);
    C3(0,0,0); drawCircle(x+19.6f,y+29,1.3f,8);
}

// === 9. Basket ===
void drawBasket(const Basket& b){
    float x=b.x, y=b.y, w=b.w, h=b.h;
    C3(0.76f,0.48f,0.18f);
    glBegin(GL_POLYGON);
    V2(x-w/2,y+h); V2(x+w/2,y+h);
    V2(x+w/2-14,y); V2(x-w/2+14,y);
    glEnd();
    C3(0.55f,0.30f,0.08f); glLineWidth(1.5f);
    glBegin(GL_LINES);
    for(float yy=y+4; yy<y+h-1; yy+=6){
        float t=(yy-y)/h;
        V2(x-w/2+14*t,yy); V2(x+w/2-14*t,yy);
    }
    for(int k=0;k<11;k++){
        float t=k/10.0f;
        float xt=x-w/2+14+(w-28)*t;
        V2(xt,y+2); V2(xt,y+h-2);
    }
    glEnd();
    C3(0.48f,0.26f,0.07f); drawRect(x-w/2-7,y+h,w+14,8);
    C3(0.85f,0.58f,0.22f); drawRect(x-w/2-7,y+h+6,w+14,2);
    C3(0.40f,0.22f,0.08f); glLineWidth(4.5f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=24;i++){
        float u=(float)i/24.0f;
        V2(x-w*0.42f+w*0.84f*u, y+h+6+sinf(PI*u)*20.0f);
    }
    glEnd(); glLineWidth(1.0f);
    if(shield>0){
        glEnable(GL_BLEND);
        float p=0.22f+0.10f*sinf(glutGet(GLUT_ELAPSED_TIME)*0.008f);
        C4(0.40f,1.0f,0.50f,p); drawCircle(x,y+h*0.7f,w*0.78f,40);
        C4(0.20f,1.0f,0.50f,0.75f); glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        for(int i=0;i<40;i++){ float a=2*PI*i/40.0f; V2(x+w*0.78f*cosf(a),y+h*0.7f+w*0.78f*sinf(a)); }
        glEnd(); glLineWidth(1.0f);
        glDisable(GL_BLEND);
        char sc[8]; sprintf(sc,"x%d",shield);
        drawText(sc,x+w*0.45f,y+h+22,F12,0.08f,0.85f,0.18f);
    }
}

// === 10. Falling objects ===
static const float pcol[OT_COUNT][3] = {
    {1,1,1},                  // EGG_NORMAL (unused via drawEgg)
    {0.30f,0.50f,1.00f},      // EGG_BLUE
    {1.00f,0.85f,0.10f},      // EGG_GOLD
    {0.34f,0.17f,0.04f},      // POOP
    {0.15f,0.15f,0.15f},      // BOMB
    {0.0f, 0.95f,1.0f},       // PERK_LARGE
    {1.0f, 0.15f,0.95f},      // PERK_SLOW
    {1.0f, 0.25f,0.25f},      // PERK_EXTRA
    {1.0f, 0.85f,0.05f},      // PERK_DOUBLE
    {0.25f,0.45f,1.0f},       // PERK_MAGNET
    {0.22f,0.88f,0.35f},      // PERK_SHIELD
    {1.0f, 0.50f,0.05f},      // DEB_SMALL
    {0.55f,0.12f,0.78f},      // DEB_REVERSE
    {0.58f,0.70f,0.82f}       // DEB_FREEZE
};
int objToEfx(ObjType t){
    switch(t){
        case PERK_LARGE: return EFX_LARGE; case PERK_SLOW: return EFX_SLOW;
        case PERK_DOUBLE: return EFX_DOUBLE; case PERK_MAGNET: return EFX_MAGNET;
        case DEB_SMALL: return EFX_SMALL; case DEB_REVERSE: return EFX_REVERSE;
        case DEB_FREEZE: return EFX_FREEZE; default: return -1;
    }
}
void drawEgg(float x,float y,float r,float cr,float cg,float cb,float spin){
    glPushMatrix(); glTranslatef(x,y,0); glRotatef(spin,0,0,1);
    C3(cr,cg,cb); drawEllipse(0,0,r*0.82f,r*1.18f,22);
    glEnable(GL_BLEND);
    C4(1,1,1,0.7f); drawEllipse(-r*0.28f,r*0.4f,r*0.20f,r*0.42f,14);
    C4(0.1f,0.1f,0.1f,0.25f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<22;i++){ float t=2*PI*i/22.0f; V2(r*0.82f*cosf(t),r*1.18f*sinf(t)); }
    glEnd(); glLineWidth(1.0f); glDisable(GL_BLEND);
    glPopMatrix();
}
void drawPoop(float x,float y,float r){
    C3(0.34f,0.17f,0.04f); drawCircle(x,y,r,14);
    C3(0.44f,0.25f,0.07f); drawCircle(x-1,y+r*0.8f,r*0.72f,12);
    C3(0.55f,0.32f,0.12f); drawCircle(x+1,y+r*1.4f,r*0.45f,10);
    C3(0.55f,0.75f,0.55f); glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
    V2(x-4,y+r*1.9f); V2(x-1,y+r*2.3f); V2(x-4,y+r*2.7f); V2(x-1,y+r*3.1f);
    glEnd(); glLineWidth(1.0f);
}
void drawBomb(float x,float y,float r){
    C3(0.05f,0.05f,0.05f); drawCircle(x,y,r*1.05f,22);
    C3(0.15f,0.15f,0.15f); drawCircle(x,y,r,22);
    C3(0.40f,0.40f,0.40f); drawCircle(x-r*0.32f,y+r*0.30f,r*0.25f,14);
    C3(0.42f,0.27f,0.07f); glLineWidth(3.5f);
    glBegin(GL_LINE_STRIP);
    V2(x,y+r); V2(x+r*0.30f,y+r+r*0.30f); V2(x+r*0.10f,y+r+r*0.70f);
    glEnd(); glLineWidth(1.0f);
    float k=1.0f+0.40f*sinf(glutGet(GLUT_ELAPSED_TIME)*0.05f);
    C3(1.0f,0.55f,0); drawCircle(x+r*0.10f,y+r+r*0.70f,3.0f*k,10);
    C3(1.0f,0.95f,0.30f); drawCircle(x+r*0.10f,y+r+r*0.70f,1.4f*k,8);
}
// shared icon helpers
void iconArrows(float cx,float cy,float s,bool out){
    float a=out?0.55f:0.20f, b=out?0.85f:0.50f, c=out?0.50f:0.50f;
    float d=out?0.15f:0.50f, e=out?-0.50f:-0.15f;
    glBegin(GL_LINES);
    V2(cx-(out?0.55f:0.20f)*s,cy); V2(cx+(out?0.55f:0.20f)*s,cy);
    glEnd();
    glBegin(GL_TRIANGLES);
    V2(cx-(out?0.85f: 0.15f)*s,cy);
    V2(cx-(out?0.50f: 0.50f)*s,cy+s*0.38f);
    V2(cx-(out?0.50f: 0.50f)*s,cy-s*0.38f);
    V2(cx+(out?0.85f: 0.15f)*s,cy);
    V2(cx+(out?0.50f: 0.50f)*s,cy+s*0.38f);
    V2(cx+(out?0.50f: 0.50f)*s,cy-s*0.38f);
    glEnd();
    (void)a;(void)b;(void)c;(void)d;(void)e;
}
void iconClockFace(float cx,float cy,float s){
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<24;i++){ float a=2*PI*i/24.0f; V2(cx+s*0.82f*cosf(a),cy+s*0.82f*sinf(a)); }
    glEnd();
}
void drawPerkIcon(float cx,float cy,float s,ObjType t,float r,float g,float b){
    C3(r,g,b); glLineWidth(2.5f);
    switch(t){
        case PERK_LARGE: iconArrows(cx,cy,s,true); break;
        case DEB_SMALL:  iconArrows(cx,cy,s,false); break;
        case PERK_SLOW: {
            iconClockFace(cx,cy,s);
            glLineWidth(2.8f);
            glBegin(GL_LINES);
            V2(cx,cy); V2(cx-s*0.32f,cy+s*0.48f);
            V2(cx,cy); V2(cx+s*0.62f,cy);
            glEnd();
            drawCircle(cx,cy,s*0.09f,8);
        } break;
        case PERK_EXTRA: {
            iconClockFace(cx,cy,s);
            glLineWidth(3.2f);
            glBegin(GL_LINES);
            V2(cx-s*0.38f,cy); V2(cx+s*0.38f,cy);
            V2(cx,cy-s*0.38f); V2(cx,cy+s*0.38f);
            glEnd();
        } break;
        case PERK_DOUBLE: {
            glBegin(GL_TRIANGLE_FAN); V2(cx,cy);
            for(int i=0;i<=10;i++){
                float a=PI*0.5f+2*PI*i/10.0f;
                float rr=(i%2==0)?s*0.95f:s*0.40f;
                V2(cx+rr*cosf(a),cy+rr*sinf(a));
            }
            glEnd();
        } break;
        case PERK_MAGNET: {
            glLineWidth(5.0f); C3(0.90f,0.90f,0.92f);
            glBegin(GL_LINE_STRIP);
            for(int i=0;i<=18;i++){ float a=PI+PI*i/18.0f; V2(cx+s*0.55f*cosf(a),cy-s*0.10f+s*0.50f*sinf(a)); }
            glEnd();
            glBegin(GL_LINES);
            V2(cx-s*0.55f,cy-s*0.10f); V2(cx-s*0.55f,cy+s*0.40f);
            V2(cx+s*0.55f,cy-s*0.10f); V2(cx+s*0.55f,cy+s*0.40f);
            glEnd();
            C3(0.95f,0.12f,0.12f);
            glBegin(GL_LINES); V2(cx-s*0.55f,cy+s*0.25f); V2(cx-s*0.55f,cy+s*0.62f); glEnd();
            C3(0.10f,0.35f,0.95f);
            glBegin(GL_LINES); V2(cx+s*0.55f,cy+s*0.25f); V2(cx+s*0.55f,cy+s*0.62f); glEnd();
            glLineWidth(2.5f);
        } break;
        case PERK_SHIELD: {
            glLineWidth(3.0f);
            glBegin(GL_LINE_LOOP);
            V2(cx-s*0.65f,cy+s*0.70f); V2(cx+s*0.65f,cy+s*0.70f);
            V2(cx+s*0.65f,cy); V2(cx,cy-s*0.85f); V2(cx-s*0.65f,cy);
            glEnd();
            glBegin(GL_LINES);
            V2(cx-s*0.30f,cy+s*0.15f); V2(cx+s*0.30f,cy+s*0.15f);
            V2(cx,cy-s*0.18f); V2(cx,cy+s*0.45f);
            glEnd();
            glLineWidth(2.5f);
        } break;
        case DEB_REVERSE: {
            glLineWidth(3.0f);
            glBegin(GL_LINE_STRIP);
            int n=22; float a0=PI*0.25f, a1=PI*2.0f;
            for(int i=0;i<=n;i++){ float a=a0+(a1-a0)*i/n; V2(cx+s*0.68f*cosf(a),cy+s*0.68f*sinf(a)); }
            glEnd();
            float a=PI*2.0f, ex=cx+s*0.68f*cosf(a), ey=cy+s*0.68f*sinf(a);
            glBegin(GL_TRIANGLES);
            V2(ex+s*0.28f,ey); V2(ex-s*0.08f,ey+s*0.26f); V2(ex-s*0.08f,ey-s*0.26f);
            glEnd();
            glLineWidth(2.5f);
        } break;
        case DEB_FREEZE: {
            glLineWidth(2.5f);
            glBegin(GL_LINES);
            for(int i=0;i<6;i++){
                float a=2*PI*i/6.0f, ex=cx+s*0.88f*cosf(a), ey=cy+s*0.88f*sinf(a);
                V2(cx,cy); V2(ex,ey);
                float bx=cx+s*0.50f*cosf(a), by=cy+s*0.50f*sinf(a);
                float aL=a+PI/3.5f, aR=a-PI/3.5f;
                V2(bx,by); V2(bx+s*0.24f*cosf(aL),by+s*0.24f*sinf(aL));
                V2(bx,by); V2(bx+s*0.24f*cosf(aR),by+s*0.24f*sinf(aR));
            }
            glEnd();
        } break;
        default: break;
    }
    glLineWidth(1.0f);
}
void drawPerkBlock(float x,float y,float r,ObjType t){
    float cr=pcol[t][0], cg=pcol[t][1], cb=pcol[t][2];
    glEnable(GL_BLEND); C4(cr,cg,cb,0.30f); drawCircle(x,y,r*1.7f,26); glDisable(GL_BLEND);
    C3(cr,cg,cb); drawRR(x-r,y-r,2*r,2*r,4.5f);
    glEnable(GL_BLEND); C4(1,1,1,0.30f); drawRR(x-r+2,y+2,2*r-4,r-4,3.5f); glDisable(GL_BLEND);
    float lum=0.299f*cr+0.587f*cg+0.114f*cb;
    float ir,ig,ib;
    if(lum<0.55f){ ir=ig=ib=1.0f; } else { ir=0.08f; ig=0.08f; ib=0.14f; }
    drawPerkIcon(x,y,r*0.80f,t,ir,ig,ib);
}

// === 11. Particles + floating text ===
void spawnBurst(float x,float y,float r,float g,float b,int n){
    for(int i=0;i<n;i++){
        Part p; p.x=x; p.y=y;
        float a=2*PI*((rand()%1000)/1000.0f), s=60.0f+(float)(rand()%140);
        p.dx=cosf(a)*s; p.dy=sinf(a)*s+40.0f;
        p.maxlife=0.6f+(rand()%30)/100.0f; p.life=p.maxlife;
        p.r=r; p.g=g; p.b=b;
        p.sz=2.0f+(float)(rand()%3);
        parts.push_back(p);
    }
}
void spawnFloat(float x,float y,const char* s,float r,float g,float b){
    FText f; f.x=x; f.y=y; f.vy=80.0f; f.maxlife=1.2f; f.life=f.maxlife;
    f.r=r; f.g=g; f.b=b;
    int n=0; while(n<31 && s[n]){ f.s[n]=s[n]; n++; } f.s[n]=0;
    f.fn=F18; ftxts.push_back(f);
}
void updateParts(float dt){
    for(size_t i=0;i<parts.size();i++){
        parts[i].life-=dt;
        parts[i].x+=parts[i].dx*dt; parts[i].y+=parts[i].dy*dt;
        parts[i].dy-=200*dt;
    }
    size_t w=0; for(size_t i=0;i<parts.size();i++) if(parts[i].life>0) parts[w++]=parts[i];
    parts.resize(w);
    for(size_t i=0;i<ftxts.size();i++){
        ftxts[i].life-=dt; ftxts[i].y+=ftxts[i].vy*dt; ftxts[i].vy*=0.94f;
    }
    w=0; for(size_t i=0;i<ftxts.size();i++) if(ftxts[i].life>0) ftxts[w++]=ftxts[i];
    ftxts.resize(w);
}
void drawParts(){
    glEnable(GL_BLEND);
    for(size_t i=0;i<parts.size();i++){
        float a=parts[i].life/parts[i].maxlife;
        C4(parts[i].r,parts[i].g,parts[i].b,a);
        drawCircle(parts[i].x,parts[i].y,parts[i].sz,8);
    }
    for(size_t i=0;i<ftxts.size();i++){
        float a=ftxts[i].life/ftxts[i].maxlife;
        C4(ftxts[i].r,ftxts[i].g,ftxts[i].b,a);
        glRasterPos2f(ftxts[i].x,ftxts[i].y);
        for(char* c=ftxts[i].s;*c;c++) glutBitmapCharacter(ftxts[i].fn,*c);
    }
    glDisable(GL_BLEND);
}

// === 12. HUD ===
struct EffInfo { int idx; const char* name; float r,g,b; };
static const EffInfo eff[7] = {
    {EFX_LARGE,   "Large Basket",  0.0f,  0.95f, 1.0f},
    {EFX_SMALL,   "Small Basket",  1.0f,  0.5f,  0.05f},
    {EFX_SLOW,    "Slow Time",     1.0f,  0.15f, 0.95f},
    {EFX_DOUBLE,  "Double Points", 1.0f,  0.85f, 0.05f},
    {EFX_MAGNET,  "Magnet",        0.25f, 0.45f, 1.0f},
    {EFX_REVERSE, "Reverse!",      0.55f, 0.12f, 0.78f},
    {EFX_FREEZE,  "Freeze!",       0.58f, 0.70f, 0.82f}
};
void drawHUD(){
    glEnable(GL_BLEND);
    glBegin(GL_QUADS);
    C4(0,0,0,0.55f); V2(0,winH); V2((float)winW,winH); V2((float)winW,winH-62.0f); V2(0,winH-62.0f);
    glEnd();
    glBegin(GL_QUADS);
    C4(0,0,0,0.55f); V2(0,winH-62.0f); V2((float)winW,winH-62.0f);
    C4(0,0,0,0.00f); V2((float)winW,winH-82.0f); V2(0,winH-82.0f);
    glEnd();
    glDisable(GL_BLEND);
    char buf[128];
    sprintf(buf,"Score: %d",score);
    drawText(buf,20,winH-28,FTR,1,1,1);
    sprintf(buf,"Level: %s  (x%.2f)",levels[curLvl].name,levels[curLvl].mult);
    drawText(buf,20,winH-52,F12,1,0.92f,0.45f);
    sprintf(buf,"High: %d",hi[curLvl]);
    drawText(buf,230,winH-28,F18,1,0.85f,0.32f);
    int mult=1+combo/5; if(mult>5) mult=5;
    sprintf(buf,"Combo: %d  (x%d)",combo,mult);
    float cr=(combo>=5)?0.3f:1.0f, cb=(combo>=5)?0.3f:1.0f;
    drawText(buf,400,winH-28,F18,cr,1.0f,cb);
    const char* wd=(wind<-10)?"<<< Left":(wind>10)?"Right >>>":"Calm";
    sprintf(buf,"Wind: %s",wd);
    drawText(buf,620,winH-28,F18,0.9f,1,1);
    // time ring
    float cx=winW-70.0f, cy=winH-32.0f, rr=26.0f;
    C3(0.10f,0.10f,0.10f); drawCircle(cx,cy,rr+3,30);
    C3(0.75f,0.20f,0.20f); drawCircle(cx,cy,rr,30);
    float frac=(totalTime>0)?(timeLeft/totalTime):0; if(frac<0) frac=0; if(frac>1) frac=1;
    float rcol=(frac<0.33f)?1.0f:0.25f, gcol=(frac<0.33f)?0.30f:0.92f;
    C3(rcol,gcol,0.30f); drawArcWedge(cx,cy,rr,PI*0.5f,PI*0.5f+2*PI*frac,48);
    C3(0.08f,0.08f,0.08f); drawCircle(cx,cy,rr*0.62f,24);
    sprintf(buf,"%02d",(int)timeLeft);
    int w2=textW(buf,F18); drawText(buf,cx-w2/2.0f,cy-6,F18,1,1,1);
    // badges
    float by=winH-95.0f, bx=winW-180.0f;
    for(int i=0;i<7;i++){
        if(effT[eff[i].idx]>0){
            C3(eff[i].r,eff[i].g,eff[i].b);
            drawRR(bx,by,160,22,5);
            char s[64]; sprintf(s,"%s %.1fs",eff[i].name,effT[eff[i].idx]);
            drawText(s,bx+10,by+6,F12,0.08f,0.06f,0.10f);
            by-=26;
        }
    }
    if(shield>0){
        C3(0.22f,0.88f,0.35f); drawRR(bx,by,160,22,5);
        char s[32]; sprintf(s,"Shield  x%d",shield);
        drawText(s,bx+10,by+6,F12,0.05f,0.08f,0.05f);
    }
}

// === 13. Menus ===
bool inRect(float mx,float my,float cx,float cy,float w,float h){
    return mx>=cx-w/2 && mx<=cx+w/2 && my>=cy-h/2 && my<=cy+h/2;
}
float menuBtnY(int i){ return winH*0.55f - i*78.0f; }
void drawBackdrop(){
    drawGrad(0,0,(float)winW,(float)winH, 0.10f,0.20f,0.38f, 0.55f,0.82f,0.98f);
    drawClouds();
    drawSun(180.0f,winH-140.0f,55.0f);
    drawBarn(winW-260.0f,60.0f,1.0f);
    drawGrass(1.0f);
}
void drawButton(float cx,float cy,float w,float h,const char* lbl,bool hov){
    glEnable(GL_BLEND); C4(0,0,0,0.28f); drawRR(cx-w/2+4,cy-h/2-5,w,h,10); glDisable(GL_BLEND);
    if(hov) C3(1.00f,0.88f,0.25f); else C3(0.93f,0.72f,0.08f);
    drawRR(cx-w/2,cy-h/2,w,h,10);
    if(hov) C3(1.00f,0.98f,0.55f); else C3(0.98f,0.82f,0.20f);
    drawRR(cx-w/2+4,cy-h/2+4,w-8,h-8,8);
    drawTextC(lbl,cx,cy-8,F18,0.14f,0.08f,0.02f);
}
void drawTitle(){
    drawTextC("CATCH THE EGGS!",winW/2.0f+4,winH*0.8f-4,FTR,0,0,0);
    drawTextC("CATCH THE EGGS!",winW/2.0f,winH*0.8f,FTR,1.0f,0.92f,0.22f);
    drawTextC("-- Compact Edition --",winW/2.0f,winH*0.8f-30,F18,1,1,1);
}
void drawMainMenu(){
    drawBackdrop(); drawTitle();
    static const char* lbl[4]={"PLAY","LEVEL SELECT","HELP","QUIT"};
    float cx=winW/2.0f;
    for(int i=0;i<4;i++) drawButton(cx,menuBtnY(i),300,56,lbl[i],hov==i);
    drawTextC("Click a button or press 1-4.   Esc to quit.",winW/2.0f,40,F12,1,1,0.9f);
    glutSwapBuffers();
}
void drawCardTitle(const char* name,float cx,float yTop,float maxW){
    int wOne=textW(name,F18);
    if((float)wOne<=maxW-10){ drawTextC(name,cx,yTop-20,F18,1,1,1); return; }
    int n=(int)strlen(name), mid=n/2, split=-1, bestD=99;
    for(int i=1;i<n-1;i++){
        if(name[i]==' '||name[i]=='-'){
            int d=(i>mid)?(i-mid):(mid-i);
            if(d<bestD){ bestD=d; split=i; }
        }
    }
    if(split<0){ drawTextC(name,cx,yTop-20,F12,1,1,1); return; }
    char l1[64], l2[64]; int j;
    for(j=0;j<split;j++) l1[j]=name[j]; l1[j]=0;
    int st=split+(name[split]==' '?1:0);
    for(j=st;j<n;j++) l2[j-st]=name[j]; l2[n-st]=0;
    drawTextC(l1,cx,yTop-12,F12,1,1,1);
    drawTextC(l2,cx,yTop-28,F12,1,1,1);
}
static const float cardCol[6][3]={
    {0.30f,0.85f,0.30f},{0.30f,0.60f,1.00f},{1.00f,0.80f,0.20f},
    {1.00f,0.40f,0.10f},{0.65f,0.10f,0.35f},{0.35f,0.02f,0.05f}
};
void drawLevelSelect(){
    drawBackdrop();
    drawTextC("SELECT DIFFICULTY",winW/2.0f,winH-90,FTR,1,0.92f,0.2f);
    float cardW=190, cardH=200, gap=15;
    float totalW=cardW*NUM_LEVELS+gap*(NUM_LEVELS-1);
    float startX=(winW-totalW)/2.0f+cardW/2.0f;
    float cy=winH/2.0f-20.0f;
    for(int i=0;i<NUM_LEVELS;i++){
        float cx=startX+i*(cardW+gap);
        bool h=(hov==i);
        glEnable(GL_BLEND); C4(0,0,0,0.28f); drawRR(cx-cardW/2+4,cy-cardH/2-5,cardW,cardH,12); glDisable(GL_BLEND);
        if(h) C3(1.0f,1.0f,0.95f); else C3(0.92f,0.92f,0.96f);
        drawRR(cx-cardW/2,cy-cardH/2,cardW,cardH,12);
        C3(cardCol[i][0],cardCol[i][1],cardCol[i][2]);
        drawRect(cx-cardW/2,cy+cardH/2-42,cardW,42);
        drawCardTitle(levels[i].name,cx,cy+cardH/2-8,cardW);
        char s[64];
        sprintf(s,"Chickens: %d",levels[i].chicks);   drawTextC(s,cx,cy+26,F12,0.15f,0.15f,0.15f);
        sprintf(s,"Time: %.0fs",levels[i].time);      drawTextC(s,cx,cy+5, F12,0.15f,0.15f,0.15f);
        sprintf(s,"Score x%.2f",levels[i].mult);      drawTextC(s,cx,cy-16,F12,0.15f,0.15f,0.15f);
        sprintf(s,"Best: %d",hi[i]);                  drawTextC(s,cx,cy-52,F18,0.75f,0.42f,0.08f);
        sprintf(s,"[%d]",i+1);                        drawTextC(s,cx,cy-cardH/2+10,F12,0.35f,0.35f,0.35f);
    }
    drawTextC("Click a card or press 1-6.   Esc to go back.",winW/2.0f,60,F18,1,1,1);
    glutSwapBuffers();
}
void drawHelpScreen(){
    drawBackdrop();
    glEnable(GL_BLEND); C4(0,0,0,0.78f);
    drawRR(winW*0.08f,winH*0.07f,winW*0.84f,winH*0.86f,14);
    glDisable(GL_BLEND);
    drawTextC("HOW TO PLAY",winW/2.0f,winH-80,FTR,1,0.9f,0.3f);
    float left=winW*0.13f, y=winH-130, lh=26;
    drawText("Controls",left,y,F18,0.7f,1,1); y-=lh;
    drawText("  Mouse / A D / Arrow Keys  -  Move Basket",left,y,F12,1,1,1); y-=lh;
    drawText("  P  -  Pause / Resume",left,y,F12,1,1,1); y-=lh;
    drawText("  Esc  -  Back to Menu     Q  -  Quit",left,y,F12,1,1,1); y-=lh;
    drawText("  R  -  Retry (after game over)",left,y,F12,1,1,1); y-=lh*1.5f;
    drawText("Scoring",left,y,F18,0.7f,1,1); y-=lh;
    C3(1,1,1);          drawEllipse(left+10,y+6,8,12,20);
    drawText("   Normal Egg       +1",left+32,y,F12,1,1,1); y-=lh;
    C3(0.3f,0.5f,1.0f); drawEllipse(left+10,y+6,8,12,20);
    drawText("   Blue Egg         +5",left+32,y,F12,1,1,1); y-=lh;
    C3(1.0f,0.85f,0.1f);drawEllipse(left+10,y+6,8,12,20);
    drawText("   Gold Egg        +10",left+32,y,F12,1,1,1); y-=lh;
    C3(0.4f,0.22f,0.08f);drawCircle(left+10,y+6,10,14);
    drawText("   Poop            -10  (avoid!)",left+32,y,F12,1,0.7f,0.7f); y-=lh;
    C3(0.15f,0.15f,0.15f);drawCircle(left+10,y+6,10,14);
    drawText("   Bomb            -25  (disastrous!)",left+32,y,F12,1,0.5f,0.5f); y-=lh;
    float rx=winW*0.52f, ry=winH-156;
    drawText("Power-ups (catch these!)",rx,ry+26,F18,0.6f,1,0.6f);
    static const ObjType ups[6]={PERK_LARGE,PERK_SLOW,PERK_EXTRA,PERK_DOUBLE,PERK_MAGNET,PERK_SHIELD};
    static const char* upEff[6]={"Large Basket (10s)","Slow Time (10s)","Extra Time (+10s)",
                                 "Double Points (10s)","Magnet pulls eggs (8s)","Shield blocks 1 bad (up to x2)"};
    for(int i=0;i<6;i++){
        drawPerkBlock(rx+14,ry-10,12,ups[i]);
        drawText(upEff[i],rx+42,ry-16,F12,1,1,1);
        ry-=30;
    }
    ry-=14;
    drawText("Power-downs (avoid!)",rx,ry,F18,1.0f,0.55f,0.55f); ry-=26;
    static const ObjType downs[3]={DEB_SMALL,DEB_REVERSE,DEB_FREEZE};
    static const char* downEff[3]={"Small Basket (5s)","Reverse Controls (5s)","Freeze Basket (3s)"};
    for(int i=0;i<3;i++){
        drawPerkBlock(rx+14,ry-10,12,downs[i]);
        drawText(downEff[i],rx+42,ry-16,F12,1,1,1);
        ry-=30;
    }
    ry-=20;
    drawText("Tips",rx,ry,F18,1,1,0.5f); ry-=lh;
    drawText("- Combo x5: 5 catches in a row multiplies your score!",rx,ry,F12,1,1,1); ry-=lh;
    drawText("- Wind pushes eggs sideways. Watch the Wind indicator.",rx,ry,F12,1,1,1); ry-=lh;
    drawText("- Mad Farmer: night with 4 sticks & storms.",rx,ry,F12,1,1,1); ry-=lh;
    drawText("- Ruthless Farmer: blood moon - 2x harder than Mad Farmer!",rx,ry,F12,1,0.75f,0.75f);
    drawTextC("Press Esc or M to return to menu",winW/2.0f,50,F18,1,1,0.5f);
    glutSwapBuffers();
}
void drawPauseOverlay(){
    glEnable(GL_BLEND);
    C4(0,0,0,0.55f); drawRect(0,0,(float)winW,(float)winH);
    C4(0.08f,0.08f,0.14f,0.92f); drawRR(winW/2.0f-230,winH/2.0f-110,460,220,14);
    glDisable(GL_BLEND);
    drawTextC("PAUSED",winW/2.0f,winH/2.0f+60,FTR,1,1,1);
    drawTextC("P  -  Resume",     winW/2.0f,winH/2.0f+15,F18,1,1,1);
    drawTextC("Esc  -  Main Menu",winW/2.0f,winH/2.0f-15,F18,1,1,1);
    drawTextC("Q  -  Quit Game",  winW/2.0f,winH/2.0f-45,F18,1,1,1);
}
void drawGameOver(){
    drawBackdrop();
    glEnable(GL_BLEND); C4(0,0,0,0.78f);
    drawRR(winW/2.0f-290,winH/2.0f-180,580,360,16);
    glDisable(GL_BLEND);
    drawTextC("TIME'S UP!",winW/2.0f,winH/2.0f+130,FTR,1.0f,0.42f,0.42f);
    char buf[128];
    sprintf(buf,"Level: %s",levels[curLvl].name);
    drawTextC(buf,winW/2.0f,winH/2.0f+85,F18,1,1,1);
    sprintf(buf,"Final Score:  %d",score);
    drawTextC(buf,winW/2.0f,winH/2.0f+45,FTR,1,1,0.3f);
    sprintf(buf,"Max Combo:  %d",maxCombo);
    drawTextC(buf,winW/2.0f,winH/2.0f+15,F18,1,1,1);
    sprintf(buf,"High Score: %d",hi[curLvl]);
    drawTextC(buf,winW/2.0f,winH/2.0f-15,F18,1,0.92f,0.45f);
    if(newHi){
        float p=0.75f+0.25f*sinf(glutGet(GLUT_ELAPSED_TIME)*0.01f);
        drawTextC("** NEW HIGH SCORE! **",winW/2.0f,winH/2.0f-55,FTR,p,p,0.22f);
    }
    drawTextC("[R] Retry    [M] Menu    [Q] Quit",winW/2.0f,winH/2.0f-125,F18,1,1,1);
    glutSwapBuffers();
}

// === 14. Spawning ===
void spawnObject(float x,float y){
    const Lvl& L=levels[curLvl];
    FObj o; o.x=x; o.y=y; o.act=true; o.dx=0; o.spin=(float)(rand()%360);
    int roll=rand()%100;
    if(roll<L.poop) o.t=POOP;
    else if(roll<L.poop+L.bomb) o.t=BOMB;
    else {
        int r2=rand()%100;
        if(r2<10)      o.t=EGG_GOLD;
        else if(r2<25) o.t=EGG_BLUE;
        else if(r2<60) o.t=EGG_NORMAL;
        else if(r2<72) o.t=(ObjType)(PERK_LARGE+rand()%6);
        else if(r2<78) o.t=(ObjType)(DEB_SMALL+rand()%3);
        else           o.t=EGG_NORMAL;
    }
    static const float spdMul[OT_COUNT]={1.0f,1.1f,1.4f,0.85f,1.2f,
        0.70f,0.70f,0.70f,0.70f,0.70f,0.70f, 0.70f,0.70f,0.70f};
    static const float typeR[OT_COUNT]={11,11,11,13,14, 14,14,14,14,14,14, 14,14,14};
    o.dy=-L.speed*spdMul[o.t]; o.r=typeR[o.t];
    objs.push_back(o);
}
void spawnClouds(){
    clouds.clear();
    for(int i=0;i<12;i++){
        Cloud c;
        c.x=(float)(rand()%winW);
        c.y=winH*0.55f+(float)(rand()%(winH/4));
        c.sp=10.0f+(float)(rand()%25);
        c.sc=0.6f+(rand()%80)/100.0f;
        clouds.push_back(c);
    }
}

// === 15. Game logic ===
void hitBad(float pen,float x,float y,float pr,float pg,float pb,int n,bool sh){
    if(shield>0){
        shield--;
        spawnFloat(x,y+10, pen<=10?"BLOCKED":"BLOCKED!", 0.3f,0.9f,0.3f);
        spawnBurst(x,y,0.3f,0.9f,0.3f,n>16?18:14);
        playSfx(1);
    } else {
        score-=(int)pen; combo=0;
        char b[8]; sprintf(b,"-%d",(int)pen);
        spawnFloat(x,y+10,b,1.0f,pen>=20?0.2f:0.3f,pen>=20?0.2f:0.3f);
        spawnBurst(x,y,pr,pg,pb,n);
        if(sh){ shake=0.5f; flash=0.85f; playSfx(2); playSfx(3); }
        else { playSfx(2); }
    }
}
void applyDeb(int eIdx,float dur,const char* msg,float x,float y,float r,float g,float b){
    if(shield>0){ shield--; spawnFloat(x,y+10,"BLOCKED",0.3f,0.9f,0.3f); }
    else { effT[eIdx]=dur; spawnFloat(x,y+10,msg,r,g,b); }
    playSfx(2);
}
void applyPickup(ObjType t,float x,float y){
    char buf[16];
    int mult=1+combo/5; if(mult>5) mult=5;
    float sm=levels[curLvl].mult;
    int dbl=effOn(EFX_DOUBLE)?2:1;
    static const int eggPts[3]={1,5,10};
    static const float eggCol[3][3]={{1,1,1},{0.4f,0.6f,1.0f},{1.0f,0.9f,0.2f}};
    static const int eggBurst[3]={10,16,24};
    static const int eggSfx[3]={0,1,1};
    if(t<=EGG_GOLD){
        int pts=(int)(eggPts[t]*mult*sm*dbl); if(pts<1) pts=1;
        score+=pts; combo++;
        sprintf(buf,"+%d",pts);
        spawnFloat(x,y+10,buf,eggCol[t][0],eggCol[t][1],eggCol[t][2]);
        spawnBurst(x,y,eggCol[t][0],eggCol[t][1],eggCol[t][2],eggBurst[t]);
        playSfx(eggSfx[t]);
    } else if(t==POOP){
        hitBad(10,x,y,0.40f,0.25f,0.05f,14,false);
    } else if(t==BOMB){
        hitBad(25,x,y,1.0f,0.35f,0.05f,44,true);
    } else if(t==PERK_LARGE){
        effT[EFX_LARGE]=10.0f; spawnFloat(x,y+10,"LARGE!",0,1,1); playSfx(1);
    } else if(t==PERK_SLOW){
        effT[EFX_SLOW]=10.0f; spawnFloat(x,y+10,"SLOW!",1.0f,0.3f,1.0f); playSfx(1);
    } else if(t==PERK_EXTRA){
        timeLeft+=10.0f; spawnFloat(x,y+10,"+10s",1.0f,0.3f,0.3f); playSfx(1);
    } else if(t==PERK_DOUBLE){
        effT[EFX_DOUBLE]=10.0f; spawnFloat(x,y+10,"2x POINTS",1.0f,0.88f,0.1f); playSfx(1);
    } else if(t==PERK_MAGNET){
        effT[EFX_MAGNET]=8.0f; spawnFloat(x,y+10,"MAGNET!",0.3f,0.5f,1.0f); playSfx(1);
    } else if(t==PERK_SHIELD){
        if(shield<2) shield++;
        spawnFloat(x,y+10,"SHIELD",0.3f,0.9f,0.3f); playSfx(1);
    } else if(t==DEB_SMALL){
        applyDeb(EFX_SMALL,5.0f,"SHRINK!",x,y,1.0f,0.5f,0);
    } else if(t==DEB_REVERSE){
        applyDeb(EFX_REVERSE,5.0f,"REVERSE!",x,y,0.55f,0.12f,0.78f);
    } else if(t==DEB_FREEZE){
        applyDeb(EFX_FREEZE,3.0f,"FREEZE!",x,y,0.55f,0.70f,0.90f);
    }
    if(combo>maxCombo) maxCombo=combo;
}
void updateLogic(float dt){
    if(gs!=PLAYING) return;
    const Lvl& L=levels[curLvl];
    timeLeft-=dt;
    if(flash>0) flash-=dt*1.5f;
    if(shake>0) shake-=dt;
    if(timeLeft<=0){
        timeLeft=0; gs=GAMEOVER; newHi=false;
        if(score>hi[curLvl]){ hi[curLvl]=score; newHi=true; }
        saveHiScore(); playSfx(3); return;
    }
    windT-=dt;
    if(windT<=0){
        windT=(L.wind>60)?(1.5f+(rand()%20)/10.0f):(3.0f+(rand()%40)/10.0f);
        int r=rand()%3;
        if(r==0) wind=-L.wind*(0.5f+(rand()%50)/100.0f);
        else if(r==1) wind=L.wind*(0.5f+(rand()%50)/100.0f);
        else wind=0;
    }
    for(int i=0;i<EFX_COUNT;i++){
        if(effT[i]>0){ effT[i]-=dt; if(effT[i]<0) effT[i]=0; }
    }
    bk.w=bk.bw;
    if(effOn(EFX_LARGE)) bk.w*=1.5f;
    if(effOn(EFX_SMALL)) bk.w*=0.6f;
    if(!effOn(EFX_FREEZE)){
        bool L1=keys['a']||keys['A']||sk[GLUT_KEY_LEFT];
        bool R1=keys['d']||keys['D']||sk[GLUT_KEY_RIGHT];
        if(effOn(EFX_REVERSE)){ bool t=L1; L1=R1; R1=t; }
        float sp=720.0f*dt;
        if(L1) bk.x-=sp; if(R1) bk.x+=sp;
    }
    if(bk.x<bk.w/2) bk.x=bk.w/2;
    if(bk.x>winW-bk.w/2) bk.x=winW-bk.w/2;
    for(size_t i=0;i<clouds.size();i++){
        clouds[i].x+=clouds[i].sp*dt;
        if(clouds[i].x>winW+80){ clouds[i].x=-100; clouds[i].y=winH*0.55f+(float)(rand()%(winH/4)); }
    }
    for(size_t i=0;i<chicks.size();i++){
        chicks[i].x+=chicks[i].sp*chicks[i].dir*dt;
        if(chicks[i].x<60){ chicks[i].x=60; chicks[i].dir=1; }
        if(chicks[i].x>winW-60){ chicks[i].x=winW-60; chicks[i].dir=-1; }
        chicks[i].dt0-=dt;
        if(chicks[i].dt0<=0){ chicks[i].dt0=chicks[i].di; spawnObject(chicks[i].x,chicks[i].y-10); }
    }
    for(size_t i=0;i<objs.size();i++){
        if(!objs[i].act) continue;
        float fall=objs[i].dy;
        if(effOn(EFX_SLOW)) fall*=0.5f;
        objs[i].y+=fall*dt;
        objs[i].spin+=60.0f*dt;
        if(objs[i].t<=BOMB) objs[i].x+=wind*dt;
        if(effOn(EFX_MAGNET)&&objs[i].t<=EGG_GOLD){
            float dxb=bk.x-objs[i].x, adx=fabsf(dxb);
            if(adx<280.0f){
                float pull=(1.0f-adx/280.0f)*260.0f;
                if(dxb<0) pull=-pull;
                objs[i].x+=pull*dt;
            }
        }
        if(objs[i].x<objs[i].r) objs[i].x=objs[i].r;
        if(objs[i].x>winW-objs[i].r) objs[i].x=winW-objs[i].r;
        if(objs[i].y-objs[i].r<=bk.y+bk.h && objs[i].y+objs[i].r>=bk.y &&
           objs[i].x>=bk.x-bk.w/2 && objs[i].x<=bk.x+bk.w/2){
            applyPickup(objs[i].t,objs[i].x,objs[i].y);
            objs[i].act=false;
        }
        if(objs[i].y<-30.0f){
            if(objs[i].t<=EGG_GOLD){ combo=0; spawnBurst(objs[i].x,40.0f,1,1,0.85f,10); }
            objs[i].act=false;
        }
    }
    size_t w=0; for(size_t i=0;i<objs.size();i++) if(objs[i].act) objs[w++]=objs[i];
    objs.resize(w);
    updateParts(dt);
}

// === 15b. Playing render ===
void drawPlaying(){
    const Lvl& L=levels[curLvl];
    float sx=0, sy=0;
    if(shake>0){ sx=(float)((rand()%21)-10)*shake*2.0f; sy=(float)((rand()%21)-10)*shake*2.0f; }
    glPushMatrix(); glTranslatef(sx,sy,0);
    drawScenery(L.theme);
    for(size_t i=0;i<sticks.size();i++) drawStick(sticks[i]);
    for(size_t i=0;i<chicks.size();i++) drawChicken(chicks[i].x,chicks[i].y,chicks[i].bp);
    for(size_t i=0;i<objs.size();i++){
        if(!objs[i].act) continue;
        FObj& o=objs[i];
        if(o.t<=EGG_GOLD){
            static const float ec[3][3]={{1,1,1},{0.30f,0.50f,1.00f},{1.00f,0.85f,0.10f}};
            drawEgg(o.x,o.y,o.r,ec[o.t][0],ec[o.t][1],ec[o.t][2],o.spin);
        }
        else if(o.t==POOP) drawPoop(o.x,o.y,o.r);
        else if(o.t==BOMB) drawBomb(o.x,o.y,o.r);
        else drawPerkBlock(o.x,o.y,o.r,o.t);
    }
    drawBasket(bk);
    drawParts();
    glPopMatrix();
    if(flash>0){
        glEnable(GL_BLEND);
        C4(1.0f,0.95f,0.8f,flash);
        drawRect(0,0,(float)winW,(float)winH);
        glDisable(GL_BLEND);
    }
    drawHUD();
    if(gs==PAUSED) drawPauseOverlay();
    glutSwapBuffers();
}

// === 16. Input ===
int hitMainMenu(int mx,int my){
    float cx=winW/2.0f;
    for(int i=0;i<4;i++) if(inRect((float)mx,(float)my,cx,menuBtnY(i),300,56)) return i;
    return -1;
}
int hitLevelCard(int mx,int my){
    float cardW=190, cardH=200, gap=15;
    float totalW=cardW*NUM_LEVELS+gap*(NUM_LEVELS-1);
    float startX=(winW-totalW)/2.0f+cardW/2.0f;
    float cy=winH/2.0f-20.0f;
    for(int i=0;i<NUM_LEVELS;i++){
        float cx=startX+i*(cardW+gap);
        if(inRect((float)mx,(float)my,cx,cy,cardW,cardH)) return i;
    }
    return -1;
}
void mousePassive(int x,int y){
    int my=winH-y;
    if(gs==MENU) hov=hitMainMenu(x,my);
    else if(gs==LEVEL_SELECT) hov=hitLevelCard(x,my);
    else if(gs==PLAYING && !effOn(EFX_FREEZE)){
        float bx=(float)x;
        if(effOn(EFX_REVERSE)) bx=(float)winW-x;
        bk.x=bx;
        if(bk.x<bk.w/2) bk.x=bk.w/2;
        if(bk.x>winW-bk.w/2) bk.x=winW-bk.w/2;
    }
}
void mouseClick(int btn,int st,int x,int y){
    if(btn!=GLUT_LEFT_BUTTON||st!=GLUT_DOWN) return;
    int my=winH-y;
    if(gs==MENU){
        int s=hitMainMenu(x,my);
        if(s==0||s==1) { gs=LEVEL_SELECT; hov=-1; }
        else if(s==2) { gs=HELP; }
        else if(s==3) { exit(0); }
    } else if(gs==LEVEL_SELECT){
        int s=hitLevelCard(x,my);
        if(s>=0){ curLvl=s; resetGame(); gs=PLAYING; playSfx(4); startBGM(curLvl); }
    }
}
void keyboard(unsigned char k,int x,int y){
    keys[k]=true;
    if(gs==MENU){
        if(k=='1'||k=='s'||k=='S'||k==13||k=='2') gs=LEVEL_SELECT;
        else if(k=='3'||k=='h'||k=='H') gs=HELP;
        else if(k=='4'||k=='q'||k=='Q'||k==27) exit(0);
    } else if(gs==LEVEL_SELECT){
        if(k>='1'&&k<='6'){ curLvl=k-'1'; resetGame(); gs=PLAYING; playSfx(4); startBGM(curLvl); }
        else if(k==27||k=='m'||k=='M') gs=MENU;
    } else if(gs==HELP){
        if(k==27||k=='m'||k=='M') gs=MENU;
    } else if(gs==PLAYING){
        if(k=='p'||k=='P') gs=PAUSED;
        else if(k==27||k=='q'||k=='Q') gs=MENU;
    } else if(gs==PAUSED){
        if(k=='p'||k=='P') gs=PLAYING;
        else if(k==27||k=='m'||k=='M') gs=MENU;
        else if(k=='q'||k=='Q') exit(0);
    } else if(gs==GAMEOVER){
        if(k=='m'||k=='M'||k==27) gs=MENU;
        else if(k=='r'||k=='R'){ resetGame(); gs=PLAYING; startBGM(curLvl); }
        else if(k=='q'||k=='Q') exit(0);
    }
    glutPostRedisplay();
}
void keyboardUp(unsigned char k,int x,int y){ keys[k]=false; }
void specialKeys(int k,int x,int y){ if(k<256) sk[k]=true; }
void specialKeysUp(int k,int x,int y){ if(k<256) sk[k]=false; }

// === 17. High-score I/O ===
void loadHiScore(){
    FILE* f=fopen("catcheggs_highscore.txt","r"); if(!f) return;
    for(int i=0;i<NUM_LEVELS;i++) if(fscanf(f,"%d",&hi[i])!=1) hi[i]=0;
    fclose(f);
}
void saveHiScore(){
    FILE* f=fopen("catcheggs_highscore.txt","w"); if(!f) return;
    for(int i=0;i<NUM_LEVELS;i++) fprintf(f,"%d\n",hi[i]);
    fclose(f);
}

// === 18. Reset / init ===
void resetGame(){
    const Lvl& L=levels[curLvl];
    score=combo=maxCombo=0;
    timeLeft=totalTime=L.time;
    wind=0; windT=2.0f;
    for(int i=0;i<EFX_COUNT;i++) effT[i]=0;
    shield=0; shake=0; flash=0; newHi=false;
    bk.bw=150.0f; bk.w=bk.bw; bk.h=34.0f; bk.x=winW/2.0f; bk.y=80.0f;
    sticks.clear(); chicks.clear(); objs.clear(); parts.clear(); ftxts.clear();
    float sp=130.0f;
    if(L.sticks>1){
        float fit=((float)winH-260.0f)/(float)(L.sticks-1);
        if(fit<sp) sp=fit;
    }
    for(int i=0;i<L.sticks;i++) sticks.push_back((float)winH-140.0f-i*sp);
    for(int i=0;i<L.chicks;i++){
        Chick c;
        c.si=i%L.sticks;
        c.y=sticks[c.si]+14.0f;
        c.x=100.0f+(float)(rand()%(winW-200));
        c.sp=70.0f+(float)(rand()%80);
        c.dir=(rand()%2==0)?1:-1;
        c.di=L.spawn*(0.7f+(rand()%70)/100.0f);
        c.dt0=c.di*(0.2f+(rand()%70)/100.0f);
        c.bp=(rand()%628)/100.0f;
        chicks.push_back(c);
    }
    spawnClouds();
}
void initGame(){
    srand((unsigned)time(NULL));
    lastT=glutGet(GLUT_ELAPSED_TIME);
    curLvl=0;
    loadHiScore();
    resetGame();
}

// === 19. Display / reshape / timer ===
void display(){
    if(gs==MENU) drawMainMenu();
    else if(gs==LEVEL_SELECT) drawLevelSelect();
    else if(gs==HELP) drawHelpScreen();
    else if(gs==GAMEOVER) drawGameOver();
    else drawPlaying();
}
void reshape(int w,int h){
    if(h==0) h=1;
    winW=w; winH=h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,w,0,h);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}
void timer(int v){
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=(now-lastT)/1000.0f; lastT=now;
    if(dt>0.1f) dt=0.1f;
    if(gs==PLAYING) updateLogic(dt);
    else {
        for(size_t i=0;i<clouds.size();i++){
            clouds[i].x+=clouds[i].sp*dt*0.35f;
            if(clouds[i].x>winW+80){ clouds[i].x=-100; clouds[i].y=winH*0.55f+(float)(rand()%(winH/4)); }
        }
    }
    static int bgmTick=0; bgmTick++;
    if(bgmTick>=30){ bgmTick=0; updateBGM(); }
    static float thunderT=3.5f;
    if(gs==PLAYING && curLvl==5){
        thunderT -= dt;
        if(thunderT<=0){
            playThunder();
            flash = (flash>0.85f)? flash : 0.85f;
            shake = (shake>0.3f)? shake : 0.3f;
            thunderT = 4.5f + (rand()%6000)/1000.0f;
        }
    } else { thunderT = 3.5f; }
    glutPostRedisplay();
    glutTimerFunc(16,timer,0);
}

// === 20. Main ===
int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);
    glutInitWindowSize(winW,winH);
    glutInitWindowPosition(50,50);
    glutCreateWindow("Catch The Eggs - Compact Edition");
    glutFullScreen();
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    initGame();
    char bgmp[64];
    for(int i=0;i<NUM_LEVELS;i++){ sprintf(bgmp,"catcheggs_bgm_%d.wav",i); generateBGM(bgmp,bgmConfs[i]); }
    generateThunder("catcheggs_thunder.wav");
    startBGM(0);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutPassiveMotionFunc(mousePassive);
    glutMotionFunc(mousePassive);
    glutMouseFunc(mouseClick);
    glutTimerFunc(16,timer,0);
    glutMainLoop();
    return 0;
}
