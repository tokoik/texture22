#if defined(__APPLE__)
#  define GL_SILENCE_DEPRECATION
#  include <GLUT/glut.h>
#  include <OpenGL/glext.h>
#else
#  if defined(_WIN32)
//#    pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#    define _USE_MATH_DEFINES
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#  include <GL/glut.h>
#  include <GL/glext.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* トラックボール処理用関数の宣言 */
#include "trackball.h"

/*
** 光源
*/
static const GLfloat lightpos[] = { 4.0f, 5.0f, 6.0f, 1.0f }; /* 位置　　　 */
static const GLfloat lightcol[] = { 1.0f, 1.0f, 1.0f, 1.0f }; /* 直接光強度 */
static const GLfloat lightamb[] = { 0.1f, 0.1f, 0.1f, 1.0f }; /* 環境光強度 */

/*
** テクスチャ
*/
#define TEXWIDTH  128                               /* テクスチャの幅　　　 */
#define TEXHEIGHT 128                               /* テクスチャの高さ　　 */

/*
** キューブマッピングのターゲットと，そこに向かう視線の向きと「上」の方向
*/
static const struct {
  GLenum name;                                  /* テクスチャのターゲット名 */
  GLint x, y;                                   /* ビューポートの位置　　　 */
  GLdouble cx, cy, cz;                          /* 視線の向き　　　　　　　 */
  GLdouble ux, uy, uz;                          /* 「上」の方向　　　　　　 */
} target[] = {
  { /* 左 */
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    0, TEXHEIGHT,
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
  },
  { /* 前 */
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    TEXWIDTH, TEXHEIGHT,
    0.0, 0.0, 1.0,
    0.0, 1.0, 0.0,
  },
  { /* 右 */
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    TEXWIDTH * 2, TEXHEIGHT,
    -1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
  },
  { /* 後 */
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    TEXWIDTH * 3, TEXHEIGHT,
    0.0, 0.0, -1.0,
    0.0, 1.0, 0.0,
  },
  { /* 下 */
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    TEXWIDTH, TEXHEIGHT * 2,
    0.0, 1.0, 0.0,
    0.0, 0.0, -1.0,
  },
  { /* 上 */
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    TEXWIDTH, 0,
    0.0, -1.0, 0.0,
    0.0, 0.0, 1.0,
  },
};

/*
** 星
*/
#define MAXSTARS 200
static GLuint stars;

/*
** ウィンドウサイズ
*/
static GLsizei width, height;

/*
** 初期化
*/
static void init()
{
  /* テクスチャ画像はワード単位に詰め込まれている */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  /* キューブマッピングの各ターゲットのテクスチャを割り当てる */
  for (int i = 0; i < 6; ++i) {
    glTexImage2D(target[i].name, 0, GL_RGBA, TEXWIDTH, TEXHEIGHT, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, 0);
  }

  /* テクスチャを拡大・縮小する方法の指定 */
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  /* テクスチャの繰り返し方法の指定 */
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  /* テクスチャ環境 */
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  /* キューブマッピング用のテクスチャ座標を生成する */
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);

  /* 初期設定 */
  glClearColor(0.3f, 0.3f, 1.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  /* 光源の初期設定 */
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightcol);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lightcol);
  glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);

  /* 星の生成 */
  stars = glGenLists(1);

  /* 星のディスプレイリストを作成する */
  glNewList(stars, GL_COMPILE);

#if 1
  /* 星として箱をいっぱい描く */
  for (int i = 0; i < MAXSTARS; ++i) {
    float r = 2.5f * (float)rand() / (float)RAND_MAX + 2.5f;
    float t = 6.2831853f * (float)rand() / (float)RAND_MAX;
    float p = 3.1415926f * (float)rand() / (float)RAND_MAX;
    float c[] = {
      0.9f * (float)rand() / (float)RAND_MAX + 0.1f,
      0.9f * (float)rand() / (float)RAND_MAX + 0.1f,
      0.9f * (float)rand() / (float)RAND_MAX + 0.1f,
    };

    glPushMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, c);
    glTranslatef(r * sinf(p) * cosf(t), r * cosf(p), r * sinf(p) * sinf(t));
    glScalef(0.5f, 0.5f, 0.5f);
    glutSolidCube(0.8);
    glPopMatrix();
  }
#else
  /* 星として箱を１つだけ描く */
  glPushMatrix();
  glTranslated(0.0, 0.0, 3.0);
  glutSolidCube(1.0);
  glPopMatrix();
#endif

  /* ディスプレイリストの作成終了 */
  glEndList();
}

/*
** シーンの描画
*/
static void scene()
{
  /* 星の描画 */
  glCallList(stars);

  /* 星に取り囲まれる物体の材質を設定する */
  static const GLfloat color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);

  /* テクスチャマッピング開始 */
  glEnable(GL_TEXTURE_CUBE_MAP);

  /* テクスチャ座標の自動生成を有効にする */
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_GEN_R);

#if 1
  /* ティーポットを描く */
  glutSolidTeapot(1.8);
#else
  /* 球を描く */
  glutSolidSphere(1.5, 32, 16);
#endif

  /* テクスチャ座標の自動生成を無効にする */
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_GEN_R);

  /* テクスチャマッピング終了 */
  glDisable(GL_TEXTURE_CUBE_MAP);
}


/****************************
** GLUT のコールバック関数 **
****************************/

static void display()
{
  /* 透視変換行列の設定 */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(90.0, 1.0, 1.0, 10.0);

  /* モデルビュー変換行列の設定 */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* テクスチャの６面分の画面クリア */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* テクスチャの作成 */
  for (int i = 0; i < 6; ++i) {

    /* ビューポートをテクスチャのサイズに設定する */
    glViewport(target[i].x, target[i].y, TEXWIDTH, TEXHEIGHT);

    /* 視線の方向を設定して，その向きに見えるものをレンダリングする */
    glPushMatrix();
    gluLookAt(0.0, 0.0, 0.0,
      target[i].cx, target[i].cy, target[i].cz,
      target[i].ux, target[i].uy, target[i].uz);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    glMultMatrixd(trackballRotation());
    glCallList(stars);
    glPopMatrix();

    /* レンダリングした結果をテクスチャメモリに移す */
    glCopyTexSubImage2D(target[i].name, 0, 0, 0,
      target[i].x, target[i].y, TEXWIDTH, TEXHEIGHT);
  }

  /* テクスチャ変換行列の設定 */
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glScaled(-1.0, -1.0, 1.0);

#if 1 /* ここを 0 にするとマッピングするテクスチャの方を見ることができます */
  /* 表示用の画面クリア */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* ウィンドウ全体をビューポートにする */
  glViewport(0, 0, width, height);

  /* 透視変換行列の指定 */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (double)width / (double)height, 0.1, 10.0);

  /* モデルビュー変換行列の設定 */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* 視点の移動（物体の方を奥に移動）*/
  glTranslated(0.0, 0.0, -7.0);

  /* 光源の位置を設定 */
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  /* トラックボール処理による回転 */
  glMultMatrixd(trackballRotation());

  /* シーンの描画 */
  scene();
#endif

  /* ダブルバッファリング */
  glutSwapBuffers();
}

static void resize(int w, int h)
{
  /* ウィンドウサイズの縮小を制限する */
  if (w < TEXWIDTH * 4 || h < TEXHEIGHT * 3) {
    if (w < TEXWIDTH * 4) w = TEXWIDTH * 4;
    if (h < TEXHEIGHT * 3) h = TEXHEIGHT * 3;
    glutReshapeWindow(w, h);
  }

  /* ウィンドウサイズの保存 */
  width = w;
  height = h;

  /* トラックボールする範囲 */
  trackballRegion(w, h);
}

static void idle()
{
  /* 画面の描き替え */
  glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
  switch (button) {
  case GLUT_LEFT_BUTTON:
    switch (state) {
    case GLUT_DOWN:
      /* トラックボール開始 */
      trackballStart(x, y);
      glutIdleFunc(idle);
      break;
    case GLUT_UP:
      /* トラックボール停止 */
      trackballStop(x, y);
      glutIdleFunc(0);
      break;
    default:
      break;
    }
    break;
    default:
      break;
  }
}

static void motion(int x, int y)
{
  /* トラックボール移動 */
  trackballMotion(x, y);
}

static void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 'q':
  case 'Q':
  case '\033':
    /* ESC か q か Q をタイプしたら終了 */
    exit(0);
  default:
    break;
  }
}

/*
** メインプログラム
*/
int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(TEXWIDTH * 4, TEXWIDTH * 3);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow(argv[0]);
  glutDisplayFunc(display);
  glutReshapeFunc(resize);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutKeyboardFunc(keyboard);
  init();
  glutMainLoop();
  return 0;
}
