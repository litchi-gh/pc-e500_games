; road_draw.inc: 道を描く（pokerace.asm から INCLUDE する）
;
; 手前のラインから奥へ 1 ラインずつ描く。カーブは奥ほど大きくずれ（1 ラインごとに曲がりを足していく）、
; 自分の車の横の位置は手前ほど大きくずれる。ラインの中の x は 8 を足した 1 バイト
; （中心のずれは -128〜127 なので、中心は 0〜255 に入る）。
; 描くもの: 左右の路肩（縞のときは太く、1 つ手前のラインの端までつなぐ）、車線の線 2 本（縞の反対のとき）。
; ラインごとの中心と木の位置を lined に残す（木、ゲート、車を置くため）。
;
; 速さのために: 内部 RAM は (BP+n) で書く（BP = 0。PRE が付かず 1 サイクル少ない）。表は X、lined は U、
; 線を引く先は Y で指したまま回す。中心は v = (カーブのずれ) - (横の位置 × d) を 1 本の数で持ち、
; 1 ラインごとに w += カーブ、v += w の 2 回の足し算で次のラインへ進む。

rd_v    EQU   $02               ; 中心のずれ（2 バイト、上位 + 128 が中心 + 8）
rd_w    EQU   $04               ; v の増え方（2 バイト）
rd_mask EQU   $19               ; そのラインのビット
rd_lane EQU   $18               ; 車線の線の位置
rd_pos  EQU   $1C               ; 位置の写し（2 バイト）
rd_segr EQU   $1E               ; 区間の残りの写し（2 バイト）

draw_road:
        CALL  curve_now
        MV    (BP+cc),BA
        CALL  next_seg          ; 次の区間のカーブ
        MV    A,[X+1]
        CALL  sext
        MV    (BP+nc),BA
        MV    BA,[pos]
        MV    (BP+rd_pos),BA
        MV    BA,[seg_rem]
        MV    (BP+rd_segr),BA
        MV    BA,[plat]         ; w = 横の位置、v = -(横の位置 × 22)
        MV    (BP+rd_w),BA
        ADD   BA,BA             ; × 2
        MV    I,BA
        ADD   BA,BA             ; × 4
        ADD   I,BA              ; × 6
        ADD   BA,BA
        ADD   BA,BA             ; × 16
        ADD   BA,I              ; × 22
        MV    I,BA
        MV    BA,0
        SUB   BA,I
        MV    (BP+rd_v),BA
        MV    [save_u],U
        MV    U,lined+LINES*2+2
        MV    X,road_lines
        MV    (BP+ln_d),LINES

draw_road2:
        CALL  tick_poll         ; 16 ミリ秒のタイマとエンジンの音
        MVP   (BP+base),[X++]   ; 表から 1 ライン分を読む
        MV    A,[X++]
        MV    [hrun_or+1],A     ; ビットは hrun の OR に直接書く
        MV    (BP+rd_mask),A
        MV    A,[X++]
        MV    (BP+hw),A
        MV    A,[X++]
        MV    (BP+rw),A
        MV    A,[X++]
        MV    (BP+rd_lane),A
        MV    A,[X++]
        MV    [--U],A           ; 木の位置
        MV    BA,[X++]          ; 奥行き
        CMPW  (BP+rd_segr),BA   ; 区間の境目より奥なら、ここから次の区間のカーブ
        JRC   draw_road2b
        JRNZ  draw_road2c
draw_road2b:
        MVW   (BP+cc),(BP+nc)
draw_road2c:
        MV    I,(BP+rd_pos)     ; 縞 = (奥行き + 位置) の 256 ごと
        ADD   BA,I
        MV    A,B
        AND   A,1
        MV    (BP+stripe),A
        MV    A,(BP+rd_v+1)     ; 中心 + 8 = v の上位 + 128
        XOR   A,$80
        MV    (BP+cx),A
        MV    [--U],A
        ; 左の路肩: L = 中心 - 道幅。縞のときは太くし、1 つ手前のラインの端までつなぐ
        SUB   A,(BP+hw)
        JRNC  draw_road3
        MV    A,0
draw_road3:
        MV    (BP+rb),A
        MV    (BP+ra),A
        CMP   (BP+stripe),0
        JRZ   draw_road4
        SUB   A,(BP+rw)
        JRNC  draw_road3a
        MV    A,0
draw_road3a:
        MV    (BP+ra),A
draw_road4:
        MV    A,(BP+prev_l)
        MV    (BP+prev_l),(BP+rb)
        CMP   (BP+ln_d),LINES   ; いちばん手前はつながない
        JRZ   draw_road5
        CALL  join
draw_road5:
        CALL  hrun
        ; 右の路肩: R = 中心 + 道幅
        MV    A,(BP+cx)
        ADD   A,(BP+hw)
        JRNC  draw_road6
        MV    A,255
draw_road6:
        MV    (BP+ra),A
        MV    (BP+rb),A
        CMP   (BP+stripe),0
        JRZ   draw_road7
        ADD   A,(BP+rw)
        JRNC  draw_road6a
        MV    A,255
draw_road6a:
        MV    (BP+rb),A
draw_road7:
        MV    A,(BP+prev_r)
        MV    (BP+prev_r),(BP+ra)
        CMP   (BP+ln_d),LINES
        JRZ   draw_road8
        CALL  join
draw_road8:
        CALL  hrun
        ; 車線の線（縞の反対の時。遠すぎるラインには描かない）。中心 ± rd_lane に点を打つ
        CMP   (BP+stripe),0
        JRNZ  draw_road10
        CMP   (BP+ln_d),3
        JRC   draw_road10
        MV    A,(BP+cx)
        SUB   A,(BP+rd_lane)
        JRC   draw_road9
        CALL  lane_dot
draw_road9:
        MV    A,(BP+cx)
        ADD   A,(BP+rd_lane)
        JRC   draw_road10
        CALL  lane_dot
draw_road10:
        ; 次のライン: w += カーブ、v += w
        MV    BA,(BP+cc)
        MV    I,(BP+rd_w)
        ADD   BA,I
        MV    (BP+rd_w),BA
        MV    I,(BP+rd_v)
        ADD   BA,I
        MV    (BP+rd_v),BA
        DEC   (BP+ln_d)
        JPNZ  draw_road2
        MV    U,[save_u]
        RET

; (ra)〜(rb) を、1 つ手前のラインの端 A までのばす（ra = min(ra, A+1)、rb = max(rb, A-1)）。
join:   CMP   A,255
        JRZ   join1
        INC   A
        CMP   (BP+ra),A
        JRC   join0
        MV    (BP+ra),A
join0:  DEC   A
join1:  CMP   A,0
        JRZ   join2
        DEC   A
        CMP   (BP+rb),A
        JRNC  join2
        MV    (BP+rb),A
join2:  RET

; A（x + 8）に車線の点（手前の 11 ラインは 2 ドット）。描く範囲の外には打たない。
lane_dot:
        CMP   A,VIEW_L+8
        JRC   lane_dot1
        CMP   A,VIEW_R+8
        JRNC  lane_dot1
        MV    Y,(BP+base)
        ADD   Y,A
        MV    A,[Y]
        OR    A,(BP+rd_mask)
        MV    [Y],A
        CMP   (BP+ln_d),12
        JRC   lane_dot1
        MV    A,[Y+1]
        OR    A,(BP+rd_mask)
        MV    [Y+1],A
lane_dot1:
        RET

; (ra)〜(rb)（x + 8）に横線を引く。描く範囲の外は切る。(base) と hrun_or のビットを使う。
; ra、rb、Y、IL、cnt が変わる。
hrun:   MV    A,(BP+ra)
        CMP   A,VIEW_L+8
        JRNC  hrun1
        MV    A,VIEW_L+8
        MV    (BP+ra),A
hrun1:  MV    A,(BP+rb)
        CMP   A,VIEW_R+9
        JRC   hrun2
        MV    A,VIEW_R+8
hrun2:  SUB   A,(BP+ra)
        JRC   hrun4
        INC   A
        MV    (BP+cnt),A
        MV    IL,(BP+cnt)
        MV    Y,(BP+base)
        MV    A,(BP+ra)
        ADD   Y,A
hrun3:  MV    A,[Y]
hrun_or:
        OR    A,0               ; 0 の所にラインのビットが入る
        MV    [Y++],A
        DEC   IL
        JRNZ  hrun3
hrun4:  RET
