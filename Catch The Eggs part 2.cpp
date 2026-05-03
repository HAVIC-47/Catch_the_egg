// Catch The Eggs part 2 - + Sun, barn, sticks, chickens (static)
// Build: g++ "Catch The Eggs part 2.cpp" -o "Catch The Eggs part 2.exe" -lfreeglut -lopengl32 -lglu32 -lwinmm
#include <windows.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
using namespace std;

#define PI 3.14159265358979f
#define V2(x,y) glVertex2f(x,y)
#define C3(r,g,b) glColor3f(r,g,b)
#define C4(r,g,b,a) glColor4f(r,g,b,a)

int winW=1280, winH=720;

struct Chick { float x,y,bp; };
vector<float> sticks;
vector<Chick> chicks;

void drawEllipse(float cx,float cy,float rx,float ry,int seg){
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++){ float t=2*PI*i/seg; V2(cx+rx*cosf(t),cy+ry*sinf(t)); }
    glEnd();
}
#define drawCircle(cx,cy,r,seg) drawEllipse(cx,cy,r,r,seg)

void drawRect(float x,float y,float w,float h){
    glBegin(GL_POLYGON); V2(x,y); V2(x+w,y); V2(x+w,y+h); V2(x,y+h); glEnd();
}
void drawGrad(float x,float y,float w,float h,float r1,float g1,float b1,float r2,float g2,float b2){
    glBegin(GL_QUADS);
    C3(r1,g1,b1); V2(x,y); V2(x+w,y);
    C3(r2,g2,b2); V2(x+w,y+h); V2(x,y+h);
    glEnd();
}
void drawSky(){
    drawGrad(0,0,(float)winW,(float)winH, 0.55f,0.80f,0.95f, 0.90f,0.97f,1.00f);
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
void drawGrass(){
    float br=1.0f, gh=60.0f;
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
    (void)bp;
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
void drawBasket(){
    float x=winW/2.0f, y=80.0f, w=150.0f, h=34.0f;
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
}

void initScene(){
    sticks.clear(); chicks.clear();
    sticks.push_back((float)winH-220.0f);
    sticks.push_back((float)winH-360.0f);
    Chick c1={300.0f, sticks[0]+14.0f, 0.0f};
    Chick c2={700.0f, sticks[0]+14.0f, 1.5f};
    Chick c3={500.0f, sticks[1]+14.0f, 0.7f};
    chicks.push_back(c1); chicks.push_back(c2); chicks.push_back(c3);
}

void display(){
    drawSky();
    drawSun(winW-130.0f, winH-140.0f, 50.0f);
    drawBarn(60.0f, 60.0f, 0.9f);
    drawGrass();
    for(size_t i=0;i<sticks.size();i++) drawStick(sticks[i]);
    for(size_t i=0;i<chicks.size();i++) drawChicken(chicks[i].x, chicks[i].y, chicks[i].bp);
    drawBasket();
    glutSwapBuffers();
}
void reshape(int w,int h){
    if(h==0) h=1;
    winW=w; winH=h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,w,0,h);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    initScene();
}
void timer(int v){ glutPostRedisplay(); glutTimerFunc(16,timer,0); }

int main(int argc,char** argv){
    srand((unsigned)time(NULL));
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);
    glutInitWindowSize(winW,winH);
    glutCreateWindow("Catch The Eggs - Part 2");
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    initScene();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(16,timer,0);
    glutMainLoop();
    return 0;
}
