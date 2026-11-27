; race.inc: 1 ステージを走る（pokerace.asm から INCLUDE する）
;
; race を呼ぶ前に stage と car を決めておく。戻ると race_state が RS_GOAL（ゴール）か RS_OVER（時間切れ）。
; ON/BRK なら C = 1 で戻る。demo = 1 のときは自動で走り、demo_t のコマ数か SPACE で戻る（demo_hit = 1）。
;
; 速さ speed は 0〜252（× 1.25 が km/h）。1 コマに進む量 = km/h ÷ 4 × タイマの回数（nt）。
; 時間、加速、ハンドル、カーブのふくらみは、みなタイマの回数に合わせる。

race:   CALL  load_stage
        CALL  load_car
        CALL  race_init
        CALL  make_back
        MV    A,[demo]
        CMP   A,0
        JRNZ  race_loop
        MV    A,3*TPS           ; 3、2、1 と数えてから走り出す
        MV    [cdown],A
race_loop:
        CALL  pad_game
        TEST  (pad_now),PAD_BREAK
        JPNZ  race_break
        CALL  calc_step
        MV    A,[count]         ; コマの数（点滅、計器の順番）
        INC   A
        MV    [count],A
        MV    A,[demo]
        CMP   A,0
        JRZ   race_1
        TEST  (pad_new),PAD_A   ; タイトルの後ろで走っている: SPACE で始める
        JRZ   race_0
        MV    A,1
        MV    [demo_hit],A
        JP    race_end
race_0: MV    A,[demo_t]
        DEC   A
        MV    [demo_t],A
        JPZ   race_end
        CALL  autopilot
race_1: MV    A,[cdown]
        CMP   A,0
        JRZ   race_2
        CALL  countdown
        JR    race_4
race_2: MV    A,[race_state]
        CMP   A,RS_RUN
        JRZ   race_3
        CALL  autopilot         ; ゴールや時間切れの後は、自動で流す
        MV    A,[goal_t]
        DEC   A
        MV    [goal_t],A
        JPZ   race_end
race_3: CALL  control
        CALL  advance
        CALL  cars_move
        CALL  items_move
race_4: CALL  sound
        CALL  frame
        CALL  show_msg
        CALL  show
        CALL  frame_wait
        JP    race_loop
race_end:
        CALL  snd_stop
        RC
        RET
race_break:
        CALL  snd_stop
        SC
        RET

; 音を止める
snd_stop:
        MV    A,0
        MV    [sfx_on],A
        MV    (BP+eng_step),0
        JP    sound_off

; ---- 初めの値 ----
race_init:
        MV    A,0
        MV    X,race_vars       ; race_vars〜race_vars_end を 0 に
        MV    I,race_vars_end-race_vars
race_init0:
        MV    [X++],A
        DEC   I
        JRNZ  race_init0
        MV    A,MINTICK
        MV    [nt],A
        MV    A,$FF
        MV    [sky_last],A
        MV    [pdirty],A
        MV    A,1
        MV    [text_prev],A
        MV    [row0_dirty],A
        MV    A,$FF             ; 計器をすべて描き直させる
        MV    X,hud_last
        MV    IL,8
race_init1:
        MV    [X++],A
        DEC   IL
        JRNZ  race_init1
        MV    A,1
        MV    [score_dirty],A
        MV    A,0               ; アイテムはない
        MV    [items+5],A
        MV    [items+11],A
        MV    A,[st_time]
        MV    [time],A
        MV    A,[st_irate]
        MV    [item_t],A
        MV    BA,[st_cpd]
        MV    [next_cp],BA
        MV    X,[st_track]
        MV    [seg_ptr],X
        CALL  seg_len
        MV    [seg_rem],BA
        CALL  make_map
        ; 進み具合の段: 1 段 = ステージの長さ ÷ 37
        MV    A,[st_secs]
        MV    (BP+cnt),A
        MV    BA,0
        MV    I,[st_cpd]
race_init2:
        ADD   BA,I
        DEC   (BP+cnt)
        JRNZ  race_init2
        MV    I,37
        MV    (BP+t0),0
        MV    (BP+t0+1),0
race_init3:
        SUB   BA,I
        JRC   race_init4
        MV    (BP+t1),BA
        MV    BA,(BP+t0)
        INC   BA
        MV    (BP+t0),BA
        MV    BA,(BP+t1)
        JR    race_init3
race_init4:
        MV    BA,(BP+t0)
        MV    [bar_unit],BA
        ; ほかの車: 間をあけて並べる
        MV    X,rivals
        MV    BA,1500
        MV    [t_z],BA
        MV    (cn),CARS
race_init5:
        PUSHU X
        CALL  respawn
        POPU  X
        MV    BA,[t_z]
        MV    [X],BA
        MV    I,1300
        ADD   BA,I
        MV    [t_z],BA
        MV    A,6
        ADD   X,A
        DEC   (cn)
        JRNZ  race_init5
        RET

; X の車を前の遠くに出し直す（奥行き 4000〜6040、車線、速さ、車種は乱数）。
respawn:
        PUSHU X
        CALL  random
        POPU  X
        MV    (t0),A
        MV    (t0+1),0
        MV    BA,(t0)           ; 奥行き 4000〜6040
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA
        MV    I,4000
        ADD   BA,I
        MV    [X],BA
        CALL  random_lane
        MV    (t1),BA           ; もう 1 台と同じ車線なら隣の車線へ（2 回まで）
        MV    (cnt),2
respawn0:
        MV    BA,[rivals+2]
        CMPW  (t1),BA
        JRZ   respawn0a
        MV    BA,[rivals+8]
        CMPW  (t1),BA
        JRNZ  respawn0b
respawn0a:
        MV    BA,(t1)
        MV    I,480
        ADD   BA,I
        MV    I,481             ; 480 を越えたら -480 へ
        SUB   BA,I
        MV    BA,(t1)
        MV    I,480
        JRC   respawn0c
        MV    BA,$10000-480-480
respawn0c:
        ADD   BA,I
        MV    (t1),BA
        DEC   (cnt)
        JRNZ  respawn0
respawn0b:
        MV    BA,(t1)
        MV    [X+2],BA
        MV    A,[st_crange]
        MV    (t0),A
        MV    IL,(t0)
        PUSHU X
        CALL  random_below
        POPU  X
        MV    (t0),A
        MV    A,[st_cbase]
        ADD   A,(t0)
        MV    [X+4],A
        PUSHU X
        CALL  random
        POPU  X
        AND   A,1
        MV    [X+5],A
        RET

; BA = 乱数の車線（-480、0、480）。X は変えない。
random_lane:
        PUSHU X
        MV    IL,3
        CALL  random_below
        POPU  X
        MV    (t0),A            ; BA に入れると A が変わるので先に取っておく
        MV    BA,0
        CMP   (t0),0
        JRZ   random_lane1
        MV    BA,480
        CMP   (t0),1
        JRZ   random_lane1
        MV    BA,$10000-480
random_lane1:
        RET

; ---- 時間 ----
; 1 コマで進む量 = (速さ + 速さ / 4) / 4 × 前のコマにかかったタイマの回数
calc_step:
        MV    A,[nt]
        MV    (cnt),A
        MV    A,[speed]
        MV    (t0),A
        RC
        SHR   A
        RC
        SHR   A
        ADD   A,(t0)            ; km/h（255 を越えると C）
        MV    (t0),A
        MV    A,0
        JRNC  calc_step0
        INC   A
calc_step0:
        MV    (t0+1),A
        MV    BA,(t0)           ; ÷ 4
        RC
        EX    A,B
        SHR   A
        EX    A,B
        SHR   A
        RC
        EX    A,B
        SHR   A
        EX    A,B
        SHR   A
        MV    I,BA
        MV    BA,0
calc_step1:
        ADD   BA,I
        DEC   (cnt)
        JRNZ  calc_step1
        MV    [stp],BA
        RET

; 16 ミリ秒のタイマが来ていたら数える。タイマの印は 1 回分しか残らないので、16 ミリ秒より
; 短い間隔で呼ぶ（道のライン、物、車、LCD に送る行ごと）。A とフラグのほかは変えない。
; エンジンの音もここで鳴らす: 呼ばれるたびに (eng_acc) += (eng_step)、あふれたらブザーを反転。
; 呼ばれる間隔はふぞろい（1 コマに 40 回ほど）なので、少しざらついたエンジンらしい音になる。
tick_poll:
        PUSHS A                 ; U は LCD に送る間ポインタに使うので、S に積む
        MV    A,(BP+eng_step)
        CMP   A,0
        JRZ   tick_poll0
        ADD   A,(BP+eng_acc)
        MV    (BP+eng_acc),A
        JRNC  tick_poll0
        XOR   (SCR),$10
tick_poll0:
        TEST  (ISR),MTI
        JRZ   tick_poll1
        AND   (ISR),$FF-MTI
        MV    A,[ticks]
        INC   A
        MV    [ticks],A
tick_poll1:
        POPS  A
        RET

; コマの初めから MINTICK 回たつまで待ち、たった回数を nt に入れる（6 回まで）。
frame_wait:
        CALL  tick_poll
        MV    A,[ticks]
        CMP   A,MINTICK
        JRNC  frame_wait0
        MV    I,600             ; 処理の間と同じくらいの間隔でエンジンの音を進める
        WAIT
        JR    frame_wait
frame_wait0:
        CMP   A,7
        JRC   frame_wait1
        MV    A,6
frame_wait1:
        MV    [nt],A
        MV    A,0
        MV    [ticks],A
        RET

; スタートの 3、2、1。0 になったら GO!
countdown:
        MV    A,[nt]
        MV    (t0),A
        MV    A,[cdown]
        SUB   A,(t0)
        JRNC  countdown1
        MV    A,0
countdown1:
        MV    [cdown],A
        MV    (t0),3            ; いまの数字
        CMP   A,2*TPS+1
        JRNC  countdown2
        MV    (t0),2
        CMP   A,TPS+1
        JRNC  countdown2
        MV    (t0),1
        CMP   A,0
        JRNZ  countdown2
        MV    (t0),0
countdown2:
        MV    A,[cd_last]       ; 数字が変わったら鳴らす
        CMP   (t0),A
        JRZ   countdown4
        MV    A,(t0)
        MV    [cd_last],A
        ADD   A,'0'
        MV    [cd_str],A
        CMP   A,'0'
        JRZ   countdown3
        MV    Y,sfx_count
        JP    sfx
countdown3:
        MV    X,s_go            ; GO!
        MV    A,1
        CALL  set_msg
        MV    Y,sfx_go
        JP    sfx
countdown4:
        RET

; 知らせを出す: X = 文字、A = 1 なら大きい字（0、1 行）、0 なら小さい字（0 行）
set_msg:
        MV    [msg_big],A
        MV    [msg_ptr],X
        MV    A,40
        MV    [msg],A
        RET

; ---- キーを読む: pad_read に、テンキーの 4（左）と 6（右）を足す（矢印と併用） ----
pad_game:
        CALL  pad_read
        MV    (KOL),$20         ; KO5: 4 は KI5
        NOP
        TEST  (KI),$20
        JRZ   pad_game1
        OR    (pad_now),PAD_LEFT
pad_game1:
        MV    (KOL),$80         ; KO7: 6 は KI5
        NOP
        TEST  (KI),$20
        JRZ   pad_game2
        OR    (pad_now),PAD_RIGHT
pad_game2:
        MV    (KOL),0
        MV    A,(pad_old)       ; 新しく押されたもの = 今 かつ 前でない
        XOR   A,$FF
        AND   A,(pad_now)
        MV    (pad_new),A
        RET

; ---- 自動で走る（タイトルの後ろ、ゴールの後）: 道の真ん中に戻るようにハンドルを切る ----
autopilot:
        MV    (pad_now),0
        MV    BA,[plat]
        MV    (t0),BA
        TEST  (t0+1),$80
        JRNZ  autopilot1
        MV    I,60              ; 右にいる
        SUB   BA,I
        JRC   autopilot2
        MV    (pad_now),PAD_LEFT
        JR    autopilot2
autopilot1:
        MV    I,60              ; 左にいる
        ADD   BA,I
        MV    (t0),BA
        TEST  (t0+1),$80
        JRZ   autopilot2
        MV    (pad_now),PAD_RIGHT
autopilot2:
        MV    A,[race_state]    ; 時間切れの後はブレーキ
        CMP   A,RS_OVER
        JRZ   autopilot4
        CALL  curve_now         ; きついカーブ（±50 以上）で速すぎれば（150 より上）ブレーキ
        MV    A,(vtmp)
        TEST  A,$80
        JRZ   autopilot2a
        XOR   A,$FF
        INC   A
autopilot2a:
        CMP   A,50
        JRC   autopilot3
        MV    A,[speed]
        CMP   A,151
        JRC   autopilot3
autopilot4:
        OR    (pad_now),PAD_DOWN
autopilot3:
        RET

; ---- キーで速さとハンドル（車の性能とタイマの回数 nt に合わせる） ----
control:
        MV    A,[nt]
        MV    (cnt),A
        MV    A,0
        MV    [brake],A
        MV    A,[c_top]         ; 最高速度（ターボ中は上がる）
        MV    (t1+1),A
        MV    A,[turbo]
        CMP   A,0
        JRZ   control0
        ADD   (t1+1),TURBO_V
control0:
        MV    A,[speed]
        TEST  (pad_now),PAD_DOWN+PAD_A ; ブレーキは ▼ か SPACE
        JRNZ  control2
        MV    (t0),A
        CMP   (t1+1),A          ; 最高速度より速ければ、少しずつ落とす
        JRNC  control0a
        SUB   A,(cnt)
        JR    control4
control0a:
        MV    A,[crash]         ; 当たった直後は加速しない
        CMP   A,0
        MV    A,(t0)
        JRNZ  control4
        MV    A,[c_acc]         ; 小数 += 加速 × 回数（ターボ中は 2 倍）
        MV    (t1),A
        MV    A,[turbo]
        CMP   A,0
        JRZ   control0b
        MV    A,(t1)
        ADD   A,A
        MV    (t1),A
control0b:
        MV    A,(cnt)
        MV    (ln_d),A
        MV    A,[spdf]
control0c:
        ADD   A,(t1)
        DEC   (ln_d)
        JRNZ  control0c
        MV    (t1),A            ; 速さ += 小数 ÷ 8
        AND   A,7
        MV    [spdf],A
        MV    A,(t1)
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        ADD   A,(t0)
        JRC   control1
        CMP   (t1+1),A
        JRNC  control4
control1:
        MV    A,(t1+1)
        JR    control4
control2:
        CMP   A,0               ; ブレーキ: 1 回に 3
        JRZ   control4
        MV    (t0),A
        MV    A,1
        MV    [brake],A
        MV    A,(cnt)
        ADD   A,A
        ADD   A,(cnt)
        MV    (t1),A
        MV    A,(t0)
        SUB   A,(t1)
        JRNC  control4
        MV    A,0
control4:
        MV    (t0),A
        MV    A,[offroad]
        CMP   A,0
        MV    A,(t0)
        JRZ   control5
        CMP   A,OFFSPD+1        ; 道の外は遅くなる（1 回に 2）
        JRC   control5
        SUB   A,(cnt)
        SUB   A,(cnt)
        JRC   control4a
        CMP   A,OFFSPD
        JRNC  control5
control4a:
        MV    A,OFFSPD
control5:
        MV    [speed],A
        ; ハンドル: 1 回に (速さ / 32 + 車のハンドル)、止まっていれば曲がらない
        MV    (t1),0
        MV    (t1+1),0
        CMP   A,0
        JRZ   control6
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        MV    (t0),A
        MV    A,[c_steer]
        ADD   A,(t0)
        MV    (t0),A
        MV    A,(cnt)
        MV    (ln_d),A
control5a:
        MV    A,(t1)
        ADD   A,(t0)
        MV    (t1),A
        DEC   (ln_d)
        JRNZ  control5a
control6:
        MV    A,0
        MV    [steer],A
        MV    BA,[plat]
        MV    I,(t1)
        TEST  (pad_now),PAD_LEFT
        JRZ   control7
        SUB   BA,I
        MV    (t0),A
        MV    A,1
        MV    [steer],A
        MV    A,(t0)
control7:
        TEST  (pad_now),PAD_RIGHT
        JRZ   control8
        ADD   BA,I
        MV    (t0),A
        MV    A,2
        MV    [steer],A
        MV    A,(t0)
control8:
        MV    [plat],BA
        ; 山: カーブ × (速さ / 64) × 回数 だけ流す
        CALL  curve_now
        MV    (cc),BA
        MV    A,[speed]
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        MV    (t1),A            ; t1 = 速さ / 16（あとで使う）
        RC
        SHR   A
        RC
        SHR   A
        CALL  curve_times
        MV    I,[bg]
        ADD   BA,I
        MV    [bg],BA
        ; カーブで外へふくらむ: カーブ × (速さ / 16) × 回数 × すべり / 256
        MV    A,(t1)
        CALL  curve_times
        MV    I,BA
        MV    A,[c_grip]
        MV    (t1),A
        MV    BA,0
control8a:
        ADD   BA,I
        DEC   (t1)
        JRNZ  control8a
        EX    A,B               ; A = 上位 + 128 = ふくらむ量 + 128
        XOR   A,$80
        MV    (t0),A
        MV    (t0+1),0
        MV    BA,[plat]
        MV    I,128
        ADD   BA,I
        MV    I,(t0)
        SUB   BA,I
        ; 壁: -WALL〜WALL
        MV    (t0),BA
        MV    I,WALL
        ADD   BA,I
        MV    I,WALL*2+1
        SUB   BA,I
        MV    BA,(t0)
        JRC   control11
        MV    BA,WALL
        TEST  (t0+1),$80
        JRZ   control11
        MV    BA,$10000-WALL
control11:
        MV    [plat],BA
        ; 道の外か: -EDGE〜EDGE の外
        MV    I,EDGE
        ADD   BA,I
        MV    I,EDGE*2+1
        SUB   BA,I
        MV    A,0
        JRC   control12
        MV    A,1
control12:
        MV    [offroad],A
        MV    A,[crash]
        CMP   A,0
        JRZ   control13
        DEC   A
        MV    [crash],A
control13:
        MV    A,[turbo]         ; ターボの残り
        SUB   A,(cnt)
        JRNC  control14
        MV    A,0
control14:
        MV    [turbo],A
        RET

; BA = (cc) × A × nt（符号付き 2 バイト）。t1 は変えない。
curve_times:
        MV    (t0),A
        MV    A,[nt]
        MV    (cnt),A
        MV    BA,0
        CMP   (t0),0
        JRZ   curve_times3
        MV    I,(cc)
curve_times1:
        MV    (ln_d),(t0)
curve_times2:
        ADD   BA,I
        DEC   (ln_d)
        JRNZ  curve_times2
        DEC   (cnt)
        JRNZ  curve_times1
curve_times3:
        MV    A,[nt]            ; cnt を戻す（control が使う）
        MV    (cnt),A
        RET

; BA = X の区間の長さ（表の 1 バイト × 128）
seg_len:
        MV    A,[X]
        MV    B,A
        MV    A,0
        RC
        EX    A,B
        SHR   A
        EX    A,B
        SHR   A
        RET

; BA = いまの区間のカーブ（符号付き 2 バイト）
curve_now:
        MV    X,[seg_ptr]
        MV    A,[X+1]
; A を符号付きで BA に広げる。
sext:   MV    (vtmp),A
        MV    A,0
        TEST  (vtmp),$80
        JRZ   sext1
        MV    A,$FF
sext1:  MV    B,A
        MV    A,(vtmp)
        RET

; X = 次の区間（いまの区間 + 2。長さ 0 ならコースの初め）
next_seg:
        MV    X,[seg_ptr]
        MV    A,2
        ADD   X,A
        MV    A,[X]
        CMP   A,0
        JRNZ  next_seg1
        MV    X,[st_track]
next_seg1:
        RET

; ---- 進む: 位置、区間、距離、時間、チェックポイント、ゴール ----
advance:
        MV    I,[stp]
        MV    BA,[pos]
        ADD   BA,I
        MV    [pos],BA
        MV    BA,[odo]          ; 距離（3 バイト）
        ADD   BA,I
        MV    [odo],BA
        JRNC  advance1
        MV    A,[odo+2]
        INC   A
        MV    [odo+2],A
advance1:
        MV    BA,[seg_rem]      ; 区間の残り
        SUB   BA,I
        JRC   advance2
        JRNZ  advance3
advance2:                       ; 次の区間へ
        MV    (t0),BA
        CALL  next_seg
        MV    [seg_ptr],X
        CALL  seg_len
        MV    I,(t0)
        ADD   BA,I
advance3:
        MV    [seg_rem],BA
        MV    A,[demo]
        CMP   A,0
        JPNZ  advance9
        MV    A,[race_state]
        CMP   A,RS_RUN
        JPNZ  advance9
        ; 距離の点: DIST 1 ごとに 10 点
        MV    A,[odo+1]
        MV    (t0),A
        MV    A,[dist_last]
        CMP   (t0),A
        JRZ   advance3a
        MV    A,(t0)
        MV    [dist_last],A
        MV    BA,$0010
        CALL  add_score
advance3a:
        MV    BA,[odo+1]        ; チェックポイント、ゴール
        MV    I,[next_cp]
        SUB   BA,I
        JRC   advance5
        MV    A,[section]
        INC   A
        MV    [section],A
        MV    (t0),A
        MV    A,[st_secs]
        CMP   (t0),A
        JRC   advance4
        MV    A,RS_GOAL         ; ゴール
        MV    [race_state],A
        MV    A,70
        MV    [goal_t],A
        MV    X,s_goal
        MV    A,1
        CALL  set_msg
        MV    A,70
        MV    [msg],A
        MV    Y,sfx_goal
        JP    sfx
advance4:
        MV    BA,[next_cp]      ; チェックポイント: 時間が増える、1000 点
        MV    I,[st_cpd]
        ADD   BA,I
        MV    [next_cp],BA
        MV    A,[st_bonus]
        MV    (t0),A
        MV    A,[time]
        ADD   A,(t0)
        CMP   A,100
        JRC   advance4a
        MV    A,99
advance4a:
        MV    [time],A
        MV    BA,$1000
        CALL  add_score
        MV    X,s_check
        MV    A,0
        CALL  set_msg
        MV    A,50
        MV    [msg],A
        MV    Y,sfx_check
        CALL  sfx
advance5:
        MV    A,[nt]            ; 時間: タイマ TPS 回で 1 秒
        MV    (t0),A
        MV    A,[tfrac]
        ADD   A,(t0)
        CMP   A,TPS
        JRC   advance8
        SUB   A,TPS
        MV    [tfrac],A
        MV    A,[time]
        DEC   A
        MV    [time],A
        JRZ   advance7
        CMP   A,6               ; 残り 5 秒からは毎秒鳴らす
        JRNC  advance9
        MV    Y,sfx_warn
        JP    sfx
advance7:
        MV    A,RS_OVER         ; 時間切れ
        MV    [race_state],A
        MV    A,60
        MV    [goal_t],A
        MV    X,s_timeup
        MV    A,1
        CALL  set_msg
        MV    A,60
        MV    [msg],A
        RET
advance8:
        MV    [tfrac],A
advance9:
        RET

; ---- 得点を足す: BA = 足す数（BCD 4 桁）。計器に描き直させる ----
add_score:
        MV    (bcd_b+3),A
        MV    A,B
        MV    (bcd_b+2),A
        MV    (bcd_b+1),0
        MV    (bcd_b),0
        MV    I,4
        MVL   (bcd_a),[score]
        RC
        MV    I,4
        DADL  (bcd_a+3),(bcd_b+3)
        MV    I,4
        MVL   [score],(bcd_a)
        MV    A,1
        MV    [score_dirty],A
        RET

; ---- ほかの車を動かす、追い越し、当たり ----
cars_move:
        MV    BA,[stp]
        MV    (pl),BA           ; pl = 自分の進む量
        MV    X,rivals
        MV    (cn),CARS
cars_move1:
        MV    A,[nt]            ; 車の進む量 = 車の速さ × 回数
        MV    (cnt),A
        MV    A,[X+4]
        MV    (t0),A
        MV    (t0+1),0
        MV    BA,[X]
        MV    (t1),BA
        MV    I,(t0)
cars_move1a:
        ADD   BA,I
        DEC   (cnt)
        JRNZ  cars_move1a
        MV    I,(pl)
        SUB   BA,I
        MV    [X],BA
        ; 追い越した（0 以上 → 負）、追い越された（負 → 0 以上）
        MV    A,B
        XOR   A,(t1+1)
        TEST  A,$80
        JRZ   cars_move3
        TEST  (t1+1),$80
        JRNZ  cars_move2
        MV    A,[demo]
        CMP   A,0
        JRNZ  cars_move3
        MV    A,[passes]        ; 追い越した: 100 点
        INC   A
        JRZ   cars_move1b
        MV    [passes],A
cars_move1b:
        PUSHU X
        MV    BA,$0100
        CALL  add_score
        MV    Y,sfx_pass
        CALL  sfx
        POPU  X
        JR    cars_move3
cars_move2:
        MV    A,[passes]        ; 追い越された
        CMP   A,0
        JRZ   cars_move3
        DEC   A
        MV    [passes],A
cars_move3:
        MV    BA,[X]
        MV    (t1),BA
        TEST  (t1+1),$80
        JRZ   cars_move5
        MV    I,800             ; 後ろへ遠く離れたら、前に出し直す
        ADD   BA,I
        MV    (t0),BA
        TEST  (t0+1),$80
        JRZ   cars_move9
        JR    cars_move6
cars_move5:
        MV    I,FAR             ; 前へ遠く離れたら、出し直す
        SUB   BA,I
        JRC   cars_move7
cars_move6:
        PUSHU X
        CALL  respawn
        POPU  X
        JR    cars_move9
cars_move7:
        MV    BA,(t1)           ; 当たり: 奥行き 0〜HITZ、横の差が HITX 未満
        MV    I,HITZ
        SUB   BA,I
        JRNC  cars_move9
        MV    A,[crash]
        CMP   A,0
        JRNZ  cars_move9
        MV    BA,[X+2]
        MV    I,[plat]
        SUB   BA,I
        MV    I,HITX-1
        ADD   BA,I
        MV    I,HITX*2-1
        SUB   BA,I
        JRNC  cars_move9
        CALL  crash_now
cars_move9:
        MV    A,6
        ADD   X,A
        DEC   (cn)
        JPNZ  cars_move1
        ; 遠い順に並べる（2 台を比べて入れ替える）
        MV    X,rivals
        JP    cars_order

; X の車に当たった: 遅くなり、相手は前へ押し出される。
crash_now:
        MV    A,[speed]
        RC
        SHR   A
        RC
        SHR   A
        MV    [speed],A
        MV    A,0
        MV    [turbo],A
        MV    A,30
        MV    [crash],A
        MV    Y,sfx_crash
        CALL  sfx
        MV    BA,[X]
        MV    I,500
        ADD   BA,I
        MV    [X],BA
        RET

; X と X+6 の車を、遠いほうが先になるように入れ替える（奥行きは符号付き）。
cars_order:
        MV    BA,[X]
        EX    A,B
        XOR   A,$80
        EX    A,B
        MV    (t0),BA
        MV    BA,[X+6]
        EX    A,B
        XOR   A,$80
        EX    A,B
        CMPW  (t0),BA           ; 前の車のほうが近ければ入れ替える
        JRNC  cars_order1
        MV    I,6
        MVL   (screen_buffer),[X]
        MV    I,6
        MVL   (screen_buffer+6),[X+6]
        MV    I,6
        MVL   [X],(screen_buffer+6)
        MV    Y,X
        MV    A,6
        ADD   Y,A
        MV    I,6
        MVL   [Y],(screen_buffer)
cars_order1:
        RET

; ---- アイテム: ときどき前に出し、近づいたら取る ----
items_move:
        MV    A,[demo]
        CMP   A,0
        JRNZ  items_move1
        MV    A,[nt]            ; 次を出すまで（タイマ ÷ 4 で数える）
        MV    (t0),A
        MV    A,[item_tf]
        ADD   A,(t0)
        MV    (t0),A
        AND   A,3
        MV    [item_tf],A
        MV    A,(t0)
        RC
        SHR   A
        RC
        SHR   A
        MV    (t0),A
        MV    A,[item_t]
        SUB   A,(t0)
        JRNC  items_move0
        MV    A,0
items_move0:
        MV    [item_t],A
        CMP   A,0
        JRNZ  items_move1
        CALL  item_spawn
items_move1:
        MV    X,items
        MV    (cn),NITEMS
items_move2:
        MV    A,[X+5]
        CMP   A,0
        JRZ   items_move9
        MV    BA,[X]            ; 道に止まっている: 自分が進んだ分だけ近づく
        MV    I,[stp]
        SUB   BA,I
        MV    [X],BA
        MV    (t1),BA
        TEST  (t1+1),$80
        JRZ   items_move3
        MV    A,0               ; 通り過ぎた
        MV    [X+5],A
        JR    items_move9
items_move3:
        MV    I,HITZ            ; 取る: 奥行き 0〜HITZ、横の差が 220 未満
        SUB   BA,I
        JRNC  items_move9
        MV    BA,[X+2]
        MV    I,[plat]
        SUB   BA,I
        MV    I,219
        ADD   BA,I
        MV    I,439
        SUB   BA,I
        JRNC  items_move9
        MV    A,0
        MV    [X+5],A
        MV    A,[X+4]
        PUSHU X
        CALL  item_take
        POPU  X
items_move9:
        MV    A,6
        ADD   X,A
        DEC   (cn)
        JRNZ  items_move2
        RET

; 空いている所にアイテムを出す（種類: ターボ 35%、時計 25%、コイン 30%、オイル 10%）。
item_spawn:
        MV    A,[st_irate]
        MV    [item_t],A
        MV    X,items
        MV    A,[X+5]
        CMP   A,0
        JRZ   item_spawn1
        MV    A,6
        ADD   X,A
        MV    A,[X+5]
        CMP   A,0
        JRNZ  item_spawn9
item_spawn1:
        PUSHU X
        CALL  random
        POPU  X
        MV    (t0),A
        MV    (t0+1),0
        MV    BA,(t0)
        ADD   BA,BA
        MV    I,4200
        ADD   BA,I
        MV    [X],BA
        CALL  random_lane
        MV    [X+2],BA
        PUSHU X
        MV    IL,100
        CALL  random_below
        POPU  X
        MV    (t0),0
        CMP   A,35
        JRC   item_spawn2
        MV    (t0),1
        CMP   A,60
        JRC   item_spawn2
        MV    (t0),2
        CMP   A,90
        JRC   item_spawn2
        MV    (t0),3
item_spawn2:
        MV    A,(t0)
        MV    [X+4],A
        MV    A,1
        MV    [X+5],A
item_spawn9:
        RET

; A の種類のアイテムを取った。
item_take:
        CMP   A,0
        JRNZ  item_take1
        MV    A,TURBO_T         ; ターボ
        MV    [turbo],A
        MV    X,s_turbo
        MV    Y,sfx_turbo
        JR    item_take8
item_take1:
        CMP   A,1
        JRNZ  item_take2
        MV    A,[time]          ; 時計: +5 秒
        ADD   A,5
        CMP   A,100
        JRC   item_take1a
        MV    A,99
item_take1a:
        MV    [time],A
        MV    X,s_plus5
        MV    Y,sfx_clock
        JR    item_take8
item_take2:
        CMP   A,2
        JRNZ  item_take3
        MV    BA,$0500          ; コイン: 500 点
        CALL  add_score
        MV    X,s_coin
        MV    Y,sfx_coin
        JR    item_take8
item_take3:
        MV    A,[speed]         ; オイル: 回る、速さ 3/4
        MV    (t0),A
        RC
        SHR   A
        RC
        SHR   A
        MV    (t1),A
        MV    A,(t0)
        SUB   A,(t1)
        MV    [speed],A
        MV    A,20
        MV    [crash],A
        MV    A,0
        MV    [turbo],A
        MV    X,s_oil
        MV    Y,sfx_oil
item_take8:
        CALL  sfx
        MV    A,0
        JP    set_msg

; ---- 音（1 コマに 1 回）----
; 効果音は「音の種類、コマ数」の列（sfx_*）を 1 コマずつ進める。種類 1 = 2 kHz、2 = 4 kHz（ブザーが
; 自分で鳴る）、3 = ざらざら（毎コマ乱数の高さ）、0 = 休み。効果音がないときはエンジンの音:
; 高さ (eng_step) = 30 + 速さ / 2（ターボ中は + 50、道の外ではゆれる）。鳴らすのは tick_poll。
eng_acc EQU   $4E               ; エンジンの音を数える
eng_step EQU  $4F               ; エンジンの音の高さ（0 = 鳴らさない）

sound:  MV    A,[sfx_on]
        CMP   A,0
        JRZ   sound_eng
        MV    A,[sfx_n]
        CMP   A,0
        JRNZ  sound2
        MV    X,[sfx_ptr]       ; 次の音
        MV    A,[X+1]
        CMP   A,0
        JRZ   sound_end
        MV    [sfx_n],A
        MV    A,[X]
        MV    [sfx_mode],A
        MV    A,2
        ADD   X,A
        MV    [sfx_ptr],X
        AND   (SCR),BZ_OFF
        MV    A,[sfx_mode]
        CMP   A,1
        JRNZ  sound1
        OR    (SCR),BZ_2K
sound1: CMP   A,2
        JRNZ  sound2
        OR    (SCR),BZ_4K
sound2: MV    A,[sfx_n]
        DEC   A
        MV    [sfx_n],A
        MV    (BP+eng_step),0
        MV    A,[sfx_mode]
        CMP   A,3
        JRNZ  sound9
        CALL  random            ; ざらざら: 毎コマ乱数の高さ
        OR    A,$60
        MV    (BP+eng_step),A
        RET
sound_end:
        MV    A,0
        MV    [sfx_on],A
        AND   (SCR),BZ_OFF
sound_eng:
        MV    (BP+eng_step),0
        MV    A,[demo]          ; タイトルの後ろで走るときは鳴らさない
        CMP   A,0
        JRNZ  sound9
        MV    A,[speed]         ; 30 + 速さ / 2
        RC
        SHR   A
        ADD   A,30
        MV    (BP+t0),A
        MV    A,[turbo]
        CMP   A,0
        JRZ   sound_eng1
        ADD   (BP+t0),50
sound_eng1:
        MV    A,[offroad]       ; 道の外: ゆれる
        CMP   A,0
        JRZ   sound_eng2
        CALL  random
        AND   A,31
        ADD   (BP+t0),A
sound_eng2:
        MV    A,(BP+t0)
        MV    (BP+eng_step),A
sound9: RET

; 効果音を鳴らし始める（Y = 列）。X は変えない。
sfx:    MV    [sfx_ptr],Y
        MV    A,1
        MV    [sfx_on],A
        MV    A,0
        MV    [sfx_n],A
        RET

; 効果音: 種類（0 休み、1 = 2 kHz、2 = 4 kHz、3 = ざらざら）、コマ数。コマ数 0 で終わり
sfx_count: DB 1,4,0,0
sfx_go: DB    2,12,0,0
sfx_check: DB 2,3,0,1,2,3,0,1,2,8,0,0
sfx_goal: DB  2,4,0,1,2,4,0,1,1,4,2,14,0,0
sfx_warn: DB  1,3,0,0
sfx_pass: DB  2,1,0,0
sfx_turbo: DB 1,2,2,2,1,2,2,2,1,2,2,6,0,0
sfx_clock: DB 2,2,0,2,2,2,0,0
sfx_coin: DB  2,2,0,1,2,5,0,0
sfx_oil: DB   3,14,0,0
sfx_crash: DB 3,12,1,4,0,0


; ---- 1 コマを描く ----
frame:  CALL  clear_view
        CALL  draw_sky
        CALL  draw_road
        CALL  draw_trees
        MV    A,[demo]
        CMP   A,0
        JRNZ  frame1
        CALL  draw_gate
        CALL  draw_items
frame1: CALL  draw_cars
        CALL  draw_turbo
        CALL  draw_player
        JP    draw_hud

; 道を描く所（2、3 行の VIEW_L〜VIEW_R）を消す。0、1 行は空がすべて上書きする。
clear_view:
        MV    (screen_buffer),0 ; 前から順に写るので、0 が 77 バイトに広がる
        MV    I,76
        MVL   (screen_buffer+1),(screen_buffer)
        MV    Y,screen+480+VIEW_L
        CALL  clear_view1
        MV    Y,screen+720+VIEW_L
clear_view1:
        MV    I,77
        MVL   [Y++],(screen_buffer)
        MV    I,77
        MVL   [Y++],(screen_buffer)
        RET

; 空（ステージの絵）。bg の上位バイトの分だけずれる。
; 0 行は、山が動いたときと、前のコマに字やゲートを描いたときだけ描き直す（LCD にも送らない）。
draw_sky:
        MV    A,[bg+1]
        MV    (BP+t0),A
        MV    A,[sky_last]
        CMP   (BP+t0),A
        JRNZ  draw_sky1
        MV    A,[text_prev]
        CMP   A,0
        JRNZ  draw_sky1
        MV    [row0_dirty],A
        JR    draw_sky2
draw_sky1:
        MV    A,(BP+t0)
        MV    [sky_last],A
        MV    A,0
        MV    [text_prev],A
        MV    A,1
        MV    [row0_dirty],A
        MV    X,[st_sky]
        MV    A,(BP+t0)
        ADD   X,A
        MV    Y,screen+VIEW_L
        CALL  copy154
draw_sky2:
        MV    X,[st_sky]
        MV    BA,410
        ADD   X,BA
        MV    A,[bg+1]
        ADD   X,A
        MV    Y,screen+240+VIEW_L
copy154:
        MV    I,77
        MVL   (screen_buffer),[X++]
        MV    I,77
        MVL   [Y++],(screen_buffer)
        MV    I,77
        MVL   (screen_buffer),[X++]
        MV    I,77
        MVL   [Y++],(screen_buffer)
        RET
