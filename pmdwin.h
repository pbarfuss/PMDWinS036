//=============================================================================
//  Professional Music Driver [P.M.D.] version 4.8
//          Programmed By M.Kajihara
//          Windows Converted by C60
//=============================================================================

#ifndef PMDWIN_H
#define PMDWIN_H

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include "opna.h"
void MD5(unsigned char *dst, const unsigned char *src, unsigned int len);

//=============================================================================
//  バージョン情報
//=============================================================================
#define DLLVersion           36     // 上１桁：major, 下２桁：minor version
#define InterfaceVersion    117     // 上１桁：major, 下２桁：minor version

//=============================================================================
//  DLL の戻り値
//=============================================================================
#define PMDWIN_OK                     0     // 正常終了
#define ERR_OPEN_MUSIC_FILE           1     // 曲 データを開けなかった
#define ERR_WRONG_MUSIC_FILE          2     // PMD の曲データではなかった
#define ERR_WRONG_PARTNO             30     // パート番号が不適
//#define   ERR_ALREADY_MASKED           31     // 指定パートはすでにマスクされている
#define ERR_NOT_MASKED               32     // 指定パートはマスクされていない
#define ERR_MUSIC_STOPPED            33     // 曲が止まっているのにマスク操作をした
#define ERR_EFFECT_USED              34     // 効果音で使用中なのでマスクを操作できない
#define ERR_OUT_OF_MEMORY            99     // メモリを確保できなかった
#define ERR_OTHER                   999     // その他のエラー

//----------------------------------------------------------------------------
//  ＰＭＤＷｉｎ専用の定義
//----------------------------------------------------------------------------
#define SOUND_55K             55555
#define SOUND_55K_2           55466
#define SOUND_48K             48000
#define SOUND_44K             44100
#define SOUND_22K             22050
#define SOUND_11K             11025
#define DEFAULT_REG_WAIT      15000
#define MAX_PCMDIR               64
#define MAX_MEMOBUF            1024
#define JAN_1601 0x232661280ULL /* 9435484800 1900 - 1601 in seconds */

//----------------------------------------------------------------------------
//  その他定義
//----------------------------------------------------------------------------
//#define   nbufsample            30000
#define nbufsample 8192
#define OPNAClock       (3993600*2)

#define NumOfFMPart               6
#define NumOfSSGPart              3
#define NumOFOPNARhythmPart       1
#define NumOfAllPart        (NumOfFMPart+NumOfSSGPart+NumOFOPNARhythmPart)

//#define ver       "4.8o"
//#define vers      0x48
//#define verc      "o"
//#define date      "Jun.19th 1997"

//#define max_part1 22      // ０クリアすべきパート数(for PMDPPZ)
#define max_part2   11      // 初期化すべきパート数  (for PMDPPZ)

#define mdata_def   64
#define voice_def    8
#define effect_def  64

#define fmvd_init   0       // ９８は８８よりもＦＭ音源を小さく


/******************************************************************************
;   WORK AREA
******************************************************************************/
typedef struct effworktag {
    int     *effadr;            //  effect address
    int     eswthz;             //  トーンスゥイープ周波数
    int     eswtst;             //  トーンスゥイープ増分
    int     effcnt;             //  effect count
    int     eswnhz;             //  ノイズスゥイープ周波数
    int     eswnst;             //  ノイズスゥイープ増分
    int     eswnct;             //  ノイズスゥイープカウント
    int     effon;              //  効果音　発音中
    int     psgefcnum;          //  効果音番号
} EFFWORK;

//  演奏中のデータエリア
typedef struct qqtag {
    uint8_t   *address;           //  2 ｴﾝｿｳﾁｭｳ ﾉ ｱﾄﾞﾚｽ
    uint8_t   *partloop;          //  2 ｴﾝｿｳ ｶﾞ ｵﾜｯﾀﾄｷ ﾉ ﾓﾄﾞﾘｻｷ
    int     leng;               //  1 ﾉｺﾘ LENGTH
    int     qdat;               //  1 gatetime (q/Q値を計算した値)
    uint32_t fnum;              //  2 ｴﾝｿｳﾁｭｳ ﾉ BLOCK/FNUM
    int     detune;             //  2 ﾃﾞﾁｭｰﾝ
    int     lfodat;             //  2 LFO DATA
    int     porta_num;          //  2 ポルタメントの加減値（全体）
    int     porta_num2;         //  2 ポルタメントの加減値（一回）
    int     porta_num3;         //  2 ポルタメントの加減値（余り）
    int     volume;             //  1 VOLUME
    int     shift;              //  1 ｵﾝｶｲ ｼﾌﾄ ﾉ ｱﾀｲ
    int     delay;              //  1 LFO   [DELAY] 
    int     speed;              //  1   [SPEED]
    int     step;               //  1   [STEP]
    int     time;               //  1   [TIME]
    int     delay2;             //  1   [DELAY_2]
    int     speed2;             //  1   [SPEED_2]
    int     step2;              //  1   [STEP_2]
    int     time2;              //  1   [TIME_2]
    int     lfoswi;             //  1 LFOSW. B0/tone B1/vol B2/同期 B3/porta
                                //           B4/tone B5/vol B6/同期
    int     volpush;            //  1 Volume PUSHarea
    int     mdepth;             //  1 M depth
    int     mdspd;              //  1 M speed
    int     mdspd2;             //  1 M speed_2
    int     envf;               //  1 PSG ENV. [START_FLAG] / -1でextend
    int     eenv_count;         //  1 ExtendPSGenv/No=0 AR=1 DR=2 SR=3 RR=4
    int     eenv_ar;            //  1   /AR     /旧pat
    int     eenv_dr;            //  1   /DR     /旧pv2
    int     eenv_sr;            //  1   /SR     /旧pr1
    int     eenv_rr;            //  1   /RR     /旧pr2
    int     eenv_sl;            //  1   /SL
    int     eenv_al;            //  1   /AL
    int     eenv_arc;           //  1   /ARのカウンタ   /旧patb
    int     eenv_drc;           //  1   /DRのカウンタ
    int     eenv_src;           //  1   /SRのカウンタ   /旧pr1b
    int     eenv_rrc;           //  1   /RRのカウンタ   /旧pr2b
    int     eenv_volume;        //  1   /Volume値(0〜15)/旧penv
    int     extendmode;         //  1 B1/Detune B2/LFO B3/Env Normal/Extend
    int     fmpan;              //  1 FM Panning + AMD + PMD
    int     psgpat;             //  1 PSG PATTERN [TONE/NOISE/MIX]
    int     voicenum;           //  1 音色番号
    int     loopcheck;          //  1 ループしたら１ 終了したら３
    int     carrier;            //  1 FM Carrier
    int     slot1;              //  1 SLOT 1 ﾉ TL
    int     slot3;              //  1 SLOT 3 ﾉ TL
    int     slot2;              //  1 SLOT 2 ﾉ TL
    int     slot4;              //  1 SLOT 4 ﾉ TL
    int     slotmask;           //  1 FM slotmask
    int     neiromask;          //  1 FM 音色定義用maskdata
    int     lfo_wave;           //  1 LFOの波形
    uint8_t partmask;           //  1 PartMask b0:通常 b1:効果音 b2:NECPCM用
                                //     b3:none b4:PPZ/ADE用 b5:s0時 b6:m b7:一時
    int     keyoff_flag;        //  1 KeyoffしたかどうかのFlag
    int     volmask;            //  1 音量LFOのマスク
    int     qdata;              //  1 qの値
    int     qdatb;              //  1 Qの値
    int     hldelay;            //  1 HardLFO delay
    int     hldelay_c;          //  1 HardLFO delay Counter
    int     _lfodat;            //  2 LFO DATA
    int     _delay;             //  1 LFO   [DELAY] 
    int     _speed;             //  1       [SPEED]
    int     _step;              //  1       [STEP]
    int     _time;              //  1       [TIME]
    int     _delay2;            //  1       [DELAY_2]
    int     _speed2;            //  1       [SPEED_2]
    int     _step2;             //  1       [STEP_2]
    int     _time2;             //  1       [TIME_2]
    int     _mdepth;            //  1 M depth
    int     _mdspd;             //  1 M speed
    int     _mdspd2;            //  1 M speed_2
    int     _lfo_wave;          //  1 LFOの波形
    int     _volmask;           //  1 音量LFOのマスク
    int     mdc;                //  1 M depth Counter (変動値)
    int     mdc2;               //  1 M depth Counter
    int     _mdc;               //  1 M depth Counter (変動値)
    int     _mdc2;              //  1 M depth Counter
    int     onkai;              //  1 演奏中の音階データ (0ffh:rest)
    int     sdelay;             //  1 Slot delay
    int     sdelay_c;           //  1 Slot delay counter
    int     sdelay_m;           //  1 Slot delay Mask
    int     alg_fb;             //  1 音色のalg/fb
    int     keyon_flag;         //  1 新音階/休符データを処理したらinc
    int     qdat2;              //  1 q 最低保証値
    int     onkai_def;          //  1 演奏中の音階データ (転調処理前 / ?fh:rest)
    int     shift_def;          //  1 マスター転調値
    int     qdat3;              //  1 q Random
} QQ;

/* TODO: Sort through all of these variables:
 *       - A bunch are likely not needed anymore.
 *       - Most do not need to be 32 bits wide,
 *         in fact most of them fit easily within 8 bits.
 *       - Pretty sure most of these should not be signed.
 */
typedef struct OpenWorktag {
    QQ *MusPart[NumOfAllPart];  // パートワークのポインタ
    uint8_t   *mmlbuf;            //  Musicdataのaddress+1
    uint8_t   *tondat;            //  Voicedataのaddress
    uint8_t   *efcdat;            //  FM  Effecdataのaddress
    uint8_t   *prgdat_adr;        //  曲データ中音色データ先頭番地
    uint16_t *radtbl;           //  R part offset table 先頭番地
    uint8_t   *rhyadr;            //  R part 演奏中番地
    int     rhythmmask;         //  Rhythm音源のマスク x8c/10hのbitに対応
    int     fm_voldown;         //  FM voldown 数値
    int     ssg_voldown;        //  PSG voldown 数値
    int     rhythm_voldown;     //  RHYTHM voldown 数値
    uint8_t   prg_flg;            //  曲データに音色が含まれているかflag
    int     status;             //  status1
    int     status2;            //  status2
    int     tempo_d;            //  tempo (TIMER-B)
    int     tempo_d_push;       //  tempo (TIMER-B) / 保存用
    int     syousetu_lng;       //  小節の長さ
    int     opncount;           //  最短音符カウンタ
    int     TimerAtime;         //  TimerAカウンタ
    int     lastTimerAtime;     //  一個前の割り込み時のTimerATime値
    int     effflag;            //  PSG効果音発声on/off flag(ユーザーが代入)
    int     psnoi;              //  PSG noise周波数
    int     psnoi_last;         //  PSG noise周波数(最後に定義した数値)
    int     rshot_dat;          //  リズム音源 shot flag
    int     rdat[6];            //  リズム音源 音量/パンデータ
    int     rhyvol;             //  リズムトータルレベル
    int     kshot_dat;          //  ＳＳＧリズム shot flag
    int     play_flag;          //  play flag
    int     slot_detune1;       //  FM3 Slot Detune値 slot1
    int     slot_detune2;       //  FM3 Slot Detune値 slot2
    int     slot_detune3;       //  FM3 Slot Detune値 slot3
    int     slot_detune4;       //  FM3 Slot Detune値 slot4
    int     TimerB_speed;       //  TimerBの現在値(=ff_tempoならff中)
    int     syousetu;           //  小節カウンタ
    int     port22h;            //  OPN-PORT 22H に最後に出力した値(hlfo)
    int     tempo_48;           //  現在のテンポ(clock=48 tの値)
    int     tempo_48_push;      //  現在のテンポ(同上/保存用)
    int     _fm_voldown;        //  FM voldown 数値 (保存用)
    int     _ssg_voldown;       //  PSG voldown 数値 (保存用)
    int     _rhythm_voldown;    //  RHYTHM voldown 数値 (保存用)
    int     ch3mode;            //  ch3 Mode
    unsigned char TimerAflag;           //  TimerA割り込み中？フラグ（＠不要？）
    unsigned char TimerBflag;           //  TimerB割り込み中？フラグ（＠不要？）
    unsigned int lfg_state[64];
    unsigned int lfg_index;

    /* This is from the old PMDWORK struct. */
    int     partb;              //  処理中パート番号
    int     tieflag;            //  &のフラグ(1 : tie)
    int     volpush_flag;       //  次の１音音量down用のflag(1 : voldown)
    int     rhydmy;             //  R part ダミー演奏データ
    int     fmsel;              //  FM 表(=0)か裏(=0x100)か flag
    int     omote_key[3];       //  FM keyondata表
    int     ura_key[3];         //  FM keyondata裏
    int     loop_work;          //  Loop Work
    unsigned char    ppsdrv_flag;        //  ppsdrv を使用するか？flag(ユーザーが代入)
    int     music_flag;         //  B0:次でMSTART 1:次でMSTOP のFlag
    int     slotdetune_flag;    //  FM3 Slot Detuneを使っているか
    int     slot3_flag;         //  FM3 Slot毎 要効果音モードフラグ
    int     fm3_alg_fb;         //  FM3chの最後に定義した音色のalg/fb
    int     af_check;           //  FM3chのalg/fbを設定するかしないかflag
    int     lfo_switch;         //  局所LFOスイッチ

    // for PMDWin
    unsigned int rate;               //  PCM 出力周波数(11k, 22k, 44k, 55k)
    unsigned char    fmcalc55k;                          // FM で 55kHz 合成をするか？
} OPEN_WORK;

//=============================================================================
//  PMDWin class
//=============================================================================

typedef struct _PMDWIN
{
    OPNA       opna;
    
    OPEN_WORK open_work;
    QQ FMPart[NumOfFMPart], SSGPart[NumOfSSGPart], RhythmPart;
    
    EFFWORK effwork;
    short wavbuf2[nbufsample];
    
    char    *pos2;                      // buf に余っているサンプルの先頭位置
    uint32_t us2;                       // buf に余っているサンプル数
    uint64_t upos;                      // 演奏開始からの時間(μs)
    uint8_t mdataarea[mdata_def*1024];
    uint8_t pmdstatus[9];

    int uRefCount;      // 参照カウンタ
} PMDWIN;

//============================================================================
//  DLL Export Functions
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

DLLEXPORT int getversion(void);
DLLEXPORT PMDWIN *pmdwininit(void);
DLLEXPORT void pmdwinfree(PMDWIN *pmd);
DLLEXPORT void setpcmrate(PMDWIN *pmd, unsigned int rate);
DLLEXPORT int music_load3(PMDWIN *pmd, uint8_t *musdata, unsigned int size);
DLLEXPORT void music_start(PMDWIN *pmd);
DLLEXPORT void music_stop(PMDWIN *pmd);
DLLEXPORT void getpcmdata(PMDWIN *pmd, short *buf, uint32_t nsamples);
DLLEXPORT void setfmcalc55k(PMDWIN *pmd, unsigned char flag);
DLLEXPORT unsigned int getstatus(PMDWIN *pmd, char *buf, unsigned int bufsize);
DLLEXPORT void setdevmask(PMDWIN *pmd, uint8_t mask);
DLLEXPORT void setchanmask(PMDWIN *pmd, uint32_t mask);
DLLEXPORT char * getmemo(PMDWIN *pmd, char *dest, uint8_t *musdata, int size, int al);
DLLEXPORT char *_getmemo3(PMDWIN *pmd, char *dest, uint8_t *musdata, int size, int al);
DLLEXPORT int maskon(PMDWIN *pmd, unsigned int ch);
DLLEXPORT int maskoff(PMDWIN *pmd, unsigned int ch);
DLLEXPORT void setfmvoldown(PMDWIN *pmd, int voldown);
DLLEXPORT void setssgvoldown(PMDWIN *pmd, int voldown);
DLLEXPORT void setrhythmvoldown(PMDWIN *pmd, int voldown);
DLLEXPORT int getfmvoldown(PMDWIN *pmd);
DLLEXPORT int getfmvoldown2(PMDWIN *pmd);
DLLEXPORT int getssgvoldown(PMDWIN *pmd);
DLLEXPORT int getssgvoldown2(PMDWIN *pmd);
DLLEXPORT int getrhythmvoldown(PMDWIN *pmd);
DLLEXPORT int getrhythmvoldown2(PMDWIN *pmd);
DLLEXPORT void setpos(PMDWIN *pmd, uint64_t pos);
DLLEXPORT void setpos2(PMDWIN *pmd, uint32_t pos);
DLLEXPORT uint64_t getpos(PMDWIN *pmd);
DLLEXPORT uint32_t getpos2(PMDWIN *pmd);
DLLEXPORT unsigned char getlength(PMDWIN *pmd, uint32_t *length, uint32_t *loop);
DLLEXPORT int getloopcount(PMDWIN *pmd);

#ifdef __cplusplus
}
#endif


#endif // PMDWIN_H
