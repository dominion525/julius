ここには Julian 用の文法関連ツールが収められています．

mkdfa (mkdfa.pl) 文法コンパイラ
dfa_minimize	DFA最小化ツール
generate	文ランダム生成ツール
accept_check	単語列の受理/非受理チェックツール
nextword	次単語予測チェックツール（accept_checkの高機能版）
gram2sapixml	Julian 形式の文法を SAPI XML 文法に変換するスクリプト
dfa_determinize	DFA決定化ツール

======================================================================
□コンパイル

親ディレクトリの Julius をインストールすることで，
本ツール群も自動的にコンパイル・インストールされます．

本ツール群のみをコンパイルする場合の方法を以下に説明します．
コンパイルには Julius のライブラリのソースが必要です．
下記の説明は，本ディレクトリが Julius のソースツリーの下にあることを
前提としています．

 0) コンパイルおよび実行には，以下のツールとライブラリが必要です．

	・perl (ver.5)
	・GNU bison
	・GNU flex
	・GNU readline ライブラリ
	・iconv
	・perl の Jcodeモジュール

 1) 親ディレクトリで julius をコンパイルする
    （インストールはしなくてよい）

	% cd ..
	% ./configure
	% make
	% cd gramtools

 2) 本ディレクトリで configure と make を実行する．

	% ./configure
	% make

 4) 出来上がった実行バイナリを "make install" で /usr/local/bin に
    インストールする．

	% make install



======================================================================
======================================================================
======================================================================
以下はマニュアルです．


======================================================================
□ mkdfa.pl --- 文法コンパイラ

  mkdfa.pl は Julian 用の文法コンパイラです．記述された文法ファイル.
grammar と語彙ファイル .voca から Julian用の DFA と認識辞書を生成します．

  与える文法は正規文法のクラスであることが必要です．
  フォーマットの詳細は別途ドキュメントをご覧下さい．

  mkdfa.plの使い方は，以下のように .grammar および .vocaファイルのプレ
フィックスを引数として与えます．生成された各ファイルは，上記ファイルと
同じディレクトリに格納されます．

------------------------------------------------------------
    実行例：../sample_grammars/vfr/vfr.{grammar,voca}に対して

        % mkdfa.pl ../sample_grammars/vfr/vfr

    とすると ../sample_grammars/vfr/vfr.{dfa,dict,term} が生成される．
------------------------------------------------------------

  内部では，読み込んだ文法定義からNFAを生成し，それをDFAに変換するとと
もに最小化を行ないます．.dfa ファイルにはカテゴリ単位の構文規則が，.
dict ファイルにはカテゴリごとの登録語彙とその発音の辞書が生成されます．

  生成された .dfa ファイルでは入力シンボルはカテゴリのIDとなります．
カテゴリIDと .grammar 内でのカテゴリ名の対応はコンパイル時に .term ファ
イルに書き出されます．このファイルは generate や nextword などで使用さ
れますので，残しておいて下さい．

  mkdfa の実体は，コンパイラ本体の mkfa，および実行スクリプト mkdfa.pl
の２つのプログラムからなります．通常は mkdfa.pl から起動してください．

  各入力・出力ファイルの形式の詳細については，別紙を参照して下さい．


======================================================================
□ dfa_minimize --- DFA最小化ツール

  DFA を有限オートマトンの最小化アルゴリズムに従って，
等価な最小化のDFAに変換します．

このツールは mkdfa.pl で DFA を生成する際に自動的に呼び出されます．
既存の DFA を最小化のみ行う場合は下記の要領で使ってください．

  コマンドラインで入力とするDFAのファイル名を指定します．指定がない
場合は標準入力から読み込みます．また，出力するDFAファイルを
"-o ファイル名" で指定できます．指定がない場合は標準出力へ出力します．
------------------------------------------------------------
    使用例1: 

        % dfa_minimize vfr.dfa -o vfr-min.dfa

    使用例2: 

        % cat vfr.dfa | dfa_minimize > vfr-min.dfa

------------------------------------------------------------


======================================================================
□ generate --- 文ランダム生成ツール

  文法に従って文をランダムに生成します．非文を生成（受理）しないかチェッ
クすることができます．

  実行には .dfa, .dict, .term が必要です．あらかじめ mkdfa.pl で
生成しておいて下さい．

----- 実行例 -------------------------------------------
    % bin/generate ../sample_grammars/vfr/vfr		<-- 入力
    Reading in dictionary...done
    Reading in DFA grammar...done
    Mapping dict item <-> DFA terminal (category)...done
    Reading in term file (optional)...done
    42 categories, 99 words
    DFA has 135 nodes and 198 arcs
    ----- 
     silB やめます silE
     silB 終了します silE
     silB シャツ を スーツ と 統一して 下さい silE
     silB スーツ を カッター と 同じ 色 に 統一して 下さい silE
     silB 交換して 下さい silE
     silB これ を 覚えておいて 下さい silE
     silB 覚えておいて 下さい silE
     silB 戻って 下さい silE
     silB スーツ を シャツ と 統一して 下さい silE
     silB 上着 を 橙 に して 下さい silE
    %
--------------------------------------------------------
オプション "-n num" で，生成する文数を指定できます．デフォルトは10です．
オプション "-t" で単語の代わりにカテゴリ名で出力します (.term存在時)．


======================================================================
□ accept_check  --- 単語列の受理/非受理チェックツール

  文の受理／非受理を判定するツールです．文は標準入力から分かち書きされ
た単語列として与える必要があります．使用する文法は文法は事前に mkdfa.pl
にかけて .dfa, .dict, .term を生成しておいて下さい．

  カバーしたいtranscriptionを，ファイルにまとめて書いておいて，それを
accept_check の標準入力に与えることで，目的の文が受理可能かどうかを
バッチ的にチェックできます．

  mkdfa.plと同じくプレフィックスをコマンド引数として与えて起動します．
.dfa, .dict, .termファイルを読みこんだ後，与えられたtranscriptに対して
    ・入力されたtranscript（確認のため）
    ・上記を，その単語の属するカテゴリ名に変換したもの
    ・accepted / rejected
を出力します．

  transcriptは単語の出力文字列(.vocaの第1フィールド)を空白で区切って与
えます．最初と最後には多くの場合 silB, silE が必要であることに気をつけ
て下さい．また文中の sp はtranscriptに含めないでください．また，日本語
でtranscriptを与える場合は，.vocaと同じ漢字コードで与えなければいけな
い点にも注意してください．

---- 実行例 ---------------------------------------------
    % bin/accept_check ../sample_grammars/vfr/vfr	    <-- 入力
    Reading in dictionary...done
    Reading in DFA grammar...done
    Mapping dict item <-> DFA terminal (category)...done
    Reading in term file (optional)...done
    42 categories, 99 words
    DFA has 135 nodes and 198 arcs
    ----- 
    please input word sequence>silB 白 に して 下さい silE  <-- 入力
    wseq: silB 白 に して 下さい silE
    cate: NS_B COLOR_N (NI|NI_AT) SURU_V KUDASAI_V NS_E
    accepted
    please input word sequence>
---------------------------------------------------------

語彙中に同一表記の単語が複数存在しカテゴリの解釈に曖昧性がある場合，
accept_checkは可能な全ての組み合わせを試します(上記実行例の「に」)．

なお，起動時に "-t" オプションをつけると，単語名でなくカテゴリ名を受け
付けるようになります．この場合，各カテゴリの最初の単語がカテゴリを代表
する単語として選択されます．


======================================================================
□ nextword --- 次単語予測チェックツール（accept_checkの高機能版）

  与えられた部分文に対して，文法上接続しうる次単語の集合を出力します．

  部分文入力ではヒストリ参照や単語名/カテゴリ名の補完が行えます．

  文法は事前に mkdfa.pl にかけて .dfa, .dict, .term を生成しておいて下さい．

  ！注意！ 部分文は逆向き(right-to-left)に入れる必要があります．これは
Julian が第2パスでは文章の末尾から先頭に向かって探索を行うため，単語予
測もその方向で行う必要があるからです．

---- 実行例 ---------------------------------------------
    % bin/nextword ../sample_grammars/vfr/vfr		<-- 入力
    Reading in dictionary...done
    Reading in DFA grammar...done
    Mapping dict item <-> DFA terminal (category)...done
    Reading in term file (optional)...done
    42 categories, 99 words
    DFA has 135 nodes and 198 arcs
    ----- 
    wseq > に して 下さい silE				<-- 入力
    [wseq: に して 下さい silE]
    [cate: (NI|NI_AT) SURU_V KUDASAI_V NS_E]
    PREDICTED CATEGORIES/WORDS:
                KEIDOU_A (派手 地味 )
                BANGOU_N (番 )
                  HUKU_N (服 服装 服装 )
               PATTERN_N (チェック 縦縞 横縞 ...)
                  GARA_N (柄 )
                 KANZI_N (感じ )
                   IRO_N (色 )
                 COLOR_N (赤 橙 黄 ...)
    wseq >
--------------------------------------------------------

transcript入力時には，通常のemacs風の行編集操作に加えて，
以下のキーが使用できます．

	TAB		単語名の補完．"-t"で起動時はカテゴリ名の補完．
			何回か押すと候補一覧を表示．
	Ctrl-L		補完候補の順次挿入(押すたびに切り替わる)．
	Ctrl-P，Ctrl-N	ヒストリ参照．

この他の操作については readline ライブラリのドキュメントを参照してください．

======================================================================
□ gram2sapixml	--- Julian 形式の文法を SAPI XML 文法に変換するスクリプト

gram2sapixml/gram2sapixml.txt をご覧ください．

======================================================================
□ dfa_determinize --- DFA を決定するツール

このツールは有限オートマトンの決定化アルゴリズムに従い，
非決定的な遷移のあるDFAを決定化します．

mkdfa.pl は既に決定化された DFA を出力するので，通常このツールは
用いられません．これは他のフォーマットから DFA に変換する場合などに
用いることができます．


								以上
