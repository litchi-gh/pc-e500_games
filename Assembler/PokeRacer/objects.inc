; objects.inc: 道ばたの物、ゲート、ほかの車、アイテム、自分の車、ターボの線（pokerace.asm から INCLUDE する）
;
; 道を描いた後に呼ぶ。奥行きは z_line の表でラインに直し、そのラインの中心（lined）から置く。
; 縦線（vline2）は列ごとに描くので、液晶の縦 8 ドットのバイトに合っていて速い。

vpat    EQU   $0C               ; vline2: 縦線の模様（AND する。ふつうは FFh）

; ---- 道ばたの物: 左右に TREE_SP ごと（右は半分ずらす）。種類はステージで決まる ----
draw_trees:
        MV    (BP+vpat),$FF
        MV    (ob_side),0
draw_trees1:
        MV    A,[st_objl]
        TEST  (ob_side),1
        JRZ   draw_trees1a
        MV    A,[st_objr]
draw_trees1a:
        MV    (BP+ob_type),A
        MV    BA,[pos]
        TEST  (ob_side),1
        JRZ   draw_trees2
        MV    I,TREE_SP/2
        ADD   BA,I
draw_trees2:
        EX    A,B               ; 次の物までの奥行き = TREE_SP - (位置 mod TREE_SP)
        AND   A,(TREE_SP-1)/256
        EX    A,B
        MV    I,BA
        MV    BA,TREE_SP
        SUB   BA,I
        MV    (t1),BA
draw_trees3:
        CALL  z_to_line         ; A = ライン（0 = 遠すぎる、23 = 近すぎる）
        CMP   A,5               ; ライン 4 より遠い物は描かない（小さすぎる）
        JRC   draw_trees6
        CMP   A,23
        JRZ   draw_trees5
        MV    (ob_d),A
        MV    X,lined
        ADD   X,A
        ADD   X,A
        MV    A,[X+1]           ; 中心から物までのドット数
        MV    (ob_x),A
        MV    A,[X]
        TEST  (ob_side),1
        JRNZ  draw_trees4
        SUB   A,(ob_x)
        JRC   draw_trees5
        MV    (ob_x),A
        CALL  object
        JR    draw_trees5
draw_trees4:
        ADD   A,(ob_x)
        JRC   draw_trees5
        MV    (ob_x),A
        CALL  object
draw_trees5:
        MV    BA,(t1)           ; 次の物
        MV    I,TREE_SP
        ADD   BA,I
        MV    (t1),BA
        JR    draw_trees3
draw_trees6:
        INC   (ob_side)
        CMP   (ob_side),2
        JRC   draw_trees1
        RET

; (t1) の奥行き（0〜8191）を A = ライン に直す。0 は遠すぎる、23 はいちばん手前より近い。
z_to_line:
        MV    BA,(t1)
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA
        MV    A,B               ; 奥行き ÷ 32
        MV    X,z_line
        ADD   X,A
        MV    A,[X]
        RET

; ライン (ob_d)、x + 8 = (ob_x) に種類 (ob_type) の物を 1 つ。大きさは obj_tab から。
object: CALL  tick_poll
        MV    X,obj_tab
        MV    A,(BP+ob_d)
        ADD   X,A
        ADD   X,A
        ADD   X,A
        ADD   X,A
        MV    A,[X++]
        MV    (BP+ob_top),A
        MV    A,[X++]
        MV    (BP+ob_cb),A
        MV    A,[X++]
        MV    (BP+ob_w),A
        MV    A,[X]
        MV    (BP+ob_bot),A
        MV    (BP+ob_st),2
        MV    A,(BP+ob_x)       ; x（0〜239）にする
        SUB   A,8
        JPC   tree9
        MV    (BP+ob_x),A
        MV    A,(BP+ob_type)
        CMP   A,OB_PALM
        JPZ   palm
        CMP   A,OB_CACT
        JPZ   cactus
        CMP   A,OB_BLDG
        JPZ   building
        CMP   A,OB_LAMP
        JPZ   lamp
        CMP   A,OB_POST
        JPZ   post
        CMP   A,OB_ROCK
        JPZ   rock
; 松: 幹は葉の下から根もとまで、葉は外の桁ほど 2 ドットずつ下がる三角。
tree:   MV    A,(BP+ob_x)
        MV    (BP+vx),A
        MV    (BP+vy0),(BP+ob_cb)
        MV    (BP+vy1),(BP+ob_bot)
        MV    (BP+vdx),0
        CMP   (BP+ob_w),3
        JRC   tree1
        MV    (BP+vdx),1
tree1:  CALL  vpair
        MV    (BP+ob_j),0       ; 葉: 中心から外へ、左右 2 列ずつ
        MV    (BP+vy1),(BP+ob_cb)
        MV    (BP+vy0),(BP+ob_top)
tree2:  CMP   (BP+ob_cb),(BP+vy0)       ; てっぺんが葉の下より下になったら終わり
        JPC   tree9
        CALL  col_pair
        INC   (BP+ob_j)
        CMP   (BP+ob_w),(BP+ob_j)
        JPC   tree9
        MV    A,(BP+vy0)
        ADD   A,(BP+ob_st)
        MV    (BP+vy0),A
        JR    tree2
tree9:  RET

; 中心から ob_j 桁の左右 2 列に、(vy0)〜(vy1) の縦線。
col_pair:
        MV    A,(BP+ob_x)
        SUB   A,(BP+ob_j)
        JRC   col_pair1
        MV    (BP+vx),A
        MV    A,(BP+ob_j)
        ADD   A,A
        MV    (BP+vdx),A
        JP    vpair
col_pair1:
        RET

; ヤシ: 細い幹がてっぺんまで、葉は左右に垂れる。
palm:   MV    A,(BP+ob_x)
        MV    (BP+vx),A
        MV    A,(BP+ob_top)
        INC   A
        MV    (BP+vy0),A
        MV    (BP+vy1),(BP+ob_bot)
        MV    (BP+vdx),0
        CALL  vpair
        MV    (BP+ob_j),1       ; 葉: 外へ 1 桁ごとに半ドット下がり、2 ドットの太さ
        INC   (BP+ob_w)
palm1:  MV    A,(BP+ob_j)
        RC
        SHR   A
        ADD   A,(BP+ob_top)
        MV    (BP+vy0),A
        INC   A
        MV    (BP+vy1),A
        CALL  col_pair
        INC   (BP+ob_j)
        CMP   (BP+ob_w),(BP+ob_j)
        JRNC  palm1
        RET

; サボテン: 太い幹、左右の腕（葉の下で横に出て、上へ伸びる）。
cactus: MV    A,(BP+ob_x)
        MV    (BP+vx),A
        MV    A,(BP+ob_top)
        INC   A
        MV    (BP+vy0),A
        MV    (BP+vy1),(BP+ob_bot)
        MV    (BP+vdx),0
        CMP   (BP+ob_w),2
        JRC   cactus1
        MV    (BP+vdx),1
cactus1:
        CALL  vpair
        CMP   (BP+ob_w),1
        JRC   cactus9
        MV    A,(BP+ob_w)       ; 腕: 中心から w + 1 桁、てっぺん + 2 から葉の下まで
        INC   A
        MV    (BP+ob_j),A
        MV    A,(BP+ob_top)
        ADD   A,2
        MV    (BP+vy0),A
        MV    (BP+vy1),(BP+ob_cb)
        CALL  col_pair
        MV    (BP+vy0),(BP+ob_cb)       ; 腕のつけ根（1 ドット）
        MV    (BP+ob_j),1
cactus2:
        CALL  col_pair
        INC   (BP+ob_j)
        CMP   (BP+ob_w),(BP+ob_j)
        JRNC  cactus2
cactus9:
        RET

; ビル: 屋上から根もとまで。両端の列は線、中の列は窓の模様。
building:
        MV    X,bld_tab
        MV    A,(BP+ob_d)
        ADD   X,A
        ADD   X,A
        MV    A,[X++]
        MV    (BP+vy0),A
        MV    A,[X]
        MV    (BP+ob_w),A
        MV    (BP+vy1),(BP+ob_bot)
        MV    (BP+ob_j),(BP+ob_w)       ; 両端
        CALL  col_pair
        MV    (BP+ob_j),0
building1:
        MV    (BP+vpat),$AA     ; 窓（1 ドットおき）と、間の柱
        TEST  (BP+ob_j),1
        JRZ   building2
        MV    (BP+vpat),$FF
building2:
        CALL  col_pair
        INC   (BP+ob_j)
        CMP   (BP+ob_j),(BP+ob_w)
        JRC   building1
        MV    (BP+vpat),$FF
        RET

; 街灯: 柱と、道のほうへ出た腕と明かり。
lamp:   MV    A,(BP+ob_x)
        MV    (BP+vx),A
        MV    A,(BP+ob_top)
        MV    (BP+vy0),A
        MV    (BP+vy1),(BP+ob_bot)
        MV    (BP+vdx),0
        CALL  vpair
        MV    A,(BP+ob_top)     ; 腕（1 ドット）を道のほうへ w + 1 桁
        MV    (BP+vy1),A
        MV    (BP+ob_j),0
lamp1:  INC   (BP+ob_j)
        MV    A,(BP+ob_x)
        TEST  (ob_side),1
        JRNZ  lamp2
        ADD   A,(BP+ob_j)       ; 左の街灯は右へ
        JR    lamp3
lamp2:  SUB   A,(BP+ob_j)       ; 右の街灯は左へ
lamp3:  MV    (BP+vx),A
        MV    (BP+vy0),(BP+ob_top)
        MV    A,(BP+ob_w)
        INC   A
        CMP   (BP+ob_j),A
        JRC   lamp4
        MV    A,(BP+ob_top)     ; 先は明かり（3 ドット下がる）
        ADD   A,2
        MV    (BP+vy1),A
lamp4:  MV    (BP+vdx),0
        CALL  vclip
        MV    A,(BP+ob_w)
        INC   A
        CMP   (BP+ob_j),A
        JRC   lamp1
        RET

; 柵の柱: 根もとから短い縦線（2 本ずつ）。
post:   MV    A,(BP+ob_x)
        MV    (BP+vx),A
        MV    A,(BP+ob_d)
        RC
        SHR   A
        RC
        SHR   A
        INC   A
        MV    (BP+tch),A
        MV    A,(BP+ob_bot)
        SUB   A,(BP+tch)
        MV    (BP+vy0),A
        MV    (BP+vy1),(BP+ob_bot)
        MV    (BP+vdx),2
        JP    vpair

; 岩: 丸い山。中心から外へ 1 桁ごとに 1 ドット低い。
rock:   MV    A,(BP+ob_w)
        ADD   A,2
        MV    (BP+ob_w),A
        MV    (BP+vy1),(BP+ob_bot)
        MV    (BP+ob_j),0
rock1:  MV    A,(BP+ob_bot)     ; 高さ = w - j
        SUB   A,(BP+ob_w)
        ADD   A,(BP+ob_j)
        MV    (BP+vy0),A
        CALL  col_pair
        INC   (BP+ob_j)
        CMP   (BP+ob_j),(BP+ob_w)
        JRC   rock1
        RET

; 列 (vx) と (vx)+(vdx) に縦線。両方とも描く範囲の中なら vline2 で一度に、そうでなければ 1 列ずつ。
vpair:  CMP   (BP+vx),VIEW_L
        JRC   vpair1
        MV    A,(BP+vx)
        ADD   A,(BP+vdx)
        JRC   vpair1
        CMP   A,VIEW_R+1
        JRC   vline2
vpair1: MV    A,(BP+vdx)
        PUSHS A
        MV    (BP+vdx),0
        CALL  vclip
        POPS  A
        ADD   A,(BP+vx)
        JRC   vline9
        MV    (BP+vx),A
; vline2 を、描く範囲（VIEW_L〜VIEW_R）の中だけで（vdx = 0 の 1 列）。
vclip:  MV    (BP+vdx),0
        CMP   (BP+vx),VIEW_L
        JRC   vline9
        CMP   (BP+vx),VIEW_R+1
        JRNC  vline9
; 縦線を 2 列に: x = (vx) と (vx)+(vdx)、y = (vy0)〜(vy1)。(vx)、(vdx)、(vy0)、(vy1) は変えない。
vline2: MV    A,(BP+vy1)        ; 下のマスク、上のマスク、行の数 - 1
        AND   A,7
        MV    Y,bot_mask
        ADD   Y,A
        MV    A,[Y]
        MV    (BP+vbm),A
        MV    A,(BP+vy0)
        AND   A,7
        MV    Y,top_mask
        ADD   Y,A
        MV    A,[Y]
        MV    (BP+vtm),A
        MV    A,(BP+vy1)
        MV    Y,y_row
        ADD   Y,A
        MV    A,[Y]
        MV    (BP+cnt),A
        MV    A,(BP+vy0)
        MV    Y,y_row
        ADD   Y,A
        MV    A,[Y]
        SUB   (BP+cnt),A
        MV    A,(BP+vy0)        ; X = 1 列目、Y = 2 列目の上の行
        MV    Y,y_off
        ADD   Y,A
        ADD   Y,A
        MV    I,[Y]
        MV    X,screen
        ADD   X,I
        MV    A,(BP+vx)
        ADD   X,A
        MV    Y,X
        MV    A,(BP+vdx)
        ADD   Y,A
        MV    A,(BP+vtm)        ; 模様をかける
        AND   A,(BP+vpat)
        MV    (BP+vtm),A
        MV    A,(BP+vbm)
        AND   A,(BP+vpat)
        MV    (BP+vbm),A
        CMP   (BP+cnt),0
        JRNZ  vline1
        MV    A,(BP+vtm)        ; 1 つの行の中
        AND   A,(BP+vbm)
        MV    (BP+vtm),A
        MV    A,[X]
        OR    A,(BP+vtm)
        MV    [X],A
        MV    A,[Y]
        OR    A,(BP+vtm)
        MV    [Y],A
        RET
vline1: MV    A,[X]             ; 上の行
        OR    A,(BP+vtm)
        MV    [X],A
        MV    A,[Y]
        OR    A,(BP+vtm)
        MV    [Y],A
        MV    I,240
vline2a:
        ADD   X,I
        ADD   Y,I
        DEC   (BP+cnt)
        JRZ   vline2b
        MV    A,(BP+vpat)       ; 間の行
        MV    [X],A
        MV    [Y],A
        JR    vline2a
vline2b:
        MV    A,[X]             ; 下の行
        OR    A,(BP+vbm)
        MV    [X],A
        MV    A,[Y]
        OR    A,(BP+vbm)
        MV    [Y],A
vline9: RET

; ---- チェックポイントのゲート: 次のチェックポイントまで DIST で 32 より近いとき ----
draw_gate:
        MV    BA,[next_cp]
        MV    I,[odo+1]
        SUB   BA,I
        JPC   draw_gate9
        EX    A,B
        CMP   A,0
        JPNZ  draw_gate9
        EX    A,B
        CMP   A,32
        JPNC  draw_gate9
        MV    B,A               ; 奥行き = DIST の差 × 256 - 距離の下位バイト
        MV    A,[odo]
        MV    (t0),A
        MV    (t0+1),0
        MV    A,0
        MV    I,(t0)
        SUB   BA,I
        JPC   draw_gate9
        MV    (t1),BA
        CALL  z_to_line
        CMP   A,0
        JPZ   draw_gate9
        CMP   A,23
        JPZ   draw_gate9
        MV    (ob_d),A
        MV    X,lined           ; 柱: 中心 ± (道幅 + 3)
        ADD   X,A
        ADD   X,A
        MV    A,(ob_d)
        ADD   A,(ob_d)
        ADD   A,(ob_d)
        ADD   A,3
        MV    (t0),A
        MV    A,[X]
        SUB   A,(t0)
        JRNC  draw_gate1
        MV    A,0
draw_gate1:
        MV    (ob_top),A        ; 左の柱の x + 8
        MV    A,[X]
        ADD   A,(t0)
        JRNC  draw_gate2
        MV    A,255
draw_gate2:
        MV    (ob_cb),A         ; 右の柱の x + 8
        MV    X,gate_tab
        MV    A,(ob_d)
        ADD   X,A
        ADD   X,A
        MV    A,[X++]
        MV    (vy0),A
        MV    A,[X]
        MV    (ob_w),A          ; 横の板の太さ（ゴールは 2 ドット太い）
        MV    A,[section]
        INC   A
        MV    (BP+tch),A
        MV    A,[st_secs]
        CMP   (BP+tch),A
        JRC   draw_gate2a
        ADD   (BP+ob_w),2
draw_gate2a:
        MV    (BP+vpat),$FF
        MV    A,(vy0)
        MV    (ob_bot),A        ; 柱のてっぺん（vy0 は ra、rb と同じ場所なので取っておく）
        MV    A,(ob_d)
        ADD   A,9
        MV    (vy1),A
        MV    A,1               ; 柱は 0 行に届くことがある
        MV    [text_prev],A
        MV    [row0_dirty],A
        ; 横の板: 上と下の 1 ラインは消すだけ、間はチェッカー模様（空の山にまぎれないように）
        MV    (BP+ob_j),$FF     ; ライン = 柱のてっぺん + ob_j（-1〜板の太さ）
draw_gate3:
        MV    A,(BP+ob_bot)
        ADD   A,(BP+ob_j)
        CMP   A,32
        JRNC  draw_gate5
        MV    (BP+t0),A
        MV    Y,y_off           ; そのラインの先頭 - 8 と、ビット
        ADD   Y,A
        ADD   Y,A
        MV    I,[Y]
        MV    X,screen-8
        ADD   X,I
        MV    (BP+base),X
        MV    A,(BP+t0)
        AND   A,7
        MV    Y,top_mask
        ADD   Y,A
        MV    A,[Y]
        MV    (BP+vtmp),A
        MV    Y,bot_mask
        MV    A,(BP+t0)
        AND   A,7
        ADD   Y,A
        MV    A,[Y]
        AND   A,(BP+vtmp)
        MV    (BP+vtm),A
        MV    (BP+ra),(BP+ob_top)
        MV    (BP+rb),(BP+ob_cb)
        CALL  hrun_clr
        CMP   (BP+ob_j),(BP+ob_w)
        JRNC  draw_gate5
        MV    (BP+ra),(BP+ob_top)
        MV    (BP+rb),(BP+ob_cb)
        CALL  hrun_chk
draw_gate5:
        INC   (BP+ob_j)
        MV    A,(BP+ob_w)
        INC   A
        CMP   (BP+ob_j),A
        JRC   draw_gate3
        MV    (BP+vy0),(BP+ob_bot)      ; 柱（板の後に描く）
        MV    A,(BP+ob_top)
        CALL  gate_post
        MV    A,(BP+ob_cb)
        CALL  gate_post
draw_gate9:
        RET

; ゲートの板の 1 ライン。(ra)〜(rb) を描く範囲に切り、Y = 先頭、IL = ドット数。何もなければ C = 1。
run_setup:
        MV    A,(BP+ra)
        CMP   A,VIEW_L+8
        JRNC  run_setup1
        MV    A,VIEW_L+8
        MV    (BP+ra),A
run_setup1:
        MV    A,(BP+rb)
        CMP   A,VIEW_R+9
        JRC   run_setup2
        MV    A,VIEW_R+8
run_setup2:
        SUB   A,(BP+ra)
        JRC   run_setup3
        INC   A
        MV    (BP+cnt),A
        MV    IL,(BP+cnt)
        MV    Y,(BP+base)
        MV    A,(BP+ra)
        ADD   Y,A
        RC
run_setup3:
        RET

; (ra)〜(rb) のビット (vtm) を消す。
hrun_clr:
        CALL  run_setup
        JRC   hrun_clr2
        MV    A,(BP+vtm)
        XOR   A,$FF
        MV    (BP+vtmp),A
hrun_clr1:
        MV    A,[Y]
        AND   A,(BP+vtmp)
        MV    [Y++],A
        DEC   IL
        JRNZ  hrun_clr1
hrun_clr2:
        RET

; (ra)〜(rb) に 1 ドットおきにビット (vtm) を立てる。x + ライン が偶数の所。
hrun_chk:
        CALL  run_setup
        JRC   hrun_chk2
        MV    A,(BP+ra)
        ADD   A,(BP+ob_j)
        TEST  A,1
        JRZ   hrun_chk1
        INC   Y
        DEC   IL
        JRZ   hrun_chk2
hrun_chk1:
        MV    A,[Y]
        OR    A,(BP+vtm)
        MV    [Y],A
        INC   Y
        INC   Y
        DEC   IL
        JRZ   hrun_chk2
        DEC   IL
        JRNZ  hrun_chk1
hrun_chk2:
        RET

; A（x + 8）にゲートの柱（手前の 11 ラインは 2 ドット）。
gate_post:
        SUB   A,8
        JRC   gate_post1
        MV    (BP+vx),A
        MV    (BP+vdx),0
        CMP   (BP+ob_d),12
        JRC   gate_post0
        MV    (BP+vdx),1
gate_post0:
        JP    vpair
gate_post1:
        RET

; ---- ほかの車 ----
draw_cars:
        MV    X,rivals
        MV    (ob_side),CARS    ; 数える（cn は ob_d と同じ場所なので使えない）
draw_cars1:
        PUSHU X
        CALL  tick_poll
        CALL  draw_car
        POPU  X
        MV    A,6
        ADD   X,A
        DEC   (ob_side)
        JRNZ  draw_cars1
        RET

draw_car:
        MV    BA,[X]            ; 後ろにいる車、8192 より遠い車は描かない
        MV    (t1),BA
        TEST  (t1+1),$E0
        JPNZ  draw_car9
        MV    A,[X+5]
        MV    (ob_j),A          ; 車種（コマ）
        MV    BA,[X+2]
        MV    (t0),BA           ; t0 = 横の位置
        PUSHU X
        CALL  z_to_line
        POPU  X
        CMP   A,0
        JPZ   draw_car9
        CMP   A,23
        JRNZ  draw_car1
        MV    A,22
draw_car1:
        MV    (ob_d),A
        ; x = 中心 + 横の位置 × d / 256
        MV    (cnt),A
        MV    BA,0
        MV    I,(t0)
draw_car2:
        ADD   BA,I
        DEC   (cnt)
        JRNZ  draw_car2
        EX    A,B
        XOR   A,$80             ; 上位 + 128
        MV    (t0),A
        MV    (t0+1),0
        MV    X,lined           ; 中心（+ 8）
        MV    A,(ob_d)
        ADD   X,A
        ADD   X,A
        MV    A,[X]
        MV    (t1),A
        MV    (t1+1),0
        MV    BA,(t1)
        MV    I,(t0)
        ADD   BA,I              ; x + 8 + 128
        ; 大きさ: d 17 から L、11 から M、6 から S、それより遠いと XS
        MV    X,rival_xs
        MV    I,136+2
        CMP   (ob_d),6
        JRC   draw_car4
        MV    X,rival_s
        MV    I,136+4
        CMP   (ob_d),11
        JRC   draw_car4
        MV    X,rival_m
        MV    I,136+6
        CMP   (ob_d),17
        JRC   draw_car4
        MV    X,rival_l
        MV    I,136+8
draw_car4:
        SUB   BA,I              ; 左端
        MV    (sprite_x),BA
        ; 描く範囲に全部入るときだけ描く
        MV    I,VIEW_L
        SUB   BA,I
        MV    (t1),BA
        MV    A,[X]             ; 幅
        MV    (t0),A
        MV    A,VIEW_R-VIEW_L+1
        SUB   A,(t0)
        MV    (t0),A
        MV    (t0+1),0
        MV    BA,(t0)
        MV    I,(t1)
        SUB   BA,I
        JRC   draw_car9
        MV    A,(ob_d)
        ADD   A,2
        MV    (sprite_y),A
        MV    A,(ob_j)
        CALL  sprite_draw
draw_car9:
        RET

; ---- 自分の車: まっすぐ/左/右 × タイヤ 2 コマ、ブレーキでランプ。当たった後は回る。道の外ではゆれて砂ぼこり ----
; 自分の車と砂ぼこりは 3 行目（y = 24〜31）にちょうど入るので、ずらさずに速く描く（blit）。
draw_player:
        MV    A,[steer]
        MV    (BP+ob_j),A
        MV    A,[crash]
        CMP   A,0
        JRZ   draw_player1
        MV    (BP+ob_j),1       ; 回る: 左と右を 2 コマずつ
        MV    A,[count]
        TEST  A,2
        JRZ   draw_player1
        MV    (BP+ob_j),2
draw_player1:
        MV    A,(BP+ob_j)       ; コマ = ブレーキ × 6 + 向き × 2 + タイヤ
        ADD   A,A
        MV    (BP+ob_j),A
        MV    A,[pos]
        TEST  A,$80
        JRZ   draw_player2
        INC   (BP+ob_j)
draw_player2:
        MV    A,[brake]
        CMP   A,0
        JRZ   draw_player3
        ADD   (BP+ob_j),6
draw_player3:
        MV    (BP+ob_d),112     ; x
        MV    A,[speed]
        CMP   A,0
        JRZ   draw_player5
        MV    A,[offroad]
        CMP   A,0
        JRZ   draw_player5
        MV    A,[count]         ; 道の外: 左右にゆれて、砂ぼこり
        TEST  A,1
        JRZ   draw_player4
        MV    (BP+ob_d),113
draw_player4:
        AND   A,1
        MV    (BP+ob_x),A
        CALL  dust_at
        MV    Y,screen+720+102
        CALL  blit
        MV    A,(BP+ob_x)
        XOR   A,1
        CALL  dust_at
        MV    Y,screen+720+128
        CALL  blit
draw_player5:
        MV    A,(BP+ob_j)       ; X = car_p + 4 + コマ × 32
        MV    (BP+t0),A
        MV    (BP+t0+1),0
        MV    BA,(BP+t0)
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA
        ADD   BA,BA
        MV    X,[c_spr]
        ADD   X,BA
        MV    A,4
        ADD   X,A
        MV    Y,screen+720
        MV    A,(BP+ob_d)
        ADD   Y,A
        MV    IL,16
; X の絵（マスク、絵の 2 バイトずつ）を Y から IL 桁、行にそろえて描く。
blit:   MV    A,[X++]
        MV    (BP+vtmp),A
        MV    A,[Y]
        OR    A,(BP+vtmp)
        XOR   A,(BP+vtmp)
        MV    (BP+vtmp),A
        MV    A,[X++]
        OR    A,(BP+vtmp)
        MV    [Y++],A
        DEC   IL
        JRNZ  blit
        RET

; X = 砂ぼこりの A 番目のコマ、IL = 幅
dust_at:
        MV    X,dust+4
        CMP   A,0
        JRZ   dust_at1
        MV    A,12
        ADD   X,A
dust_at1:
        MV    IL,6
        RET

; ---- アイテム: 車と同じように置く（遠い順に 3 段の大きさ、コマ = 種類） ----
draw_items:
        MV    X,items
        MV    (ob_side),NITEMS
draw_items1:
        PUSHU X
        CALL  draw_item
        POPU  X
        MV    A,6
        ADD   X,A
        DEC   (ob_side)
        JRNZ  draw_items1
        RET

draw_item:
        MV    A,[X+5]
        CMP   A,0
        JPZ   draw_item9
        MV    BA,[X]
        MV    (t1),BA
        TEST  (t1+1),$E0
        JPNZ  draw_item9
        MV    A,[X+4]
        MV    (ob_j),A          ; 種類（コマ）
        MV    BA,[X+2]
        MV    (t0),BA
        PUSHU X
        CALL  z_to_line
        POPU  X
        CMP   A,3
        JPC   draw_item9
        CMP   A,23
        JRNZ  draw_item1
        MV    A,22
draw_item1:
        MV    (ob_d),A
        MV    (cnt),A           ; x = 中心 + 横の位置 × d / 256
        MV    BA,0
        MV    I,(t0)
draw_item2:
        ADD   BA,I
        DEC   (cnt)
        JRNZ  draw_item2
        EX    A,B
        XOR   A,$80
        MV    (t0),A
        MV    (t0+1),0
        MV    X,lined
        MV    A,(ob_d)
        ADD   X,A
        ADD   X,A
        MV    A,[X]
        MV    (t1),A
        MV    (t1+1),0
        MV    BA,(t1)
        MV    I,(t0)
        ADD   BA,I              ; x + 8 + 128
        MV    X,item_s
        MV    I,136+2
        CMP   (ob_d),8
        JRC   draw_item4
        MV    X,item_m
        MV    I,136+4
        CMP   (ob_d),15
        JRC   draw_item4
        MV    X,item_l
        MV    I,136+6
draw_item4:
        SUB   BA,I
        MV    (sprite_x),BA
        MV    I,VIEW_L          ; 描く範囲に全部入るときだけ
        SUB   BA,I
        MV    (t1),BA
        MV    A,[X]
        MV    (t0),A
        MV    A,VIEW_R-VIEW_L+1
        SUB   A,(t0)
        MV    (t0),A
        MV    (t0+1),0
        MV    BA,(t0)
        MV    I,(t1)
        SUB   BA,I
        JRC   draw_item9
        MV    A,(ob_d)
        ADD   A,2
        MV    (sprite_y),A
        MV    A,(ob_j)
        CALL  sprite_draw
draw_item9:
        RET

; ---- ターボの線: 道の両側に、手前へ流れる短い横線 ----
draw_turbo:
        MV    A,[turbo]
        CMP   A,0
        JRZ   draw_turbo9
        MV    X,turbo_lines
        MV    (BP+ob_j),4
draw_turbo1:
        MV    A,[X++]           ; y
        MV    (BP+t0),A
        MV    A,[X++]           ; 流れる速さ
        MV    (BP+tch),A
        PUSHU X
        MV    A,(BP+t0)         ; そのラインの先頭 - 8 とビット
        MV    Y,y_off
        ADD   Y,A
        ADD   Y,A
        MV    I,[Y]
        MV    X,screen-8
        ADD   X,I
        MV    (BP+base),X
        MV    A,(BP+t0)
        AND   A,7
        MV    Y,top_mask
        ADD   Y,A
        MV    A,[Y]
        MV    (BP+vtmp),A
        MV    Y,bot_mask
        MV    A,(BP+t0)
        AND   A,7
        ADD   Y,A
        MV    A,[Y]
        AND   A,(BP+vtmp)
        MV    [hrun_or+1],A
        MV    A,[count]         ; 左: VIEW_L から内へ流れる
        MV    (BP+vtmp),A
        MV    A,0
draw_turbo2:
        ADD   A,(BP+vtmp)
        DEC   (BP+tch)
        JRNZ  draw_turbo2
        AND   A,31
        MV    (BP+vtmp),A
        ADD   A,VIEW_L+8
        MV    (BP+ra),A
        ADD   A,6
        MV    (BP+rb),A
        CALL  hrun
        MV    A,VIEW_R+8-6      ; 右: VIEW_R から内へ
        SUB   A,(BP+vtmp)
        MV    (BP+ra),A
        ADD   A,6
        MV    (BP+rb),A
        CALL  hrun
        POPU  X
        DEC   (BP+ob_j)
        JRNZ  draw_turbo1
draw_turbo9:
        RET
turbo_lines:                    ; y、流れる速さ（コマあたりのドット）
        DB    12,3,18,5,24,4,29,6
