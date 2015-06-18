<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=EUC-JP">

<link rel="stylesheet" type="text/css" href="style.css" />
<link rel="shortcut icon" type="image/ico" href="graphics/favicon.ico" />

<title>Open-Source Large Vocabulary CSR Engine Julius</title>

</head>

<body>

<div id="topbar">

<div id="toprightdiv">

<div>
<form action="cgi-bin/msearch/msearch.cgi" accept-charset="euc-jp">
<b>Search</b>
<a href="cgi-bin/msearch/msearch.cgi">Help(Japanese)</a>
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
<a href="http://julius.sourceforge.jp/en_index.php" title="Open-Source Large
Vocabulary CSR Engine Julius">
<img src="graphics/mic_web.png" border="0"  alt="Open-Source Large
Vocabulary CSR Engine Julius" /></a>
</div>

<div id="toptabs">
<ul>

<li style="margin-left: 1px">
<a href="en_index.php" title="Home">
<span>Home</span></a></li>

<li><a href="index.php" title="Main Site(Japanese)">
<span>Main Site(Japanese)</span></a></li>

<li><a href="http://sourceforge.jp/projects/julius/"
title="SourceForge.jp: Project Info - Julius">
<span>Development Site</span></a></li>

<li><a href="http://julius.sourceforge.jp/forum/" title="User Forum">
<span>User Forum</span></a></li>

</ul>

</div><!-- toptabs -->

<div id="toptabsline">&nbsp;</div>

</div><!-- topbar -->


<div class="maincontainer">

<div id="leftcolumn">

<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#q=top">top</a>
</div>

<ul class="listmenu">
 <li><a href="en_index.php?q=index-en.html#whats_new">What's New?</a></li>
<!-- <li><a href="en_index.php?q=index-en.html#about_julius">About Julius</a></li>
 <li><a href="en_index.php?q=index-en.html#contact">Contact</a></li>-->
</ul>


<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#about_julius">About Julius</a>
</div>
<ul class="listmenu">
 <li><a href="en_index.php?q=index-en.html#feature">Features</a></li>
 <li><a href="en_index.php?q=index-en.html#contact">Contact</a></li>
</ul>

<!--
<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#feature">Feature</a>
</div>

<ul class="listmenu">
 <li><a href="en_index.php?q=index-en.html#features">Features</a></li>
</ul>
-->
<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#download_julius">Download Julius</a>
</div>

<ul class="listmenu">
 <!--<li><a href="en_index.php?q=index-en.html#recent">Recent
 version</a></li>
 <li><a
 href="en_index.php?q=index-en.html#download">Download</a></li>
 <li><a href="en_index.php?q=index-en.html#precompiled">Download pre-compiled
binaries</a></li>-->
 <li><a href="en_index.php?q=index-en.html#get_current_ver">Get
 current version</a></li>
 <li><a href="en_index.php?q=index-en.html#get_by_cvs">Get the
 latest codes via CVS</a></li>
 <li><a href="en_index.php?q=index-en.html#sapi">Get Julius for Windows SAPI</a></li>
</ul>

<!--
<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#document">Document</a>
</div>

<ul class="listmenu">
 <li><a href="en_index.php?q=index-en.html#documentation">Documentation</a></li>
 <li><a href="en_index.php?q=index-en.html#variants">Compile-time variants</a></li>
 <li><a href="en_index.php?q=index-en.html#grammar">How to write a grammar for Julian</a></li>
 <li><a href="en_index.php?q=index-en.html#references">References</a></li>
</ul>

-->
<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#about_models">About Models</a>
</div>
<!--
<ul class="listmenu">
 <li><a href="en_index.php?q=index-en.html#about_models">About Models</a></li>
</ul>

<div class="titlebar" style="margin-top: 12px">
 <a href="en_index.php?q=index-en.html#develop">Develop</a>
</div>

<ul class="listmenu">
 <li><a href="en_index.php?q=index-en.html#development_site">Development Site</a></li>
</ul>
-->

<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#documents_and_notes">Documents and Notes</a>
</div>

<ul class="listmenu">
 <li><a
 href="en_index.php?q=index-en.html#documentation">Documentation</a></li>
<li><a
 href="en_index.php?q=index-en.html#variants">Compile-time variants</a></li>
<li><a
 href="en_index.php?q=index-en.html#grammar">How to write a
 grammar for Julian</a></li>
<li><a
 href="en_index.php?q=index-en.html#references">References</a></li>
</ul>

<div class="titlebar" style="margin-top: 12px">
<a href="en_index.php?q=index-en.html#changelog">ChangeLog</a>
</div>
<!--
<ul class="listmenu">
 <li><a href="en_index.php?q=index-en.html#ChangeLog">ChangeLog</a></li>
</ul>
-->


</div>

<div id="middlecolumn">

<?php 
  //-- q= が空だったらトップページ表示
  if (!($target = $_GET['q'])) 
    $target = "index-en.html";

  if (ereg("\/.*", $target)){
    $dir = ereg_replace("\/.*", "/", $target);
  }
  $fcontents = file($target);
  $top_num = 0;
  $title = "Open-Source Large Vocabulary CSR Engine Julius";

  $contentnum = 0;

  while (list ($line_num, $line) = each($fcontents)) {

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
    if (!eregi("http://", $line) && !eregi("mailto", $line) &&
    !eregi("php", $line)) {

      if (!eregi("a href\s?=\s?\"#", $line)){
        $line = eregi_replace("a href\s?=\s?\"", "a href=\"$dir", $line);
      }
      $line = eregi_replace("img src\s?=\s?\"", "img
        src=\"$dir", $line);

      if (eregi("a href", $line) && eregi("\.html?", $line))
        $line = preg_replace("/(a href=\")/",
        "$1en_index.php?q=", $line);
    }	  

    echo ($line);

  }

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
href="http://prdownloads.sourceforge.jp/julius/29930/julius-4.0.1-linuxbin.tar.gz"
target="_top">Binary for Linux (tarball)</a></li>
<li><a
href="http://prdownloads.sourceforge.jp/julius/29930/julius-4.0.1-win32bin.zip"
target="_top">Binary for Windows (zip)</a></li>
</ul>

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