; hud.inc: 字（自分のフォント）と計器（pokerace.asm から INCLUDE する）
;
; 字は ROM の字形を使わず、make_data.py で 1 字ずつ描いた font_s.bin（5 × 7、6 ドットおき）、
; font_b.bin（9 × 14 の斜体、12 ドットおき、2 行）、font_m.bin（計器の数字、7 × 11 の斜体）で描く。文字コードは ASCII。
; (tx) = x（ドット）、(trow) = 行（0〜3）。(tinv) = FFh で白黒を反転。

; 1 字（小）。A = 文字コード。tx が 6 進む。X と IL は変えない。
put_s:  PUSHS X
        PUSHS IL
        SUB   A,$20
        MV    (BP+tval),A
        MV    (BP+tval+1),0
        MV    BA,(BP+tval)      ; × 6
        ADD   BA,BA
        MV    I,BA
        ADD   BA,BA
        ADD   BA,I
        MV    X,font_s
        ADD   X,BA
        CALL  text_addr
        CMP   (BP+tx),235
        JRNC  put_s3
        MV    IL,6
put_s2: MV    A,[X++]
        XOR   A,(BP+tinv)
        MV    [Y++],A
        DEC   IL
        JRNZ  put_s2
put_s3: ADD   (BP+tx),6
        POPS  IL
        POPS  X
        RET

; 1 字（大、2 行）。A = 文字コード（20h〜5Fh）。tx が 12 進む。X と IL は変えない。
put_b:  PUSHS X
        PUSHS IL
        SUB   A,$20
        MV    (BP+tval),A
        MV    (BP+tval+1),0
        MV    BA,(BP+tval)      ; × 24
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA
        MV    I,BA
        ADD   BA,BA
        ADD   BA,I
        MV    X,font_b
        ADD   X,BA
        CALL  text_addr
        CMP   (BP+tx),229
        JRNC  put_b3
        MV    IL,12
put_b1: MV    A,[X++]
        XOR   A,(BP+tinv)
        MV    [Y++],A
        DEC   IL
        JRNZ  put_b1
        MV    BA,240-12
        ADD   Y,BA
        MV    IL,12
put_b2: MV    A,[X++]
        XOR   A,(BP+tinv)
        MV    [Y++],A
        DEC   IL
        JRNZ  put_b2
put_b3: ADD   (BP+tx),12
        POPS  IL
        POPS  X
        RET

; Y = screen + trow × 240 + tx
text_addr:
        MV    Y,screen
        MV    A,(BP+trow)
        CMP   A,0
        JRZ   text_addr2
        MV    I,240
text_addr1:
        ADD   Y,I
        DEC   (BP+trow)
        JRNZ  text_addr1
        MV    (BP+trow),A
text_addr2:
        MV    A,(BP+tx)
        ADD   Y,A
        RET

; 文字列（0 で終わる）。X = 文字列。
str_s:  MV    A,[X++]
        CMP   A,0
        JRZ   str_end
        CALL  put_s
        JR    str_s
str_b:  MV    A,[X++]
        CMP   A,0
        JRZ   str_end
        CALL  put_b
        JR    str_b
str_end:
        RET

; 真ん中にそろえる。X = 文字列、(trow) = 行。
center_s:
        CALL  str_len
        MV    (BP+tch),A        ; tx = (240 - 6 × 字数) / 2
        ADD   A,A
        ADD   A,(BP+tch)        ; 3 × 字数
        MV    (BP+tch),A
        MV    A,120
        SUB   A,(BP+tch)
        JRNC  center_s1
        MV    A,0
center_s1:
        MV    (BP+tx),A
        JR    str_s
center_b:
        CALL  str_len
        MV    (BP+tch),A        ; tx = (240 - 12 × 字数) / 2
        ADD   A,A
        ADD   A,(BP+tch)
        ADD   A,A               ; 6 × 字数
        MV    (BP+tch),A
        MV    A,120
        SUB   A,(BP+tch)
        JRNC  center_b1
        MV    A,0
center_b1:
        MV    (BP+tx),A
        JR    str_b

; A = X の文字列の字数（X は変えない）
str_len:
        PUSHS X
        MV    (BP+tch),0
str_len1:
        MV    A,[X++]
        CMP   A,0
        JRZ   str_len2
        INC   (BP+tch)
        JR    str_len1
str_len2:
        MV    A,(BP+tch)
        POPS  X
        RET

; I の数（0〜65535）を 5 桁の字にして nbuf に入れる（上の 0 は空白、1 の位は 0 でも書く）。X = nbuf。
num_str:
        MV    (BP+tval),I
        MV    X,nbuf
        MV    Y,powers
        MV    (BP+tch),0        ; 0 でない桁が出たら 1
num_str1:
        MV    I,[Y++]
        MV    A,'0'
        MV    (BP+mtmp),A
num_str2:
        MV    BA,(BP+tval)
        SUB   BA,I
        JRC   num_str3
        MV    (BP+tval),BA
        INC   (BP+mtmp)
        JR    num_str2
num_str3:
        MV    A,(BP+mtmp)
        CMP   A,'0'
        JRNZ  num_str4
        CMP   (BP+tch),0
        JRNZ  num_str5
        MV    A,[Y]             ; 1 の位は 0 でも書く
        CMP   A,0
        JRZ   num_str5
        MV    A,' '
        JR    num_str6
num_str4:
        MV    (BP+tch),1
num_str5:
        MV    A,(BP+mtmp)
num_str6:
        MV    [X++],A
        MV    A,[Y]
        CMP   A,0
        JRNZ  num_str1
        MV    A,0
        MV    [X],A
        MV    X,nbuf
        RET
powers: DW    10000,1000,100,10,1
        DB    0

; 得点（BCD 4 バイト）を 8 桁の字にして nbuf に入れる。X = nbuf + 1（7 桁を見せる）。
score_str:
        MV    Y,score
score_str_y:                    ; Y = BCD 4 バイト
        MV    X,nbuf
        MV    IL,4
score_str1:
        MV    A,[Y++]
        MV    (BP+tch),A
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        RC
        SHR   A
        ADD   A,'0'
        MV    [X++],A
        MV    A,(BP+tch)
        AND   A,$0F
        ADD   A,'0'
        MV    [X++],A
        DEC   IL
        JRNZ  score_str1
        MV    A,0
        MV    [X],A
        MV    X,nbuf+1
        RET

; ---- 走る画面の枠: 仕切り、計器の札（TIME、KM/H、SCORE） ----
; 左: 0、1 行に TIME の札と時間（計器の数字）、2、3 行にコースの図。
; 右: 0、1 行に速さ（計器の数字）と KM/H の札、2 行に SCORE とターボの札、3 行に得点。
make_back:
        CALL  clear_screen
        MV    X,screen+VIEW_L-1 ; 仕切りの縦線
        MV    Y,screen+VIEW_R+1
        MV    IL,4
make_back1:
        MV    A,$FF
        MV    [X],A
        MV    [Y],A
        MV    BA,240
        ADD   X,BA
        ADD   Y,BA
        DEC   IL
        JRNZ  make_back1
        MV    X,b_time
        MV    Y,screen
        CALL  put_badge
        MV    X,b_kmh
        MV    Y,screen+223
        CALL  put_badge
        MV    X,b_score
        MV    Y,screen+480+198
; 札を描く: X = 札（桁数、続けて 1 桁 1 バイト）、Y = 描く所
put_badge:
        MV    A,[X++]
        MV    (BP+tch),A
put_badge1:
        MV    A,[X++]
        MV    [Y++],A
        DEC   (BP+tch)
        JRNZ  put_badge1
        RET

; 仮想画面を消す
clear_screen:
        MV    X,screen
        MV    I,960
        MV    A,0
clear_screen1:
        MV    [X++],A
        DEC   I
        JRNZ  clear_screen1
        RET

; 計器の数字 1 字を (tx)、0、1 行（y = 4〜14）に重ねる。A = 0〜9、10 は空白。tx が 8 進む。
put_m:  PUSHS X
        MV    (BP+tval),A       ; × 18
        MV    (BP+tval+1),0
        MV    BA,(BP+tval)
        ADD   BA,BA
        MV    I,BA
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,I
        MV    X,font_m
        ADD   X,BA
        MV    Y,screen
        MV    A,(BP+tx)
        ADD   Y,A
        MV    IL,9
put_m1: MV    A,[X++]
        MV    (BP+tch),A
        MV    A,[Y]
        OR    A,(BP+tch)
        MV    [Y],A
        MV    A,[X++]
        MV    (BP+tch),A
        MV    A,[Y+240]
        OR    A,(BP+tch)
        MV    [Y+240],A
        INC   Y
        DEC   IL
        JRNZ  put_m1
        ADD   (BP+tx),8
        POPS  X
        RET

; 0、1 行の x = A から (tch) 桁を消す
clear_01:
        MV    Y,screen
        ADD   Y,A
clear_01a:
        MV    A,0
        MV    [Y],A
        MV    [Y+240],A
        INC   Y
        DEC   (BP+tch)
        JRNZ  clear_01a
        RET

; nbuf の数の字（'0'〜'9'、空白）を計器の数字で描く。X = 字、(mtmp) = 字数
str_m:  MV    A,[X++]
        SUB   A,'0'
        JRNC  str_m1
        MV    A,10              ; 空白
str_m1: CALL  put_m
        DEC   (BP+mtmp)
        JRNZ  str_m
        RET

; ---- 計器: 変わった所だけ描いて、pdirty にその行のビットを立てる（ビット 0〜3 左、4〜7 右） ----
; 1 コマに描き直すのは 1 か所だけ（時間、速さ、得点、コースの図とターボの順に回す）。どれも 4 コマ以内に新しくなる。
draw_hud:
        MV    A,[count]
        AND   A,3
        JRZ   draw_hud_t
        CMP   A,1
        JPZ   draw_hud1
        CMP   A,2
        JPZ   draw_hud2
        JP    draw_hud3
draw_hud_t:
        ; 時間（左の 0、1 行、2 桁。残り 5 秒からは点滅）
        MV    A,[time]
        MV    (BP+tch),A
        CMP   A,6
        JRNC  draw_hud0
        MV    A,[count]
        AND   A,8
        JRZ   draw_hud0
        OR    (BP+tch),$80      ; 消えている間
draw_hud0:
        MV    A,[hud_last]
        CMP   (BP+tch),A
        JPZ   hud_ret
        MV    A,(BP+tch)
        MV    [hud_last],A
        MV    (BP+tch),24       ; x = 18〜41 を消す
        MV    A,18
        CALL  clear_01
        MV    A,[hud_last]
        TEST  A,$80
        JRNZ  draw_hud0a
        MV    (BP+tval),A
        MV    (BP+tval+1),0
        MV    I,(BP+tval)
        CALL  num_str
        MV    A,[X+3]           ; 10 の位が空白なら 0 にする
        CMP   A,' '
        JRNZ  draw_hud0b
        MV    A,'0'
        MV    [X+3],A
draw_hud0b:
        MV    A,3
        ADD   X,A
        MV    (BP+tx),22
        MV    (BP+mtmp),2
        CALL  str_m
draw_hud0a:
        MV    A,[pdirty]
        OR    A,$03
        MV    [pdirty],A
        RET
draw_hud1:
        ; 速さ（右の 0、1 行、3 桁）。km/h = 速さ × 1.25
        MV    A,[speed]
        MV    (BP+tch),A
        MV    A,[hud_last+1]
        CMP   (BP+tch),A
        JPZ   hud_ret
        MV    A,(BP+tch)
        MV    [hud_last+1],A
        RC
        SHR   A
        RC
        SHR   A
        MV    (BP+tval),A       ; 速さ / 4
        MV    (BP+tval+1),0
        MV    I,(BP+tval)
        MV    A,(BP+tch)        ; + 速さ
        MV    (BP+tval),A
        MV    BA,(BP+tval)
        ADD   BA,I
        MV    I,BA
        CALL  num_str
        PUSHS X
        MV    (BP+tch),25       ; x = 198〜222 を消す
        MV    A,198
        CALL  clear_01
        POPS  X
        MV    A,2
        ADD   X,A
        MV    (BP+tx),198
        MV    (BP+mtmp),3
        CALL  str_m
        MV    A,[pdirty]
        OR    A,$30
        MV    [pdirty],A
        RET
draw_hud2:
        ; 得点（右の 3 行、7 桁）
        MV    A,[score_dirty]
        CMP   A,0
        JPZ   hud_ret
        MV    A,0
        MV    [score_dirty],A
        CALL  score_str
        MV    (BP+tx),198
        MV    (BP+trow),3
        CALL  str_s
        MV    A,[pdirty]
        OR    A,$80
        MV    [pdirty],A
        RET
draw_hud3:
        ; コースの図（左の 2、3 行）と、自分の位置（点滅）
        CALL  bar_pos
        MV    (BP+tch),A
        MV    A,[count]
        AND   A,8
        MV    (BP+mtmp),A
        MV    A,[hud_last+2]
        CMP   (BP+tch),A
        JRNZ  draw_hud3a
        MV    A,[hud_last+4]
        CMP   (BP+mtmp),A
        JRZ   draw_hud4
draw_hud3a:
        MV    A,(BP+tch)
        MV    [hud_last+2],A
        MV    A,(BP+mtmp)
        MV    [hud_last+4],A
        CALL  draw_map
        MV    A,[pdirty]
        OR    A,$0C
        MV    [pdirty],A
draw_hud4:
        ; ターボの札（右の 2 行の右端。ターボ中は点滅）
        MV    (BP+tch),0
        MV    A,[turbo]
        CMP   A,0
        JRZ   draw_hud5
        MV    A,[count]
        AND   A,4
        MV    (BP+tch),1
        JRZ   draw_hud5
        MV    (BP+tch),2
draw_hud5:
        MV    A,[hud_last+3]
        CMP   (BP+tch),A
        JRZ   hud_ret
        MV    A,(BP+tch)
        MV    [hud_last+3],A
        MV    Y,screen+480+219
        CMP   (BP+tch),1
        JRNZ  draw_hud6
        MV    X,b_turbo
        CALL  put_badge
        JR    draw_hud7
draw_hud6:
        MV    IL,21             ; 消す
        MV    A,0
draw_hud6a:
        MV    [Y++],A
        DEC   IL
        JRNZ  draw_hud6a
draw_hud7:
        MV    A,[pdirty]
        OR    A,$40
        MV    [pdirty],A
hud_ret:
        RET

; A = 進み具合の段（0〜37。コースの図の位置）: DIST ÷ bar_unit
bar_pos:
        MV    BA,[odo+1]
        MV    I,[bar_unit]
        MV    (BP+tch),0
bar_pos1:
        SUB   BA,I
        JRC   bar_pos2
        INC   (BP+tch)
        CMP   (BP+tch),37
        JRC   bar_pos1
bar_pos2:
        MV    A,(BP+tch)
        RET

; ---- コースの図 ----
; stages.inc の図（map0〜7）は、進み具合の段（0〜37）ごとの位置 x（0〜39）、y（0〜15）。
; 走り出す前に make_map が点を線でつないで map_buf（40 桁 × 2 行）に描き、チェックポイント（十字）と
; スタート、ゴール（× 印）を足す。走っている間は draw_map が map_buf を左の計器（x = 1〜40、2、3 行）に
; 写し、自分の位置に 3 × 3 の四角を重ねる（8 コマごとに点滅）。

mlax    EQU   $0E               ; 線の始め、終わり（道を描くときの ra、rb、prev_l、prev_r の場所）
mlay    EQU   $0F
mlbx    EQU   $10
mlby    EQU   $11

make_map:
        MV    X,map_buf
        MV    IL,80
        MV    A,0
make_map1:
        MV    [X++],A
        DEC   IL
        JRNZ  make_map1
        MV    X,[st_map]        ; 段 0 から 37 まで、となりの点を線でつなぐ
        MV    A,[X++]
        MV    (BP+mlbx),A
        MV    A,[X++]
        MV    (BP+mlby),A
        MV    (BP+mtmp),37
make_map2:
        MV    (BP+mlax),(BP+mlbx)
        MV    (BP+mlay),(BP+mlby)
        MV    A,[X++]
        MV    (BP+mlbx),A
        MV    A,[X++]
        MV    (BP+mlby),A
        PUSHS X
        CALL  map_line
        POPS  X
        DEC   (BP+mtmp)
        JRNZ  make_map2
        MV    A,0               ; スタートとゴール: × 印
        CALL  map_point
        MV    X,mark_x
        CALL  map_mark
        MV    A,[st_secs]       ; チェックポイント: 段 37 × k ÷ 区間の数 に十字
        MV    (BP+mtmp),A
        DEC   (BP+mtmp)
        JRZ   make_map9
        MV    (BP+mtmp2),0
make_map3:
        MV    A,(BP+mtmp2)
        ADD   A,37
        MV    (BP+mtmp2),A
        MV    (BP+tval),0       ; ÷ 区間の数
        MV    A,[st_secs]
        MV    (BP+tval+1),A
        MV    A,(BP+mtmp2)
make_map4:
        SUB   A,(BP+tval+1)
        JRC   make_map5
        INC   (BP+tval)
        JR    make_map4
make_map5:
        MV    A,(BP+tval)
        CALL  map_point
        MV    X,mark_plus
        CALL  map_mark
        DEC   (BP+mtmp)
        JRNZ  make_map3
make_map9:
        RET

; (mlbx)、(mlby) = 段 A の位置
map_point:
        MV    X,[st_map]
        ADD   X,A
        ADD   X,A
        MV    A,[X++]
        MV    (BP+mlbx),A
        MV    A,[X]
        MV    (BP+mlby),A
        RET

; (mlbx)、(mlby) のまわりに印（X = dx、dy の組の列、80h で終わり）
map_mark:
        MV    A,[X++]
        CMP   A,$80
        JRZ   map_mark9
        ADD   A,(BP+mlbx)
        MV    (BP+tch),A
        MV    A,[X++]
        ADD   A,(BP+mlby)
        CMP   A,24
        JRNC  map_mark
        CMP   (BP+tch),40
        JRNC  map_mark
        PUSHS X
        CALL  map_plot
        POPS  X
        JR    map_mark
map_mark9:
        RET
mark_plus: DB 0,0,1,0,255,0,0,1,0,255,$80
mark_x: DB    0,0,1,1,255,255,1,255,255,1,$80

; (mlax, mlay) から (mlbx, mlby) へ線: となりどうしになるまで真ん中で分けて描く
map_line:
        MV    A,(BP+mlax)
        SUB   A,(BP+mlbx)
        JRNC  map_line1
        XOR   A,$FF
        INC   A
map_line1:
        CMP   A,2
        JRNC  map_line3
        MV    A,(BP+mlay)
        SUB   A,(BP+mlby)
        JRNC  map_line2
        XOR   A,$FF
        INC   A
map_line2:
        CMP   A,2
        JRNC  map_line3
        MV    A,(BP+mlbx)       ; となり: 終わりの点を打つ
        MV    (BP+tch),A
        MV    A,(BP+mlby)
        JR    map_plot
map_line3:
        MV    A,(BP+mlbx)       ; 終わりの点を積み、真ん中までを描く
        PUSHS A
        MV    A,(BP+mlby)
        PUSHS A
        MV    A,(BP+mlax)
        ADD   A,(BP+mlbx)
        RC
        SHR   A
        MV    (BP+mlbx),A
        PUSHS A
        MV    A,(BP+mlay)
        ADD   A,(BP+mlby)
        RC
        SHR   A
        MV    (BP+mlby),A
        PUSHS A
        CALL  map_line
        POPS  A                 ; 真ん中から終わりまで
        MV    (BP+mlay),A
        POPS  A
        MV    (BP+mlax),A
        POPS  A
        MV    (BP+mlby),A
        POPS  A
        MV    (BP+mlbx),A
        JR    map_line

; map_buf の (tch, A) に点を打つ（x 0〜39、y 0〜15）
map_plot:
        MV    (BP+tval),A
        MV    Y,y_row
        ADD   Y,A
        MV    A,[Y]             ; 行 × 40
        MV    Y,map_buf
        CMP   A,0
        JRZ   map_plot2
map_plot1:
        PUSHS A
        MV    A,40
        ADD   Y,A
        POPS  A
        DEC   A
        JRNZ  map_plot1
map_plot2:
        MV    A,(BP+tch)
        ADD   Y,A
        MV    A,(BP+tval)
        AND   A,7
        MV    X,row_bits
        ADD   X,A
        MV    A,[X]
        MV    (BP+tval),A
        MV    A,[Y]
        OR    A,(BP+tval)
        MV    [Y],A
        RET
row_bits: DB  1,2,4,8,16,32,64,128

; map_buf を左の計器（x = 1〜40、2、3 行）に写し、点滅の間は自分の位置に 3 × 3 の四角を重ねる。
; (tch) = 段、(mtmp) = 0 なら四角を出す
draw_map:
        MV    X,map_buf
        MV    Y,screen+480+1
        MV    (BP+mtmp2),2
draw_map1:
        MV    I,40
        MVL   (screen_buffer),[X++]
        MV    I,40
        MVL   [Y++],(screen_buffer)
        MV    A,240-40
        ADD   Y,A
        DEC   (BP+mtmp2)
        JRNZ  draw_map1
        CMP   (BP+mtmp),0
        JRNZ  draw_map9
        MV    A,(BP+tch)
        CALL  map_point
        MV    (BP+mtmp2),$FF    ; dy = -1〜1
draw_map2:
        MV    A,(BP+mlby)       ; 画面の y = 16 + 図の y + dy
        ADD   A,16
        ADD   A,(BP+mtmp2)
        MV    (BP+tval),A
        MV    Y,y_off
        ADD   Y,A
        ADD   Y,A
        MV    I,[Y]
        MV    Y,screen
        ADD   Y,I
        MV    A,(BP+mlbx)       ; x = 1 + 図の x - 1
        ADD   Y,A
        MV    A,(BP+tval)
        AND   A,7
        MV    X,row_bits
        ADD   X,A
        MV    A,[X]
        MV    (BP+tval),A
        MV    IL,3
draw_map3:
        MV    A,[Y]             ; 図の左右の外（x = 0、41）にはみ出さない
        OR    A,(BP+tval)
        MV    [Y++],A
        DEC   IL
        JRNZ  draw_map3
        INC   (BP+mtmp2)
        CMP   (BP+mtmp2),2
        JRNZ  draw_map2
draw_map9:
        RET

; ---- 知らせ（チェックポイント、GO! など）とスタートの数字 ----
show_msg:
        MV    A,[cdown]
        CMP   A,0
        JRZ   show_msg1
        MV    X,cd_str          ; 大きい数字
        MV    (BP+trow),0
        CALL  center_b
        CALL  mark_row01
show_msg1:
        MV    A,[msg]
        CMP   A,0
        JRZ   show_msg3
        DEC   A
        MV    [msg],A
        TEST  A,4
        JRZ   show_msg3
        MV    X,[msg_ptr]
        MV    (BP+trow),0
        MV    A,[msg_big]
        CMP   A,0
        JRZ   show_msg2
        CALL  center_b
        JR    mark_row01
show_msg2:
        CALL  center_s
mark_row01:
        MV    A,1               ; 0 行が変わった。次のコマでは空を描き直して消す
        MV    [text_prev],A
        MV    [row0_dirty],A
show_msg3:
        RET
