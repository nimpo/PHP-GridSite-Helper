<?php
/**
 * gridsite.inc
 *
 * The PHP-GridSite-Helper is a php script (gridsite.inc) designed for 
 * integration with Andrew McNab's GridSite.
 * 1 It Incorporates a GridSite header and footer (usually called 
 *   gridsitehead.txt and gridsitefoot.txt) if available when the output of the
 *   PHP is html or xhtml.
 * 2 It performs a GACL access control check (online dn-lists may also be used).
 *   Checks are performed at the point when the gridsite.inc file is included.
 * 3 It can present a rendering of the GACL and which rules have been triggered
 *   providing access to that particular script. This is achieved by adding the 
 *   query ?cmd=list_acl to the resource URI, this option will take precedence
 *   over the underlying PHP file.
 *
 * This script is meant to be housed outside normal server Document Roots, i.e. 
 * not within reach of any direct request to the web server and should be 
 * referenced by the relevant "auto_prepend_file" directive in php.ini along with
 * setting the corresponding "include_path".
 *
 * Ultimately this script provides NO ADDED SECURITY.  Its function is an attempt
 * to present a coherent GridSite experience for content rendered through the php 
 * interpreter.
 *
 * Anyone with permission to edit a PHP file will essentially have the ability to
 * execute any code they write as though the web server itself executed it. For a
 * GridSite this could mean that .gacl files might be altered.  i.e. a PHP 
 * writer should be:
 * - competent, and 
 * - trusted (at least as much as the website administrator).
 *
 * Various precautions may be considered to mitigate this escalation of authority 
 * such as suPHP, suexec or glexec wrappers.  In such cases care should be taken
 * to maintain read access to .gacl files for the uid/gid of the process
 * performing the php execution.
 * 
 * @author Michael AS Jones <mike.jones@manchester.ac.uk>
 * @copyright 2009-2012 Michael AS Jones, The University of Manchester
 * @license http://opensource.org/licenses/BSD-2-Clause The BSD License
 */

/* Copyright (c) 2009-2012, Michael AS Jones, The University of Manchester
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

if ( ! array_key_exists('DOCUMENT_ROOT',$_SERVER) || $_SERVER['DOCUMENT_ROOT'] === "" ) { die ("No DOCUMENT_ROOT\n"); }
if ( ! array_key_exists('REQUEST_URI',$_SERVER)   || $_SERVER['REQUEST_URI'] === "" )   { die ("No REQUEST_URI\n"); }
if ( ! array_key_exists('REMOTE_ADDR',$_SERVER)   || $_SERVER['REMOTE_ADDR'] === "" )   { die ("No REMOTE_ADDR\n"); }
if ( ! array_key_exists('SCRIPT_NAME',$_SERVER)   || $_SERVER['SCRIPT_NAME'] === "" )   { die ("No SCRIPT_NAME\n"); }

//By default, never cache
header("Cache-Control: no-cache, must-revalidate");
header("Expires: Sat, 01 Jan 1970 00:00:00 GMT");

//Setup the GridSite stuff
$PATH=$_SERVER['DOCUMENT_ROOT'].$_SERVER['SCRIPT_NAME'];
$DIR=dirname($PATH);
$FILE=basename($PATH);
$SEC=(isset($_SERVER['HTTPS']))?'':'S';

// Find GridSite Header/Footer
( ! isset($_SERVER['DOCUMENT_ROOT']) ) && $_SERVER = array( 'DOCUMENT_ROOT' => ".");
$HN=isset($_SERVER['GRST_HEAD_FILE']) ? "/".$_SERVER['GRST_HEAD_FILE'] : "/gridsitehead.txt";
$FN=isset($_SERVER['GRST_FOOT_FILE']) ? "/".$_SERVER['GRST_FOOT_FILE'] : "/gridsitefoot.txt";
for($GH=$DIR.$HN;!file_exists($GH) && $GH != dirname($_SERVER['DOCUMENT_ROOT']).$HN;$GH=dirname(dirname($GH)).$HN); 
for($GF=$DIR.$FN;!file_exists($GF)&&$GF!=dirname($_SERVER['DOCUMENT_ROOT']).$FN;$GF=dirname(dirname($GF)).$FN);
if(file_exists($GH)&&is_readable($GH)&&file_exists($GF)&&is_readable($GF)) { $GH=file_get_contents($GH); $GF=file_get_contents($GF); }
else{ $GH=""; $GF=""; }
unset ($HN); unset ($FN);

//Find .gacl file
if ( isset($_SERVER['GRST_ACL_FORMAT'])&&$_SERVER['GRST_ACL_FORMAT']!='GACL') {  
  header("HTTP/1.0 501 Not Implemented");
  header("X-PHP-GridSite-Helper: ACL ".$_SERVER['GRST_ACL_FORMAT']." Parsing Not Implemented");
  print filter("<html><head>\n<title>501 Not Implemented</title>\n</head>\n<body>\n<h1>Not Implemented</h1>\n<p>This GridSite is configured to use an ACL Format ".
                $_SERVER['GRST_ACL_FORMAT'].", the parsing of which is not implemented in PHP-GridSite-Helper.</p>\n<hr>\n</body></html>");
  exit;
}
$GL="/.gacl";
for($GL=$DIR."/.gacl";!file_exists($GL)&&$GL!=dirname($_SERVER['DOCUMENT_ROOT'])."/.gacl";$GL=dirname(dirname($GL))."/.gacl");
if ( !file_exists($GL) ) {
  header("HTTP/1.0 403 Forbidden");
  header("X-PHP-GridSite-Helper: GACL Not Found");
  print filter("<html><head>\n<title>503 Forbidden</title>\n</head>\n<body>\n<h1>Forbidden</h1>\n<p>Insufficent rights to access ".
                $_SERVER['REQUEST_URI']."</p>\n<hr>\n</body></html>");
  exit;
}

//Get the DN of the User
$DN="";
$TYPE="";
$START="";
$END="";
$PL="";
if ( isset($_SERVER['GRST_CRED_0']) ){ list($TYPE,$START,$END,$PL,$DN)=split(" ",$_SERVER['GRST_CRED_0'],5);}
$DN=chop($DN); // for some reason we sometimes get a \n in the DN

// Get IP of user
$IP=$_SERVER['REMOTE_ADDR'];

///////////////////////////////////////////////////////////////////////////////
// GACL processing

//Initialize the XML parser
$parser      = xml_parser_create();
$Position    = array();
$Allow       = array();
$Deny        = array();
$GAllow      = array();
$GDeny       = array();
$PersonDN    = array();
$DNListURL   = array();
$DNSHostname = array();
$AURI        = array();
$AuthUser    = array();
$AnyUser     = array();
$Entries     = array();

/**
 * Function to use at the start of a GACL element
 * @ignore
 */
function xmlstart($parser,$element_name,$element_attrs)
{

  global $Position;
  global $Allow;
  global $Deny;
  global $AuthUser;
  global $AnyUser;

  array_push($Position,$element_name);

  if (count($Position) == 3) {
    if ($Position[2] == "AUTH-USER") { array_push($AuthUser,"auth"); }
    else if ($Position[2] == "ANY-USER") { array_push($AnyUser,"any"); }
  }

  if (count($Position) == 4) {
    if ($Position[0] == "GACL" && $Position[1] == "ENTRY") { 
      if ($Position[2] == "ALLOW" && $Position[3] != "") { array_push($Allow,$Position[3]); } 
      if ($Position[2] == "DENY" && $Position[3] != "" ) { array_push($Deny,$Position[3]);  }
    }
  }
}

/**
 * Function to use at the end of a GACL element
 * @ignore
 */
function xmlstop($parser,$element_name)
{
  global $DN;
  global $IP;
  global $Position;
  global $Allow;
  global $Deny;
  global $PersonDN;
  global $DNListURL;
  global $DNSHostname;
  global $AURI;
  global $AuthUser;
  global $AnyUser;
  global $GAllow;
  global $GDeny;
  global $Entries;
  global $Entry;

  $val=array_pop($Position);
  if ($element_name == "ENTRY") {
    $MATCH=0;
    $FAIL=0;

//person
    $PDN=array_unique($PersonDN);
    if( count($PDN) != 0 ) {
      $Entry.="Match Rule: Personal DN = \"".implode("\" and \"",$PDN)."\".<br />";
      if(isset($DN) && $DN != "") {
        foreach ($PDN as $PDNi) {
          if ($PDNi==$DN) { $MATCH=1; $Entry.="Your DN \"".$DN."\" matches<br />";}
          else            { $FAIL=1;  $Entry.="Your DN \"".$DN."\" does not match<br />";}
        }
      }
      else { $FAIL=1;  $Entry.="You did not supply a DN and therefore cannot match this rule.<br />";}
    }

//dn-list
    foreach ( $DNListURL as $DNLURL ) {
      $Entry.="Match Rule DN List from <a href=\"$DNLURL\">$DNLURL</a>.<br />";
      if(isset($DN)) {
        if (file_exists("/etc/grid-security/dn-lists/".urlencode($DNLURL))) {
          $Entry.="DN List $DNLURL found cached in system directory<br />";
          $str=file_get_contents("/etc/grid-security/dn-lists/".urlencode($DNLURL));
        }
        else if (file_exists($_SERVER['DOCUMENT_ROOT']."/dn-lists/".urlencode($DNLURL))) {
          $Entry.="DN List <a href=\"/dn-lists/".basename($DNLURL)."\">$DNLURL</a> found cached in local web directory<br />";
          $str=file_get_contents($_SERVER['DOCUMENT_ROOT']."/dn-lists/".urlencode($DNLURL));
        }
        else { $str=file_get_contents($DNLURL); $Entry.="DN List <a href=\"$DNLURL\">$DNLURL</a> found in remote web location<br />";}
        str_replace("\r", null, $str);
        $DNLIST=explode("\n",$str);
        $DNMATCH=0;
        foreach ($DNLIST as $DNEntry) { if ( $DNEntry == $DN ) { $DNMATCH=1;} }
        if ( $DNMATCH==1 ) { $MATCH=1;         $Entry.="Test for your DN \"$DN\" in <a href=\"$DNLURL\">$DNLURL</a> SUCCESS!<br />";}
        else               { $FAIL=1;          $Entry.="Test for your DN \"$DN\" in <a href=\"$DNLURL\">$DNLURL</a> FAIL!<br />";}
      } else {               $FAIL=1;          $Entry.="Test for no DN in <a href=\"$DNLURL\">$DNLURL</a> FAIL!<br />";}
    }

//dns
    if ( count($DNSHostname) != 0 ) {
      $Entry.="Match Rule DNS of request is from ".implode(" and ",$DNSHostname).".<br />";
      $DNSIPs=array();
      foreach ( $DNSHostname as $DNSH ) { array_push($DNSIPs,gethostbyname($DNSH));}
      $DNSIPu=array_unique($DNSIPs);
      $DNSMATCH=0;
      $DNSMISMATCH=0;
      foreach ($DNSIPu as $DNSIPs) { 
        if ( $DNSIPs == $IP ) { $MATCH=1; $Entry.="Test your IP $IP matches $DNSIPs SUCCESS!<br />";} 
        else                  { $FAIL=1;  $Entry.="Test your IP $IP matches $DNSIPs FAIL!<br />";}
      }
    }

//auri
    if ( count($AURI) != 0 ) {
      $Entry.="Match Rule AURI of request is from ".implode(" and ",$AURI).".<br />";
      $AURIMATCH=0;
      $AURIMISMATCH=0;
      for($i=0;isset($_SERVER[sprintf("GRST_CRED_AURI_%i",$i)]);$i++) {
        $CRED=$_SERVER["GRST_CRED_AURI_"].$i;
        foreach ($AURI as $AURIs) {
          if ( $AURIs == $CRED ) { $MATCH=1; $Entry.="Test your AURI $CRED matches $AURIs SUCCESS!<br />";}
          else                   { $FAIL=1;  $Entry.="Test your AURI $CRED matches $AURIs FAIL!<br />";}
        }
      }
    }

//auth-user
    if ( count($AuthUser) != 0 ) {
      $Entry.="Match Rule Authentic User<br />";
      if ( isset($DN) ) { $MATCH=1;    $Entry.="Test for valid credential $DN SUCCESS!<br />";}
      else { $FAIL=1;                  $Entry.="Test for valid credential (Not supplied) FAIL!<br />";}
    }

//any-user
    if ( count($AnyUser) != 0 ) { $MATCH=1; $Entry.="Match Rule Any User<br />Test for Any User SUCCESS!<br />";}

    $Entry.="Allow: ".implode(", ",$Allow).". Deny: ".implode(", ",$Deny)."<br />";

    if ( $MATCH == 1 && $FAIL == 0 ) {
      $Entry.="Above Rule Triggers<br />";
      foreach ($Allow as $A) { array_push($GAllow,$A); }
      foreach ($Deny as $D) { array_push($GDeny,$D);   }
    }
    else {
      $Entry.="Above Rule DOES NOT Trigger<br />";
    }

// Record Entry Analysis String
    array_push($Entries,$Entry);

// Reset Arrays
    $Allow=array();
    $Deny=array();
    $PersonDN=array();
    $DNListURL=array();
    $DNSHostname=array();
    $AURI=array();
    $AuthUser=array();
    $AnyUser=array();
    $Entry="";
  }
}

/**
 * Function to use when finding character data (Node contents) in GACL
 * @ignore
 */
function char($parser,$data)
{
  global $Position;
  global $PersonDN;
  global $DNListURL;
  global $DNSHostname;
  global $AURI;
  global $AuthUser;
  global $AnyUser;

  if (count($Position) == 4)
  {
    if ($Position[2] == "PERSON" && $Position[3] == "DN") {  array_push($PersonDN,$data); }
    else if ($Position[2] == "DN-LIST" && $Position[3] == "URL") { array_push($DNListURL,$data); }
    else if ($Position[2] == "DNS" && $Position[3] == "HOSTNAME") { array_push($DNSHostname,$data); }
    else if ($Position[2] == "CRED" && $Position[3] == "AURI") { array_push($AURI,$data); }
  }
}

//Specify element handler
xml_set_element_handler($parser,"xmlstart","xmlstop");

//Specify data handler
xml_set_character_data_handler($parser,"char");

//Open XML file
$fp=fopen($GL,"r");
if ( ! $fp ) {
  header("HTTP/1.0 403 Forbidden");
  header("X-PHP-GridSite-Helper: GACL cannot be read");
  print filter("<html><head>\n<title>403 Forbidden</title>\n</head>\n<body>\n<h1>Forbidden</h1>\n<p>The ACL for ".
               $_SERVER['REQUEST_URI']." cannot be read.</p>\n<hr>\n</body></html>");
  exit;
}

//Read data
while ($data=fread($fp,4096))
  {
  xml_parse($parser,$data,feof($fp)) or 
  die (sprintf("XML Error: %s at line %d", 
  xml_error_string(xml_get_error_code($parser)),
  xml_get_current_line_number($parser)));
  }

//Close XML file
fclose($fp);

//Free the XML parser
xml_parser_free($parser);

sort($GAllow);
sort($GDeny);

$FAllow=array_unique($GAllow);
$FDeny=array_unique($GDeny);

$GACL=array();
if ( in_array("READ",$FAllow) && ! in_array("READ",$FDeny) )   { array_push($GACL,"READ");  }
if ( in_array("LIST",$FAllow) && ! in_array("LIST",$FDeny) )   { array_push($GACL,"LIST");  }
if ( in_array("EXEC",$FAllow) && ! in_array("EXEC",$FDeny) )   { array_push($GACL,"EXEC");  }
if ( in_array("WRITE",$FAllow) && ! in_array("WRITE",$FDeny) ) { array_push($GACL,"WRITE"); }
if ( in_array("ADMIN",$FAllow) && ! in_array("ADMIN",$FDeny) ) { array_push($GACL,"ADMIN"); }

// Set GridSite Indexing footer if there's a GRST_ADMIN_FILE env variable (i.e. gridsite is installed and configured for this resource)
if ( isset($_SERVER['GRST_ADMIN_FILE']) ) {
  date_default_timezone_set('Europe/London');
  $GI="<small><hr>Last modified ".date('D&\nb\s\p;d&\nb\s\p;F&\nb\s\p;Y&\nb\s\p;',filemtime($FILE)).
    ".<a href=\"".$_SERVER['GRST_ADMIN_FILE']."?cmd=history&file=$FILE\">View&nbsp;page&nbsp;history</a><hr>".
    (($DN!='')?"You are $DN<br>":"").
    ((in_array("WRITE",$FAllow) && !in_array("WRITE",$FDeny))?"<a href=\"gridsite-admin.cgi?cmd=edit&file=$FILE\">Edit&nbsp;page</a> . ":"").
    ((in_array("LIST",$FAllow) && !in_array("LIST",$FDeny))?"<a href=\"gridsite-admin.cgi?cmd=managedir\">Manage&nbsp;directory</a> . ":"").
    "<a href=\"http".strtolower($SEC).
    "://".$_SERVER['SERVER_NAME'].$_SERVER['REQUEST_URI']."\">Switch&nbsp;to&nbsp;HTTP$SEC</a></small>";
} else { $GI=""; }

//List GACL if requested
if ( isset($_GET['cmd']) && preg_match('/^list_acl$/',$_GET['cmd'])) {
  if ( ! in_array("READ",$GACL) )  {
    header("HTTP/1.0 403 Forbidden");
    header("X-PHP-GridSite-Helper: GACL forbids");
    print filter("<html><head>\n<title>403 Forbidden</title>\n</head>\n<body>\n<h1>Forbidden</h1>\n<p>You don't have permission to read the ACL at ".
                  $_SERVER['REQUEST_URI'].".</p>\n<hr>\n</body></html>");
    exit;
  }
  else {
    header("X-PHP-GridSite-Helper: ACL List Processed");
    $listbuffer="";
    for ($i=0; $i < count($Entries); $i++) { $listbuffer.="Entry ".($i+1)." <br />".$Entries[$i]."<br />\n"; }
    $GI=str_replace("<br>", "<br><a href=\"$FILE\">Back</a> . ", $GI);
    print filter($listbuffer);
    exit;
  }
}

if ( isset($_GET['cmd']) && preg_match('/^_debug$/',$_GET['cmd'])) {
  if ( ! in_array("READ",$GACL) )  {
    header("HTTP/1.0 403 Forbidden");
    header("X-PHP-GridSite-Helper: GACL forbids");
    print filter("<html><head>\n<title>403 Forbidden</title>\n</head>\n<body>\n<h1>Forbidden</h1>\n<p>You don't have permission to read the ACL at ".
                  $_SERVER['REQUEST_URI'].".</p>\n<hr>\n</body></html>");
    exit;
  }
  else {
    header("X-PHP-GridSite-Helper: Debug");
    print "<pre>";
    print_r($_SERVER);
    print "</pre>";
    exit;
  }
}

//Don't run this PHP any further if no exec rights.
if ( ! in_array("EXEC",$GACL) )  { 
  header("HTTP/1.0 403 Forbidden");
  header("X-PHP-GridSite-Helper: GACL forbids");
  print filter("<html><head>\n<title>403 Not Found</title>\n</head>\n<body>\n<h1>Forbidden</h1>\n<p>You don't have permission to execute ".
                $_SERVER['REQUEST_URI'].".</p>\n<hr><a href=\"".$_SERVER['REQUEST_URI']."?cmd=list_acl\">List ACL</a>\n</body></html>");
  exit;
}

//Unset so that internal parameters don't get passed on 
unset ($PATH);
unset ($DIR);
unset ($FILE);
unset ($SEC);
unset ($GL);
unset ($parser);
unset ($Position);
unset ($Allow);
unset ($Deny);
unset ($GAllow);
unset ($GDeny);
unset ($PersonDN);
unset ($DNListURL);
unset ($DNSHostname);
unset ($AURI);
unset ($AuthUser);
unset ($AnyUser);
unset ($Entries);
unset ($fp);
unset ($data);

/**
 * PHP-GridSite-Helper filter 
 * 
 * As described above gridsite.inc should be included before using the 
 * auto_prepend_file directive.
 * 
 * The target PHP file is interpreted and the result is passed through the
 * PHP-GridSite-Helper filter function. An attempt is made to determine
 * whether the resulting data is HTML by checking the Content-type header.
 * If the document is deemed to be non-html the filter returns the buffer
 * with one additional header.
 * 
 * If the document is HTML:
 * 1 A doctype directive is added to the content if one has not been generated
 *   by the PHP file and is not present in the GridSite Header.
 * 2 An html start and end tag are added if required. If the html tag appears in
 *   both the GridSite header and the buffer then any attributes, if present,
 *   are merged by simple concatenation.
 * 3 A head tag is added if not supplied. Multiple head tags are treated similarly to 2;
 *   contents are added similarly by concatenation.
 * 4 A body tag is added if not supplied. Multiple head tags are treated similarly to 2;
 *   contents are handled as in 3.
 * 5 A GridSite style additional footer is presented if the GRST_ADMIN_FILE environment 
 *   is set.
 */ 

function filter($buffer)
{
  global $GI;
  global $GF;
  global $GH;

  $defaultdoctype="<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">";

  if ( preg_grep("/^Content-type: text\/html/i",headers_list()) ) { header("X-PHP-GridSite-Helper: detected text/html"); }
  elseif ( preg_grep("/^Content-type: application\/xhtml\+xml/i",headers_list()) ) { header("X-PHP-GridSite-Helper: detected application/xhtml+xml"); 
    $defaultdoctype='<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">'; }
  elseif( ! preg_grep("/^Content-type: /i",headers_list()) )      { header("X-PHP-GridSite-Helper: assumed text/html"); }
  else { header("X-PHP-GridSite-Helper: non-html header detected"); return $buffer; }

//Doctype
  if ( ! preg_match("/^<!DOCTYPE [^>]*>/is",$GH,$doctype) ) {
    if ( preg_match("/<!DOCTYPE [^>]*>/is",$GH,$doctype) ) { $GH=preg_replace("/<!DOCTYPE [^>]*>\n?/is", "", $GH); //Relocate DOCTYPE
                                                             $GH=$doctype[0]."\n<!-- PHP-GridSite-Helper Moved doctype in GH -->\n$GH"; }
    if ( ! preg_match("/^<!DOCTYPE [^>]*>/is",$GH) )       { $GH="$defaultdoctype\n$GH"; } // If no doctype in $GH add a default doctype
  }

  if ( preg_match("/<!DOCTYPE [^>]*>/is",$buffer,$bufferdoctype) ) { // If allow user override
    $GH=preg_replace("/<!DOCTYPE [^>]*>/is", $bufferdoctype[0]."\n<!-- PHP-GridSite-Helper User Document doctype override -->\n", $GH);
    $buffer=preg_replace("/<!DOCTYPE [^>]*>/is", "", $buffer);
  }
  else {
    $GH=preg_replace("/(<!DOCTYPE [^>]*>)/is","\\1\n<!-- PHP-GridSite-Helper using doctype from GH -->",$GH);
  }

//html
  if ( ! preg_match("/<\s*html(?:\s[^>]*)*>/is",$GH) )       { $GH=preg_replace("/(<!DOCTYPE [^>]*>)/is", "\\1\n<html>", $GH); }
  if ( ! preg_match("/<\s*\/\s*html(?:\s[^>]*)*>/is",$GF) )  { $GF.="</html>"; }
  if ( preg_match("/<\s*html(\s[^>]*)*>/is",$buffer,$args) ) { $GH=preg_replace("/<\s*html(\s[^>]*)*>/is","<html".$args[1]."\\1>",$GH); }
  $buffer=preg_replace("/<\s*html(?:\s[^>]*)*>/is", "", $buffer); // html element now completely in GH
  $buffer=preg_replace("/<\s*\/\s*html\s*>/is", "", $buffer);

//head
  if ( ! preg_match("/<\s*head(?:\s[^>]*)*>/is",$GH) ) { $GH=preg_replace("/(<\s*html(?:\s[^>]*)*>)/is", "\\1\n<head></head>", $GH); }
  if ( preg_match("/<head(\s[^>]*|)>(.*)<\/head>/is",$buffer,$head) ) {
    $GH=preg_replace("/<\s*head(\s[^>]*)*>(.*)<\s*\/\s*head\s*>/is","<head".$head[1]."\\1>".$head[2]."\\2</head>",$GH);
  }
  $buffer=preg_replace("/<\s*head(?:\s[^>]*)*>.*<\s*\/\s*head>|<\/head\s*\/\s*>/", "", $buffer);

//body
  if ( ! preg_match("/<\s*body(?:\s[^>]*)*>/is",$GH) )       { $GH=preg_replace("/<\s*\/\s*head\s*>/is", "</head><body>", $GH); }
  if ( ! preg_match("/<\s*\/\s*body(?:\s[^>]*)*>/is",$GF) )  { $GF=preg_replace("/<\s*\/\s*html\s*>/is", "</body></html>", $GF); }
  if ( preg_match("/<\s*body(\s[^>]*|)>/is",$buffer,$args) ) { $GH=preg_replace("/<\s*body(\s[^>]*|)>/is","<body".$args[1]."\\1>",$GH); }
  $buffer=preg_replace("/<\s*body(?:\s[^>]*)*>/is", "", $buffer);
  $buffer=preg_replace("/<\s*\/\s*body\s*>/is", "", $buffer);

  return("$GH$buffer$GI$GF\n");
}

ob_start("filter");

/**
 * Cleanup, print output to STDOUT and shutdown
 * @ignore
 */
function shutdown()
{
  ob_end_flush();
  $contents = ob_get_contents();
  @ob_end_clean();
  print "$contents";
}

register_shutdown_function('shutdown');

?>
