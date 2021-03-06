ADINTOOL(1)                                                        ADINTOOL(1)



NAME
       adintool  -  audio  tool  to  record/split/send/receive speech data for
       Julius

SYNOPSIS
       adintool -in inputdev -out outputdev [options...]

DESCRIPTION
       adintool は，音声波形データ中の音声区間の検出および記録を連続的に 行 う
       ツー ルです．入力音声に対して零交差数と振幅レベルに基づく音声区間検出を
       逐次行い，音声区間部分を連続出力します．

       adintool は adinrec の高機能版です．音声データの入力元として，マイク 入
       力・ 音 声波形ファイル・標準入力・ネットワーク入力(adinnet サーバーモー
       ド)が選択できます．また，出力先として，音声波形ファイル・標準出力・ネッ
       ト ワーク出力(adinnet クライアントモード)が選択できます．特にネットワー
       ク出力（adinnet クライアントモード）では， julius へネットワーク経由 で
       音声を送信して音声認識させることができます．

       入 力音声は音声区間ごとに自動分割され，逐次出力されます．音声区間の切り
       出しには adinrec と同じ，一定時間内の零交差数とパワー（振幅レベル）のし
       き い値を用います．音声区間開始と同時に音声出力が開始されます．出力とし
       てファイル出力を選んだ場合は，連番ファイル名で検出された区間ごとに保 存
       します．

       サンプリング周波数は任意に設定可能です．形式は 16bit monoral です．書き
       出されるデータ形式は WAV, 16bit, モノラルです．既に同じ名前のファイルが
       存在する場合は上書きします．

INPUT
       音声を読み込む入力デバイスは以下のうちどれかを指定します．

       -in mic
              マイク入力（デフォルト）．

       -in file
              音声波形ファイル．形式は RAW (16bit big endian)，WAV(無圧縮)など
              （コンパイル時の設定による）．
              なお，入力ファイル名は起動後に，プロンプトに対して入力する．

       -in adinnet
              adinnet サーバーとなってネットワーク経由で adinnet クライアン ト
              か ら音声データを受け取る． adinnet クライアントからのTCP/IP接続
              を待ち，接続が確立した後は adinnet クライアントから音声データ を
              受け取る．
              ポー ト 番号のデフォルトは 5530 である．これはオプション "-port"
              で変更可能．

       -in netaudio
              (サポートされていれば）音声データをNetaudio/DatLinkサーバーか ら
              受 け取る．サーバのホスト名とユニット名を "-NA host:unit" で指定
              する必要がある．

       -in stdin
              標準入力．音声データ形式は RAW, WAV のみ．

OUTPUT
       検出した音声区間の音声データを書き出す出力デバイスとして，以下のうち ど
       れかを指定します．

       -out file
              ファ イ ル へ 出力する．出力ファイル名は別のオプション"-filename
              foobar" の形で与える．実際には "foobar.0000" , "foobar.0001" ...
              の ように区間ごとに，指定した名前の末尾に4桁のIDをつけた名前で記
              録されなる． ID は 0 を初期値として，音声区間検出ごとに１増加 す
              る． 初 期値はオプション "-startid" で変更可能である．また，出力
              ファイル形式は 16bit WAV 形式である． RAW 形式で出力 す る に は
              "-raw" オプションを指定する．

       -out adinnet
              adinnet  クライアントとなって，ネットワーク経由で adinnet サーバ
              へ音声データを送る．入力の時とは逆に， adintool は adinnet ク ラ
              イ ア ン ト となり，adinnet サーバーへ接続後，音声データを送信す
              る．adinnet サーバーとしては， adintool および Julius  のadinnet
              入力が挙げられる．
              "-server"  で送信先のadinnetサーバのホスト名を指定する．またポー
              ト番号のデフォルトは 5530 である．これはオプション "-port" で 変
              更可能．

       -out stdout
              標 準 出力へ出力する．形式は RAW, 16bit signed (big endian) であ
              る．


OPTIONS
       -nosegment
              入力音声に対して音声区間の検出を行わず，そのまま出力へリダイレク
              ト する．ファイル出力の場合，ファイル名の末尾に4桁のIDは付与され
              なくなる．

       -oneshot
              入力開始後，一番最初の１音声区間のみを送信後，終了する．

       -freq threshold
              サンプリング周波数．単位は Hz (default: 16000)

       -lv threslevel
              波形の振幅レベルのしきい値 (0 - 32767)．(default: 2000)．

       -zc zerocrossnum
              １秒あたりの零交差数のしきい値 (default: 60)

       -headmargin msec
              音声区間開始部の直前のマージン．単位はミリ秒 (default: 400)

       -tailmargin msec
              音声区間終了部の直後のマージン．単位はミリ秒 (default: 400)

       -nostrip
              無効な 0 サンプルの自動除去を行わないようにする．デフォルトは 自
              動除去を行う．

       -zmean DC成分除去を行う．

       -raw   ファイル出力形式を RAW, 16bit signed (big endian) にする．デフォ
              ルトは WAV 形式である．

EXAMPLE
       マイクからの音声入力を，発話ごとに "data.0000.wav" から順に記録する：

           % adintool -in mic -out file -filename data

       巨大な収録音声ファイル "foobar.raw"を音声区間ごと に  "foobar.1500.wav"
       "foobar.1501.wav" ... に分割する：

           % adintool -in file -out file -filename foobar
             -startid 1500
             (起動後プロンプトに対してファイル名を入力)
             enter filename->foobar.raw

       ネットワーク経由で音声ファイルを転送する(区間検出なし)：

         [受信側]
           % adintool -in adinnet -out file -nosegment
         [送信側]
           % adintool -in file -out adinnet -server hostname
             -nosegment

       マイクからの入力音声を別サーバーの Julius に送る：

       (1) 入力データを全て送信し，Julius側で区間検出・認識：

         [Julius]
           % julius -C xxx.jconf ... -input adinnet
         [adintool]
           % adintool -in mic -out adinnet -server hostname
             -nosegment

       (2)  入力データはクライアント(adintool)側で区間検出し，検出した区間だけ
       を順に Julius へ送信・認識：

         [Julius]
           % julius -C xxx.jconf ... -input adinnet
         [adintool]
           % adintool -in mic -out adinnet -server hostname

SEE ALSO
       julius(1), adinrec(1)

BUGS
       バグ報告・問い合わせ・コメントなどは
       julius-info at lists.sourceforge.jp までお願いします．

VERSION
       This version is provided as part of Julius-3.5.1.

COPYRIGHT
       Copyright (c)   1991-2006 京都大学 河原研究室
       Copyright (c)   2000-2005 奈良先端科学技術大学院大学 鹿野研究室
       Copyright (c)   2005-2006 名古屋工業大学 Julius開発チーム

AUTHORS
       李 晃伸 (名古屋工業大学) が実装しました．

LICENSE
       Julius の使用許諾に準じます．



4.3 Berkeley Distribution            LOCAL                         ADINTOOL(1)
