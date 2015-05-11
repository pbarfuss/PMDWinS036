//=============================================================================
//  Professional Music Driver [P.M.D.] version 4.8
//          Programmed By M.Kajihara
//          Windows Converted by C60
//=============================================================================

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#if defined(WIN32) || defined(WIN64)
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#else
#include <sys/mman.h>
#endif
#include "jisx0208.h"
#include "pmdwin.h"
#include "efftbl.h"

/******************************************************************************
;   Part Table
******************************************************************************/
static const uint8_t part_table[][3] = {       // for PMDB2
    //      Part<94>ﾔ<8d><86>,Partb,<89>ｹ<8c>ｹ<94>ﾔ<8d><86>
    {    0, 1, 0    },  //A
    {    1, 2, 0    },  //B
    {    2, 3, 0    },  //C
    {    3, 1, 1    },  //D
    {    4, 2, 1    },  //E
    {    5, 3, 1    },  //F
    {    6, 1, 2    },  //G
    {    7, 2, 2    },  //H
    {    8, 3, 2    },  //I
    {    9, 1, 3    },  //J
    {   10, 3, 4    },  //K
    {   11, 3, 0    },  //c2
    {   12, 3, 0    },  //c3
    {   13, 3, 0    },  //c4
    {  255, 0, 255  },  //Rhythm
    {   22, 3, 1    },  //Effect
};

static char *notes[12] = {
    "C ", "Db", "D ", "Eb", "E ", "F ", "F#", "G ", "G#", "A ", "Bb", "B "
};

static char *drums[6] = {
    "Bass   ", "Snare  ", "RideCym", "Hi-Hat ", "Tom-Tom", "Rim    "
};

/******************************************************************************
;   <89>ｹ<8a>KDATA(FM)
******************************************************************************/
static const uint16_t fnum_data[] = {
    0x026a, //  C
    0x028f, //  D-
    0x02b6, //  D
    0x02df, //  E-
    0x030b, //  E
    0x0339, //  F
    0x036a, //  G-
    0x039e, //  G
    0x03d5, //  A-
    0x0410, //  A
    0x044e, //  B-
    0x048f  //  B
};

/******************************************************************************
;   <89>ｹ<8a>KDATA(PSG)
******************************************************************************/
static const uint16_t psg_tune_data[] = {
    0x0ee8, //  C
    0x0e12, //  D-
    0x0d48, //  D
    0x0c89, //  E-
    0x0bd5, //  E
    0x0b2b, //  F
    0x0a8a, //  G-
    0x09f3, //  G
    0x0964, //  A-
    0x08dd, //  A
    0x085e, //  B-
    0x07e6, //  B
};


/******************************************************************************
;   <82>e<82>l<89>ｹ<90>F<82>ﾌ<83>L<83><83><83><8a><83>A<82>ﾌ<83>e<81>[<83>u<83><8b>
******************************************************************************/
static const uint8_t carrier_table[] = {
    0x80, 0x80, 0x80, 0x80, 0xa0, 0xe0, 0xe0, 0xf0,
    0xee, 0xee, 0xee, 0xee, 0xcc, 0x88, 0x88, 0x00
};

/******************************************************************************
;   <83>h<83><89><83><80><83>X<97>p<83><8a><83>Y<83><80><83>f<81>[<83>^
******************************************************************************/
static const uint8_t rhydat[][3] = {
    //  PT,PAN/VOLUME,KEYON
    {   0x18, 0xdf, 0x01    },  //  <83>o<83>X
    {   0x19, 0xdf, 0x02    },  //  <83>X<83>l<83>A
    {   0x1c, 0x5f, 0x10    },  //  <83>^<83><80>[LOW]
    {   0x1c, 0xdf, 0x10    },  //  <83>^<83><80>[MID]
    {   0x1c, 0x9f, 0x10    },  //  <83>^<83><80>[HIGH]
    {   0x1d, 0xd3, 0x20    },  //  <83><8a><83><80>
    {   0x19, 0xdf, 0x02    },  //  <83>N<83><89><83>b<83>v
    {   0x1b, 0x9c, 0x88    },  //  C<83>n<83>C<83>n<83>b<83>g
    {   0x1a, 0x9d, 0x04    },  //  O<83>n<83>C<83>n<83>b<83>g
    {   0x1a, 0xdf, 0x04    },  //  <83>V<83><93><83>o<83><8b>
    {   0x1a, 0x5e, 0x04    }   //  RIDE<83>V<83><93><83>o<83><8b>
};


static void lfg_srand(OPEN_WORK *c, unsigned int seed){
    uint32_t i, tmp[4]={0};

    for(i=0; i<64; i+=4){
        tmp[0]=seed; tmp[3]=i;
        MD5((uint8_t*)tmp, (const uint8_t*)tmp, 16);
        c->lfg_state[i  ]= tmp[0];
        c->lfg_state[i+1]= tmp[1];
        c->lfg_state[i+2]= tmp[2];
        c->lfg_state[i+3]= tmp[3];
    }
    c->lfg_index=0;
}

static unsigned int lfg_rand(OPEN_WORK *c){
    c->lfg_state[c->lfg_index & 63] = c->lfg_state[(c->lfg_index-24) & 63] + c->lfg_state[(c->lfg_index-55) & 63];
    return c->lfg_state[c->lfg_index++ & 63];
}

void effend(EFFWORK *effwork, OPNA *opna)
{
    PSGSetReg(&(opna->psg), 0x0a, 0x00);
    PSGSetReg(&(opna->psg), 0x07,  ((PSGGetReg(&(opna->psg), 0x07)) & 0xdb) | 0x24);
    effwork->effon = 0;
    effwork->psgefcnum = -1;
}

void efffor(EFFWORK *effwork, OPNA *opna, const int *si)
{
    int     al, ch, cl;

    al = *si++;
    if(al == -1) {
        effend(effwork, opna);
    } else {
        effwork->effcnt = al;        // カウント数
        cl = *si;
        PSGSetReg(&(opna->psg), 4, *si++);       // 周波数セット
        ch = *si;
        PSGSetReg(&(opna->psg), 5, *si++);       // 周波数セット
        effwork->eswthz = (ch << 8) + cl;

        effwork->eswnhz = *si;
        PSGSetReg(&(opna->psg), 6, *si++);       // ノイズ

        PSGSetReg(&(opna->psg), 7, ((*si++ << 2) & 0x24) | (PSGGetReg(&(opna->psg), 0x07) & 0xdb));

        PSGSetReg(&(opna->psg), 10, *si++);      // ボリューム
        PSGSetReg(&(opna->psg), 11, *si++);      // エンベロープ周波数
        PSGSetReg(&(opna->psg), 12, *si++);      //
        PSGSetReg(&(opna->psg), 13, *si++);      // エンベロープPATTERN

        effwork->eswtst = *si++;     // スイープ増分 (TONE)
        effwork->eswnst = *si++;     // スイープ増分 (NOISE)
        effwork->eswnct = effwork->eswnst & 15;       // スイープカウント (NOISE)
        effwork->effadr = (int *)si;
    }
}

//=============================================================================
//  ＰＳＧ　ドラムス＆効果音　ルーチン
//  Ｆｒｏｍ　ＷＴ２９８
//
//  AL に 効果音Ｎｏ．を入れて　ＣＡＬＬする
//  ppsdrvがあるならそっちを鳴らす
//=============================================================================
void eff_main(EFFWORK *effwork, OPNA *opna, uint8_t *partmask, int al)
{
    int priority;
    effwork->psgefcnum = al;
    if (al == 7 || al == 8) return; // PSG hihat sounds godawful, don't play it at all
    priority = ((al > 10) ? 2 : 1);
    if(effwork->effon <= priority) {
        *partmask |= 2;       // Part Mask
        efffor(effwork, opna, efftbl[al]);             // １発目を発音
        effwork->effon = priority;
                                                //  優先順位を設定(発音開始)
    }
}

void effsweep(EFFWORK *effwork, OPNA *opna)
{
    int dl;
    effwork->eswthz += effwork->eswtst;
    PSGSetReg(&(opna->psg), 4, effwork->eswthz & 0xff);
    PSGSetReg(&(opna->psg), 5, effwork->eswthz >> 8);
    if(effwork->eswnst == 0) return;     // ノイズスイープ無し
    if(--effwork->eswnct) return;
    dl = effwork->eswnst;
    effwork->eswnct = dl & 15;
    effwork->eswnhz += dl >> 4;
    PSGSetReg(&(opna->psg), 6, effwork->eswnhz);
}

//=============================================================================
//  こーかおん　えんそう　めいん
//  Ｆｒｏｍ　ＶＲＴＣ
//=============================================================================
void effplay(EFFWORK *effwork, OPNA *opna)
{
    if(--effwork->effcnt) {
        effsweep(effwork, opna);         // 新しくセットされない
    } else {
        efffor(effwork, opna, effwork->effadr);
    }
}


//=============================================================================
//  TimerAの処理[メイン]
//=============================================================================
void TimerA_main(PMDWIN *pmd)
{
    pmd->open_work.TimerAflag = 1;
    pmd->open_work.TimerAtime++;

    if(pmd->effwork.effon) {
        effplay(&pmd->effwork, &pmd->opna);      //      SSG効果音処理
    }

    pmd->open_work.TimerAflag = 0;
}


//=============================================================================
//  KEY OFF
//=============================================================================
void kof1(PMDWIN *pmd, QQ *qq)
{
    if(pmd->open_work.fmsel == 0) {
        pmd->open_work.omote_key[pmd->open_work.partb-1]
                = (~qq->slotmask) & (pmd->open_work.omote_key[pmd->open_work.partb-1]);
        OPNASetReg(&pmd->opna, 0x28, (pmd->open_work.partb-1) | pmd->open_work.omote_key[pmd->open_work.partb-1]);
    } else {
        pmd->open_work.ura_key[pmd->open_work.partb-1]
                = (~qq->slotmask) & (pmd->open_work.ura_key[pmd->open_work.partb-1]);
        OPNASetReg(&pmd->opna, 0x28, ((pmd->open_work.partb-1) | pmd->open_work.ura_key[pmd->open_work.partb-1]) | 4);
    }
}


void keyoff(PMDWIN *pmd, QQ *qq)
{
    if(qq->onkai == 255) return;
    kof1(pmd, qq);
}


void keyoffp(PMDWIN *pmd, QQ *qq)
{
    if(qq->onkai == 255) return;        // ｷｭｳﾌ ﾉ ﾄｷ
    if(qq->envf != -1) {
        qq->envf = 2;
    } else {
        qq->eenv_count = 4;
    }
}


//=============================================================================
//  ＦＭ　ＫＥＹＯＮ
//=============================================================================
void keyon(PMDWIN *pmd, QQ *qq)
{
    int al;

    if(qq->onkai == 255) return;        // ｷｭｳﾌ ﾉ ﾄｷ
    uint32_t op = ((pmd->open_work.partb-1)&7)|(!!pmd->open_work.fmsel << 2);
    switch(op) {
        case 0:
            pmd->pmdstatus[0] = (qq->onkai & 0x0f);
            break;
        case 1:
            pmd->pmdstatus[1] = (qq->onkai & 0x0f);
            break;
        case 2:
            pmd->pmdstatus[2] = (qq->onkai & 0x0f);
            break;
        case 4:
            pmd->pmdstatus[3] = (qq->onkai & 0x0f);
            break;
        case 5:
            pmd->pmdstatus[4] = (qq->onkai & 0x0f);
            break;
        case 6:
            pmd->pmdstatus[5] = (qq->onkai & 0x0f);
            break;
        default:
            break;
    }

    if(pmd->open_work.fmsel == 0) {
        al = pmd->open_work.omote_key[pmd->open_work.partb-1] | qq->slotmask;
        if(qq->sdelay_c) {
            al &= qq->sdelay_m;
        }

        pmd->open_work.omote_key[pmd->open_work.partb-1] = al;
        OPNASetReg(&pmd->opna, 0x28, (pmd->open_work.partb-1) | al);
    } else {
        al = pmd->open_work.ura_key[pmd->open_work.partb-1] | qq->slotmask;
        if(qq->sdelay_c) {
            al &= qq->sdelay_m;
        }

        pmd->open_work.ura_key[pmd->open_work.partb-1] = al;
        OPNASetReg(&pmd->opna, 0x28, ((pmd->open_work.partb-1) | al) | 4);
    }
}


//=============================================================================
//  ＰＳＧ　ＫＥＹＯＮ
//=============================================================================
void keyonp(PMDWIN *pmd, QQ *qq)
{
    int ah, al;

    if(qq->onkai == 255) return;        // ｷｭｳﾌ ﾉ ﾄｷ
    int op = ((pmd->open_work.partb-1)&3);
    switch(op) {
        case 0:
            pmd->pmdstatus[6] = (qq->onkai & 0x0f);
            break;
        case 1:
            pmd->pmdstatus[7] = (qq->onkai & 0x0f);
            break;
        case 2:
            pmd->pmdstatus[8] = (qq->onkai & 0x0f);
            break;
        default:
            break;
    }

    ah=(1 << (pmd->open_work.partb -1)) | (1 << (pmd->open_work.partb +2));
    al = PSGGetReg(&(pmd->opna.psg), 0x07) | ah;
    ah = ~(ah & qq->psgpat);
    al &= ah;
    PSGSetReg(&(pmd->opna.psg), 7, al);

    // PSG ﾉｲｽﾞ ｼｭｳﾊｽｳ ﾉ ｾｯﾄ
    if(pmd->open_work.psnoi != pmd->open_work.psnoi_last && pmd->effwork.effon == 0) {
        PSGSetReg(&(pmd->opna.psg), 6, pmd->open_work.psnoi);
        pmd->open_work.psnoi_last = pmd->open_work.psnoi;
    }
}


//=============================================================================
//  FM音源のdetuneでオクターブが変わる時の修正
//      input   CX:block / AX:fnum+detune
//      output  CX:block / AX:fnum
//=============================================================================
void fm_block_calc(int *cx, int *ax)
{
    while(*ax >= 0x26a) {
        if(*ax < (0x26a*2)) return;

        *cx += 0x800;           // oct.up
        if(*cx != 0x4000) {
            *ax -= 0x26a;       // 4d2h-26ah
        } else {                // ﾓｳ ｺﾚｲｼﾞｮｳ ｱｶﾞﾝﾅｲﾖﾝ
            *cx = 0x3800;
            if(*ax >= 0x800)
                *ax = 0x7ff;    // 4d2h
            return;
        }
    }

    while(*ax < 0x26a) {
        *cx -= 0x800;           // oct.down
        if(*cx >= 0) {
            *ax += 0x26a;       // 4d2h-26ah
        } else {                // ﾓｳ ｺﾚｲｼﾞｮｳ ｻｶﾞﾝﾅｲﾖﾝ
            *cx = 0;
            if(*ax < 8) {
                *ax = 8;
            }
            return;
        }
    }
}


//=============================================================================
//  ch3=効果音モード を使用する場合の音程設定
//          input CX:block AX:fnum
//=============================================================================
static void ch3_special(PMDWIN *pmd, QQ *qq, int ax, int cx)
{
    int     ax_, bh, ch, si;
    int     shiftmask = 0x80;

    si = cx;

    if((qq->volmask & 0x0f) == 0) {
        bh = 0xf0;          // all
    } else {
        bh = qq->volmask;   // bh=lfo1 mask 4321xxxx
    }

    if((qq->_volmask & 0x0f) == 0) {
        ch = 0xf0;          // all
    } else {
        ch = qq->_volmask;  // ch=lfo2 mask 4321xxxx
    }

    //  slot    4
    if(qq->slotmask & 0x80) {
        ax_ = ax;
        ax += pmd->open_work.slot_detune4;
        if((bh & shiftmask) && (qq->lfoswi & 0x01)) ax += qq->lfodat;
        if((ch & shiftmask) && (qq->lfoswi & 0x10)) ax += qq->_lfodat;
        shiftmask >>= 1;

        cx = si;
        fm_block_calc(&cx, &ax);
        ax |= cx;

        OPNASetReg(&pmd->opna, 0xa6, ax >> 8);
        OPNASetReg(&pmd->opna, 0xa2, ax & 0xff);

        ax = ax_;
    }

    //  slot    3
    if(qq->slotmask & 0x40) {
        ax_ = ax;
        ax += pmd->open_work.slot_detune3;
        if((bh & shiftmask) && (qq->lfoswi & 0x01)) ax += qq->lfodat;
        if((ch & shiftmask) && (qq->lfoswi & 0x10)) ax += qq->_lfodat;
        shiftmask >>= 1;

        cx = si;
        fm_block_calc(&cx, &ax);
        ax |= cx;

        OPNASetReg(&pmd->opna, 0xac, ax >> 8);
        OPNASetReg(&pmd->opna, 0xa8, ax & 0xff);

        ax = ax_;
    }

    //  slot    2
    if(qq->slotmask & 0x20) {
        ax_ = ax;
        ax += pmd->open_work.slot_detune2;
        if((bh & shiftmask) && (qq->lfoswi & 0x01)) ax += qq->lfodat;
        if((ch & shiftmask) && (qq->lfoswi & 0x10)) ax += qq->_lfodat;
        shiftmask >>= 1;

        cx = si;
        fm_block_calc(&cx, &ax);
        ax |= cx;

        OPNASetReg(&pmd->opna, 0xae, ax >> 8);
        OPNASetReg(&pmd->opna, 0xaa, ax & 0xff);

        ax = ax_;
    }

    //  slot    1
    if(qq->slotmask & 0x10) {
        ax_ = ax;
        ax += pmd->open_work.slot_detune1;
        if((bh & shiftmask) && (qq->lfoswi & 0x01)) ax += qq->lfodat;
        if((ch & shiftmask) && (qq->lfoswi & 0x10)) ax += qq->_lfodat;
        cx = si;
        fm_block_calc(&cx, &ax);
        ax |= cx;

        OPNASetReg(&pmd->opna, 0xad, ax >> 8);
        OPNASetReg(&pmd->opna, 0xa9, ax & 0xff);

        ax = ax_;
    }
}


static void otodasi(PMDWIN *pmd, QQ *qq)
{
    int     ax, cx;

    if(qq->fnum == 0) return;
    if(qq->slotmask == 0) return;

    cx = (qq->fnum & 0x3800);       // cx=BLOCK
    ax = (qq->fnum) & 0x7ff;        // ax=FNUM

    // Portament/LFO/Detune SET
    ax += qq->porta_num + qq->detune;

    if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0 && pmd->open_work.ch3mode != 0x3f) {
        ch3_special(pmd, qq, ax, cx);
    } else {
        if(qq->lfoswi & 1) {
            ax += qq->lfodat;
        }

        if(qq->lfoswi & 0x10) {
            ax += qq->_lfodat;
        }

        fm_block_calc(&cx, &ax);

        // SET BLOCK/FNUM TO OPN
        ax |= cx;
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + pmd->open_work.partb+0xa4-1, ax >> 8);
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + pmd->open_work.partb+0xa4-5, ax & 0xff);
    }
}


static void cm_clear(PMDWIN *pmd, int *ah, int *al)
{
    *al ^= 0xff;
    if((pmd->open_work.slot3_flag &= *al) == 0) {
        if(pmd->open_work.slotdetune_flag != 1) {
            *ah = 0x3f;
        } else {
            *ah = 0x7f;
        }
    } else {
        *ah = 0x7f;
    }
}


//=============================================================================
//  FM3のmodeを設定する
//=============================================================================
static void ch3mode_set(PMDWIN *pmd, QQ *qq)
{
    int     ah, al;

    if(qq == &pmd->FMPart[3-1]) {
        al = 1;
    } else {
        al = 8;
    }

    if((qq->slotmask & 0xf0) == 0) {    // s0
        cm_clear(pmd, &ah, &al);
    } else if(qq->slotmask != 0xf0) {
        pmd->open_work.slot3_flag |= al;
        ah = 0x7f;
    } else if((qq->volmask & 0x0f) == 0) {
        cm_clear(pmd, &ah, &al);
    } else if((qq->lfoswi & 1) != 0) {
        pmd->open_work.slot3_flag |= al;
        ah = 0x7f;
    } else if((qq->_volmask & 0x0f) == 0) {
        cm_clear(pmd, &ah, &al);
    } else if(qq->lfoswi & 0x10) {
        pmd->open_work.slot3_flag |= al;
        ah = 0x7f;
    } else {
        cm_clear(pmd, &ah, &al);
    }

    if(ah == pmd->open_work.ch3mode) return;     // 以前と変更無しなら何もしない
    pmd->open_work.ch3mode = ah;
    OPNASetReg(&pmd->opna, 0x27, ah & 0xcf);         // Resetはしない

    //  効果音モードに移った場合はそれ以前のFM3パートで音程書き換え
    if(ah == 0x3f || qq == &pmd->FMPart[2]) return;
    if(pmd->FMPart[2].partmask == 0) otodasi(pmd, &pmd->FMPart[2]);
}


//=============================================================================
//  パートを判別してch3ならmode設定
//=============================================================================
static unsigned int ch3_setting(PMDWIN *pmd, QQ *qq)
{
    if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
        ch3mode_set(pmd, qq);
        return 1;
    } else {
        return 0;
    }
}


//=============================================================================
//  'p' COMMAND [FM PANNING SET]
//=============================================================================


//=============================================================================
//  0b4h〜に設定するデータを取得 out.dl
//=============================================================================
static uint8_t calc_panout(QQ *qq)
{
    int dl = qq->fmpan;
    if(qq->hldelay_c) dl &= 0xc0;   // HLFO Delayが残ってる場合はパンのみ設定
    return dl;
}


static void panset_main(PMDWIN *pmd, QQ *qq, int al)
{
    qq->fmpan = ((qq->fmpan & 0x3f) | ((al << 6) & 0xc0));
    if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
        //  FM3の場合は 4つのパート総て設定
        pmd->FMPart[2].fmpan = qq->fmpan;
    }
    if(qq->partmask == 0) {     // パートマスクされているか？
        // dl = al;
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + pmd->open_work.partb+0xb4-1,
                                                    calc_panout(qq));
    }
}


static uint8_t *panset(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    panset_main(pmd, qq, *si++);
    return si;
}


//=============================================================================
//  Pan setting Extend
//=============================================================================
static uint8_t *panset_ex(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *(char *)si++;
    si++;       // 逆走flagは読み飛ばす

    if(al > 0) {
        al = 2;
        panset_main(pmd, qq, al);
    } else if(al == 0) {
        al = 3;
        panset_main(pmd, qq, al);
    } else {
        al = 1;
        panset_main(pmd, qq, al);
    }
    return si;
}


//=============================================================================
//  ＦＭ　BLOCK,F-NUMBER SET
//      INPUTS  -- AL [KEY#,0-7F]
//=============================================================================
static void fnumset(PMDWIN *pmd, QQ *qq, int al)
{
    int     ax,bx;

    if((al & 0x0f) != 0x0f) {       // 音符の場合
        qq->onkai = al;

        // BLOCK/FNUM CALICULATE
        bx = al & 0x0f;     // bx=onkai
        ax = fnum_data[bx];

        // BLOCK SET
        ax |= (((al >> 1) & 0x38) << 8);
        qq->fnum = ax;
    } else {                        // 休符の場合
        qq->onkai = 255;
        if((qq->lfoswi & 0x11) == 0) {
            qq->fnum = 0;           // 音程LFO未使用
        }
    }
}


//-----------------------------------------------------------------------------
//  音量LFO用サブ
//-----------------------------------------------------------------------------
static void fmlfo_sub(PMDWIN *pmd, QQ *qq, int al, int bl, uint8_t *vol_tbl)
{
    if(bl & 0x80) vol_tbl[0] = Limit(vol_tbl[0] - al, 255, 0);
    if(bl & 0x40) vol_tbl[1] = Limit(vol_tbl[1] - al, 255, 0);
    if(bl & 0x20) vol_tbl[2] = Limit(vol_tbl[2] - al, 255, 0);
    if(bl & 0x10) vol_tbl[3] = Limit(vol_tbl[3] - al, 255, 0);
}


//-----------------------------------------------------------------------------
//  スロット毎の計算 & 出力 マクロ
//          in. dl  元のTL値
//              dh  Outするレジスタ
//              al  音量変動値 中心=80h
//-----------------------------------------------------------------------------
static void volset_slot(PMDWIN *pmd, int dh, int dl, int al)
{
    if((al += dl) > 255) al = 255;
    if((al -= 0x80) < 0) al = 0;
    OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, al);
}


//=============================================================================
//  ＦＭ音量設定メイン
//=============================================================================
static void volset(PMDWIN *pmd, QQ *qq)
{
    int bh, bl, cl, dh;
    uint8_t vol_tbl[4] = {0, 0, 0, 0};

    if(qq->slotmask == 0) return;
    if(qq->volpush) {
        cl = qq->volpush - 1;
    } else {
        cl = qq->volume;
    }

    //--------------------------------------------------------------------
    //  音量down計算
    //--------------------------------------------------------------------
    if(pmd->open_work.fm_voldown) {
        cl = ((256-pmd->open_work.fm_voldown) * cl) >> 8;
    }

    //------------------------------------------------------------------------
    //  音量をcarrierに設定 & 音量LFO処理
    //      input   cl to Volume[0-127]
    //          bl to SlotMask
    //------------------------------------------------------------------------

    bh=0;                   // Vol Slot Mask
    bl=qq->slotmask;        // ch=SlotMask Push

    vol_tbl[0] = 0x80;
    vol_tbl[1] = 0x80;
    vol_tbl[2] = 0x80;
    vol_tbl[3] = 0x80;

    cl = 255-cl;            // cl=carrierに設定する音量+80H(add)
    bl &= qq->carrier;      // bl=音量を設定するSLOT xxxx0000b
    bh |= bl;

    if(bl & 0x80) vol_tbl[0] = cl;
    if(bl & 0x40) vol_tbl[1] = cl;
    if(bl & 0x20) vol_tbl[2] = cl;
    if(bl & 0x10) vol_tbl[3] = cl;

    if(cl != 255) {
        if(qq->lfoswi & 2) {
            bl = qq->volmask;
            bl &= qq->slotmask;     // bl=音量LFOを設定するSLOT xxxx0000b
            bh |= bl;
            fmlfo_sub(pmd, qq, qq->lfodat, bl, vol_tbl);
        }

        if(qq->lfoswi & 0x20) {
            bl = qq->_volmask;
            bl &= qq->slotmask;
            bh |= bl;
            fmlfo_sub(pmd, qq, qq->_lfodat, bl, vol_tbl);
        }
    }

    dh = 0x4c - 1 + pmd->open_work.partb;      // dh=FM Port Address

    if(bh & 0x80) volset_slot(pmd, dh,    qq->slot4, vol_tbl[0]);
    if(bh & 0x40) volset_slot(pmd, dh-8,  qq->slot3, vol_tbl[1]);
    if(bh & 0x20) volset_slot(pmd, dh-4,  qq->slot2, vol_tbl[2]);
    if(bh & 0x10) volset_slot(pmd, dh-12, qq->slot1, vol_tbl[3]);
}


//=============================================================================
//  TONE DATA START ADDRESS を計算
//      input   dl  tone_number
//      output  bx  address
//=============================================================================
static uint8_t *toneadr_calc(PMDWIN *pmd, QQ *qq, int dl)
{
    uint8_t *bx;

    if(pmd->open_work.prg_flg == 0) {
        return NULL;
        //return pmd->open_work.tondat + (dl << 5);
    } else {
        bx = pmd->open_work.prgdat_adr;
        while(*bx != dl) {
            bx += 26;
        }
        return bx+1;
    }
}


//=============================================================================
//  [PartB]のパートの音を完璧に消す (TL=127 and RR=15 and KEY-OFF)
//      cy=1 ･･･ 全スロットneiromaskされている
//=============================================================================
static unsigned int silence_fmpart(PMDWIN *pmd, QQ *qq)
{
    int     dh;

    if(qq->neiromask == 0) {
        return 1;
    }

    dh = pmd->open_work.partb + 0x40 - 1;

    if(qq->neiromask & 0x80) {
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, 127);
        OPNASetReg(&pmd->opna, (pmd->open_work.fmsel + 0x40) + dh, 127);
    }
    dh += 4;

    if(qq->neiromask & 0x40) {
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, 127);
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + 0x40 + dh, 127);
    }
    dh += 4;

    if(qq->neiromask & 0x20) {
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, 127);
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + 0x40 + dh, 127);
    }
    dh += 4;

    if(qq->neiromask & 0x10) {
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, 127);
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + 0x40 + dh, 127);
    }

    kof1(pmd, qq);
    return 0;
}



//=============================================================================
//  音色の設定
//      INPUTS  -- [PARTB]
//          -- dl [TONE_NUMBER]
//          -- di [PART_DATA_ADDRESS]
//=============================================================================
static void neiroset(PMDWIN *pmd, QQ *qq, int dl)
{
    uint8_t   *bx;
    int     ah, al, dh;

    bx = toneadr_calc(pmd, qq, dl);
    if(silence_fmpart(pmd, qq)) {
        // neiromask=0の時 (TLのworkのみ設定)
        bx += 4;

        // tl設定
        qq->slot1 = bx[0];
        qq->slot3 = bx[1];
        qq->slot2 = bx[2];
        qq->slot4 = bx[3];
        return;
    }

    //=========================================================================
    //  音色設定メイン
    //=========================================================================
    //-------------------------------------------------------------------------
    //  AL/FBを設定
    //-------------------------------------------------------------------------

    dh = 0xb0 - 1 + pmd->open_work.partb;
    if(pmd->open_work.af_check) {      // ALG/FBは設定しないmodeか？
        dl = qq->alg_fb;
    } else {
        dl = bx[24];
    }

    if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
        if(pmd->open_work.af_check != 0) { // ALG/FBは設定しないmodeか？
            dl = pmd->open_work.fm3_alg_fb;
        } else {
            if((qq->slotmask & 0x10) == 0) {    // slot1を使用しているか？
                dl = (pmd->open_work.fm3_alg_fb & 0x38) | (dl & 7);
            }
            pmd->open_work.fm3_alg_fb = dl;
        }
    }

    OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    qq->alg_fb = dl;
    dl &= 7;        // dl = algo

    //-------------------------------------------------------------------------
    //  Carrierの位置を調べる (VolMaskにも設定)
    //-------------------------------------------------------------------------

    if((qq->volmask & 0x0f) == 0) {
        qq->volmask = carrier_table[dl];
    }

    if((qq->_volmask & 0x0f) == 0) {
        qq->_volmask = carrier_table[dl];
    }

    qq->carrier = carrier_table[dl];
    ah = carrier_table[dl+8];   // slot2/3の逆転データ(not済み)
    al = qq->neiromask;
    ah &= al;               // AH=TL用のmask / AL=その他用のmask

    //-------------------------------------------------------------------------
    //  各音色パラメータを設定 (TLはモジュレータのみ)
    //-------------------------------------------------------------------------

    dh = 0x30 - 1 + pmd->open_work.partb;
    dl = *bx++;             // DT/ML
    if(al & 0x80) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x40) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x20) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x10) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;             // TL
    if(ah & 0x80) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(ah & 0x40) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(ah & 0x20) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(ah & 0x10) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;             // KS/AR
    if(al & 0x08) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x04) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x02) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x01) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;             // AM/DR
    if(al & 0x80) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x40) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x20) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x10) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;             // SR
    if(al & 0x08) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x04) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x02) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x01) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;             // SL/RR
    if(al & 0x80) {
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    }
    dh+=4;

    dl = *bx++;
    if(al & 0x40) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x20) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x10) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

/*
    dl = *bx++;             // SL/RR
    if(al & 0x80) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x40) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x20) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;

    dl = *bx++;
    if(al & 0x10) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
    dh+=4;
*/

    //-------------------------------------------------------------------------
    //  SLOT毎のTLをワークに保存
    //-------------------------------------------------------------------------
    bx -= 20;
    qq->slot1 = bx[0];
    qq->slot3 = bx[1];
    qq->slot2 = bx[2];
    qq->slot4 = bx[3];
}


//=============================================================================
//  COMMAND '@' [PROGRAM CHANGE]
//=============================================================================
static uint8_t *comat(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    uint8_t   *bx;
    int     al, dl;

    qq->voicenum = al = *si++;
    dl = qq->voicenum;

    if(qq->partmask == 0) { // パートマスクされているか？
        neiroset(pmd, qq, dl);
        return si;
    } else {
        bx = toneadr_calc(pmd, qq, dl);
        qq->alg_fb = dl = bx[24];
        bx += 4;

        // tl設定
        qq->slot1 = bx[0];
        qq->slot3 = bx[1];
        qq->slot2 = bx[2];
        qq->slot4 = bx[3];

        //  FM3chで、マスクされていた場合、fm3_alg_fbを設定
        if(pmd->open_work.partb == 3 && qq->neiromask) {
            if(pmd->open_work.fmsel == 0) {
                // in. dl = alg/fb
                if((qq->slotmask & 0x10) == 0) {
                    al = pmd->open_work.fm3_alg_fb & 0x38;     // fbは前の値を使用
                    dl = (dl & 7) | al;
                }

                pmd->open_work.fm3_alg_fb = dl;
                qq->alg_fb = al;
            }
        }

    }
    return si;
}


//=============================================================================
//  ＦＭ音源ハードＬＦＯの設定（Ｖ２．４拡張分）
//=============================================================================
static uint8_t *hlfo_set(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    qq->fmpan = (qq->fmpan & 0xc0) | *si++;

    if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
        // 2608の時のみなので part_eはありえない
        //  FM3の場合は 4つのパート総て設定
        pmd->FMPart[2].fmpan = qq->fmpan;
    }

    if(qq->partmask == 0) {     // パートマスクされているか？
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + pmd->open_work.partb + 0xb4 - 1,
            calc_panout(qq));
    }
    return si;
}


//=============================================================================
//  FM slotmask set
//=============================================================================
static uint8_t *slotmask_set(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    uint8_t   *bx;
    int     ah, al, bl;

    ah = al = *si++;

    if(al &= 0x0f) {
        qq->carrier = al << 4;
    } else {
        if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
            bl = pmd->open_work.fm3_alg_fb;
        } else {
            bx = toneadr_calc(pmd, qq, qq->voicenum);
            bl = bx[24];
        }
        qq->carrier = carrier_table[bl & 7];
    }

    ah &= 0xf0;
    if(qq->slotmask != ah) {
        qq->slotmask = ah;
        if((ah & 0xf0) == 0) {
            qq->partmask |= 0x20;   // s0の時パートマスク
        } else {
            qq->partmask &= 0xdf;   // s0以外の時パートマスク解除
        }

        if(ch3_setting(pmd, qq)) {       // FM3chの場合のみ ch3modeの変更処理
            // ch3なら、それ以前のFM3パートでkeyon処理
            if(qq != &pmd->FMPart[2]) {
                if(pmd->FMPart[2].partmask == 0 &&
                  (pmd->FMPart[2].keyoff_flag & 1) == 0) {
                    keyon(pmd, &pmd->FMPart[2]);
                }
            }
        }

        ah = 0;
        if(qq->slotmask & 0x80) ah += 0x11;     // slot4
        if(qq->slotmask & 0x40) ah += 0x44;     // slot3
        if(qq->slotmask & 0x20) ah += 0x22;     // slot2
        if(qq->slotmask & 0x10) ah += 0x88;     // slot1
        qq->neiromask = ah;
    }
    return si;
}


//=============================================================================
//  Slot Detune Set
//=============================================================================
static uint8_t *slotdetune_set(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     ax, bl;

    if(pmd->open_work.partb != 3 || pmd->open_work.fmsel) {
        return si+3;
    }

    bl = *si++;
    ax = *(short *)si;
    si+=2;

    if(bl & 1) {
        pmd->open_work.slot_detune1 = ax;
    }

    if(bl & 2) {
        pmd->open_work.slot_detune2 = ax;
    }

    if(bl & 4) {
        pmd->open_work.slot_detune3 = ax;
    }

    if(bl & 8) {
        pmd->open_work.slot_detune4 = ax;
    }

    if(pmd->open_work.slot_detune1 || pmd->open_work.slot_detune2 ||
            pmd->open_work.slot_detune3 || pmd->open_work.slot_detune4) {
            pmd->open_work.slotdetune_flag = 1;
    } else {
        pmd->open_work.slotdetune_flag = 0;
        pmd->open_work.slot_detune1 = 0;
    }
    ch3mode_set(pmd, qq);
    return si;
}


//=============================================================================
//  Slot Detune Set(相対)
//=============================================================================
static uint8_t *slotdetune_set2(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     ax, bl;

    if(pmd->open_work.partb != 3 || pmd->open_work.fmsel) {
        return si+3;
    }

    bl = *si++;
    ax = *(short *)si;
    si+=2;

    if(bl & 1) {
        pmd->open_work.slot_detune1 += ax;
    }

    if(bl & 2) {
        pmd->open_work.slot_detune2 += ax;
    }

    if(bl & 4) {
        pmd->open_work.slot_detune3 += ax;
    }

    if(bl & 8) {
        pmd->open_work.slot_detune4 += ax;
    }

    if(pmd->open_work.slot_detune1 || pmd->open_work.slot_detune2 ||
            pmd->open_work.slot_detune3 || pmd->open_work.slot_detune4) {
        pmd->open_work.slotdetune_flag = 1;
    } else {
        pmd->open_work.slotdetune_flag = 0;
        pmd->open_work.slot_detune1 = 0;
    }
    ch3mode_set(pmd, qq);
    return si;
}


//=============================================================================
//  音量マスクslotの設定
//=============================================================================
static uint8_t *volmask_set(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *si++ & 0x0f;
    if(al) {
        al = (al << 4) | 0x0f;
        qq->volmask = al;
    } else {
        qq->volmask = qq->carrier;
    }
    ch3_setting(pmd, qq);
    return si;
}


//=============================================================================
//  0c0hの追加special命令
//=============================================================================
static uint8_t *_vd_fm(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *(char *)si++;
    if(al) {
        pmd->open_work.fm_voldown = Limit(al + pmd->open_work.fm_voldown, 255, 0);
    } else {
        pmd->open_work.fm_voldown = pmd->open_work._fm_voldown;
    }
    return si;
}


static uint8_t *_vd_ssg(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *(char *)si++;
    if(al) {
        pmd->open_work.ssg_voldown = Limit(al + pmd->open_work.ssg_voldown, 255, 0);
    } else {
        pmd->open_work.ssg_voldown = pmd->open_work._ssg_voldown;
    }
    return si;
}


static uint8_t *_vd_rhythm(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *(char *)si++;
    if(al) {
        pmd->open_work.rhythm_voldown = Limit(al + pmd->open_work.rhythm_voldown, 255, 0);
    } else {
        pmd->open_work.rhythm_voldown = pmd->open_work._rhythm_voldown;
    }
    return si;
}


static uint8_t *special_0c0h(PMDWIN *pmd, QQ *qq, uint8_t *si, uint8_t al)
{
    switch(al) {
        case 0xff : pmd->open_work.fm_voldown = *si++; break;
        case 0xfe : si = _vd_fm(pmd, qq, si); break;
        case 0xfd : pmd->open_work.ssg_voldown = *si++; break;
        case 0xfc : si = _vd_ssg(pmd, qq, si); break;
        case 0xfb : si++; break;
        case 0xfa : si = NULL; break;
        case 0xf9 : pmd->open_work.rhythm_voldown = *si++; break;
        case 0xf8 : si = _vd_rhythm(pmd, qq, si); break;
        case 0xf7 : si++; break;
        case 0xf6 : si++; break;
        case 0xf5 : break;
        default :
            si--;
            *si = 0x80;
    }
    return si;
}


//=============================================================================
//  FM音源の音色を再設定
//=============================================================================
static void neiro_reset(PMDWIN *pmd, QQ *qq)
{
    int     dh, al, s1, s2, s3, s4;

    if(qq->neiromask == 0) return;

    s1 = qq->slot1;
    s2 = qq->slot2;
    s3 = qq->slot3;
    s4 = qq->slot4;
    pmd->open_work.af_check = 1;
    neiroset(pmd, qq, qq->voicenum);     // 音色復帰
    pmd->open_work.af_check = 0;
    qq->slot1 = s1;
    qq->slot2 = s2;
    qq->slot3 = s3;
    qq->slot4 = s4;

    al = ((~qq->carrier) & qq->slotmask) & 0xf0;
        // al<- TLを再設定していいslot 4321xxxx
    if(al) {
        dh = 0x4c - 1 + pmd->open_work.partb;  // dh=TL FM Port Address
        if(al & 0x80) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, s4);

        dh -= 8;
        if(al & 0x40) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, s3);

        dh += 4;
        if(al & 0x20) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, s2);

        dh -= 8;
        if(al & 0x10) OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, s1);
    }

    dh = pmd->open_work.partb + 0xb4 - 1;
    OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, calc_panout(qq));
}


static uint8_t *_volmask_set(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *si++ & 0x0f;
    if(al) {
        al = (al << 4) | 0x0f;
        qq->_volmask = al;
    } else {
        qq->_volmask = qq->carrier;
    }
    ch3_setting(pmd, qq);
    return si;
}


//=============================================================================
//  演奏中パートのマスクon/off
//=============================================================================
static uint8_t *fm_mml_part_mask(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *si++;
    if(al >= 2) {
        si = special_0c0h(pmd, qq, si, al);
    } else if(al) {
        qq->partmask |= 0x40;
        if(qq->partmask == 0x40) {
            silence_fmpart(pmd, qq); // 音消去
        }
    } else {
        if((qq->partmask &= 0xbf) == 0) {
            neiro_reset(pmd, qq);        // 音色再設定
        }
    }
    return si;
}


static uint8_t *ssg_mml_part_mask(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int ah, al = *si++;
    if(al >= 2) {
        si = special_0c0h(pmd, qq, si, al);
    } else if(al) {
        qq->partmask |= 0x40;
        if(qq->partmask == 0x40) {
            ah = ((1 << (pmd->open_work.partb-1)) | (4 << pmd->open_work.partb));
            al = PSGGetReg(&(pmd->opna.psg), 0x07);
            PSGSetReg(&(pmd->opna.psg), 0x07, ah | al);      // PSG keyoff
        }
    } else {
        qq->partmask &= 0xbf;
    }
    return si;
}


static uint8_t *rhythm_mml_part_mask(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al = *si++;
    if(al >= 2) {
        si = special_0c0h(pmd, qq, si, al);
    } else if(al) {
        qq->partmask |= 0x40;
    } else {
        qq->partmask &= 0xbf;
    }
    return si;
}


//=============================================================================
//  TL変化
//=============================================================================
static uint8_t *tl_set(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     ah, al, ch, dh, dl;

    dh = 0x40 - 1 + pmd->open_work.partb;      // dh=TL FM Port Address
    al = *(char *)si++;
    ah = al & 0x0f;
    ch = (qq->slotmask >> 4) | ((qq->slotmask << 4) & 0xf0);
    ah &= ch;                           // ah=変化させるslot 00004321
    dl = *(char *)si++;

    if(al >= 0) {
        dl &= 127;
        if(ah & 1) {
            qq->slot1 = dl;
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
        }

        dh += 8;
        if(ah & 2) {
            qq->slot2 = dl;
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
        }

        dh -= 4;
        if(ah & 4) {
            qq->slot3 = dl;
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
        }

        dh += 8;
        if(ah & 8) {
            qq->slot4 = dl;
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
        }
    } else {
        //  相対変化
        al = dl;
        if(ah & 1) {
            if((dl = (int)qq->slot1 + al) < 0) {
                dl = 0;
                if(al >= 0) dl = 127;
            }
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
            qq->slot1 = dl;
        }

        dh += 8;
        if(ah & 2) {
            if((dl = (int)qq->slot2 + al) < 0) {
                dl = 0;
                if(al >= 0) dl = 127;
            }
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
            qq->slot2 = dl;
        }

        dh -= 4;
        if(ah & 4) {
            if((dl = (int)qq->slot3 + al) < 0) {
                dl = 0;
                if(al >= 0) dl = 127;
            }
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
            qq->slot3 = dl;
        }

        dh += 8;
        if(ah & 8) {
            if((dl = (int)qq->slot4 + al) < 0) {
                dl = 0;
                if(al >= 0) dl = 127;
            }
            if(qq->partmask == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            }
            qq->slot4 = dl;
        }
    }
    return si;
}


//=============================================================================
//  FB変化
//=============================================================================
static uint8_t *fb_set(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     al, dh, dl;

    dh = pmd->open_work.partb + 0xb0 - 1;  // dh=ALG/FB port address
    al = *(char *)si++;
    if(al >= 0) {
        // in   al 00000xxx 設定するFB
        al = ((al << 3) & 0xff) | (al >> 5);

        // in   al 00xxx000 設定するFB
        if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
            if((qq->slotmask & 0x10) == 0) return si;
            dl = (pmd->open_work.fm3_alg_fb & 7) | al;
            pmd->open_work.fm3_alg_fb = dl;
        } else {
            dl = (qq->alg_fb & 7) | al;
        }
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
        qq->alg_fb = dl;
        return si;
    } else {
        if((al & 0x40) == 0) al &= 7;
        if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
            dl = pmd->open_work.fm3_alg_fb;
        } else {
            dl = qq->alg_fb;
        }

        dl = (dl >> 3) & 7;
        if((al += dl) >= 0) {
            if(al >= 8) {
                al = 0x38;
                if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
                    if((qq->slotmask & 0x10) == 0) return si;

                    dl = (pmd->open_work.fm3_alg_fb & 7) | al;
                    pmd->open_work.fm3_alg_fb = dl;
                } else {
                    dl = (qq->alg_fb & 7) | al;
                }
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
                qq->alg_fb = dl;
                return si;
            } else {
                // in   al 00000xxx 設定するFB
                al = ((al << 3) & 0xff) | (al >> 5);

                // in   al 00xxx000 設定するFB
                if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
                    if((qq->slotmask & 0x10) == 0) return si;
                    dl = (pmd->open_work.fm3_alg_fb & 7) | al;
                    pmd->open_work.fm3_alg_fb = dl;
                } else {
                    dl = (qq->alg_fb & 7) | al;
                }
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
                qq->alg_fb = dl;
                return si;
            }
        } else {
            al = 0;
            if(pmd->open_work.partb == 3 && pmd->open_work.fmsel == 0) {
                if((qq->slotmask & 0x10) == 0) return si;

                dl = (pmd->open_work.fm3_alg_fb & 7) | al;
                pmd->open_work.fm3_alg_fb = dl;
            } else {
                dl = (qq->alg_fb & 7) | al;
            }
            OPNASetReg(&pmd->opna, pmd->open_work.fmsel + dh, dl);
            qq->alg_fb = dl;
            return si;
        }
    }
}


//=============================================================================
//  T->t 変換
//      input   [tempo_d]
//      output  [tempo_48]
//=============================================================================
static void calc_tb_tempo(PMDWIN *pmd)
{
//  TEMPO = 0x112C / [ 256 - TB ]   timerB -> tempo
    int temp;

    if(256 - pmd->open_work.tempo_d == 0) {
        temp = 255;
    } else {
        temp = (0x112c * 2 / (256 - pmd->open_work.tempo_d) + 1) / 2;
        if(temp > 255) temp = 255;
    }

    pmd->open_work.tempo_48 = temp;
    pmd->open_work.tempo_48_push = temp;
}


//=============================================================================
//  t->T 変換
//      input   [tempo_48]
//      output  [tempo_d]
//=============================================================================
static void calc_tempo_tb(PMDWIN *pmd)
{
    int     al;

    //  TB = 256 - [ 112CH / TEMPO ]    tempo -> timerB
    if(pmd->open_work.tempo_48 >= 18) {
        al = 256 - 0x112c / pmd->open_work.tempo_48;
        if(0x112c % pmd->open_work.tempo_48 >= 128) {
            al--;
        }
        //al = 256 - (0x112c * 2 / pmd->open_work.tempo_48 + 1) / 2;
    } else {
        al = 0;
    }
    pmd->open_work.tempo_d = al;
    pmd->open_work.tempo_d_push = al;
}


//=============================================================================
//  COMMAND 't' [TEMPO CHANGE1]
//  COMMAND 'T' [TEMPO CHANGE2]
//  COMMAND 't±' [TEMPO CHANGE 相対1]
//  COMMAND 'T±' [TEMPO CHANGE 相対2]
//=============================================================================
static uint8_t *comt(PMDWIN *pmd, uint8_t *si)
{
    int     al;

    al = *si++;
    if(al < 251) {
        pmd->open_work.tempo_d = al;     // T (FC)
        pmd->open_work.tempo_d_push = al;
        calc_tb_tempo(pmd);
    } else if(al == 0xff) {
        al = *si++;                 // t (FC FF)
        if(al < 18) al = 18;
        pmd->open_work.tempo_48 = al;
        pmd->open_work.tempo_48_push = al;
        calc_tempo_tb(pmd);
    } else if(al == 0xfe) {
        al = (char)(*si++);           // T± (FC FE)
        if(al >= 0) {
            al += pmd->open_work.tempo_d_push;
        } else {
            al += pmd->open_work.tempo_d_push;
            if(al < 0) {
                al = 0;
            }
        }
        if(al > 250) al = 250;
        pmd->open_work.tempo_d = al;
        pmd->open_work.tempo_d_push = al;
        calc_tb_tempo(pmd);
    } else {
        al = (char)(*si++);           // t± (FC FD)
        if(al >= 0) {
            al += pmd->open_work.tempo_48_push;
            if(al > 255) {
                al = 255;
            }
        } else {
            al += pmd->open_work.tempo_48_push;
            if(al < 0) al = 18;
        }
        pmd->open_work.tempo_48 = al;
        pmd->open_work.tempo_48_push = al;
        calc_tempo_tb(pmd);
    }
    return si;
}


//=============================================================================
//  COMMAND '[' [ﾙｰﾌﾟ ｽﾀｰﾄ]
//=============================================================================
static uint8_t *comstloop(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    uint8_t *ax = pmd->open_work.mmlbuf;
    ax[*(uint16_t *)si+1] = 0;
    si+=2;
    return si;
}


//=============================================================================
//  COMMAND ']' [ﾙｰﾌﾟ ｴﾝﾄﾞ]
//=============================================================================
static uint8_t *comedloop(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     ah, al, ax;
    ah = *si++;

    if(ah) {
        (*si)++;
        al = *si++;
        if(ah == al) {
            si+=2;
            return si;
        }
    } else {            // 0 ﾅﾗ ﾑｼﾞｮｳｹﾝ ﾙｰﾌﾟ
        si++;
        qq->loopcheck = 1;
    }

    ax = *(uint16_t *)si + 2;
    si = pmd->open_work.mmlbuf + ax;
    return si;
}


//=============================================================================
//  COMMAND ':' [ﾙｰﾌﾟ ﾀﾞｯｼｭﾂ]
//=============================================================================
static uint8_t *comexloop(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    uint8_t   *bx;
    int     dl;
    bx = pmd->open_work.mmlbuf;
    bx += *(uint16_t *)si;
    si+=2;

    dl = *bx++ - 1;
    if(dl != *bx) return si;
    si = bx + 3;
    return si;
}


//=============================================================================
//  PSG ENVELOPE SET
//=============================================================================
static uint8_t *psgenvset(QQ *qq, uint8_t *si)
{
    qq->eenv_ar = *si; qq->eenv_arc = *si++;
    qq->eenv_dr = *(char *)si++;
    qq->eenv_sr = *si; qq->eenv_src = *si++;
    qq->eenv_rr = *si; qq->eenv_rrc = *si++;
    if(qq->envf == -1) {
        qq->envf = 2;       // RR
        qq->eenv_volume = -15;      // volume
    }
    return si;
}


//=============================================================================
//  "¥?" COMMAND [ OPNA Rhythm Keyon/Dump ]
//=============================================================================
static uint8_t *rhykey(PMDWIN *pmd, uint8_t *si)
{
    int dl;

    if((dl = (*si++ & pmd->open_work.rhythmmask)) == 0) {
        return si;
    }

    if(dl < 0x80) {
        if(dl & 0x01) OPNASetReg(&pmd->opna, 0x18, pmd->open_work.rdat[0]);
        if(dl & 0x02) OPNASetReg(&pmd->opna, 0x19, pmd->open_work.rdat[1]);
        if(dl & 0x04) OPNASetReg(&pmd->opna, 0x1a, pmd->open_work.rdat[2]);
        if(dl & 0x08) OPNASetReg(&pmd->opna, 0x1b, pmd->open_work.rdat[3]);
        if(dl & 0x10) OPNASetReg(&pmd->opna, 0x1c, pmd->open_work.rdat[4]);
        if(dl & 0x20) OPNASetReg(&pmd->opna, 0x1d, pmd->open_work.rdat[5]);
    }

    OPNASetReg(&pmd->opna, 0x10, dl);
    if(dl >= 0x80) {
        pmd->open_work.rshot_dat &= (~dl);
    } else {
        pmd->open_work.rshot_dat |= dl;
    }
    return si;
}


//=============================================================================
//  "¥v?n" COMMAND
//=============================================================================
static uint8_t *rhyvs(PMDWIN *pmd, uint8_t *si)
{
    int     *bx;
    int     dh, dl;

    dl = *si & 0x1f;
    dh = *si++ >> 5;
    bx = &pmd->open_work.rdat[dh-1];
    dh = 0x18 - 1 + dh;
    dl |= (*bx & 0xc0);
    *bx = dl;

    OPNASetReg(&pmd->opna, dh, dl);
    return si;
}


static uint8_t *rhyvs_sft(PMDWIN *pmd, uint8_t *si)
{
    int     *bx;
    int     al, dh, dl;

    bx = &pmd->open_work.rdat[*si-1];
    dh = *si++ + 0x18 - 1;
    dl = *bx & 0x1f;

    al = (*(char *)si++ + dl);
    if(al >= 32) {
        al = 31;
    } else if(al < 0) {
        al = 0;
    }

    dl = (al &= 0x1f);
    dl = *bx = ((*bx & 0xe0) | dl);
    OPNASetReg(&pmd->opna, dh, dl);
    return si;
}


//=============================================================================
//  "¥p?" COMMAND
//=============================================================================
static uint8_t *rpnset(PMDWIN *pmd, uint8_t *si)
{
    int     *bx;
    int     dh, dl;

    dl = (*si & 3) << 6;

    dh = (*si++ >> 5) & 0x07;
    bx = &pmd->open_work.rdat[dh-1];

    dh += 0x18 - 1;
    dl |= (*bx & 0x1f);
    *bx = dl;
    OPNASetReg(&pmd->opna, dh, dl);
    return si;
}


//=============================================================================
//  "¥Vn" COMMAND
//=============================================================================
static uint8_t *rmsvs(PMDWIN *pmd, uint8_t *si)
{
    int     dl;
    dl = *si++;
    if(pmd->open_work.rhythm_voldown) {
        dl = ((256 - pmd->open_work.rhythm_voldown) * dl) >> 8;
    }
    pmd->open_work.rhyvol = dl;
    OPNASetReg(&pmd->opna, 0x11, dl);
    return si;
}


static uint8_t *rmsvs_sft(PMDWIN *pmd, uint8_t *si)
{
    int     dl;
    dl = pmd->open_work.rhyvol + *(char *)si++;
    if(dl >= 64) {
        if(dl & 0x80) {
            dl = 0;
        } else {
            dl = 63;
        }
    }
    pmd->open_work.rhyvol = dl;
    OPNASetReg(&pmd->opna, 0x11, dl);
    return si;
}


//=============================================================================
//  ボリュームを次の一個だけ変更（Ｖ２．７拡張分）
//=============================================================================
static uint8_t *vol_one_up_fm(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     al;

    al = (int)qq->volume + 1 + *si++;
    if(al > 128) al = 128;

    qq->volpush = al;
    pmd->open_work.volpush_flag = 1;
    return si;
}


//=============================================================================
//  ボリュームを次の一個だけ変更（Ｖ２．７拡張分）
//=============================================================================
static uint8_t *vol_one_up_psg(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     al;

    al = qq->volume + *si++;
    if(al > 15) al = 15;
    qq->volpush = ++al;
    pmd->open_work.volpush_flag = 1;
    return si;
}


static uint8_t *vol_one_down(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     al;

    al = qq->volume - *si++;
    if(al < 0) {
        al = 0;
    } else {
        if(al >= 255) al = 254;
    }

    qq->volpush = ++al;
    pmd->open_work.volpush_flag = 1;
    return si;
}


//=============================================================================
//  'w' COMMAND [PSG NOISE ﾍｲｷﾝ ｼｭｳﾊｽｳ]
//=============================================================================
static uint8_t *psgnoise_move(PMDWIN *pmd, uint8_t *si)
{
    pmd->open_work.psnoi += *(char *)si++;
    if(pmd->open_work.psnoi < 0) pmd->open_work.psnoi = 0;
    if(pmd->open_work.psnoi > 31) pmd->open_work.psnoi = 31;
    return si;
}


//=============================================================================
//  PSG Envelope set (Extend)
//=============================================================================
static uint8_t *extend_psgenvset(QQ *qq, uint8_t *si)
{
    qq->eenv_ar = *si++ & 0x1f;
    qq->eenv_dr = *si++ & 0x1f;
    qq->eenv_sr = *si++ & 0x1f;
    qq->eenv_rr = *si & 0x0f;
    qq->eenv_sl = ((*si++ >> 4) & 0x0f) ^ 0x0f;
    qq->eenv_al = *si++ & 0x0f;

    if(qq->envf != -1) {    // ノーマル＞拡張に移行したか？
        qq->envf = -1;
        qq->eenv_count = 4;     // RR
        qq->eenv_volume = 0;    // Volume
    }
    return si;
}


static uint8_t *mdepth_count(QQ *qq, uint8_t *si)
{
    int     al;

    al = *si++;

    if(al >= 0x80) {
        if((al &= 0x7f) == 0) al = 255;
        qq->_mdc = al;
        qq->_mdc2 = al;
        return si;
    }

    if(al == 0) al = 255;
    qq->mdc = al;
    qq->mdc2 = al;
    return si;
}


//=============================================================================
//  SHIFT[di] 分移調する
//=============================================================================
static int oshift(PMDWIN *pmd, QQ *qq, int al)
{
    int bl, bh, dl;

    if(al == 0x0f) return al;

    if(al == 0x0c) {
        if((al = qq->onkai) >= 0x80) {
            return 0x0f;
        } else {
            return al;  //@暫定対応
        }
    }

    dl = qq->shift + qq->shift_def;
    if(dl == 0) return al;

    bl = (al & 0x0f);       // bl = ONKAI
    bh = (al & 0xf0) >> 4;  // bh = OCT

    if(dl < 0) {
        // - ﾎｳｺｳ ｼﾌﾄ
        if((bl += dl) < 0) {
            do {
                bh--;
            } while((bl+=12) < 0);
        }
        if(bh < 0) bh = 0;
        return (bh << 4) | bl;

    } else {
        // + ﾎｳｺｳ ｼﾌﾄ
        bl += dl;
        while(bl >= 0x0c) {
            bh++;
            bl -= 12;
        }

        if(bh > 7) bh = 7;
        return (bh << 4) | bl;
    }
}


static int oshiftp(PMDWIN *pmd, QQ *qq, int al)
{
    return oshift(pmd, qq, al);
}


//=============================================================================
//  PSG TUNE SET
//=============================================================================
static void fnumsetp(PMDWIN *pmd, QQ *qq, int al)
{
    int ax, bx, cl;

    if((al & 0x0f) == 0x0f) {       // ｷｭｳﾌ ﾅﾗ FNUM ﾆ 0 ｦ ｾｯﾄ
        qq->onkai = 255;
        if(qq->lfoswi & 0x11) return;
        qq->fnum = 0;   // 音程LFO未使用
        return;
    }

    qq->onkai = al;

    cl = (al >> 4) & 0x0f;  // cl=oct
    bx = al & 0x0f;         // bx=onkai

    ax = psg_tune_data[bx];
    if(cl > 0) {
        ax = (ax + 1) >> cl;
    }

    qq->fnum = ax;
}


//=============================================================================
//  Q値の計算
//      break   dx
//=============================================================================
static uint8_t *calc_q(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int dh, dl;

    if(*si == 0xc1) {       // &&
        si++;
        qq->qdat = 0;
        return si;
    }

    dl = qq->qdata;
    if(qq->qdatb) {
        dl += (qq->leng * qq->qdatb) >> 8;
    }

#if 0
    if(qq->qdat3) {     //  Random-Q
        ax = rand() % ((qq->qdat3 & 0x7f) + 1);
        if((qq->qdat3 & 0x80) == 0) {
            dl += ax;
        } else {
            dl -= ax;
            if(dl < 0) dl = 0;
        }
    }
#endif

    if(qq->qdat2) {
        if((dh = qq->leng - qq->qdat2) < 0) {
            qq->qdat = 0;
            return si;
        }

        if(dl < dh) {
            qq->qdat = dl;
        } else {
            qq->qdat = dh;
        }
    } else {
        qq->qdat = dl;
    }

    return si;
}


//=============================================================================
//  ＰＳＧ　ＶＯＬＵＭＥ　ＳＥＴ
//=============================================================================
static void volsetp(PMDWIN *pmd, QQ *qq)
{
    int     ax, dl;

    if(qq->envf == 3 || (qq->envf == -1 && qq->eenv_count == 0)) return;

    if(qq->volpush) {
        dl = qq->volpush-1;
    } else {
        dl = qq->volume;
    }

    //------------------------------------------------------------------------
    //  音量down計算
    //------------------------------------------------------------------------
    dl = ((256-pmd->open_work.ssg_voldown) * dl) >> 8;

    //------------------------------------------------------------------------
    //  ENVELOPE 計算
    //------------------------------------------------------------------------
    if(dl <= 0) {
        OPNASetReg(&pmd->opna, pmd->open_work.partb+8-1, 0);
        return;
    }

    if(qq->envf == -1) {
        if(qq->eenv_volume == 0) {
            OPNASetReg(&pmd->opna, pmd->open_work.partb+8-1, 0);
            return;
        }
        dl = ((((dl * (qq->eenv_volume + 1))) >> 3) + 1) >> 1;
    } else {
        dl += qq->eenv_volume;
        if(dl <= 0) {
            OPNASetReg(&pmd->opna, pmd->open_work.partb+8-1, 0);
            return;
        }
        if(dl > 15) dl = 15;
    }

    //--------------------------------------------------------------------
    //  音量LFO計算
    //--------------------------------------------------------------------
    if((qq->lfoswi & 0x22) == 0) {
        OPNASetReg(&pmd->opna, pmd->open_work.partb+8-1, dl);
        return;
    }

    if(qq->lfoswi & 2) {
        ax = qq->lfodat;
    } else {
        ax = 0;
    }

    if(qq->lfoswi & 0x20) {
        ax += qq->_lfodat;
    }

    dl = dl + ax;
    if(dl <  0) {
        OPNASetReg(&pmd->opna, pmd->open_work.partb+8-1, 0);
        return;
    }
    if(dl > 15) dl = 15;

    //------------------------------------------------------------------------
    //  出力
    //------------------------------------------------------------------------
    OPNASetReg(&pmd->opna, pmd->open_work.partb+8-1, dl);
}


//=============================================================================
//  ＰＳＧ　音程設定
//=============================================================================
static void otodasip(PMDWIN *pmd, QQ *qq)
{
    int     ax, dx;

    if(qq->fnum == 0) return;

    // PSG Portament set
    ax = qq->fnum + qq->porta_num;
    dx = 0;

    // PSG Detune/LFO set
    if((qq->extendmode & 1) == 0) {
        ax -= qq->detune;
        if(qq->lfoswi & 1) {
            ax -= qq->lfodat;
        }

        if(qq->lfoswi & 0x10) {
            ax -= qq->_lfodat;
        }
    } else {
        // 拡張DETUNE(DETUNE)の計算
        if(qq->detune) {
            dx = (ax * qq->detune) >> 12;       // dx:ax=ax * qq->detune
            if(dx >= 0) {
                dx++;
            } else {
                dx--;
            }
            ax -= dx;
        }
        // 拡張DETUNE(LFO)の計算
        if(qq->lfoswi & 0x11) {
            if(qq->lfoswi & 1) {
                dx = qq->lfodat;
            } else {
                dx = 0;
            }

            if(qq->lfoswi & 0x10) {
                dx += qq->_lfodat;
            }

            if(dx) {
                dx = (ax * dx) >> 12;
                if(dx >= 0) {
                    dx++;
                } else {
                    dx--;
                }
            }
            ax -= dx;
        }
    }

    // TONE SET
    if(ax >= 0x1000) {
        if(ax >= 0) {
            ax = 0xfff;
        } else {
            ax = 0;
        }
    }

    OPNASetReg(&pmd->opna, (pmd->open_work.partb-1) * 2, ax & 0xff);
    OPNASetReg(&pmd->opna, (pmd->open_work.partb-1) * 2 + 1, ax >> 8);
}


//=============================================================================
//  MDコマンドの値によってSTEP値を変更
//=============================================================================
static void md_inc(PMDWIN *pmd, QQ *qq)
{
    int     al;

    if(--qq->mdspd) return;

    qq->mdspd = qq->mdspd2;

    if(qq->mdc == 0) return;        // count = 0
    if(qq->mdc <= 127) {
        qq->mdc--;
    }

    if(qq->step < 0) {
        al = qq->mdepth - qq->step;
        if(al >= 0) {
            qq->step = -al;
        } else {
            if(qq->mdepth < 0) {
                qq->step = 0;
            } else {
                qq->step = -127;
            }
        }
    } else {
        al = qq->step + qq->mdepth;
        if(al >= 0) {
            qq->step = al;
        } else {
            if(qq->mdepth < 0) {
                qq->step = 0;
            } else {
                qq->step = 127;
            }
        }
    }
}


static void lfo_main(PMDWIN *pmd, QQ *qq)
{
    int     al, ax;

    if(qq->speed != 1) {
        if(qq->speed != 255) qq->speed--;
        return;
    }

    qq->speed = qq->speed2;
    if(qq->lfo_wave == 0 || qq->lfo_wave == 4 || qq->lfo_wave == 5) {
        //  三角波      lfowave = 0,4,5
        if(qq->lfo_wave == 5) {
            ax = abs(qq->step) * qq->step;
        } else {
            ax = qq->step;
        }

        if((qq->lfodat += ax) == 0) {
            md_inc(pmd, qq);
        }

        al = qq->time;
        if(al != 255) {
            if(--al == 0) {
                al = qq->time2;
                if(qq->lfo_wave != 4) {
                    al += al;   // lfowave=0,5の場合 timeを反転時２倍にする
                }
                qq->time = al;
                qq->step = -qq->step;
                return;
            }
        }
        qq->time = al;

    } else if(qq->lfo_wave == 2) {
        //  矩形波      lfowave = 2
        qq->lfodat = (qq->step * qq->time);
        md_inc(pmd, qq);
        qq->step = -qq->step;

    } else if(qq->lfo_wave == 6) {
        //  ワンショット    lfowave = 6
        if(qq->time) {
            if(qq->time != 255) {
                qq->time--;
            }
            qq->lfodat += qq->step;
        }
    } else if(qq->lfo_wave == 1) {
        //ノコギリ波    lfowave = 1
        qq->lfodat += qq->step;
        al = qq->time;
        if(al != -1) {
            al--;
            if(al == 0)  {
                qq->lfodat = -qq->lfodat;
                md_inc(pmd, qq);
                al = (qq->time2) * 2;
            }
        }
        qq->time = al;

    } else {
        //  ランダム波  lfowave = 3
        ax = abs(qq->step) * qq->time;
        qq->lfodat = ax - (lfg_rand(&pmd->open_work) % (ax * 2));
        md_inc(pmd, qq);
    }
}


//=============================================================================
//  ＬＦＯ処理
//      Don't Break cl
//      output      cy=1    変化があった
//=============================================================================
static unsigned int lfo(PMDWIN *pmd, QQ *qq)
{
    int     ax, ch;

    if(qq->delay) {
        qq->delay--;
        return 0;
    }

    if(qq->extendmode & 2) {    // TimerAと合わせるか？
                                // そうじゃないなら無条件にlfo処理
        ch = pmd->open_work.TimerAtime - pmd->open_work.lastTimerAtime;
        if(ch == 0) return 0;
        ax = qq->lfodat;

        for(; ch > 0; ch--) {
            lfo_main(pmd, qq);
        }
    } else {
        ax = qq->lfodat;
        lfo_main(pmd, qq);
    }

    if(ax == qq->lfodat) {
        return 0;
    }
    return 1;
}


static inline void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}


//=============================================================================
//  LFO1<->LFO2 change
//=============================================================================
static void lfo_change(PMDWIN *pmd, QQ *qq)
{
    swap(&qq->lfodat, &qq->_lfodat);
    qq->lfoswi = ((qq->lfoswi & 0x0f) << 4) + (qq->lfoswi >> 4);
    qq->extendmode = ((qq->extendmode & 0x0f) << 4) + (qq->extendmode >> 4);

    swap(&qq->delay, &qq->_delay);
    swap(&qq->speed, &qq->_speed);
    swap(&qq->step, &qq->_step);
    swap(&qq->time, &qq->_time);
    swap(&qq->delay2, &qq->_delay2);
    swap(&qq->speed2, &qq->_speed2);
    swap(&qq->step2, &qq->_step2);
    swap(&qq->time2, &qq->_time2);
    swap(&qq->mdepth, &qq->_mdepth);
    swap(&qq->mdspd, &qq->_mdspd);
    swap(&qq->mdspd2, &qq->_mdspd2);
    swap(&qq->lfo_wave, &qq->_lfo_wave);
    swap(&qq->mdc, &qq->_mdc);
    swap(&qq->mdc2, &qq->_mdc2);
}


static void lfo_exit(PMDWIN *pmd, QQ *qq)
{
    if((qq->lfoswi & 3) != 0) {     // 前が & の場合 -> 1回 LFO処理
        lfo(pmd, qq);
    }

    if((qq->lfoswi & 0x30) != 0) {  // 前が & の場合 -> 1回 LFO処理
        lfo_change(pmd, qq);
        lfo(pmd, qq);
        lfo_change(pmd, qq);
    }
}


static void lfoinit_main(PMDWIN *pmd, QQ *qq)
{
    qq->lfodat = 0;
    qq->delay = qq->delay2;
    qq->speed = qq->speed2;
    qq->step = qq->step2;
    qq->time = qq->time2;
    qq->mdc = qq->mdc2;

    if(qq->lfo_wave == 2 || qq->lfo_wave == 3) {    // 矩形波 or ランダム波？
        qq->speed = 1;  // delay直後にLFOが掛かるようにする
    } else {
        qq->speed++;    // それ以外の場合はdelay直後のspeed値を +1
    }
}


//=============================================================================
//  LFO ﾊﾟﾗﾒｰﾀ ｾｯﾄ
//=============================================================================
static uint8_t *lfoset(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    qq->delay = *si;
    qq->delay2 = *si++;
    qq->speed = *si;
    qq->speed2 = *si++;
    qq->step = *(char *)si;
    qq->step2 = *(char *)si++;
    qq->time = *si;
    qq->time2 = *si++;
    lfoinit_main(pmd, qq);
    return si;
}


//=============================================================================
//  LFO SWITCH
//=============================================================================
static uint8_t *lfoswitch(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int al;

    al = *si++;

    if(al & 0xf8) {
        al = 1;
    }
    al &= 7;
    qq->lfoswi = (qq->lfoswi & 0xf8) | al;
    lfoinit_main(pmd, qq);
    return si;
}


static uint8_t *_lfoswitch(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    qq->lfoswi = (qq->lfoswi & 0x8f) | ((*si++ & 7) << 4);
    lfo_change(pmd, qq);
    lfoinit_main(pmd, qq);
    lfo_change(pmd, qq);
    return si;
}


//=============================================================================
//  ＬＦＯ初期化
//=============================================================================
static void lfin1(PMDWIN *pmd, QQ *qq)
{
    qq->hldelay_c = qq->hldelay;

    if(qq->hldelay) {
        OPNASetReg(&pmd->opna, pmd->open_work.fmsel + pmd->open_work.partb+0xb4-1, (qq->fmpan) & 0xc0);
    }

    qq->sdelay_c = qq->sdelay;

    if(qq->lfoswi & 3) {    // LFOは未使用
        if((qq->lfoswi & 4) == 0) { //keyon非同期か?
            lfoinit_main(pmd, qq);
        }
        lfo(pmd, qq);
    }

    if(qq->lfoswi & 0x30) { // LFOは未使用
        if((qq->lfoswi & 0x40) == 0) {  //keyon非同期か?
            lfo_change(pmd, qq);
            lfoinit_main(pmd, qq);
            lfo_change(pmd, qq);
        }

        lfo_change(pmd, qq);
        lfo(pmd, qq);
        lfo_change(pmd, qq);
    }
}


//=============================================================================
//  ＬＦＯとＰＳＧのソフトウエアエンベロープの初期化
//=============================================================================
//=============================================================================
//  ＦＭ音源用　Entry
//=============================================================================
static void lfoinit(PMDWIN *pmd, QQ *qq, int al)
{
    int ah = al & 0x0f;
    if(ah == 0x0c) {
        al = qq->onkai_def;
        ah = al & 0x0f;
    }

    qq->onkai_def = al;
    if(ah == 0x0f) {                // ｷｭｰﾌ ﾉ ﾄｷ ﾊ INIT ｼﾅｲﾖ
        lfo_exit(pmd, qq);
        return;
    }

    qq->porta_num = 0;              // ポルタメントは初期化
    if((pmd->open_work.tieflag & 1) == 0) {
        lfin1(pmd, qq);
    } else {
        lfo_exit(pmd, qq);
    }
}

//=============================================================================
//  ＰＳＧ音源用　Entry
//=============================================================================
static void lfoinitp(PMDWIN *pmd, QQ *qq, int al)
{
    int ah = al & 0x0f;
    if(ah == 0x0c) {
        al = qq->onkai_def;
        ah = al & 0x0f;
    }

    qq->onkai_def = al;
    if(ah == 0x0f) {        // ｷｭｰﾌ ﾉ ﾄｷ ﾊ INIT ｼﾅｲﾖ
        lfo_exit(pmd, qq);
        return;
    }

    qq->porta_num = 0;              // ポルタメントは初期化
    if(pmd->open_work.tieflag & 1) {
        lfo_exit(pmd, qq);
        return;
    }

    //------------------------------------------------------------------------
    //  ソフトウエアエンベロープ初期化
    //------------------------------------------------------------------------
    if(qq->envf != -1) {
        qq->envf = 0;
        qq->eenv_volume = 0;
        qq->eenv_ar = qq->eenv_arc;

        if(qq->eenv_ar == 0) {
            qq->envf = 1;   // ATTACK=0 ... ｽｸﾞ Decay ﾆ
            qq->eenv_volume = qq->eenv_dr;
        }

        qq->eenv_sr = qq->eenv_src;
        qq->eenv_rr = qq->eenv_rrc;
        lfin1(pmd, qq);
    } else {
        //  拡張ssg_envelope用
        qq->eenv_arc = qq->eenv_ar - 16;
        if(qq->eenv_dr < 16) {
            qq->eenv_drc = (qq->eenv_dr-16)*2;
        } else {
            qq->eenv_drc = qq->eenv_dr-16;
        }

        if(qq->eenv_sr < 16) {
            qq->eenv_src = (qq->eenv_sr-16)*2;
        } else {
            qq->eenv_src = qq->eenv_sr-16;
        }

        qq->eenv_rrc = (qq->eenv_rr) * 2 - 16;
        qq->eenv_volume = qq->eenv_al;
        qq->eenv_count = 1;
        if(qq->eenv_arc > 0) {
            qq->eenv_volume += qq->eenv_arc;
            if(qq->eenv_volume < 15) {
                qq->eenv_arc = qq->eenv_ar-16;
            } else {
                qq->eenv_volume = 15;
                qq->eenv_count++;
                if(qq->eenv_sl == 15) qq->eenv_count++;       // SL=0の場合はすぐSRに
            }
        } else {
            if(qq->eenv_ar != 0) qq->eenv_arc++;
        }
        lfin1(pmd, qq);
    }
}


//=============================================================================
//  ポルタメント計算なのね
//=============================================================================
void porta_calc(QQ *qq)
{
    qq->porta_num += qq->porta_num2;
    if(qq->porta_num3 == 0) return;
    if(qq->porta_num3 > 0) {
        qq->porta_num3--;
        qq->porta_num++;
    } else {
        qq->porta_num3++;
        qq->porta_num--;
    }
}


//=============================================================================
//  ポルタメント(FM)
//=============================================================================
static uint8_t *porta(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     ax, cx, cl, bx, bh;

    if(qq->partmask) {
        qq->fnum = 0;       //休符に設定
        qq->onkai = 255;
        qq->leng = *(si+2);
        qq->keyon_flag++;
        qq->address = si+3;

        if(--pmd->open_work.volpush_flag) {
            qq->volpush = 0;
        }

        pmd->open_work.tieflag = 0;
        pmd->open_work.volpush_flag = 0;
        pmd->open_work.loop_work &= qq->loopcheck;

        return si+3;        // 読み飛ばす   (Mask時)
    }

    lfoinit(pmd, qq, *si);
    fnumset(pmd, qq, oshift(pmd, qq, *si++));

    cx = qq->fnum;
    cl = qq->onkai;
    fnumset(pmd, qq, oshift(pmd, qq, *si++));
    bx = qq->fnum;          // bx=ポルタメント先のfnum値
    qq->onkai = cl;
    qq->fnum = cx;          // cx=ポルタメント元のfnum値

    bh = (int)((bx / 256) & 0x38) - ((cx / 256) & 0x38);    // 先のoctarb - 元のoctarb
    if(bh) {
        bh /= 8;
        ax = bh * 0x26a;            // ax = 26ah * octarb差
    } else {
        ax = 0;
    }

    bx  = (bx & 0x7ff) - (cx & 0x7ff);
    ax += bx;               // ax=26ah*octarb差 + 音程差

    qq->leng = *si++;
    si = calc_q(pmd, qq, si);

    qq->porta_num2 = ax / qq->leng; // 商
    qq->porta_num3 = ax % qq->leng; // 余り
    qq->lfoswi |= 8;                // Porta ON

    if(qq->volpush && qq->onkai != 255) {
        if(--pmd->open_work.volpush_flag) {
            pmd->open_work.volpush_flag = 0;
            qq->volpush = 0;
        }
    }

    volset(pmd, qq);
    otodasi(pmd, qq);
    keyon(pmd, qq);

    qq->keyon_flag++;
    qq->address = si;

    pmd->open_work.tieflag = 0;
    pmd->open_work.volpush_flag = 0;

    if(*si == 0xfb) {       // '&'が直後にあったらkeyoffしない
        qq->keyoff_flag = 2;
    } else {
        qq->keyoff_flag = 0;
    }
    pmd->open_work.loop_work &= qq->loopcheck;
    return si;
}


//=============================================================================
//  ポルタメント(PSG)
//=============================================================================
static uint8_t *portap(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     ax, al_, bx_;

    if(qq->partmask) {
        qq->fnum = 0;       //休符に設定
        qq->onkai = 255;
        qq->leng = *(si+2);
        qq->keyon_flag++;
        qq->address = si+3;

        if(--pmd->open_work.volpush_flag) {
            qq->volpush = 0;
        }

        pmd->open_work.tieflag = 0;
        pmd->open_work.volpush_flag = 0;
        pmd->open_work.loop_work &= qq->loopcheck;
        return si+3;        // 読み飛ばす   (Mask時)
    }

    lfoinitp(pmd, qq, *si);
    fnumsetp(pmd, qq, oshiftp(pmd, qq, *si++));

    bx_ = qq->fnum;
    al_ = qq->onkai;
    fnumsetp(pmd, qq, oshiftp(pmd, qq, *si++));
    ax = qq->fnum;          // ax = ポルタメント先のpsg_tune値

    qq->onkai = al_;
    qq->fnum = bx_;         // bx = ポルタメント元のpsg_tune値
    ax -= bx_;

    qq->leng = *si++;
    si = calc_q(pmd, qq, si);

    qq->porta_num2 = ax / qq->leng;     // 商
    qq->porta_num3 = ax % qq->leng;     // 余り
    qq->lfoswi |= 8;                // Porta ON

    if(qq->volpush && qq->onkai != 255) {
        if(--pmd->open_work.volpush_flag) {
            pmd->open_work.volpush_flag = 0;
            qq->volpush = 0;
        }
    }

    volsetp(pmd, qq);
    otodasip(pmd, qq);
    keyonp(pmd, qq);

    qq->keyon_flag++;
    qq->address = si;

    pmd->open_work.tieflag = 0;
    pmd->open_work.volpush_flag = 0;
    qq->keyoff_flag = 0;

    if(*si == 0xfb) {           // '&'が直後にあったらkeyoffしない
        qq->keyoff_flag = 2;
    }

    pmd->open_work.loop_work &= qq->loopcheck;
    return si;
}


//=============================================================================
//  各種特殊コマンド処理
//=============================================================================
uint8_t * commands(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     al;

    al = *si++;
    switch(al) {
        case 0xff : si = comat(pmd, qq, si); break;
        case 0xfe : qq->qdata = *si++; qq->qdat3 = 0; break;
        case 0xfd : qq->volume = *si++; break;
        case 0xfc : si = comt(pmd, si); break;

        case 0xfb : pmd->open_work.tieflag |= 1; break;
        case 0xfa : qq->detune = *(short *)si; si+=2; break;
        case 0xf9 : si = comstloop(pmd, qq, si); break;
        case 0xf8 : si = comedloop(pmd, qq, si); break;
        case 0xf7 : si = comexloop(pmd, qq, si); break;
        case 0xf6 : qq->partloop = si; break;
        case 0xf5 : qq->shift = *(char *)si++; break;
        case 0xf4 : if((qq->volume+=4) > 127) qq->volume = 127; break;
        case 0xf3 : if(qq->volume < 4) qq->volume=0; else qq->volume-=4; break;
        case 0xf2 : si = lfoset(pmd, qq, si); break;
        case 0xf1 : si = lfoswitch(pmd, qq, si); ch3_setting(pmd, qq); break;
        case 0xf0 : si+=4; break;

        case 0xef : OPNASetReg(&pmd->opna, pmd->open_work.fmsel + *si, *(si+1)); si+=2; break;
        case 0xee : si++; break;
        case 0xed : si++; break;
        case 0xec : si = panset(pmd, qq, si); break;             // FOR SB2
        case 0xeb : si = rhykey(pmd, si); break;
        case 0xea : si = rhyvs(pmd, si); break;
        case 0xe9 : si = rpnset(pmd, si); break;
        case 0xe8 : si = rmsvs(pmd, si); break;
        //
        case 0xe7 : qq->shift += *(char *)si++; break;
        case 0xe6 : si = rmsvs_sft(pmd, si); break;
        case 0xe5 : si = rhyvs_sft(pmd, si); break;
        //
        case 0xe4 : qq->hldelay = *si++; break;
        //追加 for V2.3
        case 0xe3 : if((qq->volume += *si++) > 127) qq->volume = 127; break;
        case 0xe2 :
            if(qq->volume < *si) qq->volume = 0; else qq->volume -= *si;
            si++;
            break;
        //
        case 0xe1 : si = hlfo_set(pmd, qq, si); break;
        case 0xe0 : pmd->open_work.port22h = *si; OPNASetReg(&pmd->opna, 0x22, *si++); break;
        //
        case 0xdf : pmd->open_work.syousetu_lng = *si++; break;
        //
        case 0xde : si = vol_one_up_fm(pmd, qq, si); break;
        case 0xdd : si = vol_one_down(pmd, qq, si); break;
        //
        case 0xdc : pmd->open_work.status = *si++; break;
        case 0xdb : pmd->open_work.status += *si++; break;
        //
        case 0xda : si = porta(pmd, qq, si); break;
        //
        case 0xd9 : si++; break;
        case 0xd8 : si++; break;
        case 0xd7 : si++; break;
        //
        case 0xd6 : qq->mdspd = qq->mdspd2 = *si++; qq->mdepth = *(char *)si++; break;
        case 0xd5 : qq->detune += *(short *)si; si+=2; break;
        //
        case 0xd4 : si++; break;
        case 0xd3 : si++; break;
        case 0xd2 :
            si++;
            break;
        //
        case 0xd1 : si++; break;
        case 0xd0 : si++; break;
        //
        case 0xcf : si = slotmask_set(pmd, qq, si); break;
        case 0xce : si+=6; break;
        case 0xcd : si+=5; break;
        case 0xcc : si++; break;
        case 0xcb : qq->lfo_wave = *si++; break;
        case 0xca :
            qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
            break;

        case 0xc9 : si++; break;
        case 0xc8 : si = slotdetune_set(pmd, qq, si); break;
        case 0xc7 : si = slotdetune_set2(pmd, qq, si); break;
        case 0xc6 : break;
        case 0xc5 : si = volmask_set(pmd, qq, si); break;
        case 0xc4 : qq->qdatb = *si++; break;
        case 0xc3 : si = panset_ex(pmd, qq, si); break;
        case 0xc2 : qq->delay = qq->delay2 = *si++; lfoinit_main(pmd, qq); break;
        case 0xc1 : break;
        case 0xc0 : si = fm_mml_part_mask(pmd, qq, si); break;
        case 0xbf : lfo_change(pmd, qq); si = lfoset(pmd, qq, si); lfo_change(pmd, qq);break;
        case 0xbe : si = _lfoswitch(pmd, qq, si); ch3_setting(pmd, qq); break;
        case 0xbd :
            lfo_change(pmd, qq);
            qq->mdspd = qq->mdspd2 = *si++;
            qq->mdepth = *(char *)si++;
            lfo_change(pmd, qq);
            break;

        case 0xbc : lfo_change(pmd, qq); qq->lfo_wave=*si++; lfo_change(pmd, qq); break;
        case 0xbb :
            lfo_change(pmd, qq);
            qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
            lfo_change(pmd, qq);
            break;

        case 0xba : si = _volmask_set(pmd, qq, si); break;
        case 0xb9 :
            lfo_change(pmd, qq);
            qq->delay = qq->delay2 = *si++; lfoinit_main(pmd, qq);
            lfo_change(pmd, qq);
            break;

        case 0xb8 : si = tl_set(pmd, qq, si); break;
        case 0xb7 : si = mdepth_count(qq, si); break;
        case 0xb6 : si = fb_set(pmd, qq, si); break;
        case 0xb5 :
            qq->sdelay_m = (~(*si++) << 4) & 0xf0;
            qq->sdelay_c = qq->sdelay = *si++;
            break;

        case 0xb4 : si+=16; break;
        case 0xb3 : qq->qdat2 = *si++; break;
        case 0xb2 : qq->shift_def = *(char *)si++; break;
        case 0xb1 : qq->qdat3 = *si++; break;

        default :
            si--;
            *si = 0x80;
    }

    return si;
}


//=============================================================================
//  ＦＭ音源演奏メイン
//=============================================================================
void fmmain(PMDWIN *pmd, QQ *qq)
{
    uint8_t   *si;

    if(qq->address == NULL) return;

    si = qq->address;

    qq->leng--;

    if(qq->partmask) {
        qq->keyoff_flag = -1;
    } else {
        // KEYOFF CHECK & Keyoff
        if((qq->keyoff_flag & 3) == 0) {        // 既にkeyoffしたか？
            if(qq->leng <= qq->qdat) {
                keyoff(pmd, qq);
                qq->keyoff_flag = -1;
            }
        }
    }

    // LENGTH CHECK
    if(qq->leng == 0) {
        if(qq->partmask == 0) qq->lfoswi &= 0xf7;

        while(1) {
            if(*si > 0x80 && *si != 0xda) {
                si = commands(pmd, qq, si);
            } else if(*si == 0x80) {
                qq->address = si;
                qq->loopcheck = 3;
                qq->onkai = 255;
                if(qq->partloop == NULL) {
                    if(qq->partmask) {
                        pmd->open_work.tieflag = 0;
                        pmd->open_work.volpush_flag = 0;
                        pmd->open_work.loop_work &= qq->loopcheck;
                        return;
                    } else {
                        break;
                    }
                }
                // "L"があった時
                si = qq->partloop;
                qq->loopcheck = 1;
            } else {
                if(*si == 0xda) {       // ポルタメント
                    si = porta(pmd, qq, ++si);
                    pmd->open_work.loop_work &= qq->loopcheck;
                    return;
                } else if(qq->partmask == 0) {
                    //  TONE SET
                    lfoinit(pmd, qq, *si);
                    fnumset(pmd, qq, oshift(pmd, qq, *si++));

                    qq->leng = *si++;
                    si = calc_q(pmd, qq, si);

                    if(qq->volpush && qq->onkai != 255) {
                        if(--pmd->open_work.volpush_flag) {
                            pmd->open_work.volpush_flag = 0;
                            qq->volpush = 0;
                        }
                    }

                    volset(pmd, qq);
                    otodasi(pmd, qq);
                    keyon(pmd, qq);

                    qq->keyon_flag++;
                    qq->address = si;

                    pmd->open_work.tieflag = 0;
                    pmd->open_work.volpush_flag = 0;

                    if(*si == 0xfb) {       // '&'が直後にあったらkeyoffしない
                        qq->keyoff_flag = 2;
                    } else {
                        qq->keyoff_flag = 0;
                    }
                    pmd->open_work.loop_work &= qq->loopcheck;
                    return;
                } else {
                    si++;
                    qq->fnum = 0;       //休符に設定
                    qq->onkai = 255;
                    qq->onkai_def = 255;
                    qq->leng = *si++;
                    qq->keyon_flag++;
                    qq->address = si;

                    if(--pmd->open_work.volpush_flag) {
                        qq->volpush = 0;
                    }

                    pmd->open_work.tieflag = 0;
                    pmd->open_work.volpush_flag = 0;
                    break;
                }
            }
        }
    }

    if(qq->partmask == 0) {
        // LFO & Portament & Fadeout 処理 をして終了
        if(qq->hldelay_c) {
            if(--qq->hldelay_c == 0) {
                OPNASetReg(&pmd->opna, pmd->open_work.fmsel +
                        (pmd->open_work.partb - 1 + 0xb4), qq->fmpan);
            }
        }

        if(qq->sdelay_c) {
            if(--qq->sdelay_c == 0) {
                if((qq->keyoff_flag & 1) == 0) {    // 既にkeyoffしたか？
                    keyon(pmd, qq);
                }
            }
        }

        if(qq->lfoswi) {
            pmd->open_work.lfo_switch = qq->lfoswi & 8;
            if(qq->lfoswi & 3) {
                if(lfo(pmd, qq)) {
                    pmd->open_work.lfo_switch  |= (qq->lfoswi & 3);
                }
            }

            if(qq->lfoswi & 0x30) {
                lfo_change(pmd, qq);
                if(lfo(pmd, qq)) {
                    lfo_change(pmd, qq);
                    pmd->open_work.lfo_switch |= (qq->lfoswi & 0x30);
                } else {
                    lfo_change(pmd, qq);
                }
            }

            if(pmd->open_work.lfo_switch & 0x19) {
                if(pmd->open_work.lfo_switch & 8) {
                    porta_calc(qq);

                }
                otodasi(pmd, qq);
            }

            if(pmd->open_work.lfo_switch & 0x22) {
                volset(pmd, qq);
                pmd->open_work.loop_work &= qq->loopcheck;
                return;
            }
        }
    }

    pmd->open_work.loop_work &= qq->loopcheck;
}


uint8_t * commandsp(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     al;

    al = *si++;
    switch(al) {
        case 0xff : si++; break;
        case 0xfe : qq->qdata = *si++; qq->qdat3 = 0; break;
        case 0xfd : qq->volume = *si++; break;
        case 0xfc : si = comt(pmd, si); break;

        case 0xfb : pmd->open_work.tieflag |= 1; break;
        case 0xfa : qq->detune = *(short *)si; si+=2; break;
        case 0xf9 : si = comstloop(pmd, qq, si); break;
        case 0xf8 : si = comedloop(pmd, qq, si); break;
        case 0xf7 : si = comexloop(pmd, qq, si); break;
        case 0xf6 : qq->partloop = si; break;
        case 0xf5 : qq->shift = *(char *)si++; break;
        case 0xf4 : if(qq->volume < 15) qq->volume++; break;
        case 0xf3 : if(qq->volume > 0) qq->volume--; break;
        case 0xf2 : si = lfoset(pmd, qq, si); break;
        case 0xf1 : si = lfoswitch(pmd, qq, si); break;
        case 0xf0 : si = psgenvset(qq, si); break;

        case 0xef : OPNASetReg(&pmd->opna, *si, *(si+1)); si+=2; break;
        case 0xee : pmd->open_work.psnoi = *si++; break;
        case 0xed : qq->psgpat = *si++; break;
        //
        case 0xec : si++; break;
        case 0xeb : si = rhykey(pmd, si); break;
        case 0xea : si = rhyvs(pmd, si); break;
        case 0xe9 : si = rpnset(pmd, si); break;
        case 0xe8 : si = rmsvs(pmd, si); break;
        //
        case 0xe7 : qq->shift += *(char *)si++; break;
        case 0xe6 : si = rmsvs_sft(pmd, si); break;
        case 0xe5 : si = rhyvs_sft(pmd, si); break;
        //
        case 0xe4 : si++; break;
        //追加 for V2.3
        case 0xe3 : if((qq->volume + *si) < 16) qq->volume += *si; si++;break;
        case 0xe2 : if((qq->volume - *si) >= 0) qq->volume -= *si; si++;break;
        //
        case 0xe1 : si++; break;
        case 0xe0 : si++; break;
        //
        case 0xdf : pmd->open_work.syousetu_lng = *si++; break;
        //
        case 0xde : si = vol_one_up_psg(pmd, qq, si); break;
        case 0xdd : si = vol_one_down(pmd, qq, si); break;
        //
        case 0xdc : pmd->open_work.status = *si++; break;
        case 0xdb : pmd->open_work.status += *si++; break;
        //
        case 0xda : si = portap(pmd, qq, si); break;
        //
        case 0xd9 : si++; break;
        case 0xd8 : si++; break;
        case 0xd7 : si++; break;
        //
        case 0xd6 : qq->mdspd = qq->mdspd2 = *si++; qq->mdepth = *(char *)si++; break;
        case 0xd5 : qq->detune += *(short *)si; si+=2; break;
        //
        case 0xd4 : si++; break;
        case 0xd3 : si++; break;
        case 0xd2 :
            si++;
            break;
        //
        case 0xd1 : si++; break;
        case 0xd0 : si = psgnoise_move(pmd, si); break;
        //
        case 0xcf : si++; break;
        case 0xce : si+=6; break;
        case 0xcd : si = extend_psgenvset(qq, si); break;
        case 0xcc :
            qq->extendmode = (qq->extendmode & 0xfe) | (*si++ & 1);
            break;

        case 0xcb : qq->lfo_wave = *si++; break;
        case 0xca :
            qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
            break;

        case 0xc9 :
            qq->extendmode = (qq->extendmode & 0xfb) | ((*si++ & 1) << 2);
            break;

        case 0xc8 : si+=3; break;
        case 0xc7 : si+=3; break;
        case 0xc6 : si+=6; break;
        case 0xc5 : si++; break;
        case 0xc4 : qq->qdatb = *si++; break;
        case 0xc3 : si+=2; break;
        case 0xc2 : qq->delay = qq->delay2 = *si++; lfoinit_main(pmd, qq); break;
        case 0xc1 : break;
        case 0xc0 : si = ssg_mml_part_mask(pmd, qq, si); break;
        case 0xbf : lfo_change(pmd, qq); si = lfoset(pmd, qq, si); lfo_change(pmd, qq);break;
        case 0xbe :
            qq->lfoswi = (qq->lfoswi & 0x8f) | ((*si++ & 7) << 4);
            lfo_change(pmd, qq); lfoinit_main(pmd, qq); lfo_change(pmd, qq);
            break;

        case 0xbd :
            lfo_change(pmd, qq);
            qq->mdspd = qq->mdspd2 = *si++;
            qq->mdepth = *(char *)si++;
            lfo_change(pmd, qq);
            break;

        case 0xbc : lfo_change(pmd, qq); qq->lfo_wave=*si++; lfo_change(pmd, qq); break;
        case 0xbb :
            lfo_change(pmd, qq);
            qq->extendmode = (qq->extendmode & 0xfd) | ((*si++ & 1) << 1);
            lfo_change(pmd, qq);
            break;

        case 0xba : si++; break;
        case 0xb9 :
            lfo_change(pmd, qq);
            qq->delay = qq->delay2 = *si++; lfoinit_main(pmd, qq); break;
            lfo_change(pmd, qq);
            break;

        case 0xb8 : si+=2; break;
        case 0xb7 : si = mdepth_count(qq, si); break;
        case 0xb6 : si++; break;
        case 0xb5 : si+= 2; break;
        case 0xb4 : si+=16; break;
        case 0xb3 : qq->qdat2 = *si++; break;
        case 0xb2 : qq->shift_def = *(char *)si++; break;
        case 0xb1 : qq->qdat3 = *si++; break;

        default :
            si--;
            *si = 0x80;
    }

    return si;
}


//=============================================================================
//  SSGドラムを消してSSGを復活させるかどうかcheck
//      input   AL <- Command
//      output  cy=1 : 復活させる
//=============================================================================
int ssgdrum_check(PMDWIN *pmd, QQ *qq, int al)
{
    // SSGマスク中はドラムを止めない
    // SSGドラムは鳴ってない
    if((qq->partmask & 1) || ((qq->partmask & 2) == 0)) return 0;

    // 普通の効果音は消さない
    if(pmd->effwork.effon >= 2) return 0;

    al = (al & 0x0f);

    // 休符の時はドラムは止めない
    if(al == 0x0f) return 0;

    // SSGドラムはまだ再生中か？
    if(pmd->effwork.effon == 1) {
        effend(&pmd->effwork, &pmd->opna);           // SSGドラムを消す
    }

    if((qq->partmask &= 0xfd) == 0) return -1;
    return 0;
}


//=============================================================================
//  ＳＳＧ音源　演奏　メイン
//=============================================================================
int soft_env(PMDWIN *pmd, QQ *qq);
void psgmain(PMDWIN *pmd, QQ *qq)
{
    uint8_t   *si;
    int temp;

    if(qq->address == NULL) return;
    si = qq->address;

    qq->leng--;

    // KEYOFF CHECK & Keyoff
    if(qq->partmask) {
        qq->keyoff_flag = -1;
    } else {
        if((qq->keyoff_flag & 3) == 0) {        // 既にkeyoffしたか？
            if(qq->leng <= qq->qdat) {
                keyoffp(pmd, qq);
                qq->keyoff_flag = -1;
            }
        }
    }

    // LENGTH CHECK
    if(qq->leng == 0) {
        qq->lfoswi &= 0xf7;

        // DATA READ
        while(1) {
            if(*si == 0xda && ssgdrum_check(pmd, qq, *si) < 0) {
                si++;
            } else if(*si > 0x80 && *si != 0xda) {
                si = commandsp(pmd, qq, si);
            } else if(*si == 0x80) {
                qq->address = si;
                qq->loopcheck = 3;
                qq->onkai = 255;
                if(qq->partloop == NULL) {
                    if(qq->partmask) {
                        pmd->open_work.tieflag = 0;
                        pmd->open_work.volpush_flag = 0;
                        pmd->open_work.loop_work &= qq->loopcheck;
                        return;
                    } else {
                        break;
                    }
                }
                // "L"があった時
                si = qq->partloop;
                qq->loopcheck = 1;
            } else {
                if(*si == 0xda) {                       // ポルタメント
                    si = portap(pmd, qq, ++si);
                    pmd->open_work.loop_work &= qq->loopcheck;
                    return;
                } else if(qq->partmask) {
                    if(ssgdrum_check(pmd, qq, *si) == 0) {
                        si++;
                        qq->fnum = 0;       //休符に設定
                        qq->onkai = 255;
                        qq->leng = *si++;
                        qq->keyon_flag++;
                        qq->address = si;

                        if(--pmd->open_work.volpush_flag) {
                            qq->volpush = 0;
                        }

                        pmd->open_work.tieflag = 0;
                        pmd->open_work.volpush_flag = 0;
                        break;
                    }
                }

                //  TONE SET
                lfoinitp(pmd, qq, *si);
                fnumsetp(pmd, qq, oshiftp(pmd, qq, *si++));

                qq->leng = *si++;
                si = calc_q(pmd, qq, si);

                if(qq->volpush && qq->onkai != 255) {
                    if(--pmd->open_work.volpush_flag) {
                        pmd->open_work.volpush_flag = 0;
                        qq->volpush = 0;
                    }
                }

                volsetp(pmd, qq);
                otodasip(pmd, qq);
                keyonp(pmd, qq);

                qq->keyon_flag++;
                qq->address = si;

                pmd->open_work.tieflag = 0;
                pmd->open_work.volpush_flag = 0;
                qq->keyoff_flag = 0;

                if(*si == 0xfb) {       // '&'が直後にあったらkeyoffしない
                    qq->keyoff_flag = 2;
                }
                pmd->open_work.loop_work &= qq->loopcheck;
                return;
            }
        }
    }

    pmd->open_work.lfo_switch = (qq->lfoswi & 8);

    if(qq->lfoswi) {
        if(qq->lfoswi & 3) {
            if(lfo(pmd, qq)) {
                pmd->open_work.lfo_switch |= (qq->lfoswi & 3);
            }
        }

        if(qq->lfoswi & 0x30) {
            lfo_change(pmd, qq);
            if(lfo(pmd, qq)) {
                lfo_change(pmd, qq);
                pmd->open_work.lfo_switch |= (qq->lfoswi & 0x30);
            } else {
                lfo_change(pmd, qq);
            }
        }

        if(pmd->open_work.lfo_switch & 0x19) {
            if(pmd->open_work.lfo_switch & 0x08) {
                porta_calc(qq);
            }

            // SSG 3ch で休符かつ SSG Drum 発音中は操作しない
            if(!(qq == &pmd->SSGPart[2] && qq->onkai == 255 && pmd->open_work.kshot_dat)) {
                otodasip(pmd, qq);
            }
        }
    }

    temp = soft_env(pmd, qq);
    if(temp || pmd->open_work.lfo_switch & 0x22) {
        // SSG 3ch で休符かつ SSG Drum 発音中は volume set しない
        if(!(qq == &pmd->SSGPart[2] && qq->onkai == 255 && pmd->open_work.kshot_dat)) {
            volsetp(pmd, qq);
        }
    }

    pmd->open_work.loop_work &= qq->loopcheck;
}


uint8_t * commandsr(PMDWIN *pmd, QQ *qq, uint8_t *si)
{
    int     al;

    al = *si++;
    switch(al) {
        case 0xff : si++; break;
        case 0xfe : si++; break;
        case 0xfd : qq->volume = *si++; break;
        case 0xfc : si = comt(pmd, si); break;

        case 0xfb : pmd->open_work.tieflag |= 1; break;
        case 0xfa : qq->detune = *(short *)si; si+=2; break;
        case 0xf9 : si = comstloop(pmd, qq, si); break;
        case 0xf8 : si = comedloop(pmd, qq, si); break;
        case 0xf7 : si = comexloop(pmd, qq, si); break;
        case 0xf6 : qq->partloop = si; break;
        case 0xf5 : si++; break;
        case 0xf4 : if(qq->volume < 15) qq->volume++; break;
        case 0xf3 : if(qq->volume > 0) qq->volume--; break;
        case 0xf2 : si+=4; break;
        case 0xf1 : si++; break;
        case 0xf0 : si+=4; break;

        case 0xef : OPNASetReg(&pmd->opna, *si, *(si+1)); si+=2; break;
        case 0xee : si++; break;
        case 0xed : si++; break;
        //
        case 0xec : si++; break;
        case 0xeb : si = rhykey(pmd, si); break;
        case 0xea : si = rhyvs(pmd, si); break;
        case 0xe9 : si = rpnset(pmd, si); break;
        case 0xe8 : si = rmsvs(pmd, si); break;
        //
        case 0xe7 : si++; break;
        case 0xe6 : si = rmsvs_sft(pmd, si); break;
        case 0xe5 : si = rhyvs_sft(pmd, si); break;
        //
        case 0xe4 : si++; break;
        //追加 for V2.3
        case 0xe3 : if((qq->volume + *si) < 16) qq->volume += *si; si++;break;
        case 0xe2 : if((qq->volume - *si) >= 0) qq->volume -= *si; si++;break;
        //
        case 0xe1 : si++; break;
        case 0xe0 : si++; break;
        //
        case 0xdf : pmd->open_work.syousetu_lng = *si++; break;
        //
        case 0xde : si = vol_one_up_psg(pmd, qq, si); break;
        case 0xdd : si = vol_one_down(pmd, qq, si); break;
        //
        case 0xdc : pmd->open_work.status = *si++; break;
        case 0xdb : pmd->open_work.status += *si++; break;
        //
        case 0xda : si++; break;
        //
        case 0xd9 : si++; break;
        case 0xd8 : si++; break;
        case 0xd7 : si++; break;
        //
        case 0xd6 : si+=2; break;
        case 0xd5 : qq->detune += *(short *)si; si+=2; break;
        //
        case 0xd4 : si++; break;
        case 0xd3 : si++; break;
        case 0xd2 :
            si++;
            break;
        //
        case 0xd1 : si++; break;
        case 0xd0 : si++; break;
        //
        case 0xcf : si++; break;
        case 0xce : si+=6; break;
        case 0xcd : si+=5; break;
        case 0xcc : si++; break;
        case 0xcb : si++; break;
        case 0xca : si++; break;
        case 0xc9 : si++; break;
        case 0xc8 : si+=3; break;
        case 0xc7 : si+=3; break;
        case 0xc6 : si+=6; break;
        case 0xc5 : si++; break;
        case 0xc4 : si++; break;
        case 0xc3 : si+=2; break;
        case 0xc2 : si++; break;
        case 0xc1 : break;
        case 0xc0 : si = rhythm_mml_part_mask(pmd, qq, si); break;
        case 0xbf : si+=4; break;
        case 0xbe : si++; break;
        case 0xbd : si+=2; break;
        case 0xbc : si++; break;
        case 0xbb : si++; break;
        case 0xba : si++; break;
        case 0xb9 : si++; break;
        case 0xb8 : si+=2; break;
        case 0xb7 : si++; break;
        case 0xb6 : si++; break;
        case 0xb5 : si+= 2; break;
        case 0xb4 : si+=16; break;
        case 0xb3 : si++; break;
        case 0xb2 : si++; break;
        case 0xb1 : si++; break;

        default :
            si--;
            *si = 0x80;
    }

    return si;
}


//=============================================================================
//  PSGﾘｽﾞﾑ ON
//=============================================================================
uint8_t * rhythmon(PMDWIN *pmd, QQ *qq, uint8_t *bx, int al, int *result)
{
    int     cl, dl, bx_;

    if(al & 0x40) {
        bx = commandsr(pmd, qq, bx-1);
        *result = 0;
        return bx;
    }

    *result = 1;

    if(qq->partmask) {      // maskされている場合
        pmd->open_work.kshot_dat = 0;
        return ++bx;
    }

    al = ((al << 8) + *bx++) & 0x3fff;
    pmd->open_work.kshot_dat = al;
    if(al == 0) return bx;
    pmd->open_work.rhyadr = bx;

    for(cl = 0; cl < 11; cl++) {
        if(al & (1 << cl)) {
            OPNASetReg(&pmd->opna, rhydat[cl][0], rhydat[cl][1]);
            if((dl = (rhydat[cl][2] & pmd->open_work.rhythmmask))) {
                if(dl < 0x80) {
                    OPNASetReg(&pmd->opna, 0x10, dl);
                } else {
                    OPNASetReg(&pmd->opna, 0x10, 0x84);
                    dl = pmd->open_work.rhythmmask & 8;
                    if(dl) {
                        OPNASetReg(&pmd->opna, 0x10, dl);
                    }
                }
            }
        }
    }

    bx_ = al;
    al = 0;
    do {
        while((bx_ & 1) == 0) {
            bx_ >>= 1;
            al++;
        }
        pmd->pmdstatus[8] = al;
        eff_main(&pmd->effwork, &pmd->opna, &(pmd->SSGPart[2].partmask), al);
        bx_ >>= 1;
    }while(pmd->open_work.ppsdrv_flag && bx_); // PPSDRVなら２音目以上も鳴らしてみる

    return pmd->open_work.rhyadr;
}


//=============================================================================
//  リズムパート　演奏　メイン
//=============================================================================
void rhythmmain(PMDWIN *pmd, QQ *qq)
{
    uint8_t   *si, *bx;
    int     al, result;

    if(qq->address == NULL) return;

    si = qq->address;

    if(--qq->leng == 0) {
        bx = pmd->open_work.rhyadr;

        rhyms00:
        do {
            result = 1;
            al = *bx++;
            if(al != 0xff) {
                if(al & 0x80) {
                    bx = rhythmon(pmd, qq, bx, al, &result);
                    if(result == 0) continue;
                } else {
                    pmd->open_work.kshot_dat = 0;    //rest
                }

                al = *bx++;
                pmd->open_work.rhyadr = bx;
                qq->leng = al;
                qq->keyon_flag++;
                pmd->open_work.tieflag = 0;
                pmd->open_work.volpush_flag = 0;
                pmd->open_work.loop_work &= qq->loopcheck;
                return;
            }
        }while(result == 0);

        while(1) {
            while((al = *si++) != 0x80) {
                if(al < 0x80) {
                    qq->address = si;

//                  bx = (uint16_t *)&pmd->open_work.radtbl[al];
//                  bx = pmd->open_work.rhyadr = &pmd->open_work.mmlbuf[*bx];
                    bx = pmd->open_work.rhyadr = &pmd->open_work.mmlbuf[pmd->open_work.radtbl[al]];
                    goto rhyms00;
                }

                // al > 0x80
                si = commandsr(pmd, qq, si-1);
            }

            qq->address = --si;
            qq->loopcheck = 3;
            bx = qq->partloop;
            if(bx == NULL) {
                pmd->open_work.rhyadr = (uint8_t *)&pmd->open_work.rhydmy;
                pmd->open_work.tieflag = 0;
                pmd->open_work.volpush_flag = 0;
                pmd->open_work.loop_work &= qq->loopcheck;
                return;
            } else {        //  "L"があった時
                si = bx;
                qq->loopcheck = 1;
            }
        }
    }

    pmd->open_work.loop_work &= qq->loopcheck;
}


//=============================================================================
//  MUSIC PLAYER MAIN [FROM TIMER-B]
//=============================================================================
void mmain(PMDWIN *pmd)
{
    int     i;

    pmd->open_work.loop_work = 3;
    for(i = 0; i < 3; i++) {
        pmd->open_work.partb = i + 1;
        psgmain(pmd, &pmd->SSGPart[i]);
    }

    pmd->open_work.fmsel = 0x100;
    for(i = 0; i < 3; i++) {
        pmd->open_work.partb = i + 1;
        fmmain(pmd, &pmd->FMPart[i+3]);
    }

    pmd->open_work.fmsel = 0;
    for(i = 0; i < 3; i++) {
        pmd->open_work.partb = i + 1;
        fmmain(pmd, &pmd->FMPart[i]);
    }
    rhythmmain(pmd, &pmd->RhythmPart);

    for(i = 0; i < 8; i++) {
        pmd->open_work.partb = i;
    }
    if(pmd->open_work.loop_work == 0) return;

    for(i = 0; i < 6; i++) {
        if(pmd->FMPart[i].loopcheck != 3) pmd->FMPart[i].loopcheck = 0;
    }

    for(i = 0; i < 3; i++) {
        if(pmd->SSGPart[i].loopcheck != 3) pmd->SSGPart[i].loopcheck = 0;
    }
    if(pmd->RhythmPart.loopcheck != 3) pmd->RhythmPart.loopcheck = 0;

    if(pmd->open_work.loop_work != 3) {
        pmd->open_work.status2++;
        if(pmd->open_work.status2 == 255) pmd->open_work.status2 = 1;
    } else {
        pmd->open_work.status2 = -1;
    }
}



//=============================================================================
//  ＰＳＧのソフトウエアエンベロープ
//=============================================================================
int soft_env_sub(PMDWIN *pmd, QQ *qq)
{
    if(qq->envf == 0) {
        // Attack
        if(--qq->eenv_ar != 0) {
            return 0;
        }

        qq->envf = 1;
        qq->eenv_volume = qq->eenv_dr;
        return 1;
    }

    if(qq->envf != 2) {
        // Decay
        if(qq->eenv_sr == 0) return 0;  // ＤＲ＝０の時は減衰しない
        if(--qq->eenv_sr != 0) return 0;
        qq->eenv_sr = qq->eenv_src;
        qq->eenv_volume--;

        if(qq->eenv_volume >= -15 || qq->eenv_volume < 15) return 0;
        qq->eenv_volume = -15;
        return 0;
    }


    // Release
    if(qq->eenv_rr == 0) {              // ＲＲ＝０の時はすぐに音消し
        qq->eenv_volume = -15;
        return 0;
    }

    if(--qq->eenv_rr != 0) return 0;
    qq->eenv_rr = qq->eenv_rrc;
    qq->eenv_volume--;

    if(qq->eenv_volume >= -15 && qq->eenv_volume < 15) return 0;
    qq->eenv_volume = -15;
    return 0;
}


void esm_sub(PMDWIN *pmd, QQ *qq, int ah)
{
    if(--ah == 0) {
        //  [[[ Attack Rate ]]]
        if(qq->eenv_arc > 0) {
            qq->eenv_volume += qq->eenv_arc;
            if(qq->eenv_volume < 15) {
                qq->eenv_arc = qq->eenv_ar-16;
                return;
            }

            qq->eenv_volume = 15;
            qq->eenv_count++;
            if(qq->eenv_sl != 15) return;       // SL=0の場合はすぐSRに
            qq->eenv_count++;
            return;
        } else {
            if(qq->eenv_ar == 0) return;
            qq->eenv_arc++;
            return;
        }
    }

    if(--ah == 0) {
        //  [[[ Decay Rate ]]]
        if(qq->eenv_drc > 0) {  // 0以下の場合はカウントCHECK
            qq->eenv_volume -= qq->eenv_drc;
            if(qq->eenv_volume < 0 || qq->eenv_volume < qq->eenv_sl) {
                qq->eenv_volume = qq->eenv_sl;
                qq->eenv_count++;
                return;
            }

            if(qq->eenv_dr < 16) {
                qq->eenv_drc = (qq->eenv_dr - 16) * 2;
            } else {
                qq->eenv_drc = qq->eenv_dr - 16;
            }
            return;
        }

        if(qq->eenv_dr == 0) return;
        qq->eenv_drc++;
        return;
    }

    if(--ah == 0) {
        //  [[[ Sustain Rate ]]]
        if(qq->eenv_src > 0) {  // 0以下の場合はカウントCHECK
            if((qq->eenv_volume -= qq->eenv_src) < 0) {
                qq->eenv_volume = 0;
            }

            if(qq->eenv_sr < 16) {
                qq->eenv_src = (qq->eenv_sr - 16) * 2;
            } else {
                qq->eenv_src = qq->eenv_sr - 16;
            }
            return;
        }

        if(qq->eenv_sr == 0) return;    // SR=0?
        qq->eenv_src++;
        return;
    }

    //  [[[ Release Rate ]]]
    if(qq->eenv_rrc > 0) {  // 0以下の場合はカウントCHECK
        if((qq->eenv_volume -= qq->eenv_rrc) < 0) {
            qq->eenv_volume = 0;
        }

        qq->eenv_rrc = (qq->eenv_rr) * 2 - 16;
        return;
    }

    if(qq->eenv_rr == 0) return;
    qq->eenv_rrc++;
}


//  拡張版
int ext_ssgenv_main(PMDWIN *pmd, QQ *qq)
{
    int     dl;

    if(qq->eenv_count == 0) return 0;

    dl = qq->eenv_volume;
    esm_sub(pmd, qq, qq->eenv_count);

    if(dl == qq->eenv_volume) return 0;
    return -1;
}


int soft_env_main(PMDWIN *pmd, QQ *qq)
{
    int     dl;

    if(qq->envf == -1) {
        return ext_ssgenv_main(pmd, qq);
    }

    dl = qq->eenv_volume;
    soft_env_sub(pmd, qq);
    if(dl == qq->eenv_volume) {
        return 0;
    } else {
        return -1;
    }
}


int soft_env(PMDWIN *pmd, QQ *qq)
{
    int     i, cl;

    if(qq->extendmode & 4) {
        if(pmd->open_work.TimerAtime == pmd->open_work.lastTimerAtime) return 0;

        cl = 0;
        for(i = 0; i < pmd->open_work.TimerAtime - pmd->open_work.lastTimerAtime; i++) {
            if(soft_env_main(pmd, qq)) {
                cl = 1;
            }
        }
        return cl;
    } else {
        return soft_env_main(pmd, qq);
    }
}


//=============================================================================
//  テンポ設定
//=============================================================================
void settempo_b(PMDWIN *pmd)
{
    if(pmd->open_work.tempo_d != pmd->open_work.TimerB_speed) {
        pmd->open_work.TimerB_speed = pmd->open_work.tempo_d;
        OPNASetReg(&pmd->opna, 0x26, pmd->open_work.TimerB_speed);
    }
}


//=============================================================================
//  小節のカウント
//=============================================================================
void syousetu_count(PMDWIN *pmd)
{
    if(pmd->open_work.opncount + 1 == pmd->open_work.syousetu_lng) {
        pmd->open_work.syousetu++;
        pmd->open_work.opncount = 0;
    } else {
        pmd->open_work.opncount++;
    }
}


//=============================================================================
//  DATA AREA の イニシャライズ
//=============================================================================
void data_init(PMDWIN *pmd)
{
    int     i;
    int     partmask, keyon_flag;

    for(i = 0; i < 6; i++) {
        partmask = pmd->FMPart[i].partmask;
        keyon_flag = pmd->FMPart[i].keyon_flag;
        memset(&pmd->FMPart[i], 0, sizeof(QQ));
        pmd->FMPart[i].partmask = partmask & 0x0f;
        pmd->FMPart[i].keyon_flag = keyon_flag;
        pmd->FMPart[i].onkai = 255;
        pmd->FMPart[i].onkai_def = 255;
    }

    for(i = 0; i < 3; i++) {
        partmask = pmd->SSGPart[i].partmask;
        keyon_flag = pmd->SSGPart[i].keyon_flag;
        memset(&pmd->SSGPart[i], 0, sizeof(QQ));
        pmd->SSGPart[i].partmask = partmask & 0x0f;
        pmd->SSGPart[i].keyon_flag = keyon_flag;
        pmd->SSGPart[i].onkai = 255;
        pmd->SSGPart[i].onkai_def = 255;
    }

    partmask = pmd->RhythmPart.partmask;
    keyon_flag = pmd->RhythmPart.keyon_flag;
    memset(&pmd->RhythmPart, 0, sizeof(QQ));
    pmd->RhythmPart.partmask = partmask & 0x0f;
    pmd->RhythmPart.keyon_flag = keyon_flag;
    pmd->RhythmPart.onkai = 255;
    pmd->RhythmPart.onkai_def = 255;

    pmd->open_work.tieflag = 0;
    pmd->open_work.status = 0;
    pmd->open_work.status2 = 0;
    pmd->open_work.syousetu = 0;
    pmd->open_work.opncount = 0;
    pmd->open_work.TimerAtime = 0;
    pmd->open_work.lastTimerAtime = 0;

    pmd->open_work.omote_key[0] = 0;
    pmd->open_work.omote_key[1] = 0;
    pmd->open_work.omote_key[2] = 0;
    pmd->open_work.ura_key[0] = 0;
    pmd->open_work.ura_key[1] = 0;
    pmd->open_work.ura_key[2] = 0;

    pmd->open_work.fm3_alg_fb = 0;
    pmd->open_work.af_check = 0;

    pmd->open_work.kshot_dat = 0;
    pmd->open_work.rshot_dat = 0;

    pmd->open_work.slotdetune_flag = 0;
    pmd->open_work.slot_detune1 = 0;
    pmd->open_work.slot_detune2 = 0;
    pmd->open_work.slot_detune3 = 0;
    pmd->open_work.slot_detune4 = 0;

    pmd->open_work.slot3_flag = 0;
    pmd->open_work.ch3mode = 0x3f;

    pmd->open_work.fmsel = 0;

    pmd->open_work.syousetu_lng = 96;

    pmd->open_work.fm_voldown = pmd->open_work._fm_voldown;
    pmd->open_work.ssg_voldown = pmd->open_work._ssg_voldown;
    pmd->open_work.rhythm_voldown = pmd->open_work._rhythm_voldown;
}


//=============================================================================
//  OPN INIT
//=============================================================================
void opn_init(PMDWIN *pmd)
{
    int     i;
    OPNASetReg(&pmd->opna, 0x29, 0x83);
    pmd->open_work.psnoi = 0;
//@ if(pmd->effwork.effon == 0) {
        PSGSetReg(&(pmd->opna.psg), 0x06, 0x00);
        pmd->open_work.psnoi_last = 0;
//@ }

    //------------------------------------------------------------------------
    //  PAN/HARDLFO DEFAULT
    //------------------------------------------------------------------------
    OPNASetReg(&pmd->opna, 0xb4, 0xc0);
    OPNASetReg(&pmd->opna, 0xb5, 0xc0);
    OPNASetReg(&pmd->opna, 0xb6, 0xc0);
    OPNASetReg(&pmd->opna, 0x1b4, 0xc0);
    OPNASetReg(&pmd->opna, 0x1b5, 0xc0);
    OPNASetReg(&pmd->opna, 0x1b6, 0xc0);

    pmd->open_work.port22h = 0x00;
    OPNASetReg(&pmd->opna, 0x22, 0x00);

    //------------------------------------------------------------------------
    //  Rhythm Default = Pan : Mid , Vol : 15
    //------------------------------------------------------------------------
    for(i = 0; i < 6; i++) {
        pmd->open_work.rdat[i] = 0xcf;
    }
    OPNASetReg(&pmd->opna, 0x10, 0xff);

    //------------------------------------------------------------------------
    //  リズムトータルレベル　セット
    //------------------------------------------------------------------------
    pmd->open_work.rhyvol = 48*4*(256-pmd->open_work.rhythm_voldown)/1024;
    OPNASetReg(&pmd->opna, 0x11, pmd->open_work.rhyvol);
}


//=============================================================================
//  ALL SILENCE
//=============================================================================
void silence(PMDWIN *pmd)
{
    OPNASetReg(&pmd->opna, 0x80, 0xff);          // FM Release = 15
    OPNASetReg(&pmd->opna, 0x81, 0xff);
    OPNASetReg(&pmd->opna, 0x82, 0xff);
    OPNASetReg(&pmd->opna, 0x84, 0xff);
    OPNASetReg(&pmd->opna, 0x85, 0xff);
    OPNASetReg(&pmd->opna, 0x86, 0xff);
    OPNASetReg(&pmd->opna, 0x88, 0xff);
    OPNASetReg(&pmd->opna, 0x89, 0xff);
    OPNASetReg(&pmd->opna, 0x8a, 0xff);
    OPNASetReg(&pmd->opna, 0x8c, 0xff);
    OPNASetReg(&pmd->opna, 0x8d, 0xff);
    OPNASetReg(&pmd->opna, 0x8e, 0xff);

    OPNASetReg(&pmd->opna, 0x180, 0xff);
    OPNASetReg(&pmd->opna, 0x181, 0xff);
    OPNASetReg(&pmd->opna, 0x184, 0xff);
    OPNASetReg(&pmd->opna, 0x185, 0xff);
    OPNASetReg(&pmd->opna, 0x188, 0xff);
    OPNASetReg(&pmd->opna, 0x189, 0xff);
    OPNASetReg(&pmd->opna, 0x18c, 0xff);
    OPNASetReg(&pmd->opna, 0x18d, 0xff);

    OPNASetReg(&pmd->opna, 0x182, 0xff);
    OPNASetReg(&pmd->opna, 0x186, 0xff);
    OPNASetReg(&pmd->opna, 0x18a, 0xff);
    OPNASetReg(&pmd->opna, 0x18e, 0xff);

    OPNASetReg(&pmd->opna, 0x28, 0x00);          // FM KEYOFF
    OPNASetReg(&pmd->opna, 0x28, 0x01);
    OPNASetReg(&pmd->opna, 0x28, 0x02);
    OPNASetReg(&pmd->opna, 0x28, 0x04);          // FM KEYOFF [URA]
    OPNASetReg(&pmd->opna, 0x28, 0x05);
    OPNASetReg(&pmd->opna, 0x28, 0x06);

// 2003.11.30 プチノイズ対策のため
//@ if(pmd->effwork.effon == 0) {
        PSGSetReg(&(pmd->opna.psg), 0x07, 0xbf);
        PSGSetReg(&(pmd->opna.psg), 0x08, 0);    // 2003.11.30 プチノイズ対策のため
        PSGSetReg(&(pmd->opna.psg), 0x09, 0);    // 2003.11.30 プチノイズ対策のため
        PSGSetReg(&(pmd->opna.psg), 0x0a, 0);    // 2003.11.30 プチノイズ対策のため
//@ } else {
//@     PSGSetReg(&(pmd->opna.psg), 0x07, (PSGGetReg(&(pmd->opna.psg), 0x07) & 0x3f) | 0x9b);
//@ }

    OPNASetReg(&pmd->opna, 0x10, 0xff);          // Rhythm dump
    OPNASetReg(&pmd->opna, 0x110, 0x80);         // TA/TB/EOS を RESET
    OPNASetReg(&pmd->opna, 0x110, 0x18);         // TIMERB/A/EOSのみbit変化あり
}


//=============================================================================
//  各パートのスタートアドレス及び初期値をセット
//=============================================================================
void play_init(PMDWIN *pmd)
{
    int     i;
    uint16_t    *p;

    //２．６追加分
    if(*pmd->open_work.mmlbuf != 2*(max_part2+1)) {
        pmd->open_work.prgdat_adr = pmd->open_work.mmlbuf + *(uint16_t *)(&pmd->open_work.mmlbuf[2*(max_part2+1)]);
        pmd->open_work.prg_flg = 1;
    } else {
        pmd->open_work.prg_flg = 0;
    }

    p = (uint16_t *)pmd->open_work.mmlbuf;

    //  Part 0,1,2,3,4,5(FM1〜6)の時
    for(i = 0; i < 6; i++) {
        if(pmd->open_work.mmlbuf[*p] == 0x80) {      //先頭が80hなら演奏しない
            pmd->FMPart[i].address = NULL;
        } else {
            pmd->FMPart[i].address = &pmd->open_work.mmlbuf[*p];
        }

        pmd->FMPart[i].leng = 1;
        pmd->FMPart[i].keyoff_flag = -1;     // 現在keyoff中
        pmd->FMPart[i].mdc = -1;             // MDepth Counter (無限)
        pmd->FMPart[i].mdc2 = -1;            // 同上
        pmd->FMPart[i]._mdc = -1;            // 同上
        pmd->FMPart[i]._mdc2 = -1;           // 同上
        pmd->FMPart[i].onkai = 255;          // rest
        pmd->FMPart[i].onkai_def = 255;      // rest
        pmd->FMPart[i].volume = 108;         // FM  VOLUME DEFAULT= 108
        pmd->FMPart[i].fmpan = 0xc0;         // FM PAN = Middle
        pmd->FMPart[i].slotmask = 0xf0;      // FM SLOT MASK
        pmd->FMPart[i].neiromask = 0xff;     // FM Neiro MASK
        p++;
    }

    //  Part 6,7,8(PSG1〜3)の時
    for(i = 0; i < 3; i++) {
        if(pmd->open_work.mmlbuf[*p] == 0x80) {      //先頭が80hなら演奏しない
            pmd->SSGPart[i].address = NULL;
        } else {
            pmd->SSGPart[i].address = &pmd->open_work.mmlbuf[*p];
        }

        pmd->SSGPart[i].leng = 1;
        pmd->SSGPart[i].keyoff_flag = -1;    // 現在keyoff中
        pmd->SSGPart[i].mdc = -1;            // MDepth Counter (無限)
        pmd->SSGPart[i].mdc2 = -1;           // 同上
        pmd->SSGPart[i]._mdc = -1;           // 同上
        pmd->SSGPart[i]._mdc2 = -1;          // 同上
        pmd->SSGPart[i].onkai = 255;         // rest
        pmd->SSGPart[i].onkai_def = 255;     // rest
        pmd->SSGPart[i].volume = 8;          // PSG VOLUME DEFAULT= 8
        pmd->SSGPart[i].psgpat = 7;          // PSG = TONE
        pmd->SSGPart[i].envf = 3;            // PSG ENV = NONE/normal
        p++;
    }
    p++;

    //  Part 10(Rhythm)の時
    if(pmd->open_work.mmlbuf[*p] == 0x80) {      //先頭が80hなら演奏しない
        pmd->RhythmPart.address = NULL;
    } else {
        pmd->RhythmPart.address = &pmd->open_work.mmlbuf[*p];
    }

    pmd->RhythmPart.leng = 1;
    pmd->RhythmPart.keyoff_flag = -1;    // 現在keyoff中
    pmd->RhythmPart.mdc = -1;            // MDepth Counter (無限)
    pmd->RhythmPart.mdc2 = -1;           // 同上
    pmd->RhythmPart._mdc = -1;           // 同上
    pmd->RhythmPart._mdc2 = -1;          // 同上
    pmd->RhythmPart.onkai = 255;         // rest
    pmd->RhythmPart.onkai_def = 255;     // rest
    pmd->RhythmPart.volume = 15;         // PPSDRV volume
    p++;

    //------------------------------------------------------------------------
    //  Rhythm のアドレステーブルをセット
    //------------------------------------------------------------------------
    pmd->open_work.radtbl = (uint16_t *)&pmd->open_work.mmlbuf[*p];
    pmd->open_work.rhyadr = (uint8_t *)&pmd->open_work.rhydmy;
}


//=============================================================================
//  インタラプト　設定
//  FM音源専用
//=============================================================================
void setint(PMDWIN *pmd)
{
    //
    // ＯＰＮ割り込み初期設定
    //
    pmd->open_work.tempo_d = 200;
    pmd->open_work.tempo_d_push = 200;

    calc_tb_tempo(pmd);
    settempo_b(pmd);

    OPNASetReg(&pmd->opna, 0x25, 0x00);          // TIMER A SET (9216μs固定)
    OPNASetReg(&pmd->opna, 0x24, 0x00);          // 一番遅くて丁度いい
    OPNASetReg(&pmd->opna, 0x27, 0x3f);          // TIMER ENABLE

    //
    //　小節カウンタリセット
    //

    pmd->open_work.opncount = 0;
    pmd->open_work.syousetu = 0;
    pmd->open_work.syousetu_lng = 96;
}


//=============================================================================
//  ＭＵＳＩＣ　ＳＴＯＰ
//=============================================================================
void mstop(PMDWIN *pmd)
{
    pmd->open_work.music_flag &= 0xfd;
    pmd->open_work.play_flag = 0;
    pmd->open_work.status2 = -1;
    silence(pmd);
}


//=============================================================================
//  演奏開始
//=============================================================================
void mstart(PMDWIN *pmd)
{
    // TimerB = 0 に設定し、Timer Reset(曲の長さを毎回そろえるため)
    pmd->open_work.tempo_d = 0;
    settempo_b(pmd);
    OPNASetReg(&pmd->opna, 0x27, 0); // TIMER RESET(timerA,Bとも)

    //------------------------------------------------------------------------
    //  演奏停止
    //------------------------------------------------------------------------
    pmd->open_work.music_flag &= 0xfe;
    mstop(pmd);

    //------------------------------------------------------------------------
    //  バッファ初期化
    //------------------------------------------------------------------------
    pmd->pos2 = (char *)pmd->wavbuf2; // buf に余っているサンプルの先頭位置
    pmd->us2 = 0;                // buf に余っているサンプル数
    pmd->upos = 0;               // 演奏開始からの時間(μsec)

    //------------------------------------------------------------------------
    //  演奏準備
    //------------------------------------------------------------------------
    data_init(pmd);
    play_init(pmd);

    //------------------------------------------------------------------------
    //  OPN初期化
    //------------------------------------------------------------------------
    opn_init(pmd);

    //------------------------------------------------------------------------
    //  音楽の演奏を開始
    //------------------------------------------------------------------------
    setint(pmd);
    pmd->open_work.play_flag = 1;
}


//=============================================================================
//  TimerBの処理[メイン]
//=============================================================================
void TimerB_main(PMDWIN *pmd)
{
    pmd->open_work.TimerBflag = 1;
    if(pmd->open_work.music_flag) {
        if(pmd->open_work.music_flag & 1) {
            mstart(pmd);
        }

        if(pmd->open_work.music_flag & 2) {
            mstop(pmd);
        }
    }

    if(pmd->open_work.play_flag) {
        mmain(pmd);
        settempo_b(pmd);
        syousetu_count(pmd);
        pmd->open_work.lastTimerAtime = pmd->open_work.TimerAtime;
    }
    pmd->open_work.TimerBflag = 0;
}


//=============================================================================
//  ＯＰＮ割り込み許可処理
//=============================================================================
void opnint_start(PMDWIN *pmd)
{
    pmd->open_work.rhythmmask = 255;
    pmd->open_work.rhydmy = 255;
    opn_init(pmd);

    PSGSetReg(&(pmd->opna.psg), 0x07, 0xbf);
    mstop(pmd);
    setint(pmd);
    OPNASetReg(&pmd->opna, 0x29, 0x83);
}


//=============================================================================
//  曲の読み込みその３（メモリから、カレントディレクトリあり）
//=============================================================================
int music_load3(PMDWIN *pmd, uint8_t *musdata, unsigned int size)
{
    if(size > sizeof(pmd->mdataarea)) {
        return ERR_WRONG_MUSIC_FILE;
    }

    // 020120 ヘッダ解析のみ Towns に対応
    if((musdata[0] > 0x0f && musdata[0] != 0xff) || (musdata[1] != 0x18 && musdata[1] != 0x1a) || musdata[2]) {
        return ERR_WRONG_MUSIC_FILE;        // not PMD data
    }

    // メモリを 0x00 で初期化する
    memcpy(pmd->mdataarea, musdata, size);
    return PMDWIN_OK;
}


//=============================================================================
//  初期化
//=============================================================================
static unsigned char pmdwin_init(PMDWIN *pmd)
{
    int i;
    struct timeval tv;
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER SystemTime;
#endif
    pmd->open_work.rate = SOUND_44K;

    // OPEN_WORK メンバの初期化
    pmd->open_work.rhyvol = 0x3c;
    pmd->open_work.TimerBflag = 0;
    pmd->open_work.TimerAflag = 0;
    pmd->open_work.TimerB_speed = 0x100;
    pmd->open_work.port22h = 0;

    if(OPNAInit(&pmd->opna, OPNAClock, SOUND_44K, 1) == 0) return 0;

    // Init RNG state
#if defined(_WIN32) || defined(_WIN64)
    NtQuerySystemTime(&SystemTime);
    SystemTime.QuadPart /= 10; /* 100ns granularity -> 1000ns (1ms) granularity */
    tv->tv_sec = (SystemTime.QuadPart/1000000) - JAN_1601;
    tv->tv_usec = (SystemTime.QuadPart%1000000);
#else
    gettimeofday(&tv, NULL);
#endif
    lfg_srand(&pmd->open_work, tv.tv_sec);

    for(i=0; i<nbufsample; i++) {
        pmd->wavbuf2[i] = 0;
    }
    pmd->pos2 = (char *)pmd->wavbuf2;
    pmd->us2 = 0;
    pmd->upos = 0;

    //----------------------------------------------------------------
    //  変数の初期化
    //----------------------------------------------------------------

    pmd->open_work.MusPart[ 0] = &pmd->FMPart[0];
    pmd->open_work.MusPart[ 1] = &pmd->FMPart[1];
    pmd->open_work.MusPart[ 2] = &pmd->FMPart[2];
    pmd->open_work.MusPart[ 3] = &pmd->FMPart[3];
    pmd->open_work.MusPart[ 4] = &pmd->FMPart[4];
    pmd->open_work.MusPart[ 5] = &pmd->FMPart[5];
    pmd->open_work.MusPart[ 6] = &pmd->SSGPart[0];
    pmd->open_work.MusPart[ 7] = &pmd->SSGPart[1];
    pmd->open_work.MusPart[ 8] = &pmd->SSGPart[2];
    pmd->open_work.MusPart[ 9] = &pmd->RhythmPart;

    pmd->mdataarea[0] = 0;
    for(i = 0; i < 12; i++) {
        pmd->mdataarea[i*2+1] = 0x18;
        pmd->mdataarea[i*2+2] = 0;
    }
    pmd->mdataarea[25] = 0x80;

    pmd->open_work.fm_voldown = fmvd_init;       // FM_VOLDOWN
    pmd->open_work._fm_voldown = fmvd_init;      // FM_VOLDOWN
    pmd->open_work.ssg_voldown = 0;              // SSG_VOLDOWN
    pmd->open_work._ssg_voldown = 0;             // SSG_VOLDOWN
    pmd->open_work.rhythm_voldown = 0;           // RHYTHM_VOLDOWN
    pmd->open_work._rhythm_voldown = 0;          // RHYTHM_VOLDOWN

    //memset(FMPart, 0, sizeof(FMPart));
    //memset(SSGPart, 0, sizeof(SSGPart));
    //memset(&RhythmPart, 0, sizeof(RhythmPart));

    pmd->open_work.music_flag =  0;
    pmd->open_work.ppsdrv_flag = 0;    // PPSDRV FLAG

    //----------------------------------------------------------------
    //  曲データ，音色データ格納番地を設定
    //----------------------------------------------------------------
    pmd->open_work.mmlbuf = &pmd->mdataarea[1];

    //----------------------------------------------------------------
    //  効果音/FMINT/EFCINTを初期化
    //----------------------------------------------------------------
    pmd->effwork.effon = 0;
    pmd->effwork.psgefcnum = 0xff;

    //----------------------------------------------------------------
    //  088/188/288/388 (同INT番号のみ) を初期設定
    //----------------------------------------------------------------
    OPNASetReg(&pmd->opna, 0x29, 0x00);
    OPNASetReg(&pmd->opna, 0x24, 0x00);
    OPNASetReg(&pmd->opna, 0x25, 0x00);
    OPNASetReg(&pmd->opna, 0x26, 0x00);
    OPNASetReg(&pmd->opna, 0x27, 0x3f);

    //----------------------------------------------------------------
    //  ＯＰＮ割り込み開始
    //----------------------------------------------------------------
    data_init(pmd);
    opnint_start(pmd);
    return 1;
}


//=============================================================================
//  合成周波数の設定
//=============================================================================
void setpcmrate(PMDWIN *pmd, unsigned int rate)
{
    if(rate == SOUND_55K || rate == SOUND_55K_2) {
        pmd->open_work.rate = SOUND_44K;
        pmd->open_work.fmcalc55k = 1;
    } else {
        pmd->open_work.rate = rate;
        pmd->open_work.fmcalc55k = 0;
    }
    OPNASetRate(&pmd->opna, pmd->open_work.rate, pmd->open_work.fmcalc55k);
}


//=============================================================================
// Mask to enable/disable usage of FM (bit 0), SSG (bit 1) and Rhythm (bit 2)
// on the OPNA. Mostly for testing purposes.
//=============================================================================
void setdevmask(PMDWIN *pmd, uint8_t mask)
{
    pmd->opna.devmask = mask;
}


void setchanmask(PMDWIN *pmd, uint32_t mask)
{
    OPNASetChannelMask(&pmd->opna, mask);
}


//=============================================================================
//  演奏開始
//=============================================================================
void music_start(PMDWIN *pmd)
{
    if(pmd->open_work.TimerAflag || pmd->open_work.TimerBflag) {
        pmd->open_work.music_flag |= 1;            //  TA/TB処理中は 実行しない
        return;
    }

    mstart(pmd);
}


//=============================================================================
//  演奏停止
//=============================================================================
void music_stop(PMDWIN *pmd)
{
    int i;
    if(pmd->open_work.TimerAflag || pmd->open_work.TimerBflag) {
        pmd->open_work.music_flag |= 2;
    } else {
        mstop(pmd);
    }

    for(i=0; i<nbufsample; i++) pmd->wavbuf2[i] = 0;
    pmd->upos = 0;
}


//=============================================================================
//  PCM データ（wave データ）の取得
//=============================================================================
void getpcmdata(PMDWIN *pmd, short *buf, uint32_t nsamples)
{
    unsigned int copysamples = 0; // コピー済みのサンプル数
    int us;

    do {
        if(nsamples - copysamples <= pmd->us2) {
            memcpy(buf, pmd->pos2, (nsamples - copysamples)*2);
            pmd->us2 -= (nsamples - copysamples);
            pmd->pos2 += (nsamples - copysamples)*2;
            copysamples = nsamples;
        } else {
            memcpy(buf, pmd->pos2, pmd->us2 * 2);
            buf += pmd->us2;
            copysamples += pmd->us2;
            pmd->pos2 = (char *)pmd->wavbuf2;

            if(OPNAReadStatus(&pmd->opna) & 0x01) {
                TimerA_main(pmd);
            }

            if(OPNAReadStatus(&pmd->opna) & 0x02) {
                TimerB_main(pmd);
            }

            OPNASetReg(&pmd->opna, 0x27, pmd->open_work.ch3mode | 0x30);  // TIMER RESET(timerA,Bとも)
            us = OPNAGetNextEvent(&pmd->opna);
            pmd->us2 = (int)((float)us * pmd->open_work.rate / 1000000.0f);
            OPNATimerCount(&pmd->opna, us);

            OPNAMix(&pmd->opna, (int16_t*)pmd->wavbuf2, pmd->us2);
            pmd->upos += us;
        }
    } while(copysamples < nsamples);
}


//=============================================================================
//  FM で 55kHz合成、一次補完するかどうかの設定
//=============================================================================
void setfmcalc55k(PMDWIN *pmd, unsigned char flag)
{
    pmd->open_work.fmcalc55k = flag;
    OPNASetRate(&pmd->opna, pmd->open_work.rate, pmd->open_work.fmcalc55k);
}


unsigned int getstatus(PMDWIN *pmd, char *buf, unsigned int bufsize)
{
    char *p = buf;
    unsigned int i = 0, len;
    while(i < 6) {
        *p++ = 'F';
        *p++ = 'M';
        *p++ = (i + 0x30);
        *p++ = ':';
        *p++ = ' ';
        *p++ = notes[pmd->pmdstatus[i]][0];
        *p++ = notes[pmd->pmdstatus[i]][1];
        *p++ = ' ';
        i++;
    }
    while(i < 9) {
        *p++ = 'P';
        *p++ = 'S';
        *p++ = 'G';
        *p++ = (i + 0x30 - 6);
        *p++ = ':';
        *p++ = ' ';
        *p++ = notes[pmd->pmdstatus[i]][0];
        *p++ = notes[pmd->pmdstatus[i]][1];
        *p++ = ' ';
        i++;
    }
    p -= 3;
    *p++ = (pmd->pmdstatus[8] + 0x30);
    *p++ = '\0';
    len = (p - buf);
    return len;
}


//=============================================================================
//  メモの取得（内部動作）
//=============================================================================
char* _getmemo(PMDWIN *pmd, char *dest, uint8_t *musdata, int size, int al)
{
    uint8_t   *si, *mmlbuf;
    int     i, dx;
    int     maxsize;

    if(musdata == NULL || size == 0) {
        mmlbuf = pmd->open_work.mmlbuf;
        maxsize = sizeof(pmd->mdataarea) - 1;
    } else {
        mmlbuf = &musdata[1];
        maxsize = size - 1;
    }
    if(maxsize < 2) {
        *dest = '\0';           //  曲データが不正
        return NULL;
    }

    if(mmlbuf[0] != 0x1a || mmlbuf[1] != 0x00) {
        *dest = '\0';           //  音色がないfile=メモのアドレス取得不能
        return dest;
    }

    if(maxsize < 0x18+1) {
        *dest = '\0';           //  曲データが不正
        return NULL;
    }

    if(maxsize < *(uint16_t *)&mmlbuf[0x18] - 4 + 3) {
        *dest = '\0';           //  曲データが不正
        return NULL;
    }

    si = &mmlbuf[*(uint16_t *)&mmlbuf[0x18] - 4];
    if(*(si + 2) != 0x40) {
        if(*(si + 3) != 0xfe || *(si + 2) < 0x41) {
            *dest = '\0';           //  音色がないfile=メモのアドレス取得不能
            return dest;
        }
    }

    if(*(si + 2) >= 0x42) {
        al++;
    }

    if(*(si + 2) >= 0x48) {
        al++;
    }

    if(al < 0) {
        *dest = '\0';               //  登録なし
        return dest;
    }

    si = &mmlbuf[*(uint16_t *)si];

    for(i = 0; i <= al; i++) {
        if(maxsize < si - mmlbuf + 1) {
            *dest = '\0';           //  曲データが不正
            return NULL;
        }

        dx = *(uint16_t *)si;
        if(dx == 0) {
            *dest = '\0';
            return dest;
        }

        if(maxsize < dx) {
            *dest = '\0';           //  曲データが不正
            return NULL;
        }

        if(mmlbuf[dx] == '/') {
            *dest = '\0';
            return dest;
        }

        si += 2;
    }

    for(i = dx; i < maxsize; i++) {
        if(mmlbuf[i] == '\0') break;
    }

    // 終端の \0 がない場合
    if(i >= maxsize) {
        memcpy(dest, &mmlbuf[dx], maxsize - dx);
        dest[maxsize - dx - 1] = '\0';
    } else {
        memcpy(dest, &mmlbuf[dx], i);
        dest[i] = '\0';
    }
    return dest;
}

/*
   Conversion between SJIS codes (s1,s2) and JISX0208 codes (c1,c2):
   Example. (s1,s2) = 0x8140, (c1,c2) = 0x2121.
   0x81 <= s1 <= 0x9F || 0xE0 <= s1 <= 0xEA,
   0x40 <= s2 <= 0x7E || 0x80 <= s2 <= 0xFC,
   0x21 <= c1 <= 0x74, 0x21 <= c2 <= 0x7E.
   Invariant:
     94*2*(s1 < 0xE0 ? s1-0x81 : s1-0xC1) + (s2 < 0x80 ? s2-0x40 : s2-0x41)
     = 94*(c1-0x21)+(c2-0x21)
   Conversion (s1,s2) -> (c1,c2):
     t1 := (s1 < 0xE0 ? s1-0x81 : s1-0xC1)
     t2 := (s2 < 0x80 ? s2-0x40 : s2-0x41)
     c1 := 2*t1 + (t2 < 0x5E ? 0 : 1) + 0x21
     c2 := (t2 < 0x5E ? t2 : t2-0x5E) + 0x21
   Conversion (c1,c2) -> (s1,s2):
     t1 := (c1 - 0x21) >> 1
     t2 := ((c1 - 0x21) & 1) * 0x5E + (c2 - 0x21)
     s1 := (t1 < 0x1F ? t1+0x81 : t1+0xC1)
     s2 := (t2 < 0x3F ? t2+0x40 : t2+0x41)
 */

static int sjis_to_ucs4(uint32_t *pwc, const uint8_t *s, int n)
{
  uint8_t c = *s;
  if (c <= 0x80) {
    if (c == 0x5c)
      *pwc = (uint32_t) 0x00a5;
    else if (c == 0x7e)
      *pwc = (uint32_t) 0x203e;
    else
      *pwc = (uint32_t) c;
    return 1;
  } else if (c >= 0xa1 && c <= 0xdf) {
    *pwc = (uint32_t) c + 0xfec0;
    return 1;
  } else {
    uint8_t s1, s2;
    s1 = c;
    if ((s1 >= 0x81 && s1 <= 0x9f) || (s1 >= 0xe0 && s1 <= 0xea)) {
      if (n < 2)
        return -2;
      s2 = s[1];
      if ((s2 >= 0x40 && s2 <= 0x7e) || (s2 >= 0x80 && s2 <= 0xfc)) {
        uint8_t t1 = (s1 < 0xe0 ? s1-0x81 : s1-0xc1);
        uint8_t t2 = (s2 < 0x80 ? s2-0x40 : s2-0x41);
        uint8_t buf[2];
        buf[0] = 2*t1 + (t2 < 0x5e ? 0 : 1) + 0x21;
        buf[1] = (t2 < 0x5e ? t2 : t2-0x5e) + 0x21;
        return jisx0208_mbtowc(pwc,buf,2);
      }
    } else if (s1 >= 0xf0 && s1 <= 0xf9) {
      /* User-defined range. See
       * Ken Lunde's "CJKV Information Processing", table 4-66, p. 206. */
      if (n < 2)
        return -2;
      s2 = s[1];
      if ((s2 >= 0x40 && s2 <= 0x7e) || (s2 >= 0x80 && s2 <= 0xfc)) {
        *pwc = 0xe000 + 188*(s1 - 0xf0) + (s2 < 0x80 ? s2-0x40 : s2-0x41);
        return 2;
      }
    }
    return -1;
  }
}

static int ucs4_to_utf8(uint8_t *utf8, uint32_t ucs4)
{
  if (ucs4 <= 0x0000007fL) {
    utf8[0] = ucs4;
    return 1;
  } else if (ucs4 <= 0x000007ffL) {
    utf8[0] = 0xc0 | ((ucs4 >>  6) & 0x1f);
    utf8[1] = 0x80 | ((ucs4 >>  0) & 0x3f);
    return 2;
  } else if (ucs4 <= 0x0000ffffL) {
    utf8[0] = 0xe0 | ((ucs4 >> 12) & 0x0f);
    utf8[1] = 0x80 | ((ucs4 >>  6) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >>  0) & 0x3f);
    return 3;
  }

  /* default */
  utf8[0] = '?';
  return 1;
}

static unsigned int sjis_to_utf8(uint8_t *outbuf, uint8_t *inbuf, unsigned int insize) {
    uint32_t wc = 0;
    uint8_t *utf8str = outbuf;
    uint8_t *utf8str_start = utf8str;
    int ret;
    while(insize > 0) {
        ret = sjis_to_ucs4(&wc, inbuf, insize);
        if(ret < 0) {
            inbuf++; insize--;
            continue;
        }
        utf8str += ucs4_to_utf8(utf8str, wc);
        inbuf += ret;
        insize -= ret;
    }
    return (utf8str - utf8str_start);
}

char* _getmemo3(PMDWIN *pmd, char *dest, uint8_t *musdata, int size, int al)
{
    char buf[mdata_def*1024];
    _getmemo(pmd, (char *)buf, musdata, size, al);
    sjis_to_utf8((uint8_t*)dest, (uint8_t*)buf, strlen(buf)+1);
    return dest;
}


//=============================================================================
//  パートのマスク
//=============================================================================
int maskon(PMDWIN *pmd, unsigned int ch)
{
    int     ah, fmseltmp;


    if(ch >= sizeof(pmd->open_work.MusPart) / sizeof(QQ *)) {
        return ERR_WRONG_PARTNO;        // part number error
    }

    if(part_table[ch][0] < 0) {
        pmd->open_work.rhythmmask = 0;   // Rhythm音源をMask
        OPNASetReg(&pmd->opna, 0x10, 0xff);  // Rhythm音源を全部Dump
    } else {
        fmseltmp = pmd->open_work.fmsel;
        if(pmd->open_work.MusPart[ch]->partmask == 0 && pmd->open_work.play_flag) {
            if(part_table[ch][2] == 0) {
                pmd->open_work.partb = part_table[ch][1];
                pmd->open_work.fmsel = 0;
                silence_fmpart(pmd, pmd->open_work.MusPart[ch]);  // 音を完璧に消す
            } else if(part_table[ch][2] == 1) {
                pmd->open_work.partb = part_table[ch][1];
                pmd->open_work.fmsel = 0x100;
                silence_fmpart(pmd, pmd->open_work.MusPart[ch]);  // 音を完璧に消す
            } else if(part_table[ch][2] == 2) {
                pmd->open_work.partb = part_table[ch][1];
                ah =  1 << (pmd->open_work.partb - 1);
                ah |= (ah << 3);
                PSGSetReg(&(pmd->opna.psg), 0x07, ah | PSGGetReg(&(pmd->opna.psg), 0x07)); // PSG keyoff
            } else if(part_table[ch][2] == 4) {
                if(pmd->effwork.psgefcnum < 11) {
                    effend(&pmd->effwork, &pmd->opna);
                }
            }
        }
        pmd->open_work.MusPart[ch]->partmask |= 1;
        pmd->open_work.fmsel = fmseltmp;
    }
    return PMDWIN_OK;
}


//=============================================================================
//  パートのマスク解除
//=============================================================================
int maskoff(PMDWIN *pmd, unsigned int ch)
{
    int     fmseltmp;

    if(ch >= sizeof(pmd->open_work.MusPart) / sizeof(QQ *)) {
        return ERR_WRONG_PARTNO;        // part number error
    }

    if(part_table[ch][0] < 0) {
        pmd->open_work.rhythmmask = 0xff;
    } else {
        if(pmd->open_work.MusPart[ch]->partmask == 0) return ERR_NOT_MASKED; // マスクされていない
                                            // 効果音でまだマスクされている
        if((pmd->open_work.MusPart[ch]->partmask &= 0xfe) != 0) return ERR_EFFECT_USED;
                                            // 曲が止まっている
        if(pmd->open_work.play_flag == 0) return ERR_MUSIC_STOPPED;

        fmseltmp = pmd->open_work.fmsel;
        if(pmd->open_work.MusPart[ch]->address != NULL) {
            if(part_table[ch][2] == 0) {        // FM音源(表)
                pmd->open_work.fmsel = 0;
                pmd->open_work.partb = part_table[ch][1];
                neiro_reset(pmd, pmd->open_work.MusPart[ch]);
            } else if(part_table[ch][2] == 1) { // FM音源(裏)
                pmd->open_work.fmsel = 0x100;
                pmd->open_work.partb = part_table[ch][1];
                neiro_reset(pmd, pmd->open_work.MusPart[ch]);
            }
        }
        pmd->open_work.fmsel = fmseltmp;

    }
    return PMDWIN_OK;
}


//=============================================================================
//  FM Volume Down の設定
//=============================================================================
void setfmvoldown(PMDWIN *pmd, int voldown)
{
    pmd->open_work.fm_voldown = pmd->open_work._fm_voldown = voldown;
}


//=============================================================================
//  SSG Volume Down の設定
//=============================================================================
void setssgvoldown(PMDWIN *pmd, int voldown)
{
    pmd->open_work.ssg_voldown = pmd->open_work._ssg_voldown = voldown;
}


//=============================================================================
//  Rhythm Volume Down の設定
//=============================================================================
void setrhythmvoldown(PMDWIN *pmd, int voldown)
{
    pmd->open_work.rhythm_voldown = pmd->open_work._rhythm_voldown = voldown;
    pmd->open_work.rhyvol = 48*4*(256-pmd->open_work.rhythm_voldown)/1024;
    OPNASetReg(&pmd->opna, 0x11, pmd->open_work.rhyvol);

}


//=============================================================================
//  FM Volume Down の取得
//=============================================================================
int getfmvoldown(PMDWIN *pmd)
{
    return pmd->open_work.fm_voldown;
}


//=============================================================================
//  FM Volume Down の取得（その２）
//=============================================================================
int getfmvoldown2(PMDWIN *pmd)
{
    return pmd->open_work._fm_voldown;
}


//=============================================================================
//  SSG Volume Down の取得
//=============================================================================
int getssgvoldown(PMDWIN *pmd)
{
    return pmd->open_work.ssg_voldown;
}


//=============================================================================
//  SSG Volume Down の取得（その２）
//=============================================================================
int getssgvoldown2(PMDWIN *pmd)
{
    return pmd->open_work._ssg_voldown;
}


//=============================================================================
//  Rhythm Volume Down の取得
//=============================================================================
int getrhythmvoldown(PMDWIN *pmd)
{
    return pmd->open_work.rhythm_voldown;
}


//=============================================================================
//  Rhythm Volume Down の取得（その２）
//=============================================================================
int getrhythmvoldown2(PMDWIN *pmd)
{
    return pmd->open_work._rhythm_voldown;
}


//=============================================================================
//  再生位置の移動(pos : ms)
//=============================================================================
void setpos(PMDWIN *pmd, uint64_t pos)
{
    uint64_t _pos;
    uint us;

    _pos = (uint64_t)pos * 1000;            // (ms -> usec への変換)

    if(pmd->upos > _pos) {
        mstart(pmd);
        pmd->pos2 = (char *)pmd->wavbuf2; // buf に余っているサンプルの先頭位置
        pmd->us2 = 0;                // buf に余っているサンプル数
        pmd->upos = 0;               // 演奏開始からの時間(μsec)
    }

    while(pmd->upos < _pos) {
        if(OPNAReadStatus(&pmd->opna) & 0x01) {
            TimerA_main(pmd);
        }

        if(OPNAReadStatus(&pmd->opna) & 0x02) {
            TimerB_main(pmd);
        }

        OPNASetReg(&pmd->opna, 0x27, pmd->open_work.ch3mode | 0x30);  // TIMER RESET(timerA,Bとも)
        us = OPNAGetNextEvent(&pmd->opna);
        OPNATimerCount(&pmd->opna, us);
        pmd->upos += us;
    }

    if(pmd->open_work.status2 == -1) {
        silence(pmd);
    }
}


//=============================================================================
//  再生位置の移動(pos : count 単位)
//=============================================================================
void setpos2(PMDWIN *pmd, unsigned int pos)
{
    int     us;

    if(pmd->open_work.syousetu_lng * pmd->open_work.syousetu + pmd->open_work.opncount > pos) {
        mstart(pmd);
        pmd->pos2 = (char *)pmd->wavbuf2; // buf に余っているサンプルの先頭位置
        pmd->us2 = 0;                // buf に余っているサンプル数
    }

    while(pmd->open_work.syousetu_lng * pmd->open_work.syousetu + pmd->open_work.opncount < pos) {
        if(OPNAReadStatus(&pmd->opna) & 0x01) {
            TimerA_main(pmd);
        }

        if(OPNAReadStatus(&pmd->opna) & 0x02) {
            TimerB_main(pmd);
        }

        OPNASetReg(&pmd->opna, 0x27, pmd->open_work.ch3mode | 0x30);  // TIMER RESET(timerA,Bとも)
        us = OPNAGetNextEvent(&pmd->opna);
        OPNATimerCount(&pmd->opna, us);
    }

    if(pmd->open_work.status2 == -1) {
        silence(pmd);
    }
}


//=============================================================================
//  再生位置の取得(pos : ms)
//=============================================================================
uint64_t getpos(PMDWIN *pmd)
{
    return ((pmd->upos * 1049) >> 20);
}


//=============================================================================
//  再生位置の取得(pos : count 単位)
//=============================================================================
uint32_t getpos2(PMDWIN *pmd)
{
    return pmd->open_work.syousetu_lng * pmd->open_work.syousetu + pmd->open_work.opncount;
}


//=============================================================================
//  曲の長さの取得(pos : ms)
//=============================================================================
unsigned char getlength(PMDWIN *pmd, uint32_t *length, uint32_t *loop)
{
    int     us;
    mstart(pmd);
    pmd->upos = 0;               // 演奏開始からの時間(μsec)
    *length = 0;

    do {
        if(OPNAReadStatus(&pmd->opna) & 0x01) {
            TimerA_main(pmd);
        }

        if(OPNAReadStatus(&pmd->opna) & 0x02) {
            TimerB_main(pmd);
        }

        OPNASetReg(&pmd->opna, 0x27, pmd->open_work.ch3mode | 0x30);  // TIMER RESET(timerA,Bとも)

        us = OPNAGetNextEvent(&pmd->opna);
        OPNATimerCount(&pmd->opna, us);
        pmd->upos += us;
        if(pmd->open_work.status2 == 1 && *length == 0) {    // ループ時
            *length = (uint)((pmd->upos * 1049) >> 20);
        } else if(pmd->open_work.status2 == -1) {            // ループなし終了時
            *length = (uint)((pmd->upos * 1049) >> 20);
            *loop = 0;
            mstop(pmd);
            return 1;
        } else if(getpos2(pmd) >= 65536) {     // 65536クロック以上なら強制終了
            *length = (uint)((pmd->upos * 1049) >> 20);
            *loop = *length;
            return 1;
        }
    } while(pmd->open_work.status2 < 2);

    // I love gcc sometimes. Apparently it thinks that on i386 that the following divide
    // as well as the two above are better done via a freaking call to __udivdi3
    // than just by doing the optimized divide-by-a-constant code that it is perfectly
    // capable of generating for this exact routine when uint64_t is a basic type.
    // The hell? Anyway, to compensate for that wonderful bit of retardation,
    // we're doing the divide via the multiply-and-shift-right method
    // manually here instead.
    *loop = (uint)((pmd->upos * 1049) >> 20) - *length;
    mstop(pmd);
    return 1;
}


//=============================================================================
//  ループ回数の取得
//=============================================================================
int getloopcount(PMDWIN *pmd)
{
    return pmd->open_work.status2;
}


#ifdef WIN32
//=============================================================================
//  DLL Export Functions
//=============================================================================
BOOL DllMain(HINSTANCE hModule,  DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            LdrDisableThreadCalloutsForDll(hModule);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif


//=============================================================================
//  バーション取得
//=============================================================================
DLLEXPORT int getversion(void)
{
    return  DLLVersion;
}

//=============================================================================
//  初期化
//=============================================================================
DLLEXPORT PMDWIN *pmdwininit(void)
{
    PMDWIN *pmd = NULL;
#if defined(WIN32) || defined(WIN64)
    size_t size = sizeof(PMDWIN);
    NtAllocateVirtualMemory(((HANDLE)-1), (void**)&pmd, 0, &size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
#else
    pmd = (PMDWIN*)mmap(NULL, sizeof(PMDWIN), PROT_READ|PROT_WRITE,
                        MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
#endif
    pmdwin_init(pmd);
    return pmd;
}

DLLEXPORT void pmdwin_free(PMDWIN *pmd)
{
#if defined(WIN32) || defined(WIN64)
    NtFreeVirtualMemory(((HANDLE)-1), (void**)&pmd, 0, MEM_RELEASE);
#else
    munmap(pmd, sizeof(PMDWIN));
#endif
}

