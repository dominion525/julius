JCONTROL(1)                                                        JCONTROL(1)



NAME
       jcontrol - simple program to control Julius / Julian module via API

SYNOPSIS
       jcontrol hostname [portnum]

DESCRIPTION
       jcontrol  は，他のホストで動作中の julius を，APIを介してコントロールす
       る簡単なコンソールプログラムです． Julius へのコ マ ン ド 送 信， お よ
       びJuliusからのメッセージ受信を行うことができます．

       起動後，jcontrol は，指定ホスト上において「モジュールモード」で動作中の
       Julius または Julian に対し，接続を試みます．接続確立 後，jcontrol   は
       ユーザーからのコマンド入力待ち状態となります．

       jcontrol   は ユーザーが入力したコマンドを解釈し，対応するAPIコマンドを
       Julius へ送信します．また，Julius から認識結果や入力トリガ情報 な ど の
       メッセージが送信されてきたときは，その内容を標準出力へ書き出します．

       APIの詳細については関連文書をご覧下さい．

OPTIONS
       hostname
              接続先のホスト名（Julius / Julian がモジュールモードで動作中）

       portnum
              (optional) ポート番号 (default=10500)

COMMANDS (COMMON)
       起 動 後，jcontrol に対して以下のコマンド文字列を標準入力から入力できま
       す．

       pause  認識を中断する．認識途中の場合，そこで入力を中断して第2パスま で
              認識が終わってから中断する．

       terminate
              認識を中断する．認識途中の場合，入力を破棄して即時中断する．

       resume 認識を再開．

       inputparam arg
              文 法 切り替え時に音声入力であった場合の入力中音声の扱いを指定．
              "TERMINATE", "PAUSE", "WAIT"のうちいずれかを指定．

       version
              バージョン文字列を返す

       status システムの状態(active/sleep)を返す．

GRAMMAR COMMANDS (Julian)
       Julian 用のコマンドです：

       changegram prefix
              認識文法を "prefix.dfa" と "prefix.dict" に切り替え る．  Julian
              内の文法は全て消去され，指定された文法に置き換わる．

       addgram prefix
              認識文法として "prefix.dfa" と "prefix.dict" を追加する．

       deletegram ID
              指定されたIDの認識文法を削除する．指定文法は Julian から削除され
              る．ID は Julian から送られる GRAMINFO 内に記述されている．

       deactivategram ID
              指定されたIDの認識文法を，一時的にOFFにする．OFFにされた文法は認
              識 処理から一時的に除外される．このOFFにされた文法は Julian 内に
              保持され， "activategram" コマンドで再び ON にできる．

       activategram ID
              一時的に OFF になっていた文法を再び ON にする．

EXAMPLE
       Julius および Julian からのメッセージは "> " を行の先頭につけてそのまま
       標 準出力に出力されます．出力内容の詳細については，関連文書を参照してく
       ださい．

       (1) Julius / Julian をモジュールモードでホスト host で起動する．
           % julian -C xxx.jconf ... -input mic -module

       (2) (他の端末で) jcontrol を起動し，通信を開始する．
           % jcontrol host
           connecting to host:10500...done
           > <GRAMINFO>
           >  # 0: [active] 99words, 42categories, 135nodes (new)
           > </GRAMINFO>
           > <GRAMINFO>
           >  # 0: [active] 99words, 42categories, 135 nodes
           >   Grobal:      99words, 42categories, 135nodes
           > </GRAMINFO>
           > <INPUT STATUS="LISTEN" TIME="1031583083"/>
        -> pause
        -> resume
           > <INPUT STATUS="LISTEN" TIME="1031583386"/>
        -> addgram test
           ....


SEE ALSO
       julius(1), julian(1)

BUGS
       バグ報告・問い合わせ・コメントなどは 
       julius-info at lists.sourceforge.jp までお願いします．

VERSION
       This version is provided as part of Julius-3.5.1.

COPYRIGHT
       Copyright (c) 2002-2006 京都大学 河原研究室
       Copyright (c) 2002-2005 奈良先端科学技術大学院大学 鹿野研究室
       Copyright (c) 2005-2006 名古屋工業大学 Julius開発チーム

AUTHORS
       李 晃伸 (名古屋工業大学) が実装しました．

LICENSE
       Julius の使用許諾に準じます．



4.3 Berkeley Distribution            LOCAL                         JCONTROL(1)
