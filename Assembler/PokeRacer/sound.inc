; sound.inc: ブザーで曲を鳴らす（pokerace.asm から INCLUDE する）
;
; ブザーは SCR のビット 4 を自分の周期で 1 と 0 に切り替えて鳴らす（2 kHz、4 kHz 以外の高さ）。
; 曲の表は make_data.py が作る sound_data.inc: 1 音 = DW 半周期のサイクル数, DW 周期の数。
; 休符は DW 0, 16 ミリ秒の回数。$FFFF で終わり。鳴らしている間は止まる（割り込みは止めてある）。

; X の曲を鳴らす。
play:   AND   (SCR),BZ_OFF
play1:  MV    BA,[X++]
        MV    (BP+snd_h),BA
        MV    BA,[X++]
        MV    (BP+snd_n),BA
        MV    BA,(BP+snd_h)
        INC   BA
        JRZ   play9             ; $FFFF
        DEC   BA
        MV    A,(BP+snd_h)
        OR    A,(BP+snd_h+1)
        JRNZ  play2
        MV    A,(BP+snd_n)      ; 休符
        PUSHS X
        CALL  game_wait
        POPS  X
        JR    play1
play2:  CALL  tone
        JR    play1
play9:  AND   (SCR),BZ_OFF
        RET

; 半周期 (snd_h) サイクル、(snd_n) 周期だけ鳴らす。1 周期 = 2 × 半周期 + 34 サイクル。
tone:   OR    (SCR),$10
        MV    I,(BP+snd_h)
        WAIT
        AND   (SCR),$8F
        MV    I,(BP+snd_h)
        WAIT
        MV    BA,(BP+snd_n)
        DEC   BA
        MV    (BP+snd_n),BA
        JRNZ  tone
        RET

; 短い音（メニューを動かしたとき）
blip:   MVW   (BP+snd_h),200
        MVW   (BP+snd_n),60
        JR    tone
; もっと短い音（集計で数えるとき）
tick:   MVW   (BP+snd_h),150
        MVW   (BP+snd_n),20
        JR    tone
