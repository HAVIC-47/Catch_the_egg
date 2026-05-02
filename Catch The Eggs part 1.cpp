// Catch The Eggs part 1 - Static scene (sky, grass, basket)
// Build: g++ "Catch The Eggs part 1.cpp" -o "Catch The Eggs part 1.exe" -lfreeglut -lopengl32 -lglu32 -lwinmm
#include <windows.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PI 3.14159265358979f
#define V2(x,y) glVertex2f(x,y)
#define C3(r,g,b) glColor3f(r,g,b)
#define C4(r,g,b,a) glColor4f(r,g,b,a)

int winW=1280, winH=720;

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

void display(){
    drawSky();
    drawGrass();
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
}
void timer(int v){ glutPostRedisplay(); glutTimerFunc(16,timer,0); }

int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);
    glutInitWindowSize(winW,winH);
    glutCreateWindow("Catch The Eggs - Part 1");
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(16,timer,0);
    glutMainLoop();
    return 0;
}
