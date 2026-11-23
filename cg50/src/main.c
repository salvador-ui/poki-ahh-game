/*
 * Squad Rush — fx-CG50 port (PrizmSDK / libfxcg).
 *
 * A straight port of the HTML version's game logic plus the "CG50 view"
 * renderer: software perspective projection, solid rectangles and convex
 * polygons drawn straight into the 384x216 RGB565 VRAM, a 3x5 pixel font.
 *
 * Controls:  LEFT/RIGHT move    EXE start/confirm    EXIT/OPTN/F1 pause
 *            MENU (while paused) back to the calculator's main menu
 */
#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/rtc.h>
#include <fxcg/file.h>
#include <fxcg/system.h>
#include <string.h>

#ifndef KEY_PRGM_EXE
#define KEY_PRGM_EXE 31      /* matrix code for EXE (not defined in this SDK version) */
#endif

typedef long long i64;
typedef unsigned int u32;
typedef unsigned short u16;

/* ---------------------------------------------------------------- screen */
#define W 384
#define H 216
static u16 *vram;

/* ---------------------------------------------------------------- world */
#define ROAD 50.0f
#define PZ 10.0f
#define FAR 440.0f
#define CAMD 50.0f
#define CAMH 64.0f
#define FOC 131.0f
#define HOR 8.0f
#define FS 0.55f
#define BOSS_STOP (PZ + 16.0f)
#define MAX_DRAWN 60      /* used for formation maths (same as HTML) */
#define CG_DRAWN 30       /* soldiers actually drawn per group */
#define MAX_BULLETS 80
#define MAX_OBJS 32
#define MAX_FX 60
#define MAX_TEXTS 8
#define MAX_N 1000000000000000LL   /* 1e15: effectively unlimited */

static const signed char FORM[60][2] = {{0,0},{-2,2},{0,-4},{3,4},{-6,-1},{6,-3},{-2,6},{-4,-6},{9,2},{-9,3},{4,-7},{3,8},{-10,-4},{11,-2},{-7,8},{-2,-10},{10,7},{-13,0},{10,-8},{-1,11},{-9,-9},{15,2},{-12,7},{3,-12},{8,11},{-15,-4},{15,-5},{-6,12},{-6,-13},{15,6},{-17,4},{10,-12},{3,14},{-15,-9},{19,-1},{-13,11},{0,-15},{13,12},{-20,-1},{16,-10},{-4,16},{-11,-14},{20,4},{-19,8},{7,-16},{8,16},{-20,-7},{21,-5},{-11,15},{-5,-18},{18,10},{-23,2},{15,-14},{1,19},{-17,-13},{24,1},{-18,12},{3,-19},{14,16},{-24,-4}};
static const unsigned char RAD[61] = {7,7,9,10,10,11,12,12,13,14,14,15,15,16,16,16,17,17,18,18,18,19,19,19,20,20,20,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,28,29,29};
static unsigned char FORM_ORDER[60];

/* ---------------------------------------------------------------- colours */
#define RGB(r, g, b) ((u16)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define C_SKY       RGB(0xbf,0xe6,0xf5)
#define C_SEAFAR    RGB(0x6f,0xb4,0xd6)
#define C_SEA       RGB(0x2f,0x86,0xb8)
#define C_SEANEAR   RGB(0x2a,0x78,0xa8)
#define C_WAVE      RGB(0x7c,0xc4,0xe8)
#define C_ROAD      RGB(0xcb,0xbd,0xbb)
#define C_ROADEDGE  RGB(0xb3,0xa4,0xa3)
#define C_DASH      RGB(0xff,0xff,0xff)
#define C_WALLTOP   RGB(0xbd,0xbd,0xbd)
#define C_WALLIN    RGB(0x8e,0x8e,0x8e)
#define C_WALLFRONT RGB(0xa5,0xa5,0xa5)
#define C_PILTOP    RGB(0x9a,0x9a,0x9a)
#define C_PILFRONT  RGB(0x7b,0x7b,0x7b)
#define C_ME        RGB(0x2f,0x7b,0xf0)
#define C_MEDARK    RGB(0x1a,0x4f,0xb0)
#define C_MELIGHT   RGB(0x9c,0xc4,0xff)
#define C_FOE       RGB(0xe8,0x34,0x3a)
#define C_FOEDARK   RGB(0x9c,0x1c,0x22)
#define C_GUN       RGB(0x26,0x30,0x3a)
#define C_GOOD      RGB(0x2f,0x8c,0xff)
#define C_GOODEDGE  RGB(0xbf,0xe0,0xff)
#define C_BAD       RGB(0xff,0x3b,0x3b)
#define C_BADEDGE   RGB(0xff,0xc4,0xc4)
#define C_POST      RGB(0x55,0x60,0x6a)
#define C_CRATE     RGB(0xd4,0x88,0x3c)
#define C_CRATETOP  RGB(0xe9,0xa6,0x5e)
#define C_CRATEDARK RGB(0x8a,0x50,0x20)
#define C_BULLET    RGB(0x7f,0xd8,0xff)
#define C_GOLD      RGB(0xff,0xd2,0x3a)
#define C_WHITE     RGB(0xff,0xff,0xff)
#define C_BLACK     RGB(0x00,0x00,0x00)
#define C_SHADOW    RGB(0x20,0x30,0x3c)
#define C_PILLME    RGB(0x1c,0x3c,0x96)
#define C_PILLFOE   RGB(0x96,0x14,0x18)
#define C_PANEL     RGB(0x16,0x28,0x3a)
#define C_PANELEDGE RGB(0x3a,0x5a,0x7a)
#define C_GREY      RGB(0x9a,0xa3,0xad)
#define C_GREYDARK  RGB(0x5d,0x66,0x70)
#define C_BONE      RGB(0xf1,0xea,0xd8)
#define C_EYE       RGB(0xff,0xe1,0x4a)
#define C_HPBACK    RGB(0x30,0x00,0x00)
#define C_AQUA      RGB(0x7f,0xff,0xd4)

/* ---------------------------------------------------------------- difficulty */
typedef struct {
    const char *name; u16 col;
    float enemy; int twoGood; float boss, speed, crate, bad; int start;
} Diff;
static const Diff DIFFS[4] = {
    {"EASY",   RGB(0x3f,0xd2,0x7a), 1.0f, 25, 1.0f, 1.00f, 1, 1.0f, 5},
    {"NORMAL", RGB(0x3f,0xb0,0xff), 1.6f, 12, 1.7f, 1.12f, 2, 1.4f, 8},
    {"HARD",   RGB(0xff,0x9a,0x2e), 2.4f,  5, 2.6f, 1.25f, 3, 1.9f, 12},
    {"INSANE", RGB(0xff,0x3b,0x3b), 3.5f,  0, 4.0f, 1.40f, 5, 2.6f, 18},
};
static int diff = 0;
#define D (&DIFFS[diff])

/* ---------------------------------------------------------------- small maths */
static int iround(float f) { return (int)(f >= 0 ? f + 0.5f : f - 0.5f); }
static float ffloor(float f) { int i = (int)f; return (f < 0 && (float)i != f) ? (float)(i - 1) : (float)i; }
static float fabsf_(float f) { return f < 0 ? -f : f; }
static float fminf_(float a, float b) { return a < b ? a : b; }
static float fmaxf_(float a, float b) { return a > b ? a : b; }
static float fsign(float f) { return f > 0 ? 1.0f : (f < 0 ? -1.0f : 0.0f); }
static float fmodpos(float a, float b) { return a - b * ffloor(a / b); }
#define PI_F 3.14159265f
static float fsin(float x) {          /* good enough for animation */
    x = fmodpos(x + PI_F, 2 * PI_F) - PI_F;
    float y = 1.27323954f * x - 0.405284735f * x * fabsf_(x);
    return 0.225f * (y * fabsf_(y) - y) + y;
}
static float fcos(float x) { return fsin(x + PI_F / 2); }
static i64 isqrt64(i64 n) {
    if (n < 2) return n < 0 ? 0 : n;
    i64 x = n, y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + n / x) / 2; }
    return x;
}
static i64 imin(i64 a, i64 b) { return a < b ? a : b; }
static i64 imax(i64 a, i64 b) { return a > b ? a : b; }

/* RNG: xorshift32, same as the HTML version */
static u32 seed = 12345;
static u32 rnd(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }
static int rint_(int a, int b) { return a + (int)(rnd() % (u32)(b - a + 1)); }
static u32 hash32(u32 i) {
    i = (i ^ 0x9e3779b9u) * 0x85ebca6bu; i ^= i >> 13; i *= 0xc2b2ae35u; return i ^ (i >> 16);
}

/* ---------------------------------------------------------------- text helpers */
static int s_len(const char *s) { int n = 0; while (s[n]) n++; return n; }
static char *s_cpy(char *d, const char *s) { while ((*d = *s)) { d++; s++; } return d; }
static char *s_i64(char *d, i64 v) {
    char tmp[24]; int n = 0;
    if (v < 0) { *d++ = '-'; v = -v; }
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *d++ = tmp[--n];
    *d = 0; return d;
}
/* 1234 -> "1234", 12345 -> "12.3K" ... */
static char *fmt(char *d, i64 n) {
    static const char U[] = "KMBTQ";
    if (n < 0) n = 0;
    if (n < 10000) return s_i64(d, n);
    int i = -1; i64 div = 1;
    while (n / div >= 1000 && i < 4) { div *= 1000; i++; }
    i64 whole = n / div;
    d = s_i64(d, whole);
    if (whole < 100) { *d++ = '.'; *d++ = (char)('0' + (n * 10 / div) % 10); }
    *d++ = U[i]; *d = 0;
    return d;
}

/* ---------------------------------------------------------------- game state */
enum { T_GATE, T_ENEMY, T_CRATE, T_BOSS, T_PLAYER, T_BULLETS };
enum { OP_ADD, OP_ROOT, OP_DIV, OP_MUL, OP_POW };   /* in "both gates" order */
typedef struct { int k; i64 v; int hits; } Op;
typedef struct {
    int type, dead;
    float x, z;
    i64 n, hp, max, reward;
    Op ops[2]; int used;
    int hold, dir, planted, swing;
} Obj;
typedef struct { float x, z, pz; i64 d; int dead; } Bullet;
typedef struct { float x, y, z, vx, vy, vz; int t; u16 col; } Fx;
typedef struct { float x, y, z; int t; u16 col; char txt[20]; } Text;

enum { M_TITLE, M_PLAY, M_OVER };
static struct {
    int mode, t, overT, shake, bossAlive, newBest, saved;
    float px, dist, v, fireAcc, nextSpawn, lastBoss, nextBoss;
    i64 n, kills;
    Obj objs[MAX_OBJS]; int nobjs;
    Bullet bullets[MAX_BULLETS]; int nbullets;
    Fx fx[MAX_FX]; int nfx;
    Text texts[MAX_TEXTS]; int ntexts;
} S;
static int best[4];

static float radius(i64 n) { return RAD[n < 0 ? 0 : (n > MAX_DRAWN ? MAX_DRAWN : n)] * FS; }
static int level(void) { return (int)(S.dist / 2000.0f); }

/* ---------------------------------------------------------------- save (main memory) */
static unsigned char SAVE_DIR[] = "@SQRUSH", SAVE_ITEM[] = "BEST";
static void loadBest(void) {
    int len = 0;
    MCS_CreateDirectory(SAVE_DIR);
    if (MCSGetDlen2(SAVE_DIR, SAVE_ITEM, &len) == 0 && len == (int)sizeof(best)) {
        int buf[4];
        MCSGetData1(0, len, buf);
        for (int i = 0; i < 4; i++) best[i] = buf[i] < 0 ? 0 : buf[i];
    }
}
static void writeBest(void) {
    int len = 0;
    MCS_CreateDirectory(SAVE_DIR);
    if (MCSGetDlen2(SAVE_DIR, SAVE_ITEM, &len) == 0 && len == (int)sizeof(best))
        MCSOvwDat2(SAVE_DIR, SAVE_ITEM, sizeof(best), best, 0);
    else {
        MCSDelVar2(SAVE_DIR, SAVE_ITEM);
        MCSPutVar2(SAVE_DIR, SAVE_ITEM, sizeof(best), best);
    }
}
/* records the run's distance if it's a new best; used on game over and restart */
static int saveBest(void) {
    if (S.mode == M_TITLE || S.saved) return 0;
    S.saved = 1;
    int m = (int)(S.dist / 10.0f);
    if (m > best[diff]) { best[diff] = m; writeBest(); return 1; }
    return 0;
}

/* ---------------------------------------------------------------- spawning */
static Obj *newObj(int type) {
    if (S.nobjs >= MAX_OBJS) return 0;
    Obj *o = &S.objs[S.nobjs++];
    memset(o, 0, sizeof(*o));
    o->type = type;
    return o;
}
static void popText(float x, float y, float z, const char *txt, u16 col) {
    if (S.ntexts >= MAX_TEXTS) return;
    Text *t = &S.texts[S.ntexts++];
    t->x = x; t->y = y; t->z = z; t->t = 32; t->col = col;
    int n = s_len(txt); if (n > 19) n = 19;
    memcpy(t->txt, txt, n); t->txt[n] = 0;
}
static void burst(float x, float y, float z, u16 col, int k) {
    for (int i = 0; i < k && S.nfx < MAX_FX; i++) {
        Fx *p = &S.fx[S.nfx++];
        p->x = x; p->y = y; p->z = z;
        p->vx = (rint_(0, 40) - 20) / 20.0f; p->vy = rint_(5, 25) / 10.0f; p->vz = (rint_(0, 40) - 20) / 20.0f;
        p->t = rint_(8, 16); p->col = col;
    }
}
static Op goodOp(int lv) {
    Op o = {OP_ADD, 0, 0};
    int r = rint_(0, 99);
    if (r < 55) { o.k = OP_ADD; o.v = rint_(3, 8) + lv * 3; }
    else if (r < 80) { o.k = OP_MUL; o.v = 2; }
    else if (r < 90) { o.k = OP_MUL; o.v = 3; }
    else if (r < 97 || S.n < 20) { o.k = OP_POW; o.v = 2; }
    else { o.k = OP_POW; o.v = 3; }
    return o;
}
static Op badOp(int lv) {
    Op o = {OP_ADD, 0, 0};
    int r = rint_(0, 9);
    if (r < 6) { o.v = -(i64)((rint_(2, 6) + lv * 3) * D->bad + (float)S.n * 0.05f * D->bad + 0.5f); return o; }
    if (r < 9 - iround(D->bad - 1)) { o.k = OP_DIV; o.v = 2; return o; }
    if (D->bad >= 1.4f && rnd() % 3 == 0) { o.k = OP_ROOT; o.v = 2; return o; }
    if (D->bad >= 2.5f) { o.k = OP_DIV; o.v = 3; return o; }
    o.v = rint_(1, 3);
    return o;
}
static void spawnGates(float z) {
    int lv = level();
    Op a = goodOp(lv), b = rint_(0, 99) < D->twoGood ? goodOp(lv) : badOp(lv);
    if (rnd() & 1) { Op t = a; a = b; b = t; }
    Obj *o = newObj(T_GATE); if (!o) return;
    o->z = z; o->ops[0] = a; o->ops[1] = b;
}
static void spawnEnemies(float z) {
    int lv = level(), groups = rint_(0, 2) == 0 ? 2 : 1;
    for (int i = 0; i < groups; i++) {
        float n = (rint_(3, 6) + lv * rint_(3, 6) + (float)S.n / rint_(3, 6)) * D->enemy;
        Obj *o = newObj(T_ENEMY); if (!o) return;
        o->n = imin(MAX_N, (i64)(n + 0.5f));
        o->x = groups == 2 ? (i ? 25.0f : -25.0f) : (float)rint_(-30, 30);
        o->z = z + i * 12;
    }
}
static void spawnCrate(float z) {
    int lv = level();
    i64 hp = (i64)((rint_(8, 16) + lv * 8) * (0.7f + 0.3f * D->crate) + 0.5f);
    Obj *o = newObj(T_CRATE); if (!o) return;
    o->x = (float)rint_(-36, 36); o->z = z; o->hp = hp; o->reward = imax(3, hp / 3);
}
static void spawnBoss(void) {
    i64 hp = (i64)((120 + level() * 90 + (float)S.n * 0.4f) * D->boss + 0.5f);
    Obj *o = newObj(T_BOSS); if (!o) return;
    o->x = 0; o->z = FAR; o->hp = o->max = hp; o->dir = 1;
    S.bossAlive = 1;
    popText(0, 30, 150, "BOSS!", C_BAD);
}
static void spawnPattern(void) {
    int r = rint_(0, 99);
    if (r < 38) { spawnGates(FAR); S.nextSpawn += 200; }
    else if (r < 68) { spawnEnemies(FAR); S.nextSpawn += 170; }
    else if (r < 82) { spawnCrate(FAR); S.nextSpawn += 140; }
    else { spawnGates(FAR); spawnEnemies(FAR + 120); S.nextSpawn += 290; }
}

/* ---------------------------------------------------------------- gates */
/* label chars: '*' = times, '%' = divide, '\1' = superscript 2, '\2' = superscript 3, '\3' = root */
static void opLabel(const Op *op, char *d) {
    switch (op->k) {
    case OP_ADD: if (op->v >= 0) *d++ = '+'; s_i64(d, op->v); break;
    case OP_MUL: *d++ = '*'; s_i64(d, op->v); break;
    case OP_DIV: *d++ = '%'; s_i64(d, op->v); break;
    case OP_POW: d[0] = 'X'; d[1] = op->v == 2 ? '\1' : '\2'; d[2] = 0; break;
    default:     d[0] = '\3'; d[1] = 'X'; d[2] = 0; break;
    }
}
static int opGood(const Op *op) { return op->k == OP_MUL || op->k == OP_POW || (op->k == OP_ADD && op->v > 0); }
static i64 applyOp(const Op *op, i64 n) {
    i64 r;
    switch (op->k) {
    case OP_ADD: r = n + op->v; break;
    case OP_MUL: r = n > MAX_N / op->v ? MAX_N : n * op->v; break;
    case OP_POW:
        if (op->v == 2) r = n > 31622776LL ? MAX_N : n * n;
        else r = n > 100000LL ? MAX_N : n * n * n;
        break;
    case OP_ROOT: r = isqrt64(n); break;
    default: r = n / op->v; break;
    }
    return imax(0, imin(MAX_N, r));
}
static int straddling(void) { return fabsf_(S.px) < fmaxf_(2.5f, radius(S.n) * 0.55f); }

static void newGame(int play) {
    saveBest();
    S.mode = play ? M_PLAY : M_TITLE;
    S.t = S.overT = S.shake = S.bossAlive = S.newBest = S.saved = 0;
    S.px = 0; S.n = D->start; S.dist = 0; S.v = 1.6f; S.fireAcc = 0;
    S.nextSpawn = 120; S.lastBoss = 0; S.nextBoss = 3600; S.kills = 0;
    S.nobjs = S.nbullets = S.nfx = S.ntexts = 0;
    seed = (u32)RTC_GetTicks() * 2654435761u | 1u;
}
static void bossDown(Obj *o) {
    char buf[24];
    i64 bonus = 25 + level() * 25;
    o->dead = 1; S.bossAlive = 0; S.n += bonus; S.shake = 10;
    s_i64(s_cpy(buf, "BOSS DOWN +"), bonus);
    popText(o->x, 30, o->z, buf, C_GOLD);
    burst(o->x, 10, o->z, C_FOE, 30);
}

/* ---------------------------------------------------------------- update (30 Hz) */
static void update(int dir) {
    char buf[24];
    S.t++;
    if (S.mode != M_PLAY) { if (S.mode == M_OVER) S.overT++; return; }

    if (dir) S.px += dir * 2.6f;
    float pr = radius(S.n);
    S.px = fmaxf_(-ROAD + pr, fminf_(ROAD - pr, S.px));

    S.v = fminf_(3.4f, 1.6f + S.dist / 6000.0f) * D->speed;
    Obj *boss = 0;
    for (int i = 0; i < S.nobjs; i++) if (S.objs[i].type == T_BOSS) boss = &S.objs[i];
    float v = boss ? (boss->planted ? 0 : S.v * 0.3f) : S.v;
    S.dist += v;
    if (!S.bossAlive) {
        if (S.dist >= S.nextBoss) { spawnBoss(); S.lastBoss = S.nextBoss; S.nextBoss += 4400; S.nextSpawn = S.dist + 320; }
        else if (S.dist >= S.nextSpawn) spawnPattern();
    }

    /* fire */
    S.fireAcc += fminf_(0.3f + (float)S.n * 0.045f, 4.0f);
    while (S.fireAcc >= 1) {
        S.fireAcc -= 1;
        if (S.nbullets < MAX_BULLETS) {
            int k = (int)imin(S.n, MAX_DRAWN);
            const signed char *f = FORM[rint_(0, (k > 0 ? k : 1) - 1)];
            Bullet *b = &S.bullets[S.nbullets++];
            b->x = S.px + f[0] * FS; b->z = b->pz = PZ + f[1] * FS;
            b->d = imax(1, S.n / 150); b->dead = 0;
        }
    }
    for (int i = 0; i < S.nbullets; i++) { S.bullets[i].pz = S.bullets[i].z; S.bullets[i].z += 9; }

    /* move objects */
    for (int i = 0; i < S.nobjs; i++) {
        Obj *o = &S.objs[i];
        if (o->type == T_GATE || o->type == T_CRATE) o->z -= v;
        else if (o->type == T_ENEMY) {
            if (o->hold > 0) o->hold--;
            else {
                o->z -= v + 1.0f;
                if (o->z < 300) o->x += fsign(S.px - o->x) * fminf_(0.5f, fabsf_(S.px - o->x));
            }
            float er = radius(o->n);
            o->x = fmaxf_(-ROAD + er, fminf_(ROAD - er, o->x));
        } else if (o->type == T_BOSS) {
            if (!o->planted) {
                o->z -= o->z > 180 ? 1.4f : 0.5f;
                o->x += o->dir * 0.5f;
                if (o->x < -25 || o->x > 25) o->dir = -o->dir;
                if (o->z <= BOSS_STOP) { o->z = BOSS_STOP; o->planted = 1; S.shake = 8; }
            } else o->x += fsign(S.px - o->x) * fminf_(0.8f, fabsf_(S.px - o->x));
            o->swing++;
        }
    }

    /* bullets vs objects (swept along z) */
    for (int bi = 0; bi < S.nbullets; bi++) {
        Bullet *b = &S.bullets[bi];
        if (b->z > FAR + 20) { b->dead = 1; continue; }
        for (int i = 0; i < S.nobjs; i++) {
            Obj *o = &S.objs[i];
            if (o->dead) continue;
            if (o->type == T_GATE) {
                if (o->used || !(b->pz <= o->z && b->z >= o->z)) continue;
                Op *op = &o->ops[b->x < 0 ? 0 : 1];
                b->dead = 1;
                if (op->k == OP_ADD && ++op->hits >= 4) { op->hits = 0; op->v++; }
                break;
            }
            float hx, hz;
            if (o->type == T_ENEMY) hx = hz = radius(o->n);
            else if (o->type == T_CRATE) { hx = 8; hz = 6; }
            else { hx = 14; hz = 6; }
            if (fabsf_(b->x - o->x) >= hx || b->z < o->z - hz || b->pz > o->z + hz) continue;
            b->dead = 1;
            if (o->type == T_ENEMY) {
                i64 k = imin(b->d, o->n); o->n -= k; S.kills += k;
                burst(b->x, 3, o->z - hz, C_FOE, 2);
                if (o->n <= 0) { o->dead = 1; burst(o->x, 3, o->z, C_FOE, 10); }
            } else if (o->type == T_CRATE) {
                if ((o->hp -= b->d) <= 0) {
                    o->dead = 1; S.n += o->reward;
                    buf[0] = '+'; fmt(buf + 1, o->reward);
                    popText(o->x, 16, o->z, buf, C_GOLD); burst(o->x, 6, o->z, C_CRATE, 14);
                }
            } else {
                if ((o->hp -= b->d) <= 0) bossDown(o);
                else if ((o->hp & 7) == 0) burst(b->x, 10, o->z - hz, C_FOE, 1);
            }
            break;
        }
    }
    { int j = 0; for (int i = 0; i < S.nbullets; i++) if (!S.bullets[i].dead) S.bullets[j++] = S.bullets[i]; S.nbullets = j; }

    /* objects vs squad */
    for (int i = 0; i < S.nobjs; i++) {
        Obj *o = &S.objs[i];
        if (o->dead) continue;
        if (o->type == T_GATE) {
            if (!o->used && o->z <= PZ) {
                i64 before = S.n;
                o->used = 1;
                if (straddling()) {          /* both gates: adds first, then multipliers */
                    const Op *a = &o->ops[0], *c = &o->ops[1];
                    if (a->k > c->k) { const Op *t = a; a = c; c = t; }
                    S.n = applyOp(c, applyOp(a, S.n));
                    popText(S.px, 24, PZ, "BOTH!", C_AQUA);
                } else S.n = applyOp(&o->ops[S.px < 0 ? 0 : 1], S.n);
                buf[0] = S.n >= before ? '+' : '-';
                fmt(buf + 1, S.n >= before ? S.n - before : before - S.n);
                popText(S.px, 14, PZ, buf, S.n >= before ? C_GOLD : C_BAD);
            }
            if (o->z < -30) o->dead = 1;
        } else if (o->type == T_ENEMY) {
            float r = radius(o->n) + radius(S.n) - 2;
            if (fabsf_(o->x - S.px) < r && fabsf_(o->z - PZ) < r) {
                i64 m = imin(o->n, S.n);
                i64 d = imin(imin(imax(1, (m + 9) / 10), o->n), S.n);
                o->n -= d; S.n -= d; o->hold = 2; S.shake = 3;
                burst((o->x + S.px) / 2, 3, (o->z + PZ) / 2, C_FOE, 2);
                if (o->n <= 0) o->dead = 1;
            }
            if (o->z < -30) o->dead = 1;
        } else if (o->type == T_CRATE) {
            if (fabsf_(o->x - S.px) < 8 + radius(S.n) && fabsf_(o->z - PZ) < 8) {
                i64 d = imin(S.n, (i64)((2 + level() * 2) * D->crate + (float)S.n * 0.03f * D->crate + 0.5f));
                S.n -= d; o->dead = 1; S.shake = 6;
                buf[0] = '-'; fmt(buf + 1, d);
                popText(S.px, 14, PZ, buf, C_BAD); burst(o->x, 6, o->z, C_CRATE, 14);
            }
            if (o->z < -30) o->dead = 1;
        } else if (o->type == T_BOSS) {
            if (o->planted && o->swing % 8 == 0) {
                i64 d = imin(S.n, imax(2, (i64)((float)S.n * 0.018f * D->boss) + 1));
                i64 back = imin(o->hp, d);
                S.n -= d; o->hp -= back; S.shake = 5;
                burst(S.px, 4, PZ + 4, C_ME, 4);
                buf[0] = '-'; fmt(buf + 1, d);
                popText(S.px, 12, PZ + 4, buf, C_BAD);
                if (o->hp <= 0) bossDown(o);
            }
        }
    }
    { int j = 0; for (int i = 0; i < S.nobjs; i++) if (!S.objs[i].dead) S.objs[j++] = S.objs[i]; S.nobjs = j; }

    { int j = 0;
      for (int i = 0; i < S.nfx; i++) {
          Fx *p = &S.fx[i];
          p->x += p->vx; p->y += p->vy; p->z += p->vz; p->vy -= 0.35f; p->t--;
          if (p->t > 0 && p->y > -2) S.fx[j++] = *p;
      }
      S.nfx = j; }
    { int j = 0;
      for (int i = 0; i < S.ntexts; i++) { S.texts[i].y += 0.6f; if (--S.texts[i].t > 0) S.texts[j++] = S.texts[i]; }
      S.ntexts = j; }
    if (S.shake > 0) S.shake--;
    if (S.n > MAX_N) S.n = MAX_N;
    if (S.n <= 0) { S.n = 0; S.mode = M_OVER; S.overT = 0; S.newBest = saveBest(); }
}

/* ================================================================ rendering */
static int shakeX, shakeY;

static void rect(int x0, int y0, int x1, int y1, u16 c) {      /* fills [x0,x1) x [y0,y1) */
    x0 += shakeX; x1 += shakeX; y0 += shakeY; y1 += shakeY;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > W) x1 = W;
    if (y1 > H) y1 = H;
    if (x0 >= x1 || y0 >= y1) return;
    u32 c2 = ((u32)c << 16) | c;
    for (int y = y0; y < y1; y++) {
        u16 *p = vram + y * W + x0, *e = vram + y * W + x1;
        if (((u32)p & 2) && p < e) *p++ = c;
        while (p + 2 <= e) { *(u32 *)p = c2; p += 2; }
        if (p < e) *p = c;
    }
}
static void rectf(float x0, float y0, float x1, float y1, u16 c) { rect(iround(x0), iround(y0), iround(x1), iround(y1), c); }

/* Convex polygon, scanline fill (pts: x,y pairs, max 8 points).
 * Vertices are converted to 16.16 fixed point once, each edge gets one slope
 * division, and the per-row work is integer only: the CG50 has no FPU for
 * add-ins, so per-row float maths would be very slow. */
static int fx16(float f) {
    if (f > 8000) f = 8000;          /* keeps every 16.16 difference inside 32 bits */
    if (f < -8000) f = -8000;
    return (int)(f * 65536.0f);
}
static void poly(const float *pts, int n, u16 c) {
    int X[8], Y[8], slope[8];
    int ymin = 0x7fffffff, ymax = -0x7fffffff;
    for (int i = 0; i < n; i++) {
        X[i] = fx16(pts[i * 2]); Y[i] = fx16(pts[i * 2 + 1]);
        if (Y[i] < ymin) ymin = Y[i];
        if (Y[i] > ymax) ymax = Y[i];
    }
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n, dy = Y[j] - Y[i];
        slope[i] = dy ? (int)(((i64)(X[j] - X[i]) << 16) / dy) : 0;
    }
    int y0 = (ymin + 32768) >> 16, y1 = (ymax + 32768) >> 16;   /* rows whose centre is inside */
    if (y0 < -shakeY) y0 = -shakeY;
    if (y1 > H - shakeY) y1 = H - shakeY;
    for (int y = y0; y < y1; y++) {
        int yc = (y << 16) + 32768, xl = 0x7fffffff, xr = -0x7fffffff;
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n, ya = Y[i], yb = Y[j];
            if ((ya <= yc && yb > yc) || (yb <= yc && ya > yc)) {
                int x = X[i] + (int)(((i64)(yc - ya) * slope[i]) >> 16);
                if (x < xl) xl = x;
                if (x > xr) xr = x;
            }
        }
        if (xl <= xr) rect((xl + 32768) >> 16, y, (xr + 32768) >> 16, y + 1, c);
    }
}
static float sc(float z) { float d = z + CAMD; if (d < 4) d = 4; return FOC / d; }
static float sx(float x, float s) { return W / 2 + x * s; }
static float sy(float y, float s) { return HOR + (CAMH - y) * s; }
/* world-space quad: 4 x (x, y, z) */
static void quad(float x0, float y0, float z0, float x1, float y1, float z1,
                 float x2, float y2, float z2, float x3, float y3, float z3, u16 c) {
    float p[8], s;
    s = sc(z0); p[0] = sx(x0, s); p[1] = sy(y0, s);
    s = sc(z1); p[2] = sx(x1, s); p[3] = sy(y1, s);
    s = sc(z2); p[4] = sx(x2, s); p[5] = sy(y2, s);
    s = sc(z3); p[6] = sx(x3, s); p[7] = sy(y3, s);
    poly(p, 4, c);
}

/* ---- 3x5 pixel font ---- */
static const char *glyph(char ch) {
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);
    static const char *DIG[10] = {"111101101101111","010110010010111","111001111100111","111001111001111",
        "101101111001001","111100111001111","111100111101111","111001001010010","111101111101111","111101111001111"};
    static const char *ALPHA[26] = {"010101111101101","110101110101110","011100100100011","110101101101110",
        "111100110100111","111100110100100","011100101101011","101101111101101","111010010010111",
        "001001001101010","101101110101101","100100100100111","101111111101101","110101101101101",
        "010101101101010","110101110100100","010101101110011","110101110101101","011100010001110",
        "111010010010010","101101101101111","101101101101010","101101111111101","101101010101101",
        "101101010010010","111001010100111"};
    if (ch >= '0' && ch <= '9') return DIG[ch - '0'];
    if (ch >= 'A' && ch <= 'Z') return ALPHA[ch - 'A'];
    switch (ch) {
    case '+': return "000010111010000"; case '-': return "000000111000000";
    case '*': return "000101010101000"; case '%': return "010000111000010";
    case '\3': return "001001101011001"; case '.': return "000000000000010";
    case '!': return "010010010000010"; case '/': return "001001010100100";
    case '<': return "001010100010001"; case '>': return "100010001010100";
    case ':': return "000010000010000";
    }
    return "000000000000000";
}
static int textW(const char *s, int sz) {
    int w = 0;
    for (; *s; s++) w += (*s == '\1' || *s == '\2') ? 3 * (sz > 1 ? sz - 1 : 1) + 1 : 4 * sz;
    return w - sz;
}
static void drawGlyph(char ch, int x, int y, int sz, u16 c) {
    const char *g = glyph(ch);
    for (int r = 0; r < 5; r++) for (int k = 0; k < 3; k++)
        if (g[r * 3 + k] == '1') rect(x + k * sz, y + r * sz, x + k * sz + sz, y + r * sz + sz, c);
}
/* align: 0 = centre, 1 = left, 2 = right */
static void text(const char *s, float x, float y, int sz, u16 c, int align, int shadow) {
    int tw = textW(s, sz);
    int X = iround(align == 0 ? x - tw / 2.0f : align == 2 ? x - tw : x), Y = iround(y - 2.5f * sz);
    for (int pass = shadow ? 0 : 1; pass < 2; pass++) {
        int px = X + (pass ? 0 : 1), py = Y + (pass ? 0 : 1);
        u16 col = pass ? c : C_BLACK;
        for (const char *p = s; *p; p++) {
            if (*p == '\1' || *p == '\2') {
                int ss = sz > 1 ? sz - 1 : 1;
                drawGlyph(*p == '\1' ? '2' : '3', px, py - sz, ss, col);
                px += 3 * ss + 1;
            } else { drawGlyph(*p, px, py, sz, col); px += 4 * sz; }
        }
    }
}
static void pill(float x, float y, const char *s, int sz, u16 bg) {
    float w = textW(s, sz) + 4 * sz, h = 7 * sz;
    rectf(x - w / 2, y - h / 2, x + w / 2, y + h / 2, bg);
    rectf(x - w / 2 + 1, y - h / 2 - 1, x + w / 2 - 1, y - h / 2, bg);
    rectf(x - w / 2 + 1, y + h / 2, x + w / 2 - 1, y + h / 2 + 1, bg);
    text(s, x, y, sz, C_WHITE, 0, 0);
}
static int txtScale(float s) { return s > 2.2f ? 2 : 1; }

/* ---- world ---- */
static void drawWorld(void) {
    float d = S.dist;
    rect(0, 0, W, (int)HOR + 2, C_SKY);
    rect(0, (int)HOR + 2, W, 39, C_SEAFAR);
    rect(0, 39, W, 145, C_SEA);
    rect(0, 145, W, H, C_SEANEAR);
    int base = (int)(d / 16);
    float off = fmodpos(d, 16);
    for (int k = 0; k < 30; k++) {
        float z = k * 16 - off;
        if (z < -12) continue;
        float s0 = sc(z);
        for (int j = 0; j < 3; j++) {
            u32 h = hash32((u32)((base + k) * 7 + j));
            float x = 66 + (float)(h % 160), side = (h >> 9) & 1 ? 1.0f : -1.0f;
            float wx = sx(side * x, s0), wy = sy(-14, s0);
            rectf(wx - 3 * s0, wy, wx + 3 * s0, wy + 1, C_WAVE);
        }
    }
    const float ZN = -14, ZF = FAR + 60;
    for (int k = (int)((ZF + d) / 48) + 1; k * 48 - d > ZN - 20; k--) {
        float z = k * 48 - d;
        for (int side = -1; side <= 1; side += 2) {
            float xi = side * 55.0f, xo = side * 66.0f;
            quad(xi, 2, z + 14, xo, 2, z + 14, xo, 2, z, xi, 2, z, C_PILTOP);
            quad(xi, 2, z, xo, 2, z, xo, -14, z, xi, -14, z, C_PILFRONT);
        }
    }
    quad(-ROAD, 0, ZF, ROAD, 0, ZF, ROAD, 0, ZN, -ROAD, 0, ZN, C_ROAD);
    quad(-ROAD, 0, ZF, -ROAD + 5, 0, ZF, -ROAD + 5, 0, ZN, -ROAD, 0, ZN, C_ROADEDGE);
    quad(ROAD - 5, 0, ZF, ROAD, 0, ZF, ROAD, 0, ZN, ROAD - 5, 0, ZN, C_ROADEDGE);
    for (float z = ZF - fmodpos(ZF + d, 30); z > ZN - 30; z -= 30)
        quad(-0.8f, 0, z + 12, 0.8f, 0, z + 12, 0.8f, 0, z, -0.8f, 0, z, C_DASH);
    for (int side = -1; side <= 1; side += 2) {
        float xi = side * ROAD, xo = side * 55.0f;
        quad(xi, 0, ZF, xi, 4, ZF, xi, 4, ZN, xi, 0, ZN, C_WALLIN);
        quad(xi, 4, ZF, xo, 4, ZF, xo, 4, ZN, xi, 4, ZN, C_WALLTOP);
    }
    for (int k = (int)((ZF + d) / 24) + 1; k * 24 - d > ZN - 12; k--) {
        float z = k * 24 - d;
        for (int side = -1; side <= 1; side += 2) {
            float xi = side * (ROAD + 0.5f), xo = side * 55.0f;
            quad(xi, 4, z + 10, xi, 8, z + 10, xi, 8, z, xi, 4, z, C_WALLIN);
            quad(xi, 8, z + 10, xo, 8, z + 10, xo, 8, z, xi, 8, z, C_WALLTOP);
            quad(xi, 8, z, xo, 8, z, xo, 4, z, xi, 4, z, C_WALLFRONT);
        }
    }
}

/* ---- soldiers: a handful of rectangles each ---- */
static void soldier(float x, float z, int mine, int frame) {
    float s0 = sc(z);
    int h = iround(5 * s0);
    if (h < 2) return;
    int X = iround(sx(x, s0)), Y = iround(sy(0, s0));
    u16 col = mine ? C_ME : C_FOE, dark = mine ? C_MEDARK : C_FOEDARK;
    if (h < 5) { rect(X - 1, Y - h, X + 1, Y, col); return; }
    int bw = iround(h * 0.42f); if (bw < 2) bw = 2;
    int hw = iround(h * 0.32f); if (hw < 2) hw = 2;
    int lw = iround(h * 0.14f); if (lw < 1) lw = 1;
    int lh = iround(h * 0.3f); if (lh < 1) lh = 1;
    int up = frame & 1, bh = iround(h * 0.4f);
    int bl = X - bw / 2, br = bl + bw;
    rect(bl, Y - lh + up, bl + lw, Y, dark);                 /* legs */
    rect(br - lw, Y - lh + !up, br, Y, dark);
    rect(bl, Y - lh - bh, br, Y - lh, col);                  /* body */
    int hy = Y - lh - bh;
    rect(X - hw / 2, hy - hw, X - hw / 2 + hw, hy, col);     /* head */
    if (h >= 8) rect(X - hw / 2 + 1, hy - hw + 1, X - hw / 2 + 2, hy - hw + 2, mine ? C_MELIGHT : C_WHITE);
    if (mine) rect(br - 1, hy - 1, br + 1, Y - lh - 1, C_GUN);
}
static void group(float x, float z, i64 n, int mine) {
    int k = (int)imin(n, CG_DRAWN);
    for (int j = 0; j < 60; j++) {
        int i = FORM_ORDER[j];
        if (i >= k) continue;
        soldier(x + FORM[i][0] * FS, z + FORM[i][1] * FS, mine, (S.t >> 2) + i);
    }
}

static void drawGate(Obj *o) {
    char buf[24];
    float s = sc(o->z);
    int ts = s > 1.1f ? 2 : 1;
    for (int side = 0; side < 2; side++) {
        Op *op = &o->ops[side];
        int good = opGood(op);
        float X0 = sx(side ? 2 : -ROAD + 2, s), X1 = sx(side ? ROAD - 2 : -2, s);
        float Y0 = sy(16, s), Y1 = sy(4, s), G = sy(0, s);
        rectf(X0 - s, Y0, X0 + s, G, C_POST);
        rectf(X1 - s, Y0, X1 + s, G, C_POST);
        if (o->used) continue;
        rectf(X0, Y0, X1, Y1, good ? C_GOOD : C_BAD);
        rectf(X0, Y0, X1, Y0 + fmaxf_(1, s), good ? C_GOODEDGE : C_BADEDGE);
        opLabel(op, buf);
        text(buf, (X0 + X1) / 2, (Y0 + Y1) / 2, ts, C_WHITE, 0, 1);
    }
    if (!o->used && S.mode == M_PLAY && straddling())
        rectf(sx(-1.5f, s), sy(17, s), sx(1.5f, s), sy(0, s), C_AQUA);
}
static void drawCrate(Obj *o) {
    char buf[24];
    float x0 = o->x - 8, x1 = o->x + 8, z0 = o->z - 6, z1 = o->z + 6, h = 12;
    if (x0 > 0) quad(x0, 0, z0, x0, 0, z1, x0, h, z1, x0, h, z0, C_CRATEDARK);
    if (x1 < 0) quad(x1, 0, z0, x1, 0, z1, x1, h, z1, x1, h, z0, C_CRATEDARK);
    quad(x0, h, z1, x1, h, z1, x1, h, z0, x0, h, z0, C_CRATETOP);
    quad(x0, 0, z0, x1, 0, z0, x1, h, z0, x0, h, z0, C_CRATE);
    float s = sc(z0);
    fmt(buf, o->hp);
    pill(sx(o->x, s), sy(h / 2, s), buf, txtScale(s), C_CRATEDARK);
    float st = sc(o->z), SX = sx(o->x, st), SY = sy(h + 9, st);
    rectf(SX - 0.5f * st, SY, SX + 0.5f * st + 1, sy(h, st), C_POST);
    rectf(SX - 6.5f * st, SY - 5 * st, SX + 6.5f * st, SY + 5 * st, C_WHITE);
    buf[0] = '+'; fmt(buf + 1, o->reward);
    text(buf, SX, SY, txtScale(st), C_ME, 0, 0);
}

/* boss: polygons in "boss space" (dx right, dy up, in world units) */
static float bX, bY, bU;
static void bpoly(const float *p, int n, u16 c) {
    float q[16];
    for (int i = 0; i < n; i++) { q[i * 2] = bX + p[i * 2] * bU; q[i * 2 + 1] = bY - p[i * 2 + 1] * bU; }
    poly(q, n, c);
}
static void bbox(float x0, float y0, float x1, float y1, u16 c) {
    float p[8] = {x0, y0, x1, y0, x1, y1, x0, y1};
    bpoly(p, 4, c);
}
static float rCa, rSa, rShx, rShy;
static void rpoly(const float *p, int n, u16 c) {    /* rotated around the club shoulder */
    float q[16];
    for (int i = 0; i < n; i++) {
        float ax = p[i * 2], ay = p[i * 2 + 1];
        q[i * 2] = rShx + (ax * rCa - ay * rSa) * bU;
        q[i * 2 + 1] = rShy + (ax * rSa + ay * rCa) * bU;
    }
    poly(q, n, c);
}
static void drawBoss(Obj *o) {
    char buf[24];
    bU = sc(o->z); bX = sx(o->x, bU); bY = sy(0, bU);
    int step = o->planted ? 0 : (o->swing >> 3) & 1;
    bbox(-7, step * 2, -2, 10, C_SHADOW); bbox(2, (1 - step) * 2, 7, 10, C_SHADOW);
    bbox(-8, step * 2, -1.5f, 2 + step * 2, C_BLACK); bbox(1.5f, (1 - step) * 2, 8, 2 + (1 - step) * 2, C_BLACK);
    bbox(-16, 10, -11, 21, C_FOEDARK); bbox(-16, 8, -11, 11, C_FOE);
    { float p[] = {-8, 10, 8, 10, 12, 22, -12, 22}; bpoly(p, 4, C_FOE); }
    bbox(-8, 10, 8, 12.5f, C_BLACK); bbox(-1.5f, 10.3f, 1.5f, 12.2f, C_GOLD);
    bbox(-1, 13, 1, 20, C_FOEDARK);
    for (int sg = -1; sg <= 1; sg += 2) {
        bbox(sg * 8.0f, 18, sg * 15.0f, 25, C_GREYDARK); bbox(sg * 8.8f, 19, sg * 14.2f, 24.2f, C_GREY);
        float p[] = {sg * 10.0f, 25, sg * 11.0f, 28.5f, sg * 12.0f, 25}; bpoly(p, 3, C_BONE);
    }
    bbox(-4.5f, 22, 4.5f, 29, C_FOE);
    bbox(-5, 27, 5, 31.5f, C_GREY); bbox(-5, 27, 5, 28, C_GREYDARK);
    { float p[] = {-4.5f, 29.5f, -9, 34, -7.5f, 30, -5, 28}; bpoly(p, 4, C_BONE); }
    { float p[] = {4.5f, 29.5f, 9, 34, 7.5f, 30, 5, 28}; bpoly(p, 4, C_BONE); }
    { float p[] = {-3.5f, 25.8f, -0.8f, 25, -0.8f, 24.2f, -3.5f, 24.8f}; bpoly(p, 4, C_EYE); }
    { float p[] = {3.5f, 25.8f, 0.8f, 25, 0.8f, 24.2f, 3.5f, 24.8f}; bpoly(p, 4, C_EYE); }
    bbox(-2.2f, 22.8f, 2.2f, 23.6f, C_BLACK);
    /* club arm */
    float smash = o->planted ? (o->swing % 8) / 8.0f : 0;
    float ang = o->planted ? (smash < 0.5f ? -2.6f + smash * 1.2f : -2.0f + (smash - 0.5f) * 4.4f) : -0.5f;
    rCa = fcos(ang + PI_F / 2); rSa = fsin(ang + PI_F / 2);
    rShx = bX + 11.5f * bU; rShy = bY - 20 * bU;
    { float p[] = {-2, 0, 2, 0, 2, 9, -2, 9}; rpoly(p, 4, C_FOEDARK); }
    { float p[] = {-2.5f, 8, 2.5f, 8, 2.5f, 12, -2.5f, 12}; rpoly(p, 4, C_FOE); }
    { float p[] = {-1, 11, 1, 11, 1, 20, -1, 20}; rpoly(p, 4, C_CRATEDARK); }
    { float p[] = {-3, 19, 3, 19, 3.5f, 23, 2, 27, -2, 27, -3.5f, 23}; rpoly(p, 6, C_GREYDARK); }
    { float p[] = {-3.5f, 21, -6, 22, -3.5f, 23}; rpoly(p, 3, C_BONE); }
    { float p[] = {3.5f, 21, 6, 22, 3.5f, 23}; rpoly(p, 3, C_BONE); }
    { float p[] = {-1, 27, 0, 30, 1, 27}; rpoly(p, 3, C_BONE); }
    /* HP bar */
    float hy = sy(38, bU), bw = 30 * bU, hp = (float)imax(0, o->hp), mx = (float)imax(1, o->max);
    rectf(bX - bw / 2, hy - 1.6f * bU, bX + bw / 2, hy + 1.6f * bU, C_HPBACK);
    rectf(bX - bw / 2, hy - 1.6f * bU, bX - bw / 2 + bw * hp / mx, hy + 1.6f * bU, C_BAD);
    fmt(buf, o->hp);
    text(buf, bX, hy - 5 * bU, txtScale(bU), C_WHITE, 0, 1);
}

static void drawHUD(void) {
    char buf[24];
    float pw = 130, X = W / 2 - pw / 2, Y = 5;
    rectf(X, Y, X + pw, Y + 15, C_PANEL);
    float prog = S.bossAlive ? 1 : fminf_(1, (S.dist - S.lastBoss) / (S.nextBoss - S.lastBoss));
    rectf(X + 2, Y + 2, X + 2 + fmaxf_(4, (pw - 4) * prog), Y + 13, S.bossAlive ? C_BAD : C_GOOD);
    if (S.bossAlive) s_cpy(buf, "BOSS FIGHT"); else s_i64(s_cpy(buf, "LVL "), level() + 1);
    text(buf, W / 2, Y + 8, 1, C_WHITE, 0, 1);
    s_cpy(s_i64(buf, (i64)(S.dist / 10)), "M");
    text(buf, 6, 12, 2, C_WHITE, 1, 1);
    s_i64(s_cpy(buf, "BEST "), best[diff]);
    text(buf, W - 6, 10, 1, C_WHITE, 2, 1);
    text(D->name, W - 6, 20, 1, D->col, 2, 1);
}
static void panel(void) {
    int bx0 = 52, by0 = 30, bx1 = W - 52, by1 = H - 22;
    rect(bx0 - 2, by0 - 2, bx1 + 2, by1 + 2, C_PANELEDGE);
    rect(bx0, by0, bx1, by1, C_PANEL);
}
static void drawOverlay(int paused) {
    char buf[40];
    if (S.mode == M_PLAY && !paused) return;
    panel();
    if (S.mode == M_TITLE) {
        text("SQUAD RUSH", W / 2, 54, 4, C_GOLD, 0, 1);
        text("GROW YOUR SQUAD - BLAST THE HORDE", W / 2, 78, 1, C_WHITE, 0, 1);
        for (int i = 0; i < 12; i++) rect(W / 2 - 50 + i * 9, 94, W / 2 - 46 + i * 9, 104, i % 2 ? C_ME : C_MEDARK);
        s_cpy(s_cpy(s_cpy(buf, "< "), D->name), " >");
        text(buf, W / 2, 124, 2, D->col, 0, 1);
        text("LEFT/RIGHT: DIFFICULTY", W / 2, 146, 1, C_WHITE, 0, 1);
        if ((S.t >> 4) & 1) text("EXE: START", W / 2, 158, 1, C_GOLD, 0, 1);
        s_cpy(s_i64(s_cpy(buf, "BEST "), best[diff]), "M");
        text(buf, W / 2, 176, 1, C_GREY, 0, 1);
        text("MENU: QUIT", W / 2, 186, 1, C_GREY, 0, 1);
    } else if (S.mode == M_OVER) {
        text("SQUAD WIPED OUT", W / 2, 60, 3, C_BAD, 0, 1);
        char *p = s_cpy(s_i64(buf, (i64)(S.dist / 10)), "M   ");
        s_cpy(fmt(p, S.kills), " KILLS");
        text(buf, W / 2, 92, 2, C_WHITE, 0, 1);
        if (S.newBest) text("NEW BEST!", W / 2, 116, 2, C_GOLD, 0, 1);
        s_cpy(s_i64(s_cpy(buf, "BEST "), best[diff]), "M");
        text(buf, W / 2, 138, 1, C_GREY, 0, 1);
        if ((S.t >> 4) & 1) text("EXE: RETRY", W / 2, 160, 1, C_WHITE, 0, 1);
        text("EXIT: TITLE", W / 2, 172, 1, C_GREY, 0, 1);
    } else {
        text("PAUSED", W / 2, 70, 3, C_WHITE, 0, 1);
        text("EXE: RESUME", W / 2, 104, 1, C_WHITE, 0, 1);
        text("EXIT: QUIT TO TITLE", W / 2, 118, 1, C_WHITE, 0, 1);
        text("MENU: CALCULATOR MENU", W / 2, 132, 1, C_GREY, 0, 1);
    }
}

static void draw(int paused) {
    char buf[24];
    Obj *order[MAX_OBJS + 2];
    static Obj playerItem, bulletItem;
    int n = 0;

    if (S.shake && !paused) { shakeX = (int)(rnd() % 3) - 1; shakeY = (int)(rnd() % 3) - 1; }
    else shakeX = shakeY = 0;
    drawWorld();

    for (int i = 0; i < S.nobjs; i++) order[n++] = &S.objs[i];
    if (S.mode != M_TITLE && S.n > 0) { playerItem.type = T_PLAYER; playerItem.z = PZ; order[n++] = &playerItem; }
    if (S.nbullets) { bulletItem.type = T_BULLETS; bulletItem.z = PZ + 14; order[n++] = &bulletItem; }
    for (int i = 1; i < n; i++) {                         /* far to near */
        Obj *t = order[i]; int j = i - 1;
        while (j >= 0 && order[j]->z < t->z) { order[j + 1] = order[j]; j--; }
        order[j + 1] = t;
    }
    for (int i = 0; i < n; i++) {
        Obj *o = order[i];
        switch (o->type) {
        case T_ENEMY: {
            group(o->x, o->z, o->n, 0);
            float s = sc(o->z);
            fmt(buf, o->n);
            pill(sx(o->x, s), sy(12, s) - radius(o->n) * s * 0.4f, buf, txtScale(s), C_PILLFOE);
            break; }
        case T_GATE: drawGate(o); break;
        case T_CRATE: drawCrate(o); break;
        case T_BOSS: drawBoss(o); break;
        case T_PLAYER: group(S.px, PZ, S.n, 1); break;
        case T_BULLETS:
            for (int k = 0; k < S.nbullets; k++) {
                Bullet *b = &S.bullets[k];
                float s0 = sc(b->z), s1 = sc(b->z + 7), X = sx(b->x, s0);
                rectf(X, sy(3.5f, s1), X + 1, sy(3.5f, s0) + 1, C_BULLET);
            }
            break;
        }
    }
    for (int i = 0; i < S.nfx; i++) {
        Fx *p = &S.fx[i]; float s0 = sc(p->z), X = sx(p->x, s0), Y = sy(p->y, s0);
        rectf(X, Y, X + 2, Y + 2, p->col);
    }
    shakeX = shakeY = 0;
    if (S.mode != M_TITLE && S.n > 0) {
        float s0 = sc(PZ);
        fmt(buf, S.n);
        pill(sx(S.px, s0), sy(9, s0) - radius(S.n) * s0 * 0.45f, buf, 2, C_PILLME);
    }
    for (int i = 0; i < S.ntexts; i++) {
        Text *t = &S.texts[i]; float s0 = sc(t->z);
        text(t->txt, sx(t->x, s0), sy(t->y, s0), 2, t->col, 0, 1);
    }
    if (S.mode != M_TITLE) drawHUD();
    drawOverlay(paused);
}

/* ================================================================ input */
/* Real-time key state straight from the keyboard matrix registers. */
static const volatile u16 *KEYREG = (const volatile u16 *)0xA44B0000;
static u16 keyNow[8], keyPrev[8];
static void keyUpdate(void) {
    memcpy(keyPrev, keyNow, sizeof(keyNow));
    for (int i = 0; i < 8; i++) keyNow[i] = KEYREG[i];
}
static int keyBit(const u16 *st, int code) {
    int row = code % 10, col = code / 10 - 1;
    return (st[row >> 1] >> (col + ((row & 1) << 3))) & 1;
}
static int keyDown(int code) { return keyBit(keyNow, code); }
static int keyPressed(int code) { return keyBit(keyNow, code) && !keyBit(keyPrev, code); }
static int anyKeyDown(void) { for (int i = 0; i < 8; i++) if (keyNow[i]) return 1; return 0; }

/* Blocking OS GetKey loop for the pause screen: lets MENU work natively. */
/* The OS may restore its status bar after visiting the main menu. */
static void screenSetup(void) {
    Bdisp_EnableColor(1);
    EnableStatusArea(3);           /* hide the status bar: full 384x216 */
    vram = (u16 *)GetVRAMAddress();
}
static int pauseScreen(void) {     /* returns 1 = resume, 0 = quit to title */
    int key;
    draw(1);
    Bdisp_PutDisp_DD();
    for (;;) {
        GetKey(&key);
        screenSetup();
        if (key == KEY_CTRL_EXE) return 1;
        if (key == KEY_CTRL_EXIT) return 0;
        /* after returning from the main menu the screen must be redrawn */
        draw(1); Bdisp_PutDisp_DD();
    }
}

/* ================================================================ main */
int main(void) {
    screenSetup();

    for (int i = 0; i < 60; i++) FORM_ORDER[i] = (unsigned char)i;
    for (int i = 1; i < 60; i++) {                     /* far-to-near order */
        unsigned char t = FORM_ORDER[i]; int j = i - 1;
        while (j >= 0 && FORM[FORM_ORDER[j]][1] < FORM[t][1]) { FORM_ORDER[j + 1] = FORM_ORDER[j]; j--; }
        FORM_ORDER[j + 1] = t;
    }
    loadBest();
    newGame(0);

    int last = RTC_GetTicks(), acc = 0;
    keyUpdate(); keyUpdate();
    for (;;) {
        int now = RTC_GetTicks(), dt = now - last;
        last = now;
        if (dt < 0 || dt > 32) dt = 4;     /* midnight wrap / returned from menu */
        acc += dt * 30;                     /* RTC is 128 Hz; logic runs at 30 Hz */
        int steps = 0;
        while (acc >= 128 && steps < 3) {
            keyUpdate();
            if (S.mode == M_TITLE) {
                if (keyPressed(KEY_PRGM_LEFT) || keyPressed(KEY_PRGM_UP)) diff = (diff + 3) % 4;
                if (keyPressed(KEY_PRGM_RIGHT) || keyPressed(KEY_PRGM_DOWN)) diff = (diff + 1) % 4;
                if (keyPressed(KEY_PRGM_EXE)) newGame(1);
                if (keyPressed(KEY_PRGM_MENU)) {
                    /* hand control to the OS: MENU goes to the main menu */
                    int key;
                    do GetKey(&key); while (key != KEY_CTRL_EXE && key != KEY_CTRL_EXIT);
                    screenSetup();
                    last = RTC_GetTicks(); acc = 0;
                    keyUpdate(); keyUpdate();
                }
            } else if (S.mode == M_OVER) {
                if (S.overT > 20 && keyPressed(KEY_PRGM_EXE)) newGame(1);
                if (S.overT > 20 && keyPressed(KEY_PRGM_EXIT)) newGame(0);
            } else {
                if (keyPressed(KEY_PRGM_EXIT) || keyPressed(KEY_PRGM_OPTN) ||
                    keyPressed(KEY_PRGM_F1) || keyPressed(KEY_PRGM_MENU)) {
                    if (!pauseScreen()) newGame(0);
                    while (anyKeyDown()) keyUpdate();
                    keyUpdate();
                    last = RTC_GetTicks(); acc = 0;
                    break;
                }
            }
            int dir = 0;
            if (S.mode == M_PLAY) dir = keyDown(KEY_PRGM_RIGHT) - keyDown(KEY_PRGM_LEFT);
            update(dir);
            acc -= 128; steps++;
        }
        if (acc > 128 * 3) acc = 0;         /* too slow: drop time rather than spiral */
        if (steps) { draw(0); Bdisp_PutDisp_DD(); }
    }
    return 0;
}
