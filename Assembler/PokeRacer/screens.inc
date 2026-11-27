; screens.inc: タイトル、車の選択、ステージの紹介、集計、GAME OVER、エンディング、名前の入力（pokerace.asm から INCLUDE する）
;
; どの画面も仮想画面に描いてから show_all で送る。ON/BRK が押されたら C = 1 で戻る。

; ---- タイトル: ロゴ → 自動で走る → ランキング をくり返す。SPACE で C = 0、ON/BRK で C = 1 ----
title_screen:
        MV    A,0
        MV    [demo],A
title_s1:
        CALL  copy_title        ; ロゴと曲（1 回。PUSH SPACE とハイスコアを交互に）
        MV    (BP+mtmp2),0
        MV    X,m_title
        MV    [bgm_ptr],X
title_s2:
        MV    A,(BP+mtmp2)      ; 8 音ごとに PUSH SPACE とハイスコアを入れ替える
        AND   A,$07
        JRNZ  title_s4
        CALL  clear_row3
        MV    (BP+trow),3
        TEST  (BP+mtmp2),$08
        JRNZ  title_s3
        MV    X,s_push
        CALL  center_s
        JR    title_s3a
title_s3:
        CALL  hiscore_line
title_s3a:
        CALL  show_all
title_s4:
        CALL  bgm_frame         ; 曲を 1 音鳴らし、キーを見る（曲が終わったら Z = 1、C = 0 で A = 1）
        JRC   title_quit
        JRNZ  title_start
        CMP   A,0
        JRNZ  title_s4a
        INC   (BP+mtmp2)
        JR    title_s2
title_s4a:
        ; 自動で走る（ステージは順に）
        MV    A,[car]
        MV    [car_save],A
        MV    A,1
        MV    [car],A
        MV    [demo],A
        MV    A,[demo_stage]
        MV    [stage],A
        INC   A
        CMP   A,STAGES
        JRC   title_s5
        MV    A,0
title_s5:
        MV    [demo_stage],A
        MV    A,240
        MV    [demo_t],A
        CALL  race
        PUSHS F
        MV    A,[car_save]
        MV    [car],A
        MV    A,0
        MV    [demo],A
        POPS  F
        JRC   title_quit
        MV    A,[demo_hit]
        CMP   A,0
        JRNZ  title_start
        CALL  ranking           ; ランキング（約 4 秒）
        MV    A,120
        CALL  pause
        JRC   title_quit
        JRNZ  title_start
        JP    title_s1
title_start:
        CALL  blip
        RC
        RET
title_quit:
        SC
        RET

; タイトルの絵を仮想画面へ
copy_title:
        MV    X,title_pic
        MV    Y,screen
        MV    (BP+mtmp),10
copy_title1:
        MV    I,96
        MVL   (screen_buffer),[X++]
        MV    I,96
        MVL   [Y++],(screen_buffer)
        DEC   (BP+mtmp)
        JRNZ  copy_title1
        RET

; 3 行目を消す
clear_row3:
        MV    X,screen+720
        MV    IL,240
        MV    A,0
clear_row3a:
        MV    [X++],A
        DEC   IL
        JRNZ  clear_row3a
        RET

; 3 行目に「HI 名前 得点」
hiscore_line:
        MV    (BP+trow),3
        MV    (BP+tx),66
        MV    X,s_hi
        CALL  str_s
        ADD   (BP+tx),6
        MV    X,hiscores
        MV    IL,3
hiscore_line1:
        MV    A,[X++]
        CALL  put_s
        DEC   IL
        JRNZ  hiscore_line1
        ADD   (BP+tx),6
        MV    Y,X
        CALL  score_str_y
        JP    str_s

; ランキングの画面: 左に大きく BEST 4、右に順位、名前、得点
ranking:
        CALL  clear_screen
        MV    (BP+trow),1
        MV    (BP+tx),6
        MV    X,s_best
        CALL  str_b
        MV    X,hiscores
        MV    (BP+mtmp),0
ranking1:
        MV    A,(BP+mtmp)
        MV    (BP+trow),A
        MV    (BP+tx),84
        ADD   A,'1'
        CALL  put_s
        MV    A,'.'
        CALL  put_s
        ADD   (BP+tx),6
        MV    IL,3
ranking2:
        MV    A,[X++]
        CALL  put_s
        DEC   IL
        JRNZ  ranking2
        ADD   (BP+tx),12
        MV    Y,X
        PUSHS X
        CALL  score_str_y
        CALL  str_s
        POPS  X
        MV    A,4
        ADD   X,A
        INC   (BP+mtmp)
        CMP   (BP+mtmp),RANKS
        JRC   ranking1
        JP    show_all

; タイトルの曲を 1 音（休みも 1 つ）鳴らしてキーを見る: ON/BRK なら C = 1、SPACE なら Z = 0、
; 曲が終わったら A = 1
bgm_frame:
        MV    X,[bgm_ptr]
        MV    BA,[X++]
        MV    (BP+snd_h),BA
        MV    BA,[X++]
        MV    (BP+snd_n),BA
        MV    [bgm_ptr],X
        MV    BA,(BP+snd_h)
        INC   BA
        JRNZ  bgm_frame1
        CALL  pad_game          ; 終わり
        TEST  (pad_now),PAD_BREAK
        JRNZ  key_frame1
        RC
        MV    A,1
        TEST  (pad_new),PAD_A
        RET
bgm_frame1:
        MV    A,(BP+snd_h)
        OR    A,(BP+snd_h+1)
        JRNZ  bgm_frame2
        MV    A,(BP+snd_n)      ; 休み
        CALL  game_wait
        JR    bgm_frame3
bgm_frame2:
        CALL  tone
        AND   (SCR),BZ_OFF
bgm_frame3:
        CALL  pad_game
        TEST  (pad_now),PAD_BREAK
        JRNZ  key_frame1
        RC
        MV    A,0
        TEST  (pad_new),PAD_A
        RET

; 1 コマ待ってキーを見る: ON/BRK なら C = 1、SPACE が押されたら Z = 0
key_frame:
        CALL  pad_game
        TEST  (pad_now),PAD_BREAK
        JRNZ  key_frame1
        MV    A,2
        CALL  game_wait
        RC
        TEST  (pad_new),PAD_A
        RET
key_frame1:
        SC
        RET

; A コマ（約 32 ミリ秒ずつ）待つ。ON/BRK なら C = 1、SPACE が押されたら Z = 0
pause:  MV    (BP+mtmp),A
pause1: CALL  key_frame
        JRC   pause2
        JRNZ  pause2
        DEC   (BP+mtmp)
        JRNZ  pause1
        RC                      ; 時間がたった（Z = 1）
pause2: RET

; ---- 車を選ぶ: ◀ ▶ で選び、SPACE で決める ----
select_car:
        MV    A,[car]
        MV    (BP+sel),A
select_car1:
        CALL  draw_select
        CALL  show_all
select_car2:
        CALL  pad_game
        TEST  (pad_now),PAD_BREAK
        JRNZ  select_car9
        TEST  (pad_new),PAD_A
        JRNZ  select_car8
        TEST  (pad_new),PAD_LEFT
        JRZ   select_car3
        DEC   (BP+sel)
        CMP   (BP+sel),$FF
        JRNZ  select_car4
        MV    (BP+sel),2
        JR    select_car4
select_car3:
        TEST  (pad_new),PAD_RIGHT
        JRZ   select_car5
        INC   (BP+sel)
        CMP   (BP+sel),3
        JRC   select_car4
        MV    (BP+sel),0
select_car4:
        CALL  blip
        JR    select_car1
select_car5:
        MV    A,2
        CALL  game_wait
        JR    select_car2
select_car8:
        MV    A,(BP+sel)
        MV    [car],A
        CALL  blip
        RC
        RET
select_car9:
        SC
        RET

; 車を選ぶ画面を描く
draw_select:
        CALL  clear_screen
        MV    A,(BP+sel)        ; 表の値を見るために読む
        MV    [car],A
        CALL  load_car
        MV    (BP+trow),0
        MV    (BP+tx),0
        MV    X,s_select
        CALL  str_s
        MV    (BP+trow),1       ; 矢印
        MV    (BP+tx),0
        MV    A,'<'
        CALL  put_s
        MV    (BP+tx),44
        MV    A,'>'
        CALL  put_s
        MV    X,car_big         ; 大きい車（x = 8〜39、1、2 行）
        MV    A,(BP+sel)
        CMP   A,0
        JRZ   draw_select2
draw_select1:
        PUSHS A
        MV    A,64
        ADD   X,A
        POPS  A
        DEC   A
        JRNZ  draw_select1
draw_select2:
        MV    Y,screen+240+8
        MV    I,32
        MVL   (screen_buffer),[X++]
        MV    I,32
        MVL   [Y++],(screen_buffer)
        MV    Y,screen+480+8
        MV    I,32
        MVL   (screen_buffer),[X++]
        MV    I,32
        MVL   [Y++],(screen_buffer)
        MV    X,car_names       ; 名前（大きい字）
        CALL  sel_ptr
        MV    (BP+trow),1
        MV    (BP+tx),60
        CALL  str_b
        MV    X,car_descs       ; 説明
        CALL  sel_ptr
        MV    (BP+trow),3
        MV    (BP+tx),60
        CALL  str_s
        MV    X,s_spd           ; 性能の棒
        MV    (BP+trow),0
        MV    A,[c_bars]
        CALL  stat_bar
        MV    X,s_acc
        MV    (BP+trow),1
        MV    A,[c_bars+1]
        CALL  stat_bar
        MV    X,s_grp
        MV    (BP+trow),2
        MV    A,[c_bars+2]
        CALL  stat_bar
        MV    (BP+trow),3       ; 何台目か
        MV    (BP+tx),210
        MV    A,(BP+sel)
        ADD   A,'1'
        CALL  put_s
        MV    A,'/'
        CALL  put_s
        MV    A,'3'
        JP    put_s

; X = X の表（DP × 3）の sel 番目の文字列
sel_ptr:
        MV    A,(BP+sel)
        ADD   X,A
        ADD   X,A
        ADD   X,A
        MV    X,[X]
        RET

; 性能の棒: X = 名前、A = 1〜5、(trow) = 行。x = 162 から名前、x = 186 から四角 5 つ
stat_bar:
        MV    (BP+mtmp),A
        MV    (BP+tx),162
        CALL  str_s
        CALL  text_addr
        MV    A,186-162-18
        ADD   Y,A
        MV    (BP+mtmp2),0
stat_bar1:
        MV    A,$3E             ; 埋まった四角（5 × 5）
        MV    (BP+tch),$3E
        CMP   (BP+mtmp2),(BP+mtmp)
        JRC   stat_bar2
        MV    (BP+tch),$22      ; 空の四角
stat_bar2:
        MV    [Y++],A
        MV    A,(BP+tch)
        MV    [Y++],A
        MV    [Y++],A
        MV    [Y++],A
        MV    A,$3E
        MV    [Y++],A
        INC   Y
        INC   (BP+mtmp2)
        CMP   (BP+mtmp2),5
        JRC   stat_bar1
        RET

; ---- ステージの紹介: STAGE n、名前、カナの名前、曲 ----
stage_intro:
        CALL  load_stage
        CALL  clear_screen
        MV    (BP+trow),0
        MV    (BP+tx),78
        MV    X,s_stage
        CALL  str_b
        ADD   (BP+tx),12
        MV    A,[stage]
        ADD   A,'1'
        CALL  put_b
        MV    X,[st_name]
        MV    (BP+trow),2
        CALL  center_s
        MV    X,[st_sub]
        MV    (BP+trow),3
        CALL  center_s
        CALL  show_all
        MV    X,m_start
        CALL  play
        MV    A,40
        JP    game_wait

; ---- ステージクリア: 残り時間 × 100、追い越し × 200 を数えて足す ----
stage_clear:
        CALL  clear_screen
        MV    X,s_clear
        MV    (BP+trow),0
        CALL  center_b
        CALL  show_all
        MV    X,m_clear
        CALL  play
stage_clear1:                   ; 残り時間
        CALL  draw_tally
        CALL  show_all
        MV    A,[time]
        CMP   A,0
        JRZ   stage_clear2
        DEC   A
        MV    [time],A
        MV    BA,$0100
        CALL  add_score
        CALL  tick
        JR    stage_clear1
stage_clear2:                   ; 追い越し
        MV    A,[passes]
        CMP   A,0
        JRZ   stage_clear3
        DEC   A
        MV    [passes],A
        MV    BA,$0200
        CALL  add_score
        CALL  tick
        CALL  draw_tally
        CALL  show_all
        JR    stage_clear2
stage_clear3:
        MV    A,90
        CALL  pause
        RET

; 集計の 2、3 行: ノコリタイム nn *100、オイヌキ nn *200、右に得点
draw_tally:
        MV    X,screen+480
        MV    I,480
        MV    A,0
draw_tally1:
        MV    [X++],A
        DEC   I
        JRNZ  draw_tally1
        MV    (BP+trow),2
        MV    (BP+tx),12
        MV    X,s_left
        CALL  str_s
        MV    A,[time]
        CALL  tally_num
        MV    X,s_x100
        CALL  str_s
        MV    (BP+trow),3
        MV    (BP+tx),12
        MV    X,s_pass
        CALL  str_s
        MV    A,[passes]
        CALL  tally_num
        MV    X,s_x200
        CALL  str_s
        MV    (BP+trow),2       ; 得点（大きい字）
        MV    (BP+tx),144
        CALL  score_str
        JP    str_b

; A を 3 桁で（前に空白 1 つ）
tally_num:
        MV    (BP+tval),A
        MV    (BP+tval+1),0
        MV    I,(BP+tval)
        CALL  num_str
        MV    A,2
        ADD   X,A
        JP    str_s

; ---- GAME OVER ----
over_screen:
        CALL  clear_screen
        MV    X,s_over
        MV    (BP+trow),0
        CALL  center_b
        CALL  score_line
        CALL  show_all
        MV    X,m_over
        CALL  play
        MV    A,90
        CALL  pause
        RET

; 3 行目の真ん中に「SCORE 得点」
score_line:
        MV    (BP+trow),3
        MV    (BP+tx),69
        MV    X,s_score
        CALL  str_s
        ADD   (BP+tx),6
        CALL  score_str
        JP    str_s

; ---- 全コースクリア ----
ending:
        CALL  clear_screen
        MV    X,s_congra
        MV    (BP+trow),0
        CALL  center_b
        MV    X,s_allclr
        MV    (BP+trow),2
        CALL  center_s
        CALL  score_line
        CALL  show_all
        MV    X,m_ending
        CALL  play
        MV    A,200
        CALL  pause
        RET

; ---- 名前を入れる（ハイスコアのとき）: ◀ ▶ で字を選び、SPACE で次へ ----
enter_name:
        ; 何位か: 得点がその順位の得点より大きい最初の所
        MV    X,hiscores+3
        MV    (BP+sel),0
enter_name1:
        PUSHS X
        MV    Y,score
        MV    IL,4
enter_name2:
        MV    A,[X++]           ; 上の桁から比べる
        MV    (BP+tch),A
        MV    A,[Y++]
        CMP   (BP+tch),A
        JRC   enter_name4       ; 表 < 得点: ここに入る
        JRNZ  enter_name3       ; 表 > 得点
        DEC   IL
        JRNZ  enter_name2
enter_name3:
        POPS  X
        MV    A,7
        ADD   X,A
        INC   (BP+sel)
        CMP   (BP+sel),RANKS
        JRC   enter_name1
        RC                      ; 入らない
        RET
enter_name4:
        POPS  X
        ; sel 番目から下を 1 つずつずらす（下から）
        MV    (BP+mtmp),RANKS-1
enter_name5:
        CMP   (BP+mtmp),(BP+sel)
        JRZ   enter_name6
        JRC   enter_name6
        MV    A,(BP+mtmp)       ; X = hiscores + (mtmp - 1) × 7
        DEC   A
        CALL  rank_addr
        MV    Y,X
        MV    A,7
        ADD   Y,A
        MV    I,7
        MVL   (screen_buffer),[X]
        MV    I,7
        MVL   [Y],(screen_buffer)
        DEC   (BP+mtmp)
        JR    enter_name5
enter_name6:
        MV    A,(BP+sel)        ; そこへ得点と、名前 AAA
        CALL  rank_addr
        MV    [name_ptr],X
        MV    A,'A'
        MV    [X++],A
        MV    [X++],A
        MV    [X++],A
        MV    I,4
        MVL   (screen_buffer),[score]
        MV    I,4
        MVL   [X],(screen_buffer)
        MV    X,m_hiscore
        CALL  play
        MV    (BP+mtmp2),0      ; 何文字目か
enter_name7:
        CALL  draw_name
        CALL  show_all
enter_name8:
        CALL  pad_game
        TEST  (pad_now),PAD_BREAK
        JRNZ  enter_name_q
        TEST  (pad_new),PAD_A
        JRNZ  enter_name_n
        MV    (BP+tch),1
        TEST  (pad_new),PAD_RIGHT+PAD_UP
        JRNZ  enter_name9
        MV    (BP+tch),$FF
        TEST  (pad_new),PAD_LEFT+PAD_DOWN
        JRNZ  enter_name9
        MV    A,2
        CALL  game_wait
        MV    A,[blink]         ; 8 コマごとに、いまの字を消したり出したり
        INC   A
        MV    [blink],A
        TEST  A,7
        JRNZ  enter_name8
        JP    enter_name7
enter_name9:                    ; 字を変える（name_chars の中で次、前）
        MV    X,[name_ptr]
        MV    A,(BP+mtmp2)
        ADD   X,A
        MV    A,[X]
        PUSHS X
        MV    Y,name_chars
        MV    (BP+mtmp),0
enter_name10:
        MV    (BP+tval),A
        MV    A,[Y++]
        CMP   (BP+tval),A
        JRZ   enter_name11
        MV    A,(BP+tval)
        INC   (BP+mtmp)
        CMP   (BP+mtmp),NCHARS
        JRC   enter_name10
        MV    (BP+mtmp),0
enter_name11:
        MV    A,(BP+mtmp)
        ADD   A,(BP+tch)
        CMP   A,$FF
        JRNZ  enter_name12
        MV    A,NCHARS-1
enter_name12:
        CMP   A,NCHARS
        JRC   enter_name13
        MV    A,0
enter_name13:
        MV    Y,name_chars
        ADD   Y,A
        MV    A,[Y]
        POPS  X
        MV    [X],A
        MV    A,0               ; 変えた字はすぐ見せる
        MV    [blink],A
        CALL  blip
        JP    enter_name7
enter_name_n:                   ; 次の字
        CALL  blip
        INC   (BP+mtmp2)
        CMP   (BP+mtmp2),3
        JRC   enter_name7
        CALL  ranking
        MV    A,90
        CALL  pause
        JRC   enter_name_q
        RC
        RET
enter_name_q:
        SC
        RET

; X = hiscores + A × 7
rank_addr:
        MV    X,hiscores
        CMP   A,0
        JRZ   rank_addr2
rank_addr1:
        PUSHS A
        MV    A,7
        ADD   X,A
        POPS  A
        DEC   A
        JRNZ  rank_addr1
rank_addr2:
        RET

; 名前を入れる画面
draw_name:
        CALL  clear_screen
        MV    (BP+trow),0
        MV    (BP+tx),0
        MV    X,s_hiscore
        CALL  str_s
        ADD   (BP+tx),6
        MV    X,s_name
        CALL  str_s
        MV    X,[name_ptr]      ; 大きい 3 字。いまの字は点滅
        MV    (BP+trow),1
        MV    (BP+tx),102
        MV    (BP+mtmp),0
draw_name1:
        MV    A,[X++]
        CMP   (BP+mtmp),(BP+mtmp2)
        JRNZ  draw_name1a
        PUSHS A
        MV    A,[blink]
        TEST  A,1
        POPS  A
        JRZ   draw_name1a
        MV    A,' '
draw_name1a:
        CALL  put_b
draw_name2:
        INC   (BP+mtmp)
        CMP   (BP+mtmp),3
        JRC   draw_name1
        MV    (BP+trow),1       ; 順位
        MV    (BP+tx),60
        MV    A,(BP+sel)
        ADD   A,'1'
        CALL  put_b
        JP    score_line

NCHARS EQU 40
name_chars: DB "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .-!"
name_ptr: DP  hiscores
car_save: DB  1
demo_stage: DB 0
bgm_ptr: DP   m_title
blink:  DB    0
