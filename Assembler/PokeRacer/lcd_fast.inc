; lcd_fast.inc: 仮想画面を LCD に速く送る（pokerace.asm から INCLUDE する）
;
; 毎コマ送るのは道の所（x = 43〜196、616 バイト）だけにして、計器は変わったときだけ送る。
;
; データは 14 サイクル以上の間をあけて書く。LCD コントローラは 1 バイト受け取ると
; しばらく BUSY になり、その間に来たデータを捨てる。実機（PC-E650）で試すと、待たずに書く間隔が
; 6、8、10 サイクルでは取りこぼし、14 サイクルでは取りこぼさなかった。ROM は 1 バイトに
; 23〜28 サイクルかけている。エミュレータは BUSY を真似しないので、エミュレータでは崩れない。
;
; LCD のアドレスは 0A00xh 台を使う。LCD は A0〜A3、A12、A13 だけを見るので 2000h 台と同じ
; コントローラに届く。2000h〜3FFFh は CPU が LCD の窓として 1 回 7 ステートに遅くする範囲で、
; 0A00xh は ROM が表示に使っているアドレス（解析 4 章）。
; 左から右へ並ぶ所は U をポインタにして POPU A で読む。U を戻すまで PUSHU は使えない。

LCD_A2  EQU   $A008             ; 中央のコントローラ: 命令、+1 状態、+2 データ
LCD_B2  EQU   $A004             ; 両端のコントローラ

; 道の所と、変わった計器の行を送る。
show:   CALL  show_view
        MV    A,[pdirty]
        CMP   A,0
        JRZ   show1
        JP    show_panels
show1:  RET

; 画面全体を送る（メニューの画面）。
show_all:
        MV    A,1
        MV    [row0_dirty],A
        MV    A,$FF
        MV    [pdirty],A
        JR    show

; 道の所（x = 43〜196）を送る。
show_view:
        MV    [save_u],U
        MV    X,screen
        MV    (cnt),0           ; 行
        MV    A,[row0_dirty]    ; 0 行が前と同じなら 1 行目から
        CMP   A,0
        JRNZ  show_view1
        MV    (cnt),1
        MV    BA,240
        ADD   X,BA
show_view1:
        CALL  tick_poll
        MV    (t0),LCD_COLUMN+43        ; x = 43〜63: 両端のコントローラ、桁 43 から
        MV    A,(cnt)
        ADD   A,LCD_PAGE
        CALL  set_b2
        MV    U,X
        MV    A,43
        ADD   U,A
        MV    Y,LCD_B2+2
        MV    IL,21
        CALL  put_fwd
        MV    (t0),LCD_COLUMN           ; x = 64〜119: 中央、桁 0 から
        MV    A,(cnt)
        ADD   A,LCD_PAGE
        CALL  set_a2
        MV    Y,LCD_A2+2
        MV    IL,56
        CALL  put_fwd
        MV    A,176                     ; x = 175〜120: 中央の下半分（右から左）
        ADD   X,A
        MV    A,(cnt)
        ADD   A,LCD_PAGE+4
        CALL  set_a2
        MV    Y,LCD_A2+2
        MV    IL,56
        CALL  put_rev
        MV    A,77                      ; x = 196〜176: 両端の下半分、桁 43 から
        ADD   X,A
        MV    (t0),LCD_COLUMN+43
        MV    A,(cnt)
        ADD   A,LCD_PAGE+4
        CALL  set_b2
        MV    Y,LCD_B2+2
        MV    IL,21
        CALL  put_rev
        MV    A,64                      ; 次の行
        ADD   X,A
        INC   (cnt)
        CMP   (cnt),4
        JRNZ  show_view1
        MV    U,[save_u]
        RET

; 計器（x = 0〜42 と 197〜239。どちらも両端のコントローラ）のうち、pdirty のビットの行を送る。
show_panels:
        MV    A,[pdirty]
        MV    (BP+t1),A
        MV    A,0
        MV    [pdirty],A
        MV    [save_u],U
        MV    X,screen
        MV    (cnt),0
show_panels1:
        CALL  tick_poll
        TEST  (BP+t1),$01       ; 左
        JRZ   show_panels2
        MV    (t0),LCD_COLUMN
        MV    A,(cnt)
        ADD   A,LCD_PAGE
        CALL  set_b2
        MV    U,X
        MV    Y,LCD_B2+2
        MV    IL,43
        CALL  put_fwd
show_panels2:
        TEST  (BP+t1),$10       ; 右（x = 239〜197）
        JRZ   show_panels3
        MV    (t0),LCD_COLUMN
        MV    A,(cnt)
        ADD   A,LCD_PAGE+4
        CALL  set_b2
        PUSHS X
        MV    A,240
        ADD   X,A
        MV    Y,LCD_B2+2
        MV    IL,43
        CALL  put_rev
        POPS  X
show_panels3:
        MV    A,240
        ADD   X,A
        RC
        SHR   (BP+t1)
        INC   (cnt)
        CMP   (cnt),4
        JRNZ  show_panels1
        MV    U,[save_u]
        RET

; コントローラのページ A、桁 (t0) にする（BUSY を待つ）。U は使わない。
set_a2: MV    Y,LCD_A2
        JR    set_ab2
set_b2: MV    Y,LCD_B2
set_ab2:
        MV    (t0+1),A
set_ab2a:
        MV    A,[Y+1]
        TEST  A,LCD_BUSY
        JRNZ  set_ab2a
        MV    A,(t0+1)
        MV    [Y],A
set_ab2b:
        MV    A,[Y+1]
        TEST  A,LCD_BUSY
        JRNZ  set_ab2b
        MV    A,(t0)
        MV    [Y],A
set_ab2c:
        MV    A,[Y+1]
        TEST  A,LCD_BUSY
        JRNZ  set_ab2c
        RET

; [U++] から [Y] へ IL バイト。書く間隔は 14 サイクル（NOP 2 つで合わせる）。
put_fwd:
        POPU  A
        NOP
        NOP
        MV    [Y],A
        DEC   IL
        JRNZ  put_fwd
        RET

; [--X] から [Y] へ IL バイト。書く間隔は 15 サイクル。
put_rev:
        MV    A,[--X]
        MV    [Y],A
        DEC   IL
        JRNZ  put_rev
        RET
