<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=EUC-JP">
<link rel="stylesheet" type="text/css" href="style.css" />

<link rel="shortcut icon" type="image/ico" href="graphics/favicon.ico" />

</head>

<body>

<div id="topbar">

<div id="toprightdiv">

<div>
<form action="cgi-bin/msearch/msearch.cgi" accept-charset="euc-jp">
<b>サイト内検索</b>
<a href="cgi-bin/msearch/msearch.cgi">ヘルプ</a>
</div>

<tr><td colspan="2" align="center">
    <input type="hidden" name="index" value="">
    <input type="hidden" name="config" value="">
    <input type="text" size="15" name="query" value="">
    <input type="submit" value="submit">
</td></tr>
    <input type="hidden" name="set" value="1">
    <input type="hidden" name="num" value="10">
    <input type="hidden" name="hint" value="">
</form>

</div><!-- toprightdiv-->


<div id="logodiv">
<a href="http://julius.sourceforge.jp" title="大語彙連続音声認識
エンジンJulius"><img src="graphics/mic_web.png" border="0"  alt="大語彙
連続音声認識エンジンJulius" /></a>
</div>

<div id="toptabs">
<ul>

<li style="margin-left: 1px">
<a href="index.php" title="Home">
<span>Home</span></a></li>

<li><a href="index.php?q=whatis.html" title="About Julius">
<span>About Julius</span></a></li>

<li><a href="index.php?q=whatis.html#spec" title="Spec">
<span>動作環境</span></a></li>

<li><a href="http://julius.sourceforge.jp/cgi-bin/cbbs/cbbs.cgi" title="BBS">
<span>BBS</span></a></li>

<li><a href="http://julius.sourceforge.jp/julius_faq/"
title="Julius FAQ">
<span>FAQ</span></a></li>	
</ul>

<ul> 
<li><a href="en_index.php?q=index-en.html" title="English site">
<span>English Home</span></a></li>

<li><a href="http://sourceforge.jp/projects/julius/"
title="SourceForge.jp: Project Info - Julius">
<span>開発サイト</span></a></li>

<li><a href="http://julius.sourceforge.jp/forum/"
title="User Forum">
<span>ユーザフォーラム</span></a></li> 
</ul>

</div><!-- toptabs -->

<div id="toptabsline">&nbsp;</div>


</div><!-- topbar -->


<div class="maincontainer">

<div id="leftcolumn">

<div class="titlebar" style="margin-top: 12px">
<a href="index.php?q=download.html">ダウンロード</a></div>

<ul class="listmenu">
<li><a href="index.php?q=newjulius.html">Julius/Julian最新版</a></li>
<li><a href="index.php?q=juliuskit.html">ディクテーションキット</a></li>
<li><a href="index.php?q=juliankit.html">文法認識キット</a></li>
<li><a href="index.php?q=julicus.html">カスタム実行キット</a></li>
<li><a href="index.php?q=ouyoukit.html">応用キット</a></li>
</ul>

<div class="titlebar" style="margin-top: 12px">
<a href="index.php?q=contrib.html">派生版Julius</a>
</div>

<ul class="listmenu">
<li><a href="index.php?q=sapi/index.html">Julius for SAPI</a></li>
<li><a href="http://www.itakura.nuee.nagoya-u.ac.jp/people/banno/julius.html">JuliusLib/JuliusGUI</a></li>
<li><a href="http://www.furui.cs.titech.ac.jp/mband_julius/">マ
ルチバンド版 Julius</a></li>
</ul>

<!-- Most recently commented on -->

<div class="titlebar">
<a href="index.php?q=documents.html">ドキュメント</a>
</div>

<ul class="listmenu">
<li><a href="index.php?q=documents.html#beginner">チュートリアル・解説</a></li>
<li><a href="index.php?q=documents.html#install">インストール</a></li>
<li><a href="index.php?q=documents.html#application">認識用文法の書き方</a></li>
<li><a href="index.php?q=documents.html#manual">マニュアル・ソース資料</a></li>
<li><a href="index.php?q=documents.html#performance">性能評価
</a></li>
<li><a href="http://julius.sourceforge.jp/forum/">ユーザフォーラム</a></li></ul>

</div>

<div id="middlecolumn">

<?php 
  //-- q= が空だったらトップページ表示
  if (!($target = $_GET['q'])) 
    $target = "history.html";

  $whatsnew_mode = 0;
  $history_mode = 0;

  if ( $target == "history.html" || $target == "newjulius.html" )
    $whatsnew_mode = 1;

  if ( $_GET['m'] == 'history' && $whatsnew_mode ){
    $history_mode = 1;
    $whatsnew_mode = 0;
  }
    
  if (ereg("\/.*", $target)){
    $dir = ereg_replace("\/.*", "/", $target);
  }
  $fcontents = file($target);
  $whatisjulius = file("whatisjulius.html");
  $top_num = 0;
  $title = "大語彙連続音声認識エンジン Julius";

  $contentnum = 0;

  while (list ($line_num, $line) = each($fcontents)) {

    if ( $whatsnew_mode ) {
 
	while (list($line_num2, $line2) = each($whatisjulius)) {
	      echo($line2);
	}

     if (ereg("\<h2\>", $line) && $target == "history.html" ) {
        $line =
      eregi_replace(".*\<h2\>(.*)\</h2\>.*","<h2>What's New?</h2>", $line);
      }

      if (ereg("\<h3", $line)) {
        if ($contentnum == 0){
          $line =
        eregi_replace("(.*)\<h3 class(.*)\</h3\>(.*)","\\1<h3 class\\2
        <font color=\"#cc0000\">New!</font></h3>",
        $line);
        }
        $contentnum++; 
      }

      if ($contentnum > 3) {
        echo ("<hr><p><a href=\"index.php?q=$target&m=history\">-> 過去ログ</a></p>");
	break 1;
      }
  
    } else {

      if ( $history_mode ){
        
        if (!ereg("\<h3", $line)) {
	  if ($contentnum == 0) $line = "";	  
        } else { 
	  if ($contentnum == 0 && $target == "newjulius.html") 
	    echo "<h2>Julius アーカイブ</h2>";

   	  $contentnum++;
        }
      }
    }
 
      //-- page title assignment
      if ($top_num == 0 && ereg("\<h1\>", $line)){
        $title = eregi_replace(".*\<h1\>(.*)\</h1\>.*","\\1", $line);
        $top_num = 1;
      } else if ($top_num == 0 && ereg("\<h2\>", $line)) {
        $title = eregi_replace(".*\<h2\>(.*)\</h2\>.*","\\1", $line);
        $top_num = 2;
      }
    

    //-- replace path description 
    //-- http で指定していない場合
    if (!eregi("http://", $line) && !eregi("mailto", $line)) {

      if (!eregi("a href\s?=\s?\"#", $line)){
        $line = eregi_replace("a href\s?=\s?\"", "a href=\"$dir", $line);
      }
      $line = eregi_replace("img src\s?=\s?\"", "img
        src=\"$dir", $line);

      if (eregi("a href", $line) && eregi("\.html?", $line))
        $line = preg_replace("/(a href=\")/",
        "$1index.php?q=", $line);
    }	  

    echo ($line);

  }

  if ( $history_mode ) {
    echo ("<hr><p><a href=\"index.php?q=$target\"><- 戻る</a></p>\n");
  }

  if ( $whatsnew_mode && $target == "history.html" )
    $title = "大語彙連続音声認識エンジン Julius";

  echo "<title>$title</title>"; 

  //-- ACCESS ANALYSIS
  $w3a_send_title="$title";
  include_once("w3a/writelog.php");

 ?>

</div>

<div id="rightcolumn">

<div class="titlebar">Quick Download</div>

<ul class="listmenu">

<li><a href="http://prdownloads.sourceforge.jp/julius/29930/julius-4.0.1.tar.gz"
target="_top">Source (tarball)</a></li>
<li><a
href="http://prdownloads.sourceforge.jp/julius/29930/julius4.0.1-linuxbin.tar.gz"
target="_top">Binary for Linux (tarball)</a></li>
<li><a
href="http://prdownloads.sourceforge.jp/julius/29930/julius-4.0.1-win32bin.zip"
target="_top">Binary for Windows (zip)</a></li>
<li><a
href="http://sourceforge.jp/projects/julius/files/?release_id=28551#28551"
target="_top">旧バージョン</a></li>
</ul>

<!--<li><a href="http://prdownloads.sourceforge.jp/julius/23331/julius-3.5.3.tar.gz"
//target="_top">Source (tarball)</a></li>
//<li><a
//href="http://prdownloads.sourceforge.jp/julius/23331/julius-3.5.3-linuxbin.tar.gz"
//target="_top">Binary for Linux (tarball)</a></li>
//<li><a
//href="http://prdownloads.sourceforge.jp/julius/23331/julius-3.5.3-win32bin.zip"
//target="_top">Binary for Windows (zip)</a></li>
//</ul> -->

<div class="titlebar">etc</div>

<p align="center">
<a href="http://sourceforge.jp">
<img src="http://sourceforge.jp/sflogo.php?group_id=232" width="96"
height="31" border="0" alt="SourceForge.jp"></a>
</p>

<p align="center">
<script src="http://trackfeed.com/usr/368e4943cf.js"></script>
<noscript>
<a href="http://trackfeed.com/">
<img src="http://trackfeed.com/img/tfg.gif" alt="track feed" border="0"></a>
</noscript>
</div>
</p>

<!-- rightcolumn -->

<div class="clearfix"></div>

</div>

<div id="toptabsline">&nbsp;</div>


<div id="footerarea">

<div class="maincontainer" style="background-color: transparent; border-width: 0">
Copyright 2008 Julius development team
</div>

</div><!-- footerarea -->


</body>
</html>