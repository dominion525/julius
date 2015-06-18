======================================================================
                  Large Vocabulary Continuous Speech
                          Recognition Engine

                                Julius

                                                (Rev 1.0   1998/02/20)
                                                (Rev 2.0   1999/02/20)
                                                (Rev 3.0   2000/02/14)
                                                (Rev 3.1   2000/05/11)
                                                (Rev 3.2   2001/06/18)
                                                (Rev 3.3   2002/09/12)
                                                (Rev 3.4   2003/10/01)
                                                (Rev 3.4.1 2004/02/25)
                                                (Rev 3.4.2 2004/04/30)
                                                (Rev 3.5   2005/11/11)
                                                (Rev 3.5.1 2006/03/31)
                                                (Rev 3.5.2 2006/07/31)
                                                (Rev 3.5.3 2006/12/29)

 Copyright (c) 1991-2006 京都大学 河原研究室
 Copyright (c) 1997-2000 情報処理振興事業協会(IPA)
 Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室
 Copyright (c) 2005-2006 名古屋工業大学 Julius開発チーム
 All rights reserved

======================================================================

Julius-3.5.3
=============

Julius-3.5.3 では，パフォーマンスの改善，文法関連ツールの追加，
およびグラフや音響特徴量の指定に関する修正と拡張が行われました．
以前のバージョンをお使いの方は，このバージョンへの移行を推奨します．

主な変更点は以下の通りです．

● 処理速度改善：コード最適化により処理速度が 1.1倍～1.5倍上昇．
○ 音響分析条件の設定方法を拡張
    - HTK Config を直接読み込めるようになった (-htkconf)
    - バイナリHMMへ設定を埋め込めるようになった
○ 新たな文法最適化ツール：dfa_minimize, dfa_determinize
○ その他の機能追加：
    - 単語グラフ生成で左右のコンテキストごとに異なる仮説の生成をサポート
    - オンラインでのエネルギー項正規化の暫定サポート
○ バグ修正およびコードの改善


すべての変更点は Release-ja.txt にまとめられていますので，ご覧下さい．
なお，認識の精度は前バージョンと同一です．

Windows で DirectSound を使用した Julius をコンパイルするには，
DirectX のヘッダがインストールされている必要があります．
無い場合， configure スクリプトが自動的に古い（性能の低い）ドライバ
を使用します．インストールの詳細については Web のインストールマニュア
ルをご覧下さい．


ファイルの構成
===============

	00readme-ja.txt		最初に読む文書（このファイル）
	LICENSE.txt		ライセンス条項
	Release-ja.txt		リリースノート/変更履歴
	configure		configureスクリプト
	configure.in		
	Sample.jconf.ja		Julius用jconfファイルサンプル
	Sample-julian.jconf.ja	Julian用jconfファイルサンプル
	julius/			Julius/Julian 3.5.3 本体ソース
	libsent/		Julius/Julian 3.5.3 ライブラリソース
	adinrec/		録音ツール adinrec
	adintool/		音声録音/送受信ツール adintool
	gramtools/		文法作成ツール群
	jcontrol/		サンプルネットワーククライアント jcontrol
	mkbingram/		バイナリN-gram作成ツール mkbingram
	mkbinhmm/		バイナリHMM作成ツール mkbinhmm
	mkgshmm/		GMS用音響モデル変換ツール mkgshmm
	mkss/			ノイズ平均スペクトル算出ツール mkss
	support/		コンパイル用スクリプト
	olddoc/			3.2以前の変更履歴


ドキュメントについて
=====================

・ドキュメント

	すべての説明や解説は，Julius の Web ページ上で公開しています．
	Webページでは，Julius のチュートリアルから様々な使用方法，各機
	能の紹介，制限事項などについて書かれていますので，そちらへ
	アクセスして下さい．

	ホームページ：http://julius.sourceforge.jp/

・最新版について

	Release-ja.txt に以前のバージョンからの変更点がまとめられてい
	ます．変更点の詳細については Release-ja.txt をご覧下さい．

・オンラインマニュアル

	Julius，Julian および関連ツールのオンラインマニュアルは，
	ソースからコンパイルする際に自動的にシステムにインストールされます．
	また，それぞれのソースディレクトリにテキスト形式のマニュアルが
	置いてありますので，そちらもご覧ください．
	  00readme-ja.txt：テキスト形式
	  *.man.ja: MAN形式

・ライセンス

	Julius/Julian はフリーのオープンソースソフトウェアです．
	私的用途・学術用途・商用を含め，利用に関して特に制限はありません．
	許諾については同梱の文書 "LICENSE.txt" をご参照下さい．


ホームページについて
=====================

Julius/Julian の最新版の公開やドキュメントの整備，掲示板・ユーザML等に
関する情報は，以下のサイトにまとめられています．ご活用ください．

	http://julius.sourceforge.jp/

コンタクト
===========

Julius/Julian に関するご質問・お問い合わせは，上記Webページ，あるいは
下記のメールアドレスまでお問い合わせ下さい ('at' を '@' に読み替えてく
ださい)

	julius-info at lists.sourceforge.jp

以上
