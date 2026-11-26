; ポケレーサー: 後ろから見たレースゲーム（疑似 3D）。8 ステージ、車 3 台、アイテム、ハイスコア
; アクセルは自動。▼ でブレーキ、◀ ▶ でハンドル。ON/BRK で BASIC へ戻る。
;
; ファイル:
;   pokerace.asm   流れ、ステージと車の表、変数（このファイル）
;   screens.inc    タイトル、車の選択、ステージの紹介、集計、GAME OVER、名前の入力、エンディング
;   race.inc       走る処理: キー、進む、ほかの車、アイテム、チェックポイント、ゴール、得点
;   road_draw.inc  道
;   objects.inc    道ばたの物、ゲート、ほかの車、アイテム、自分の車
;   hud.inc        字（自分のフォント）、計器
;   lcd_fast.inc   仮想画面を LCD に送る
;   sound.inc      曲
;   make_data.py が作るもの: フォント、タイトル、車、空、表、文字列、曲、ステージとコースとコースの図
        ORG   $B9400            ; 置くアドレス。BFBFF までに収める
        INCLUDE "hardware.inc"

; ---- 決まった数 ----
VIEW_L  EQU   43                ; 道を描く範囲（x）。左右は計器
VIEW_R  EQU   196
LINES   EQU   22                ; 道のライン数（y = 10〜31）
OFFSPD  EQU   64                ; 道の外での最高速度（× 1.25 が km/h）
EDGE    EQU   768               ; 道の端（横の位置）
WALL    EQU   1100              ; これより外へは出ない
CARS    EQU   2                 ; ほかの車の数
NITEMS   EQU   2                 ; 道に出るアイテムの数
HITZ    EQU   480               ; これより近い車、アイテムと当たる（奥行き）
HITX    EQU   200               ; 横がこれより近い車と当たる
FAR     EQU   7000              ; これより遠くへ行った車は出し直す
MINTICK EQU   2                 ; 1 コマの最短（16 ミリ秒の 2 回）
TPS     EQU   62                ; 1 秒のタイマの回数（16 ミリ秒ごと）
TURBO_T EQU   190               ; ターボの長さ（タイマの回数、約 3 秒）
TURBO_V EQU   40                ; ターボで上がる最高速度
TREE_SP EQU   1024              ; 道ばたの物の間隔（左右それぞれ。右は半分ずらす）
STAGES  EQU   8
RANKS   EQU   4                 ; ハイスコアの数
screen_buffer EQU $60           ; 作業用（60h〜BFh）

; 道ばたの物の種類
OB_PINE EQU   0
OB_PALM EQU   1
OB_CACT EQU   2
OB_BLDG EQU   3
OB_LAMP EQU   4
OB_POST EQU   5
OB_ROCK EQU   6

; 走りの様子（race_state）
RS_RUN  EQU   0                 ; 走っている
RS_GOAL EQU   1                 ; ゴールした
RS_OVER EQU   2                 ; 時間切れ

; ---- 内部 RAM の変数（BP = 0 にして (BP+n) で使う） ----
ln_d    EQU   $00               ; 描いているライン（22〜1）
cx      EQU   $01               ; そのラインの道の中心 + 8
xo      EQU   $02
dx      EQU   $04
acc     EQU   $06
cc      EQU   $08               ; いまの区間のカーブ（2 バイト、符号付き）
nc      EQU   $0A               ; 次の区間のカーブ（2 バイト）
pl      EQU   $0C               ; 作業用（2 バイト）
ra      EQU   $0E               ; 横に引く線の左端 + 8
rb      EQU   $0F               ; 右端 + 8
prev_l  EQU   $10               ; 1 つ手前のラインの左端 + 8
prev_r  EQU   $11               ; 右端 + 8
base    EQU   $12               ; 描く行の先頭 - 8（3 バイト）
stripe  EQU   $15
hw      EQU   $16
rw      EQU   $17
ptr     EQU   $18               ; 3 バイト
cnt     EQU   $1B               ; 数える
t0      EQU   $1C               ; 作業用（2 バイト）
t1      EQU   $1E               ; 作業用（2 バイト）
; 道を描き終えた後（物、ゲート、車、計器）は、上の場所を別の名前で使う
cn      EQU   $00
ob_d    EQU   $00
ob_x    EQU   $01
ob_side EQU   $04
ob_top  EQU   $05
ob_cb   EQU   $06
ob_bot  EQU   $07
ob_w    EQU   $08
ob_st   EQU   $09
ob_j    EQU   $0A
ob_type EQU   $0B
vtmp    EQU   $0D
vx      EQU   $0E
vy0     EQU   $0F
vy1     EQU   $10
vdx     EQU   $11
vtm     EQU   $15
vbm     EQU   $16
; 字、計器、得点、音、メニュー（48h〜5Fh。INCLUDE ファイルは使わない所）
tx      EQU   $48               ; 字の x（ドット）
trow    EQU   $49               ; 字の行（0〜3）
tinv    EQU   $4A               ; 字を反転するとき FFh
tch     EQU   $4B
tval    EQU   $4C               ; 数（2 バイト）
bcd_a   EQU   $50               ; 得点の計算（4 バイト、上の桁が先）
bcd_b   EQU   $54
snd_h   EQU   $58               ; 半周期（2 バイト）
snd_n   EQU   $5A               ; 周期の数（2 バイト）
sel     EQU   $5C               ; メニューの選択
mtmp    EQU   $5D
mtmp2   EQU   $5E

start:  CALL  setup
title:  CALL  title_screen      ; SPACE が押されるまで（ON/BRK なら C = 1）
        JPC   quit
        CALL  select_car
        JPC   quit
        CALL  new_game
stage_go:
        CALL  stage_intro
        CALL  race
        JPC   quit              ; ON/BRK
        MV    A,[race_state]
        CMP   A,RS_GOAL
        JRNZ  game_over
        CALL  stage_clear
        MV    A,[stage]
        INC   A
        MV    [stage],A
        CMP   A,STAGES
        JRC   stage_go
        CALL  ending
        JR    after_game
game_over:
        CALL  over_screen
after_game:
        CALL  enter_name
        JPC   quit
        JP    title

quit:   MV    A,[save_bp]
        MV    ($EC),A
        CALL  game_end
        RC
        RETF

; ---- 始める: 内部 RAM の退避、BP = 0（内部 RAM を (BP+n) の速い形で使う） ----
setup:  CALL  game_start
        MV    A,($EC)
        MV    [save_bp],A
        MV    ($EC),0
        MV    (BP+tinv),0
        MV    (BP+$4F),0        ; エンジンの音を止めておく（eng_step）
        RET

; ---- 新しいゲーム ----
new_game:
        MV    A,0
        MV    [stage],A
        MV    X,score
        MV    [X++],A
        MV    [X++],A
        MV    [X++],A
        MV    [X],A
        RET

; ---- ステージの値を読む（stage → st_*） ----
load_stage:
        MV    A,[stage]
        MV    (BP+t0),A         ; X = stage_tab + stage × 25
        MV    (BP+t0+1),0
        MV    BA,(BP+t0)
        MV    I,BA              ; × 1
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA             ; × 8
        ADD   I,BA              ; × 9
        ADD   BA,BA             ; × 16
        ADD   BA,I              ; × 25
        MV    X,stage_tab
        ADD   X,BA
        MV    Y,st_track
        MV    I,ST_SIZE
        MVL   (screen_buffer),[X++]
        MV    I,ST_SIZE
        MVL   [Y++],(screen_buffer)
        RET

; ---- 選んだ車の値を読む（car → c_*） ----
load_car:
        MV    A,[car]
        MV    X,car_tab
        CMP   A,0
        JRZ   load_car2
load_car1:
        PUSHS A
        MV    A,CAR_SIZE
        ADD   X,A
        POPS  A
        DEC   A
        JRNZ  load_car1
load_car2:
        MV    Y,c_top
        MV    I,CAR_SIZE
        MVL   (screen_buffer),[X++]
        MV    I,CAR_SIZE
        MVL   [Y++],(screen_buffer)
        RET

        INCLUDE "screens.inc"
        INCLUDE "race.inc"
        INCLUDE "road_draw.inc"
        INCLUDE "objects.inc"
        INCLUDE "hud.inc"
        INCLUDE "lcd_fast.inc"
        INCLUDE "sound.inc"

; ---- 車の表（10 バイトずつ）: 最高速度、加速（1 回に ÷ 8）、ハンドル、すべり（1〜3）、
;      表の棒（速さ、加速、グリップ 1〜5）、DP 絵 ----
CAR_SIZE EQU  10
car_tab:
        DB    176,14,9,1,2,5,5
        DP    car0
        DB    192,12,7,2,3,3,3
        DP    car1
        DB    212,9,6,3,5,2,1
        DP    car2
car_names:
        DP    s_car0,s_car1,s_car2
car_descs:
        DP    s_desc0,s_desc1,s_desc2

; ---- 外部メモリの変数 ----
; ステージの値（stage_tab から写す）
st_track: DS  3
st_sky: DS    3
st_name: DS   3
st_sub: DS    3
st_map: DS    3
st_objl: DB   0
st_objr: DB   0
st_time: DB   0
st_bonus: DB  0
st_secs: DB   0
st_cbase: DB  0
st_crange: DB 0
st_irate: DB  0
st_cpd: DW    0
; 車の値（car_tab から写す）
c_top:  DB    0
c_acc:  DB    0
c_steer: DB   0
c_grip: DB    0
c_bars: DS    3
c_spr:  DS    3
; ゲーム
stage:  DB    0                 ; 0〜7
car:    DB    1                 ; 0〜2
score:  DS    4                 ; 得点（BCD、上の桁が先）
hiscores:                       ; 名前 3 字、得点 4 バイト（BCD）× RANKS
        DB    $50,$4F,$4B,$00,$03,$00,$00
        DB    $52,$41,$43,$00,$02,$00,$00
        DB    $45,$35,$30,$00,$01,$00,$00
        DB    $41,$41,$41,$00,$00,$50,$00
; 走り（race_vars〜race_vars_end は race_init で 0 にする）
race_vars:
race_state: DB 0
goal_t: DB    0                 ; ゴールや時間切れの後のコマ
speed:  DB    0                 ; 速さ（× 1.25 が km/h）
spdf:   DB    0                 ; 速さの小数（÷ 8）
plat:   DW    0                 ; 自分の車の横の位置（符号付き、道の端が ±EDGE）
pos:    DW    0                 ; コースの位置（下位 8 ビットは縞の中）
stp:    DW    0                 ; このコマに進む量
seg_ptr: DP   track0            ; いまの区間（長さ ÷ 128、カーブの 2 バイト）
seg_rem: DW   0                 ; 区間の残り
bg:     DW    0                 ; 山のずれ（上位バイトがドット）
odo:    DS    3                 ; 走った距離（上の 2 バイトが DIST）
next_cp: DW   0                 ; 次のチェックポイント（ゴール）の DIST
section: DB   0                 ; いまの区間（0 から。st_secs - 1 がゴールへの区間）
time:   DB    0                 ; 残り時間（秒）
tfrac:  DB    0                 ; 1 秒の中のタイマの回数
nt:     DB    0                 ; 前のコマにかかったタイマの回数
ticks:  DB    0                 ; このコマで過ぎたタイマの回数
cdown:  DB    0                 ; スタートまでのタイマの回数
cd_last: DB   0                 ; 前に出した数字
passes: DB    0                 ; このステージで追い越した数
crash:  DB    0                 ; 当たった後の残りコマ
turbo:  DB    0                 ; ターボの残り（タイマの回数）
item_t: DB    0                 ; 次のアイテムまで
sfx_on: DB    0                 ; 1 = 効果音を鳴らしている
sfx_n:  DB    0                 ; いまの音の残りコマ
sfx_mode: DB  0                 ; いまの音の種類
sfx_ptr: DP   0                 ; 次の音
msg:    DB    0                 ; 知らせの残りコマ
msg_ptr: DP   s_check           ; 知らせの文字
msg_x:  DB    0                 ; 知らせの x
msg_big: DB   0                 ; 1 = 大きい字
steer:  DB    0                 ; 0 まっすぐ、1 左、2 右
brake:  DB    0                 ; 1 = ブレーキ中
offroad: DB   0                 ; 1 = 道の外
count:  DB    0                 ; コマの数
pdirty: DB    0                 ; 送る計器の行（ビット 0〜3 左、4〜7 右）
row0_dirty: DB 1                ; 1 = 0 行も LCD に送る
text_prev: DB 1                 ; 1 = 前のコマで 0、1 行に字やゲートを描いた
sky_last: DB  $FF               ; 前に描いた山のずれ
hud_last: DS  8                 ; 計器に描いた時間、速さ、進み具合、ターボ、点滅
demo_hit: DB  0                 ; 1 = 自動で走っている間に SPACE が押された
dist_last: DB 0                 ; 点を足した DIST（下位バイト）
score_dirty: DB 0               ; 1 = 得点を描き直す
item_tf: DB   0                 ; アイテムの時間の端数
bar_unit: DW  0                 ; 進み具合のバー 1 ドットの DIST
race_vars_end:
demo:   DB    0                 ; 1 = 自動で走る（タイトルの後ろ）
demo_t: DB    0                 ; 自動で走るコマの残り
cd_str: DB    "3",0             ; スタートの数字
save_u: DS    3
save_bp: DB   0
t_z:    DW    0
nbuf:   DS    9                 ; 数を字にする所（8 桁 + 0）
map_buf: DS   80                ; コースの図（40 桁 × 2 行）
lined:  DS    (LINES+1)*2       ; ラインごとの道の中心 + 8、物の位置。d × 2 の位置
rivals: DS    CARS*6            ; 車: 奥行き（2、符号付き）、横の位置（2）、速さ（1）、車種（1）
items:  DS    NITEMS*6           ; アイテム: 奥行き（2）、横の位置（2）、種類（1）、あり（1）
screen: DS    960               ; 仮想画面（240 × 32）

; ---- 絵、字、表 ----
font_s: INCBIN "font_s.bin"
font_b: INCBIN "font_b.bin"
font_m: INCBIN "font_m.bin"
title_pic: INCBIN "title.bin"
car0:   INCBIN "car0.spr"
car1:   INCBIN "car1.spr"
car2:   INCBIN "car2.spr"
car_big: INCBIN "car_big.bin"
dust:   INCBIN "dust.spr"
rival_xs: INCBIN "rival_xs.spr"
rival_s: INCBIN "rival_s.spr"
rival_m: INCBIN "rival_m.spr"
rival_l: INCBIN "rival_l.spr"
item_s: INCBIN "item_s.spr"
item_m: INCBIN "item_m.spr"
item_l: INCBIN "item_l.spr"
sky0:   INCBIN "sky0.bin"
sky1:   INCBIN "sky1.bin"
sky2:   INCBIN "sky2.bin"
sky3:   INCBIN "sky3.bin"
sky4:   INCBIN "sky4.bin"
sky5:   INCBIN "sky5.bin"
sky6:   INCBIN "sky6.bin"
sky7:   INCBIN "sky7.bin"
        INCLUDE "road.inc"
        INCLUDE "stages.inc"
        INCLUDE "text.inc"
        INCLUDE "sound_data.inc"

        INCLUDE "game.inc"
        INCLUDE "sprite.inc"
        INCLUDE "pad.inc"
        INCLUDE "random.inc"
        END
