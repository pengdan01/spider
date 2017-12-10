<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=gb2312" />
<title>赛迪网博客－中国最具人气的IT博客－赛迪网IT人家园</title>

<meta name="keywords" content="IT博客,IT评论,IT技术,信息化博客,产业博客,业界博客,博客" />
<meta name="description" content="IT博客－中国最具人气的IT博客－赛迪网IT人家园 赛迪网是国内权威IT门户网站，赛迪IT博客是各媒体关注焦点，您的博客信息能最大程度受到关注。赛迪博客浏览速度极快，一分钟注册，系统稳定。大批高端用户，关注业界动态共享IT技术，是权威的IT精英交流平台。博友圈结交IT界名人，《IT博客周刊》汇集博文精华" />
<meta name="MSSmartTagsPreventParsing" content="True" />
<meta http-equiv="MSThemeCompatible" content="Yes" />
<base href="http://ygg159ngy.smt.smt.co.blog.ccidnet.com/" />
<link href="template/default/blog/style/css/reset.css" rel="stylesheet" type="text/css" />
<link href="template/default/blog/style/css.css" rel="stylesheet" type="text/css" />
<script src="static/js/common.js" type="text/javascript"></script>
<script src="template/default/blog/jquery-1.3.2.js" type="text/javascript"></script>
<script>
//加入收藏
function AddToFavorite() {
if (document.all) {
window.external.addFavorite(document.URL, document.title);
} else if (window.sidebar) {
window.sidebar.addPanel(document.title, document.URL, "赛迪博客");
}
}
//设为首页
function setHomepage() {
if (document.all) {
document.body.style.behavior = 'url(#default#homepage)';
document.body.setHomePage(document.URL);
} else if (window.sidebar) {
if (window.netscape) {
try {
netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
} catch (e) {//火狐浏览器
alert("该操作被浏览器拒绝，如果想启用该功能，请在地址栏内输入 about:config, 然后将项 signed.applets.codebase_principal_support 值该为true");
}
}
var prefs = Components.classes['@mozilla.org/preferences-service;1'].getService(Components.interfaces.nsIPrefBranch);
prefs.setCharPref('browser.startup.homepage', document.URL);
}
}

</script>
<script>
$(function (){
$("#bk1").mouseover(function (){
$(".bkph1").show();
$(".bkph2").hide();
$("#bk1 a").addClass("hp-bq-hover");
$("#bk2 a").removeClass("hp-bq-hover");
})
$("#bk2").mouseover(function (){
$(".bkph1").hide();
$(".bkph2").show();
$("#bk1 a").removeClass("hp-bq-hover");
    $("#bk2 a").addClass("hp-bq-hover");
})
});
</script>
<style>
.blog-main3-right{}
.blog-main3-right h2{ background:url("./template/default/blog/style/images/h4.jpg") repeat-x scroll left top;}
.height3{height:139px !important;}
.height4{height:241px !important;}
.height5{height:259px !important;}
.height6{height:175px !important;}
.height7{height:350px !important;}

.b1{ border-top:0 !important;}

.jdtl-div1{width:705px;}
.jdtl-div1 h2{width:705px;height:24px; background: url(./template/default/blog/style/images/qybk1_1.jpg) top left no-repeat;padding-top:3px;}
.jdtl-div1 h2 span{padding-left:18px; font-size:18px; font-family: "黑体"; font-weight: normal;padding-right:30px;}
.jdtl-div1 h2 span a{font-size:18px; font-family: "黑体"; font-weight: normal; color:#000000; text-decoration:none;}
.p2{width:659px; background:url("./template/default/blog/style/images/jdtl-2.jpg") top left repeat-y;padding:17px 25px;}
.plist7{width:315px;height:105px;margin-bottom:25px;}
.plist7 h3{ color:#096c96; font-size:14px; line-height:26px;}
.plist7 h3 a{ color:#096c96; font-size:14px; line-height:26px; text-decoration:none}
.plist7-img{width:90px;height:60px; border:1px #d7d7d7 solid;padding:3px; text-align:center;}
.plist7-p{width:208px; text-align:left; color:#4d4d4d; line-height:23px;}
.plist8{width:315px;height:75px;margin-bottom:25px;}
.plist8 h3{ color:#096c96; font-size:14px; line-height:26px;}
.plist8-img{width:60px;height:60px; border:1px #d7d7d7 solid;padding:3px; text-align:center;}
.plist8-p{width:238px; text-align:left; color:#4d4d4d; line-height:23px;}
.plist8-p a{width:238px; text-align:left; color:#4d4d4d; text-decoration:none; font-size:12px;}

.plist10{width:605px;height:75px;}
.plist10 h3{ color:#096c96; font-size:14px; line-height:26px;}
.plist10-img{width:60px;height:60px; border:1px #d7d7d7 solid;padding:3px; text-align:center;}
.plist10-p{width:525px; text-align:left; color:#4d4d4d; line-height:23px; font-weight:normal;}
.plist10-p a{width:238px; text-align:left; color:#4d4d4d; text-decoration:none; font-size:12px;font-weight:normal;}
</style>
</head>

<body>
<script src="http://image.ccidnet.com/nav/CCID_JS/ccidnet_header.js" type="text/javascript"></script>

<div class="logo">
<img src="template/default/blog/style/images/blog-logo.jpg" width="186" height="61" class="nleft" />
    <div class="gn-div nright">
    	<span class="nleft padding1"><a href="" onClick="try{if(document.all) {window.external.addFavorite(' http://blog.ccidnet.com/','赛迪博客');} else {window.sidebar.addPanel('赛迪博客',' http://blog.ccidnet.com/','');}}catch(e){};return false;" target="_self">收藏本站</a><a href="blog.php"  onclick="setHomepage()">设为首页</a><a href="mailto:?subject=给你推荐一个不错的博客网站&amp;body=发现了一个不错的博客网站，赛迪网IT博客http://blog.ccidnet.com/，有时间关注一下。"  target="_blank">推荐朋友</a></span>
    	
      <span class="nleft"><form method="post" action="search.php?mod=blog&amp;searchsubmit=yes" target= "window.open( 'search.php?mod=blog&searchsubmit=yes')" name="search1"><input name="srchtxt" type="text" class="input1" value="搜索内容" /><input name="srchbtn" type="button"  onclick="window.document.search1.submit()" class="btn1" style="cursor:pointer;"/></form></span>
      
    </div>
</div>
<div class="clear"></div>
<div class="nav-h1">
<span class="nleft"><a href="blog.php"  target="_blank"  class="font1">首页</a></span>
    <span class="nleft"><a href="blog.php?mod=list&amp;type=industry&amp;catid=2,4,7,5,20,21,109,110,111"  target="_blank" class="font1">产业</a> <a href="blog.php?mod=list&amp;type=it&amp;catid=2"  target="_blank" class="font2">IT业界</a> <a href="blog.php?mod=list&amp;type=internet&amp;catid=4"  target="_blank" class="font2">互联网</a> <a href="blog.php?mod=list&amp;type=communication&amp;catid=7"  target="_blank" class="font2">通信</a> <a href="blog.php?mod=list&amp;type=infomation&amp;catid=5,20,21"  target="_blank" class="font2">信息化</a> <a href="blog.php?mod=list&amp;type=industry_head&amp;catid=109,110,111"  target="_blank" class="font2">产经</a></span>
    <span class="nleft"><a href="blog.php?mod=list&amp;type=technologyview&amp;catid=9,10,11,22"  target="_blank" class="font1">技术</a> <a href="blog.php?mod=list&amp;type=develop&amp;catid=10"  target="_blank" class="font2">开发</a> <a href="blog.php?mod=list&amp;type=internet_head&amp;catid=9"  target="_blank" class="font2">网络</a> <a href="blog.php?mod=list&amp;type=database&amp;catid=11"  target="_blank" class="font2">数据库</a> <a href="blog.php?mod=list&amp;type=system&amp;catid=22"  target="_blank" class="font2">操作系统</a></span>
    <span class="nleft"><a href="blog.php?mod=focus"  target="_blank" class="font1">焦点讨论</a></span>
    <span class="nleft"><a href="blog.php?mod=debatelist"  target="_blank" class="font1">博辩堂</a></span>
    <span class="nleft bgx1"><a href="blog.php?mod=business"  target="_blank" class="font1">企业博客</a></span>
</div>
<div class="clear"></div>
<div class="nav-h2">
<div class="nleft dlzc">
<!--  -->
<form method="post" name="loginform" autocomplete="off" id="lsform" action="member.php?mod=logging&amp;action=login&amp;loginsubmit=yes&amp;infloat=yes" onsubmit="return lsSubmit()">
    	<table width="100%" border="0" cellspacing="0" cellpadding="0">
          <tr>
            <td width="55" align="right"><label for="is_username">用户名</label></td>
            <td width="98" align="left"><input type="text" name="username" id="is_username" class="input1" tabindex="901" autocomplete="off"/></td>
            <td width="39" align="right"><label for="is_password">密码</label></td>
            <td width="105" align="left"><input type="password" name="password" id="is_password" tabindex="902" class="input1" autocomplete="off"/></td>
            <td width="24" valign="top" style="padding-top:3px;"><input type="checkbox" name="cookietime" id="is_cookietime" tabindex="903" value="2592000"/></td>
            <td width="51" valign="middle"><label for="is_cookietime">记住密码</label></td>
            <td width="71" align="center" valign="middle"><a style="color:#0A6C9B" oncilck="showWindow('login', this.href)" href="member.php?mod=logging&amp;action=login&amp;viewlostpw">找回密码</a></td>
            <td width="35" align="left" valign="middle"><input style="color:#0A6C9B; cursor:pointer;border:0;margi-right:5px;" type="submit" value="登录" ></td>
            <td width="77" align="left" valign="middle"><a style="color:#0A6C9B;" onclick="showWindow('register', this.href)" href="member.php?mod=register">注册</a></td>
          </tr>
        </table>
</form>
<!--  -->

  </div>
    <div class="nright dlch">【<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=3276&amp;do=blog&amp;id=20578537"  target="_blank">新版博客登陆通知</a>】【<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=3276&amp;do=blog&amp;id=20578544"  target="_blank">赛迪网博客新版上线1</a>】</div>
</div>
<!-- AFP Control Code/Caption.博客首页导航栏下通栏-->
<script src="http://www2.ccidnet.com/adpolestar/door/;ap=3ABFDB77_827A_E4D2_AF71_BEF06AF073A7;ct=js;pu=ccid;/?" type="text/javascript"></script>
<NOSCRIPT>
<IFRAME SRC="http://www2.ccidnet.com/adpolestar/door/;ap=3ABFDB77_827A_E4D2_AF71_BEF06AF073A7;ct=if;pu=ccid;/?" NAME="adFrame_3ABFDB77_827A_E4D2_AF71_BEF06AF073A7" WIDTH="950" HEIGHT="90" FRAMEBORDER="no" BORDER="0" MARGINWIDTH="0" MARGINHEIGHT="0" SCROLLING="no">
</IFRAME>
</NOSCRIPT>
<!-- AFP Control Code End/No.-->
<div class="clear"></div>
<div style="margin:5px 0;"></div>
<div class="clear"></div>
<!-- AFP Control Code/Caption.博客首页导航栏下通栏2-->
<script src=" http://www2.ccidnet.com/adpolestar/door/;ap=3EEEA804_F182_520D_22C5_3A4790FFF689;ct=js;pu=ccid;/?" type="text/javascript"></script>
<div class="clear"></div>
<div class="blog-main1">
<div class="nleft blog-main1-left">
        <div class="flash nleft"><div id="flashcontent01" class="zc-flash"></div>                
 
<script src="http://image.ccidnet.com/nav/miit/miitflash_1.js" type="text/javascript"></script>
                <script type="text/javascript">
var speed = 4000;
var focus_width=328;
var focus_height=227;
var text_height=3;
var swf_height = focus_height+text_height; 	
var
pics='http://blog.ccidnet.com/upload/edithtml/1.jpg|http://blog.ccidnet.com/upload/edithtml/2.jpg|http://blog.ccidnet.com/upload/edithtml/3.jpg|http://blog.ccidnet.com/upload/edithtml/4.jpg|http://blog.ccidnet.com/upload/edithtml/5.jpg';

var mylinks='http://blog.ccidnet.com/blog-htm-do-showone-uid-11711-itemid-20698716-type-blog.html|http://blog.ccidnet.com/blog-htm-do-showone-uid-269701-itemid-20697712-type-blog.html|http://blog.ccidnet.com/blog-htm-do-showone-uid-60117-itemid-20698250-type-blog.html|http://blog.ccidnet.com/blog-htm-do-showone-uid-244311-itemid-20639351-type-blog.html|http://blog.ccidnet.com/blog-htm-do-showone-uid-365925-itemid-20639020-type-blog.html';
var sohuFlash2 = new sohuFlash("http://image.ccidnet.com/nav/miit/showplay.swf","flashcontent01",focus_width,focus_height,text_height,"#ffffff");

var texts='拜苹果教徒 君知耻否|诺基亚WP来了，运营商怎么办|凡客诚品IPO是不是黄粱一梦|黄金宝马 团购网站如何另辟蹊径|中国移动如何解决渠道间的矛盾冲突';

sohuFlash2.addParam("quality", "medium");

sohuFlash2.addParam("wmode", "opaque");						sohuFlash2.addVariable("speed",speed);
sohuFlash2.addVariable("p",pics);	
sohuFlash2.addVariable("l",mylinks);
sohuFlash2.addVariable("icon",texts);
sohuFlash2.write("flashcontent01");
</script> </div>
        <div class="jdxw nleft">
            <div class="jdxw-top"><img src="template/default/blog/style/images/h1-1.jpg" width="376" height="6" /></div>
            <div class="jdxw-h2">
               <h2><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=95516&amp;do=blog&amp;id=20768088"  target="_blank" >腾讯搜狐爱奇艺抱团 版权采买回归理性</a></h2>
                <p><SPAN .="FONT-SIZE: 12pt; FONT-FAMILY: 楷体"><SPAN .="FONT-SIZE: 12pt; FONT-FAMILY: 楷体"></SPAN>UGC领域的两大冤家优酷、土豆合并了，目前正在整合中，PPS传闻要卖了.....<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60630&amp;do=blog&amp;id=20758500"  target="_blank">详细>></a></p>

                <ul>
                    <li><A href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60117&amp;do=blog&amp;id=20767938" target="_blank">相互抱团导致版权方受伤
</A></li>
                    <li><A href="http://news.ccidnet.com/art/1032/20120425/3807895_1.html" target="_blank">AA制买版权</A></li>
                </ul>
                <div class="clear"></div>
               <h2><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=239197&amp;do=blog&amp;id=20755123"  target="_blank" >360诉腾讯：当粘性成为霸权的时候
</a></h2>
                <p>2011年，腾讯诉360扣扣保镖构成不正当竞争，360扣扣保镖是掀起3Q.....<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=299771&amp;do=blog&amp;id=20733967"  target="_blank">详细>></a></p>

                <ul>
                    <li><A href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=244311&amp;do=blog&amp;id=20757902" target="_blank">腾讯为何怕网易新闻
</A></li>
                    <li><A href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60630&amp;do=blog&amp;id=20756751" target="_blank">360起诉腾讯的警示意义</A></li>
                </ul>
            </div>
          <div class="jdxw-bottom"><img src="template/default/blog/style/images/h1-2.jpg" width="376" height="4" /></div>
        </div>
        <div class="clear"></div>
        
    </div>
    <div class=" blog-main1-right nright">
    	<ul>
        	<li><!-- AFP Control Code/Caption.博客首页右侧按钮一-->
<script src="http://www2.ccidnet.com/adpolestar/door/;ap=0501DC5C_9DF2_3A80_472D_0AFB65CAFA20;ct=js;pu=ccid;/?" type="text/javascript"></script>
<NOSCRIPT>
<IFRAME SRC="http://www2.ccidnet.com/adpolestar/door/;ap=0501DC5C_9DF2_3A80_472D_0AFB65CAFA20;ct=if;pu=ccid;/?" NAME="adFrame_0501DC5C_9DF2_3A80_472D_0AFB65CAFA20" WIDTH="227" HEIGHT="109" FRAMEBORDER="no" BORDER="0" MARGINWIDTH="0" MARGINHEIGHT="0" SCROLLING="no">
</IFRAME>
</NOSCRIPT>
<!-- AFP Control Code End/No.-->
</li>
            <li><!-- AFP Control Code/Caption.博客首页右侧按钮二-->
<script src="http://www2.ccidnet.com/adpolestar/door/;ap=BAEFFFF3_A2DB_FABB_69F9_D5F3C3847ADF;ct=js;pu=ccid;/?" type="text/javascript"></script>
<NOSCRIPT>
<IFRAME SRC="http://www2.ccidnet.com/adpolestar/door/;ap=BAEFFFF3_A2DB_FABB_69F9_D5F3C3847ADF;ct=if;pu=ccid;/?" NAME="adFrame_BAEFFFF3_A2DB_FABB_69F9_D5F3C3847ADF" WIDTH="227" HEIGHT="109" FRAMEBORDER="no" BORDER="0" MARGINWIDTH="0" MARGINHEIGHT="0" SCROLLING="no">
</IFRAME>
</NOSCRIPT>
<!-- AFP Control Code End/No.--></li>
        </ul>
    </div>
</div>
 <div class="clear"></div>
<div class="blog-main2">

<div class="blog-main2-left nleft">
    <div class="rdgz-h2"><img src="template/default/blog/style/images/h2-2.jpg" width="714" height="31" /></div>
        <h2>
          <div class="nleft w1"><span class="nleft" style="padding-top:5px;">焦点讨论</span><span class="nright"><a href="blog.php?mod=focus" target="_blank"><img src="template/default/blog/style/images/more.jpg" width="25" height="27" /></a></span></div>
          <div class="nright w2"><span class="nleft" style="padding-top:5px;">博辩堂</span><span class="nright"><a href="blog.php?mod=debatelist" target="_blank"><img src="template/default/blog/style/images/more.jpg" width="25" height="27" /></a></span></div>
        </h2> 
        <div class="blog-main2-left-div">
        	<div class="nleft tl">
            	<div class="plist1">
                <div class="plist1-img nleft"><img src="http://blog.ccidnet.com/upload/edithtml/11.jpg" width="90" height="60" /></div> 
<div class="plist1-zi nleft" style="margin-left:8px;">
<a href="http://blog.ccidnet.com/portal.php?mod=topic&amp;topicid=41"  target="_blank"><b>热议视频网站抱团取暖</b></a><br>
如果再联系酷六的曲线上市，56网被纳入人人网怀抱，可以说......<a href="http://blog.ccidnet.com/portal.php?mod=topic&amp;topicid=41" target="_blank">[详细]</a>
</div>
                </div>
                <div class="clear"></div>
                <ul class="tl-list">
                                	<li><a href="portal.php?mod=topic&amp;topic=Instagram收购"  target="_blank" class="nleft">热议Instagram被高价收购</a></li>
                	                	<li><a href="portal.php?mod=topic&amp;topic=土豆优酷合并"  target="_blank" class="nleft">热议土豆优酷合并 折射视频业形式</a></li>
                	                	<li><a href="portal.php?mod=topic&amp;topic=阿里巴巴私有化"  target="_blank" class="nleft">热议阿里巴巴私有化B2B业务</a></li>
                	                </ul>
            </div>
          <div class="nright bbt">
            <h3><a href="blog.php?mod=debate"  target="_blank"> 3Q大战复燃 对反垄断的推进还是无聊的死磕</a></h3>
<form name="affirmform" method="post" action="blog.php?mod=debate">
<input type="hidden" name="stand" value=1>
</form>
<form name="negaform" method="post" action="blog.php?mod=debate">
<input type="hidden" name="stand" value=2>
</form>
                <div class="clear"></div>
            <div class="nleft tp1">
                	<div>
                    <table width="90%" border="0" align="center" cellpadding="0" cellspacing="0">
                      <tr>
                        <td width="65%"><table width="95%" border="0" cellspacing="0" cellpadding="0">
                          <tr>
                            <td align="center" class="font4">正&nbsp;&nbsp;方</td>
                          </tr>
                          <tr>
                            <td align="center"><span class="font7"> 679</span><span class="font4">票</span></td>
                          </tr>
                        </table></td>
                        <td width="35%" height="75"><img src="upload/edithtml/tp-img1.jpg" style="cursor:pointer;" onclick="document.affirmform.submit();" width="56" height="49" /></td>
                      </tr>
                      <tr>
                        <td colspan="2" class="font3">意义非凡 对互联网业反垄断反不正当竞争的推进具有法律借鉴价值</td>
                      </tr>
                    </table>
                </div>
            </div>
                <div class="nright tp2">
                	<table width="90%" border="0" align="center" cellpadding="0" cellspacing="0">
                      <tr>
                        <td width="65%"><table width="95%" border="0" cellspacing="0" cellpadding="0">
                          <tr>
                            <td align="center" class="font6">反&nbsp;&nbsp;方</td>
                          </tr>
                          <tr>
                            <td align="center"><span class="font8"> 172</span><span class="font6">票</span></td>
                          </tr>
                        </table></td>
                        <td width="35%" height="75"><img src="upload/edithtml/tp-img2.jpg"  style="cursor:pointer;" onclick="document.negaform.submit();" /></td>
                      </tr>
                      <tr>
                        <td colspan="2" class="font3">一场无聊的战争 用户伤不起 
</td>
                      </tr>
                    </table>
                </div>          </div>
      </div>
         
    </div>
    <div class="blog-main2-right nright">
    	<h2><span class="nleft">大话IT</span></h2>
        <div class="dhit">
        <ul> 
            	<li><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1895768&amp;extra=page%3D1"  

target="_blank">窝窝团副总裁陈拥军加盟乐蜂网</a></li>
                <li><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1898679&amp;extra=page%3D1"  

target="_blank">Lady GaGa今晚香港开唱 晒素颜照</a></li>
                <li><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1898624&amp;extra=page%3D1"  

target="_blank">《黄金》《匹夫》《杀生》票房跌破</a></li>
                <li><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1895782&amp;extra=page%3D1"  

target="_blank">优酷土豆联姻 资本红娘牵线</a></li>
                <li><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1895779&amp;extra=page%3D1"  

target="_blank">3亿注资巴诺：微软的现实与理想？</a></li>
                <li><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1895769&amp;extra=page%3D1"  

target="_blank">微软必应搜索悄然改版：页面清新</a></li>
                <li><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1893394&amp;extra=page%3D1"  

target="_blank">跨界的Win8，真有那么美？</a></li>
                <li><a href="http://bbs.ccidnet.com/forum.php?mod=redirect&amp;tid=1893376&amp;goto=lastpost#lastpost"  

target="_blank">玩转Win8下Hyper-V网络设置</a></li>
            </ul>        </div>
  </div>
</div>
<div class="clear"></div>
<div class="blog-main3">
<div class="blog-main3-left nleft">
    	<div class="rdgz-h2">
   	    <img src="template/default/blog/style/images/h5.jpg" width="714" height="31" /></div>
        <h2>
          <div class="nleft w1"><span class="nleft" style="padding-top:5px;">产业</span><span class="nright"><a href="blog.php?mod=list&amp;type=industry_recommend&amp;catid=110,109,111,2,7,5,4,20,21" target="_blank"><img src="template/default/blog/style/images/more.jpg" width="25" height="27" /></a></span></div>
          <div class="nright w2"><span class="nleft" style="padding-top:5px;">技术</span><span class="nright"><a href="blog.php?mod=list&amp;type=technology_recommend&amp;catid=22,11,10,9" target="_blank"><img src="template/default/blog/style/images/more.jpg" width="25" height="27" /></a></span></div>
        </h2>
        <div class="clear"></div> 
        <div class="blog-main3-left-div">
        <div id="portal_block_6" class="block move-span"><div id="portal_block_6_content" class="content"><ul class="nleft left-list1"><li><a href="home.php?mod=space&uid=269701&do=blog&id=20784754" class="nleft"  title="破解版免费WIFI可信否？"  target="_blank">破解版免费WIFI可信否？</a><span class="nright"  title="马继华"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=269701&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">马继华</a></span>
</li><li><a href="home.php?mod=space&uid=57180&do=blog&id=20791694" class="nleft"  title="独家联袂英国ITV，激动网进一步树立内容优势"  target="_blank">独家联袂英国ITV，激动网进一步树</a><span class="nright"  title="IT爱好者的创作坊"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=57180&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">IT爱好者</a></span>
</li><li><a href="home.php?mod=space&uid=60630&do=blog&id=20791536" class="nleft"  title="团购企业的“流水”之殇"  target="_blank">团购企业的“流水”之殇</a><span class="nright"  title="罗会祥IT风云"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=60630&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">罗会祥IT</a></span>
</li><li><a href="home.php?mod=space&uid=244311&do=blog&id=20790981" class="nleft"  title="Win8来袭，微软向苹果挥出的迷踪拳"  target="_blank">Win8来袭，微软向苹果挥出的迷踪</a><span class="nright"  title="康斯坦丁"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=244311&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">康斯坦丁</a></span>
</li><li><a href="home.php?mod=space&uid=269701&do=blog&id=20790412" class="nleft"  title="云存储已经成为智能手机的标配"  target="_blank">云存储已经成为智能手机的标配</a><span class="nright"  title="马继华"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=269701&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">马继华</a></span>
</li><li><a href="home.php?mod=space&uid=43492&do=blog&id=20790171" class="nleft"  title="飞利浦你是在忽悠消费者吗？我使用W820的真实情况。"  target="_blank">飞利浦你是在忽悠消费者吗？我使</a><span class="nright"  title="时间倒流"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=43492&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">时间倒流</a></span>
</li><li><a href="home.php?mod=space&uid=983311&do=blog&id=20790132" class="nleft"  title="腾讯证人姜奇平 渡人难渡已"  target="_blank">腾讯证人姜奇平 渡人难渡已</a><span class="nright"  title="城宇"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=983311&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">城宇</a></span>
</li><li><a href="home.php?mod=space&uid=269701&do=blog&id=20784754" class="nleft"  title="破解版免费WIFI可信否？"  target="_blank">破解版免费WIFI可信否？</a><span class="nright"  title="马继华"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=269701&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">马继华</a></span>
</li><li><a href="home.php?mod=space&uid=9507&do=blog&id=20783823" class="nleft"  title="市场公关呈现三大趋势"  target="_blank">市场公关呈现三大趋势</a><span class="nright"  title="曹健"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=9507&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">曹健</a></span>
</li><li><a href="home.php?mod=space&uid=244311&do=blog&id=20785641" class="nleft"  title="1号店进入图书网购市场大猜想"  target="_blank">1号店进入图书网购市场大猜想</a><span class="nright"  title="康斯坦丁"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=244311&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">康斯坦丁</a></span>
</li><li><a href="home.php?mod=space&uid=239786&do=blog&id=20785559" class="nleft"  title="开发商京东的闹剧"  target="_blank">开发商京东的闹剧</a><span class="nright"  title="谭涛"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=239786&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">谭涛</a></span>
</li><li><a href="home.php?mod=space&uid=60117&do=blog&id=20785542" class="nleft"  title="淘宝腐败将马云推入尴尬处境"  target="_blank">淘宝腐败将马云推入尴尬处境</a><span class="nright"  title="于斌"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=60117&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">于斌</a></span>
</li></ul>
</div></div>            <div id="portal_block_7" class="block move-span"><div id="portal_block_7_content" class="content"><ul class="nleft left-list1"><li><a href="home.php?mod=space&uid=62827&do=blog&id=20777887" class="nleft"  title="未能解析引用的程序集，因为它对不在当前目标框架，请删除对不在目标框架中的程序集的 ..."  target="_blank">未能解析引用的程序集，因为它对</a><span class="nright"  title="程序人家"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=62827&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">程序人家</a></span>
</li><li><a href="home.php?mod=space&uid=324810&do=blog&id=20694101" class="nleft"  title="星巴克、COACH免费送原来是骗局！"  target="_blank">星巴克、COACH免费送原来是骗局！</a><span class="nright"  title="趋势科技-云安全博客"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科技</a></span>
</li><li><a href="home.php?mod=space&uid=324810&do=blog&id=20690945" class="nleft"  title="IP地址会泄露你哪些信息？"  target="_blank">IP地址会泄露你哪些信息？</a><span class="nright"  title="趋势科技-云安全博客"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科技</a></span>
</li><li><a href="home.php?mod=space&uid=37742&do=blog&id=20680193" class="nleft"  title="白话SCRUM 之四：燃尽图"  target="_blank">白话SCRUM 之四：燃尽图</a><span class="nright"  title="任甲林"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=37742&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">任甲林</a></span>
</li><li><a href="home.php?mod=space&uid=20630&do=blog&id=20680771" class="nleft"  title="Matlab通过JDBC建立到Oracle数据库的连接"  target="_blank">Matlab通过JDBC建立到Oracle数据</a><span class="nright"  title="zhangxinjie blog  MSN:zhang-xinjie@163.com"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=20630&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">zhangxin</a></span>
</li><li><a href="home.php?mod=space&uid=975104&do=blog&id=20680173" class="nleft"  title="position下的fixed完美兼容IE6/IE7/IE8/Firefox"  target="_blank">position下的fixed完美兼容IE6/I</a><span class="nright"  title="李晓蒙"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=975104&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">李晓蒙</a></span>
</li><li><a href="home.php?mod=space&uid=324810&do=blog&id=20675325" class="nleft"  title="Android市场上的又一波**"  target="_blank">Android市场上的又一波**</a><span class="nright"  title="趋势科技-云安全博客"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科技</a></span>
</li><li><a href="home.php?mod=space&uid=324810&do=blog&id=20668233" class="nleft"  title="APT持续性渗透攻击未来将出现更多挑战"  target="_blank">APT持续性渗透攻击未来将出现更多</a><span class="nright"  title="趋势科技-云安全博客"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科技</a></span>
</li><li><a href="home.php?mod=space&uid=348450&do=blog&id=20667941" class="nleft"  title="Visual Studio 11 Beta  和 .NET 框架 4.5 Beta 预览"  target="_blank">Visual Studio 11 Beta  和 .NET</a><span class="nright"  title="李英林"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=348450&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">李英林</a></span>
</li><li><a href="home.php?mod=space&uid=975104&do=blog&id=20667917" class="nleft"  title="ie6 下 overflow:hidden 失效"  target="_blank">ie6 下 overflow:hidden 失效</a><span class="nright"  title="李晓蒙"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=975104&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">李晓蒙</a></span>
</li><li><a href="home.php?mod=space&uid=40080&do=blog&id=20661247" class="nleft"  title="输入框限制输入字数脚本"  target="_blank">输入框限制输入字数脚本</a><span class="nright"  title="[敞篷帅哥]的赛迪博客"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=40080&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">[敞篷帅哥</a></span>
</li><li><a href="home.php?mod=space&uid=40080&do=blog&id=20657010" class="nleft"  title="IE6中Form.submit不提交的问题"  target="_blank">IE6中Form.submit不提交的问题</a><span class="nright"  title="[敞篷帅哥]的赛迪博客"> <a href="http://blog.ccidnet.com/home.php?mod=space&uid=40080&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">[敞篷帅哥</a></span>
</li></ul></div></div>        </div>
    </div>
    <div class="blog-main3-right nright">
    	<h2><span class="nleft">热门关键字</span></h2>
    	<div class="gjz"><a href="search.php?mod=blog&amp;srchtxt=阿里巴巴私有化&amp;searchsubmit=yes"  target="_blank" class="gjz-c3 gjz-italic gjz-font12 gjz-font16">阿里巴巴私有化</a> 
            <a href="search.php?mod=blog&amp;srchtxt=安卓&amp;searchsubmit=yes"  target="_blank" class="gjz-c1 gjz-normal gjz-font22  gjz-italic">安卓</a>  
            <a href="search.php?mod=blog&amp;srchtxt=Windows8&amp;searchsubmit=yes"  target="_blank" class="gjz-c2 gjz-bold gjz-font12 gjz-font14">Windows8</a>  
            <a href="search.php?mod=blog&amp;srchtxt=ipad3&amp;searchsubmit=yes"  target="_blank" class="gjz-c3 gjz-italic gjz-font12 gjz-font16">ipad3</a>  
            <a href="search.php?mod=blog&amp;srchtxt=联通宽带&amp;searchsubmit=yes"  target="_blank"  class="gjz-c4 gjz-bold gjz-font16 gjz-font18">联通宽带</a>  
            <a href="search.php?mod=blog&amp;srchtxt=诺基亚&amp;searchsubmit=yes"  target="_blank" class="gjz-c5 gjz-normal gjz-font12 gjz-font16">诺基亚</a>  
            <a href="search.php?mod=blog&amp;srchtxt=两会&amp;searchsubmit=yes"  target="_blank"  class="gjz-c6 gjz-bold gjz-font12 gjz-font14">两会</a>  
            <a href="search.php?mod=blog&amp;srchtxt=智能手机&amp;searchsubmit=yes"  target="_blank"  class="gjz-c7 gjz-normal gjz-font12 gjz-font12">智能手机</a>  
            <a href="search.php?mod=blog&amp;srchtxt=团购&amp;searchsubmit=yes"  target="_blank" class="gjz-c8 gjz-bold gjz-font18 gjz-font14">团购</a>  
            <a href="search.php?mod=blog&amp;srchtxt=乔布斯&amp;searchsubmit=yes"  target="_blank" class="gjz-c1 gjz-italic gjz-font12 gjz-font12">乔布斯</a>　 
            <a href="search.php?mod=blog&amp;srchtxt=三网融合&amp;searchsubmit=yes"  target="_blank"  class="gjz-c2 gjz-bold gjz-font12 gjz-font16">三网融合</a>　 
                   </div>
        <SCRIPT LANGUAGE="JavaScript">
<!--
// #########  start checkEmail #################
            function checkEmail(emailStr) {

               if (emailStr.length == 0) {

                   return true;
               }
               var emailPat=/^(.+)@(.+)$/;
               var specialChars="\\(\\)<>@,;:\\\\\\\"\\.\\[\\]";
               var validChars="\[^\\s" + specialChars + "\]";
               var quotedUser="(\"[^\"]*\")";
               var ipDomainPat=/^(\d{1,3})[.](\d{1,3})[.](\d{1,3})[.](\d{1,3})$/;
               var atom=validChars + '+';
               var word="(" + atom + "|" + quotedUser + ")";
               var userPat=new RegExp("^" + word + "(\\." + word + ")*$");
               var domainPat=new RegExp("^" + atom + "(\\." + atom + ")*$");
               var matchArray=emailStr.match(emailPat);
               if (matchArray == null) {
                   return false;
               }
               var user=matchArray[1];
               var domain=matchArray[2];
               if (user.match(userPat) == null) {
                   return false;
               }
               var IPArray = domain.match(ipDomainPat);
               if (IPArray != null) {
                   for (var i = 1; i <= 4; i++) {
                      if (IPArray[i] > 255) {
                         return false;
                      }
                   }
                   return true;
               }
               var domainArray=domain.match(domainPat);
               if (domainArray == null) {
                   return false;
               }
               var atomPat=new RegExp(atom,"g");
               var domArr=domain.match(atomPat);
               var len=domArr.length;
               if ((domArr[domArr.length-1].length < 2) ||
                   (domArr[domArr.length-1].length > 3)) {
                   return false;
               }
               if (len < 2) {
                   return false;
               }
               return true;
            }
// ########## end checkEmail #########################
function change(m){

 if (checkEmail(document.maillist_sub.email.value))
{
  if(m==1){
document.maillist_sub.cmd.value="1";
  }
  if(m==2){
document.maillist_sub.cmd.value="2";
  }
document.maillist_sub.submit();
}
else
{
alert("Email 错误，请确认。");
return false;
}

}

//-->

</SCRIPT>
       <h2 class="x1t"><span class="nleft">IT博客周刊</span><a target="_blank" href="http://blog.ccidnet.com/blog-htm-do-showone-uid-22055-type-blog-itemid-10665703.html" class="nright"  target="_blank"><img src="template/default/blog/style/images/more1.jpg" width="28" height="9" /></a></h2>
        <form name="maillist_sub" method="post" action="http://java.ccidnet.com/image/mailweek1.jsp" target="_blank">
        <input name="id" type="hidden" value="35"> 
        <input name="cmd" type="hidden" value="1"> 
        <input name="sendnote" type="hidden" value="yes">
        <h3><input name="email" type="text" class="input2" /></h3>
        <h4><input name="but1" type="button" class="btn3" style="cursor:pointer;" onclick="JavaScript:change(1);"/>&nbsp;&nbsp;&nbsp;&nbsp;<input name="but2" type="button" class="btn4" style="cursor:pointer;" onclick="JavaScript:change(2);"/></h4>
       <ul class="itzk"> 
            <li><a href="http://news.ccidnet.com/art/1032/20110919/2768905_1.html"  target="_blank">9月16日：微软借Windows8崛起</a></li>
            <li><a href="http://blog.ccidnet.com/zhuanti/blogzk/198.htm"  target="_blank">第198期：腾讯反击乱投医</a></li>
            <li><a href="http://blog.ccidnet.com/zhuanti/blogzk/197.htm"  target="_blank">第197期：Symbian寿终正寝</a></li>
            <li><a href="http://blog.ccidnet.com/zhuanti/blogzk/196.htm"  target="_blank">第196期：惠普CEO辞职 </a></li>
            <li><a href="http://blog.ccidnet.com/zhuanti/blogzk/195.htm"  target="_blank">第195期：苹果版蒙娜丽莎</a></li>
            
        </ul>    </div>
  </form>
</div>
<div class="clear"></div>
<div class="blog-main4">
<div class="nleft blog-main4-left">
    <div class="blog-main4-nei">
            <div class="h6-cont1 height1 nleft">
                <h2><span class="nleft"><img src="template/default/blog/style/images/h6.jpg" width="357" height="51" border="0" usemap="#Map" />
                    <map name="Map" id="Map">
                      <area shape="rect" coords="9,4,148,44" href="blog.php?mod=list&amp;type=it&amp;catid=2" target="_blank"/>
                    </map>
                </span></h2>
                <div class="clear"></div>
                 <div class="plist2">    
                    <div class="plist2-img nleft"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=9507&amp;do=blog&amp;id=20672062" target="_blank"><img src="http://image.ccidnet.com/imgupload/img/1330995882cj.jpg" width="60" height="60" /></a></div>
                    <div class="plist2-zi nright"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=9507&amp;do=blog&amp;id=20672062"  target="_blank"><b>曹健：柯达没落的根源</b></a><br>
                    要不是柯达申请破产保护，人们可能很难想象，巨人的倒下竟然也能如此迅速......<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=9507&amp;do=blog&amp;id=20672062" target="_blank">[详细]</a></div>
                </div>
                <div class="clear"></div>
                <h3>业界名博</h3>
                <div class="rwmz"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=839932&amp;view=admin"  target="_blank">杨孝文</a> | <a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=40429&amp;view=admin"  target="_blank">刘兴亮</a> | <a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=968908&amp;view=admin"  target="_blank">何&nbsp;&nbsp;玺</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-88995.html"  target="_blank">贾敬华</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-39509.html"  target="_blank">项有建</a> | <a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=46358&amp;view=admin"  target="_blank">赵玉勇</a><br />
     <a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=248922&amp;view=admin"  target="_blank">于&nbsp;&nbsp;刚</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-11711.html"  target="_blank">张书乐</a> | <a href="http://joywlp.blog.ccidnet.com/blog-htm-uid-349850.html"  target="_blank">朱&nbsp;&nbsp;帅</a> | <a href="http://lizhongcun.blog.ccidnet.com/blog-htm-uid-58468.html"  target="_blank">李忠存</a> | <a href="http://mimi19730930.blog.ccidnet.com/blog-htm-uid-67711.html"  target="_blank">米晓彬</a> | <a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=54269&amp;view=admin"  target="_blank">杨宇良</a></div>      </div>
            
            <div class="h6-list nright ">
            <h2 class="h6-bg"></h2>
            <div class="clear"></div>
           <div id="portal_block_8" class="block move-span"><div id="portal_block_8_content" class="content"><ul class="nleft left-list1"><li><a href="home.php?mod=space&uid=88995&do=blog&id=20810793" class="nleft"  title="2012GMIC马化腾强调移动安全 腾讯再次出招" target="_blank">2012GMIC马化腾强调移动安全 腾讯</a><span class="nright" title="贾敬华"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=88995&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">贾敬华</a></span></li><li><a href="home.php?mod=space&uid=244311&do=blog&id=20809522" class="nleft"  title="百科行家：开启专家类社交新时代" target="_blank">百科行家：开启专家类社交新时代</a><span class="nright" title="康斯坦丁"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=244311&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">康斯坦</a></span></li><li><a href="home.php?mod=space&uid=239786&do=blog&id=20808995" class="nleft"  title="美团对“流水门”保持沉默的猜想" target="_blank">美团对“流水门”保持沉默的猜想</a><span class="nright" title="谭涛"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=239786&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">谭涛</a></span></li><li><a href="home.php?mod=space&uid=11711&do=blog&id=20807699" class="nleft"  title="问答让你直击消费者" target="_blank">问答让你直击消费者</a><span class="nright" title="张书乐—武当派张三疯"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=11711&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">张书乐</a></span></li><li><a href="home.php?mod=space&uid=269701&do=blog&id=20806761" class="nleft"  title="智能手机售后服务不能要钱不要命" target="_blank">智能手机售后服务不能要钱不要命</a><span class="nright" title="马继华"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=269701&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">马继华</a></span></li><li><a href="home.php?mod=space&uid=77345&do=blog&id=20806102" class="nleft"  title="雅虎被谁毁了" target="_blank">雅虎被谁毁了</a><span class="nright" title="宏毅"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=77345&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">宏毅</a></span></li><li><a href="home.php?mod=space&uid=244311&do=blog&id=20802651" class="nleft"  title="科技绽放超极本事：2012惠普科技影响力峰会" target="_blank">科技绽放超极本事：2012惠普科技</a><span class="nright" title="康斯坦丁"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=244311&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">康斯坦</a></span></li></ul>
</div></div>            </div>
          <div class="h6-bottom"><img src="template/default/blog/style/images/h6-bg.jpg" width="714" height="5" /></div>
        </div>
    <div class="blog-main4-nei margin1">
            <div class="h6-cont1 height1 nleft">
                <h2><span class="nleft"><img src="template/default/blog/style/images/h7.jpg" width="357" height="51" border="0" usemap="#Map2" />
                    <map name="Map2" id="Map2">
                      <area shape="rect" coords="9,4,145,45" href="blog.php?mod=list&amp;type=internet&amp;catid=4" target="_blank"/>
                    </map>
                </span></h2>
                <div class="clear"></div>
                <div class="plist2">    
                    <div class="plist2-img nleft"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=11711&amp;do=blog&amp;id=20671338" target="_blank"><img src="http://image.ccidnet.com/imgupload/img/1330995968zsl.jpg" width="60" height="60" /></a></div>
  <div class="plist2-zi nright"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=11711&amp;do=blog&amp;id=20671338"  target="_blank"><b>张书乐：团购战走入精细为王</b></a><br>
       互联网最近又刮起一阵决定风，又出现了所谓的“决定体”，网络世界被席卷......<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=11711&amp;do=blog&amp;id=20671338" target="_blank">[详细]</a></div>
                </div>
                <div class="clear"></div>
                <h3>互联网名博</h3>
                <div class="rwmz"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=681050&amp;view=admin"  target="_blank">姜洪军</a> | <a href="http://blog.ccidnet.com/?62265"  target="_blank">小刀马</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-50707.html"  target="_blank">瞬&nbsp;&nbsp;雨</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-285656.html"  target="_blank">方礼勇</a> | <a href="http://blog.ccidnet.com/?961617"  target="_blank">刘晶晶</a> | <a href="http://blog.ccidnet.com/?77345"  target="_blank">宏&nbsp;&nbsp;毅</a><br />
     <a href="http://blog.ccidnet.com/?244311"  target="_blank">王&nbsp;&nbsp;鲲</a> | <a href="http://blog.ccidnet.com/?285973"  target="_blank">谢&nbsp;&nbsp;青</a> | <a href="http://blog.ccidnet.com/?308618"  target="_blank">安明明</a> | <a href="http://jeszhang.blog.ccidnet.com/blog-htm-uid-50115.html"  target="_blank">张&nbsp;&nbsp;毅</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-60019.html"  target="_blank">金错刀</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-95516.html"  target="_blank">赵福军</a></div>      </div>
            
            <div class="h6-list nright ">
            <h2 class="h6-bg"></h2>
            <div class="clear"></div>
            <div id="portal_block_9" class="block move-span"><div id="portal_block_9_content" class="content"><ul class="nleft left-list1"><li><a href="home.php?mod=space&uid=53507&do=blog&id=20621091" class="nleft"  title="域名何来“拆迁办”？保护还是添乱？" target="_blank">域名何来“拆迁办”？保护还是添</a><span class="nright" title="徐祖哲"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=53507&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">徐祖哲</a></span></li><li><a href="home.php?mod=space&uid=285973&do=blog&id=20620783" class="nleft"  title="天际网：让人脉搭建更有效率" target="_blank">天际网：让人脉搭建更有效率</a><span class="nright" title="寻找一颗星的独一无二领地"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=285973&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">寻找一</a></span></li><li><a href="home.php?mod=space&uid=242481&do=blog&id=20620360" class="nleft"  title="当全能明星“邂逅”惠普激光一体机" target="_blank">当全能明星“邂逅”惠普激光一体</a><span class="nright" title="鲨鱼"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=242481&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">鲨鱼</a></span></li><li><a href="home.php?mod=space&uid=54269&do=blog&id=20620034" class="nleft"  title="佳品网遭黑：契约精神与客户至上" target="_blank">佳品网遭黑：契约精神与客户至上</a><span class="nright" title="杨宇良的BLOG"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=54269&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">杨宇良</a></span></li><li><a href="home.php?mod=space&uid=299771&do=blog&id=20619986" class="nleft"  title="阿里巴巴推手机志在支付 或选错合作对象" target="_blank">阿里巴巴推手机志在支付 或选错合</a><span class="nright" title="曾高飞"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=299771&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">曾高飞</a></span></li><li><a href="home.php?mod=space&uid=862003&do=blog&id=20619925" class="nleft"  title="Q+开放平台：开发应用必不可少的四大要素" target="_blank">Q+开放平台：开发应用必不可少的</a><span class="nright" title="梦里秦淮"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=862003&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">梦里秦</a></span></li><li><a href="home.php?mod=space&uid=862003&do=blog&id=20619924" class="nleft"  title="电商外包未来可能成为一种主流渠道方式" target="_blank">电商外包未来可能成为一种主流渠</a><span class="nright" title="梦里秦淮"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=862003&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">梦里秦</a></span></li></ul></div></div>            </div>
            <div class="h6-bottom"><img src="template/default/blog/style/images/h6-bg.jpg" width="714" height="5" /></div>
      </div>
    </div>
    <div class="nright blog-main4-right">
    	<h2><span class="nleft">博客之星</span></h2>
        <div class="plist3">
            <div class="plist3-img nleft"><a href="http://blog.ccidnet.com/blog-htm-uid-954177.html" target="_blank"><img src="upload/edithtml/hjy6.jpg" width="60" height="60" /></a></div>
            <div class="plist3-zi nright"><a href="http://blog.ccidnet.com/blog-htm-uid-954177.html"  target="_blank">黄井洋：中国通信及IT互联网行业的观察者。</a></div>
        </div>
        <div class="clear"></div>
         <div class="plist3">
            <div class="plist3-img nleft"><a href="http://blog.ccidnet.com/space-uid-62265.html" target="_blank"><img src="upload/edithtml/xdm.jpg" width="60" height="60" /></a></div>
            <div class="plist3-zi nright"><a href=" http://blog.ccidnet.com/space-uid-62265.html"  target="_blank">小刀马：互联网著名独立评论专家</a></div>
        </div>
        <div class="clear"></div>
         <div class="plist3">
            <div class="plist3-img nleft"><a href=" http://blog.ccidnet.com/home.php?mod=space&uid=40429&view=admin" target="_blank"><img src="upload/edithtml/lxl.jpg" width="60" height="60" /></a></div>
            <div class="plist3-zi nright"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=40429&amp;view=admin"  target="_blank">刘兴亮：IT评论人 IT龙门阵创始人</a></div>
        </div>
        <div class="clear"></div>
         <div class="plist3">
            <div class="plist3-img nleft"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=299771&amp;view=admin" target="_blank"><img src="upload/edithtml/zgf.jpg" width="60" height="60" /></a></div>
            <div class="plist3-zi nright"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=299771&amp;view=admin"  target="_blank">曾高飞：人民网副主编 专注于通信、家电领域 </a></div>
        </div>
        <div class="clear"></div>
        <div class="plist3">
            <div class="plist3-img nleft"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=50707&amp;view=admin" target="_blank"><img src="upload/edithtml/sy.jpg" width="60" height="60" /></a></div>
            <div class="plist3-zi nright"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=50707&amp;view=admin"  target="_blank">瞬雨：技术、管理、产经等领域 观察家</a></div>
        </div>    </div>
</div>
<div class="clear"></div>
<div class="blog-main4">
<div class="nleft blog-main4-left">
    	<div class="blog-main4-nei">
            <div class="h6-cont1 height1 nleft">
                <h2><span class="nleft"><img src="template/default/blog/style/images/h8.jpg" width="357" height="51" border="0" usemap="#Map3" />
                    <map name="Map3" id="Map3">
                      <area shape="rect" coords="8,3,130,45" href="blog.php?mod=list&amp;type=communication&amp;catid=7" target="_blank"/>
                    </map>
                </span></h2>
                <div class="clear"></div>
               <div class="plist2">  
                    <div class="plist2-img nleft"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=348450&amp;do=blog&amp;id=20672981" target="_blank"><img src="http://image.ccidnet.com/imgupload/img/1330996105lyl.jpg" width="60" height="60" /></a></div>
  <div class="plist2-zi nright"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=348450&amp;do=blog&amp;id=20672981"  target="_blank"><b>李英林：透过安卓Android看世界
</b></a><br>智能手机风靡世界当始于苹果的iPhone。我曾迷恋其工业设计的优雅......<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=348450&amp;do=blog&amp;id=20672981" target="_blank">[详细]</a></div>
                </div>
                <div class="clear"></div>
                <h3>通信名博</h3>
                <div class="rwmz"><a href="http://blog.ccidnet.com/?5759"  target="_blank">项立刚</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-501.html"  target="_blank">付&nbsp;&nbsp;亮</a> | <a href="http://blog.ccidnet.com/?269701"  target="_blank">马继华</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-5329.html"  target="_blank">曾剑秋</a> | <a href="http://blog.ccidnet.com/?59435"  target="_blank">毛启盈</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-299771.html"  target="_blank">曾高飞</a><br />
     <a href="http://blog.ccidnet.com/?6062"  target="_blank">辛鹏骏</a> | <a href="http://blog.ccidnet.com/blog-htm-uid-3712.html"  target="_blank">毕朝晖</a> | <a href="http://blog.ccidnet.com/?389160"  target="_blank">陈志刚</a> | <a href="http://zhousy.blog.ccidnet.com/blog-htm-itemid-19027425-uid-269783-do-showone-type-blog.html"  target="_blank">周双阳</a> | <a href="http://blog.ccidnet.com/?64063"  target="_blank">王艳辉</a> | <a href="http://blog.ccidnet.com/?784864"  target="_blank">梁振鹏</a></div>
      </div>
            
            <div class="h6-list nright ">
            <h2 class="h6-bg"></h2>
            <div class="clear"></div>
            <div id="portal_block_10" class="block move-span"><div id="portal_block_10_content" class="content"><ul class="nleft left-list1"><li><a href="home.php?mod=space&uid=258637&do=blog&id=20621115" class="nleft"  title="原创|一块钱改变在线旅行的革命力量" target="_blank">原创|一块钱改变在线旅行的革命力</a><span class="nright" title="IT界"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=258637&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">IT界</a></span></li><li><a href="home.php?mod=space&uid=60117&do=blog&id=20620156" class="nleft"  title="南京电信打人罚跪说明急需内部整改" target="_blank">南京电信打人罚跪说明急需内部整</a><span class="nright" title="于斌"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=60117&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">于斌</a></span></li><li><a href="home.php?mod=space&uid=299785&do=blog&id=20619791" class="nleft"  title="国产手机如何把握云时代？" target="_blank">国产手机如何把握云时代？</a><span class="nright" title="兰的IT世界"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=299785&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">兰的I</a></span></li><li><a href="home.php?mod=space&uid=299771&do=blog&id=20619657" class="nleft"  title="苹果太硬 三星撤诉服软为专利大战解套" target="_blank">苹果太硬 三星撤诉服软为专利大战</a><span class="nright" title="曾高飞"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=299771&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">曾高飞</a></span></li><li><a href="home.php?mod=space&uid=299771&do=blog&id=20619656" class="nleft"  title="诺基亚西方大溃败 中印难成救世主" target="_blank">诺基亚西方大溃败 中印难成救世主</a><span class="nright" title="曾高飞"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=299771&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">曾高飞</a></span></li><li><a href="home.php?mod=space&uid=59435&do=blog&id=20618519" class="nleft"  title="云通信带动国际电话会议的革命" target="_blank">云通信带动国际电话会议的革命</a><span class="nright" title="毛启盈"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=59435&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">毛启盈</a></span></li><li><a href="home.php?mod=space&uid=60117&do=blog&id=20618292" class="nleft"  title="诺基亚的发展仍然存在变数" target="_blank">诺基亚的发展仍然存在变数</a><span class="nright" title="于斌"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=60117&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">于斌</a></span></li></ul></div></div>            </div>
            <div class="h6-bottom"><img src="template/default/blog/style/images/h6-bg.jpg" width="714" height="5" /></div>
        </div>
        <div class="blog-main4-nei margin1">
            <div class="h6-cont1 nleft height2">
                <h2><span class="nleft"><img src="template/default/blog/style/images/h9.jpg" width="357" height="51" border="0" usemap="#Map4" />
                    <map name="Map4" id="Map4">
                      <area shape="rect" coords="10,7,178,42" href="blog.php?mod=list&amp;type=industryview&amp;catid=5,20,21,109,110,111" target="_blank"/>
                    </map>
                </span></h2>
              <div class="clear"></div>
                <div class="plist5">
                <div class="sp-zi nleft">信息化</div>
                    <div class="plist5-img nleft"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=62265&amp;do=blog&amp;id=20613166" target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1306890501.jpg" width="60" height="60" /></a></div>
                    <div class="plist5-zi nright"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=62265&amp;do=blog&amp;id=20613166"  target="_blank"style="color:#4d4d4d;"><b>小刀马：云计算落到实处才有大发展</b></a><br>众所周知，云计算概念被提出来已经很长时间了，一些云计算的应用......<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=62265&amp;do=blog&amp;id=20613166" target="_blank" style="color:#4d4d4d">[详细]</div>
                </div>                <div class="clear"></div>
                <div id="portal_block_11" class="block move-span"><div id="portal_block_11_content" class="content"><ul class="list-ul1"><li><a href="home.php?mod=space&uid=55399&do=blog&id=20798988" class="nleft"  title="化妆品电商NALAshop推出社区网站纳米汇" target="_blank">化妆品电商NALAshop推出社区网站</a><span class="nright" title="赵卫东"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=55399&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">赵卫东</a></span></li><li><a href="home.php?mod=space&uid=53507&do=blog&id=20795262" class="nleft"  title="紧急：国防电子展“OA技术促进应急灾备民用论坛”" target="_blank">紧急：国防电子展“OA技术促进应</a><span class="nright" title="徐祖哲"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=53507&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">徐祖哲</a></span></li><li><a href="home.php?mod=space&uid=825979&do=blog&id=20789535" class="nleft"  title="云计算产品系统必须是低熵系统" target="_blank">云计算产品系统必须是低熵系统</a><span class="nright" title="赵文银"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=825979&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">赵文银</a></span></li></ul>
</div></div>      </div>
            
            <div class="h6-list nright ">
            <h2 class="h6-bg"></h2>
            <div class="clear"></div>
          <div class="h6-cont1 margin2">
                <div class="plist5">  
  <div class="sp-zi nleft">产经</div>
                    <div class="plist5-img nleft"><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=758554&amp;do=blog&amp;id=20581294" target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1298861672.jpg" width="60" height="60" /></a></div>
  <div class="plist5-zi nright">"<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=758554&amp;do=blog&amp;id=20581294"  target="_blank"style="color:#4d4d4d;"><b>张曙光：土地房屋乃民生之本</b></a><br>衣、食、住、行是人们的基本生存活动。随着科学技术的发展......<a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=758554&amp;do=blog&amp;id=20581294" target="_blank" style="color:#4d4d4d">[详细]</div>
                </div>                <div class="clear"></div>
                <div id="portal_block_12" class="block move-span"><div id="portal_block_12_content" class="content"><ul class="list-ul1"><li><a href="home.php?mod=space&uid=758532&do=blog&id=20808123" class="nleft"  title="按价值投资中国股市最多值1500点" target="_blank">按价值投资中国股市最多值1500点</a><span class="nright" title="段绍译快乐理财游学苑"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=758532&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">段绍译</a></span></li><li><a href="home.php?mod=space&uid=37839&do=blog&id=20802415" class="nleft"  title="IT人怎么看中菲南海对峙？" target="_blank">IT人怎么看中菲南海对峙？</a><span class="nright" title="蒙圣光"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=37839&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">蒙圣光</a></span></li><li><a href="home.php?mod=space&uid=758532&do=blog&id=20800407" class="nleft"  title="快乐理财游学苑" target="_blank">快乐理财游学苑</a><span class="nright" title="段绍译快乐理财游学苑"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=758532&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">段绍译</a></span></li></ul>
</div></div>      </div>
            </div>
            <div class="h6-bottom"><img src="template/default/blog/style/images/h6-bg.jpg" width="714" height="5" /></div>
        </div>
    </div>
    <div class="nright blog-main5-right">
    	<div class="fg1">
            <h2><span class="nleft">博客圈子</span><a href="blog.php?mod=group" class="nright"  target="_blank"><img src="template/default/blog/style/images/more1.jpg" width="28" height="9" /></a></h2>
            <div class="plist4 nleft">
<div class="plist4-img"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=2557" target="_blank"><img src="upload/edithtml/q1.jpg" alt="IT评论圈" width="93" height="74" /></a></div>
<div class="plist4-zi"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=2557"  target="_blank">IT评论圈</a></div>
</div>
<div class="plist4 nleft">
<div class="plist4-img"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=2800" target="_blank"><img src="upload/edithtml/q2.jpg" alt="技术学堂" width="93" height="74" /></a></div>
<div class="plist4-zi"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=2800"  target="_blank">技术学堂</a></div>
</div>
<div class="plist4 nleft">
<div class="plist4-img"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=2788" target="_blank"><img src="upload/edithtml/q3.jpg" alt="电信沙龙" width="93" height="74" /></a></div>
<div class="plist4-zi"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=2788"  target="_blank">电信沙龙</a></div>
</div>
<div class="plist4 nleft">
<div class="plist4-img"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=411" target="_blank"><img src="upload/edithtml/gs.jpg" alt="休闲灌水" width="93" height="74" /></a></div>
<div class="plist4-zi"><a href="http://blog.ccidnet.com/blog.php?mod=group&amp;fid=411"  target="_blank">休闲灌水</a></div>
</div>        </div>
        <div class="clear"></div>
        <div class="fg2">
        <h2><span class="nleft">新人加入</span></h2>
        <div class="zxjr">
        <a href="home.php?mod=space&amp;uid=2388508"  target="_blank">白德玉</a>　
<a href="home.php?mod=space&amp;uid=2388364"  target="_blank">beiyesi</a>　
<a href="home.php?mod=space&amp;uid=260270"  target="_blank">lindom</a>　
<a href="home.php?mod=space&amp;uid=21960"  target="_blank">策划专家徐宪明</a>　
<a href="home.php?mod=space&amp;uid=975104"  target="_blank">李晓蒙</a>　
<a href="home.php?mod=space&amp;uid=984273"  target="_blank">汪浚</a>　
<a href="home.php?mod=space&amp;uid=983311"  target="_blank">城宇</a>　
<a href="home.php?mod=space&amp;uid=983029"  target="_blank">姜伯静</a>　
<a href="home.php?mod=space&amp;uid=981917"  target="_blank">田立哲</a>　
<a href="home.php?mod=space&amp;uid=975685"  target="_blank">吴纯勇</a>　
<a href="home.php?mod=space&amp;uid=975670"  target="_blank">中国网友报 秦磊</a>　
<a href="home.php?mod=space&amp;uid=975237"  target="_blank">顾文军</a>　
<a href="home.php?mod=space&amp;uid=975274"  target="_blank">陈利亚</a>　
<a href="home.php?mod=space&amp;uid=975209"  target="_blank">路不韦</a>　
<a href="home.php?mod=space&amp;uid=927455"  target="_blank">陆新之</a>　
 
        </div>
        </div>
    </div>
</div>
<div class="clear"></div>
<div class="blog-main4">
<div class="nleft blog-main4-left">
    	<div class="blog-main4-nei">
            <div class="h10-cont1">
                <h2><span class="nleft"><img src="template/default/blog/style/images/h15.jpg" width="714" height="42" border="0" usemap="#Map5" />
                    <map name="Map5" id="Map5">
                      <area shape="rect" coords="10,5,172,41" href="blog.php?mod=list&amp;type=technologyview&amp;catid=9,10,11,22" target="_blank"/>
                    </map>
                </span></h2>
                <div class="clear"></div>
                <div id="portal_block_20" class="block move-span"><div id="portal_block_20_content" class="content"><ul class=''><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=10">开发</a>]<a href="home.php?mod=space&uid=40080&do=blog&id=20649620"   title="jquery操作checkbox（"  target="_blank">jquery操作checkbox（</a></span><span class="nright"  title="[敞篷帅哥]"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=40080&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">[敞篷帅哥]</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=10">开发</a>]<a href="home.php?mod=space&uid=40080&do=blog&id=20800102"   title="东方时尚约车技巧"  target="_blank">东方时尚约车技巧</a></span><span class="nright"  title="[敞篷帅哥]的赛迪博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=40080&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">[敞篷帅</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=22">操作系统</a>]<a href="home.php?mod=space&uid=54683&do=blog&id=20807713"   title="方舟子在微博上发合成的韩寒“入狱照”违法吗？"  target="_blank">方舟子在微博上发合成</a></span><span class="nright"  title="游云庭"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=54683&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">游云庭</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=22">操作系统</a>]<a href="home.php?mod=space&uid=62265&do=blog&id=20807644"   title="云的价值在于构筑用户需求"  target="_blank">云的价值在于构筑用户</a></span><span class="nright"  title="刀马物语"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=62265&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">刀马物</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=10">开发</a>]<a href="home.php?mod=space&uid=40080&do=blog&id=20800102"   title="东方时尚约车技巧"  target="_blank">东方时尚约车技巧</a></span><span class="nright"  title="[敞篷帅哥]的赛迪博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=40080&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">[敞篷帅</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=22">操作系统</a>]<a href="home.php?mod=space&uid=324810&do=blog&id=20799647"   title="高考志愿填报小心钓鱼网站 趋势科技PC-cillin 2012保护学生网上报名无忧"  target="_blank">高考志愿填报小心钓鱼</a></span><span class="nright"  title="趋势科技-云安全博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=22">操作系统</a>]<a href="home.php?mod=space&uid=248922&do=blog&id=20799263"   title="京东家电“赶苏超美”指日可待"  target="_blank">京东家电“赶苏超美”</a></span><span class="nright"  title="于刚"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=248922&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">于刚</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=9">网络</a>]<a href="home.php?mod=space&uid=324810&do=blog&id=20796937"   title="通过电子邮件实施的APT攻击"  target="_blank">通过电子邮件实施的AP</a></span><span class="nright"  title="趋势科技-云安全博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=9">网络</a>]<a href="home.php?mod=space&uid=324810&do=blog&id=20796782"   title="警惕以伦敦奥运为借口的贼喊捉贼"  target="_blank">警惕以伦敦奥运为借口</a></span><span class="nright"  title="趋势科技-云安全博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=9">网络</a>]<a href="home.php?mod=space&uid=324810&do=blog&id=20796779"   title="警惕以伦敦奥运为借口的贼喊捉贼"  target="_blank">警惕以伦敦奥运为借口</a></span><span class="nright"  title="趋势科技-云安全博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=9">网络</a>]<a href="home.php?mod=space&uid=324810&do=blog&id=20796300"   title="暗藏木马的假冒Skype加密软件"  target="_blank">暗藏木马的假冒Skype加</a></span><span class="nright"  title="趋势科技-云安全博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=22">操作系统</a>]<a href="home.php?mod=space&uid=62265&do=blog&id=20796267"   title="SMG百视通入股风行网搅动几多池水？"  target="_blank">SMG百视通入股风行网搅</a></span><span class="nright"  title="刀马物语"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=62265&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">刀马物</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=22">操作系统</a>]<a href="home.php?mod=space&uid=60117&do=blog&id=20796134"   title="团购行业过于理想化导致进退两难"  target="_blank">团购行业过于理想化导</a></span><span class="nright"  title="于斌"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=60117&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">于斌</a></span></li><li><span class="nleft c1">[<a href="blog.php?mod=list&type=&catid=9">网络</a>]<a href="home.php?mod=space&uid=324810&do=blog&id=20794845"   title="信息安全中，人是最薄弱的环节"  target="_blank">信息安全中，人是最薄</a></span><span class="nright"  title="趋势科技-云安全博客"><a href="http://blog.ccidnet.com/home.php?mod=space&uid=324810&view=admin" onmouseover="this.style.color='#f54f03'"  onmouseout="this.style.color='#4e4e4e'" style="color:#4e4e4e;text-decoration:none;"  target="_blank">趋势科</a></span></li></ul>
</div></div>          </div>
            <div class="h6-bottom"><img src="template/default/blog/style/images/h6-bg.jpg" width="714" height="5" /></div>
        </div>
        
        <div class="blog-main4-nei margin1">
          <div class="h11-cont1">
                <h2><span class="nleft"><img src="template/default/blog/style/images/h11.jpg" width="714" height="42" border="0" usemap="#Map6" />
                    <map name="Map6" id="Map6">
                      <area shape="rect" coords="8,6,135,37" href="blog.php?mod=business" target="_blank"/>
                    </map>
                </span></h2>
                <div class="clear"></div>
              <ul>    
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967254&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293616936.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=404167&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617004.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967260&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617038.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967306&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617401.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=965760&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617128.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=324810&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617150.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967317&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617227.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967323&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617240.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967326&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617254.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967328&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617264.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967331&amp;view=admin 
"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617281.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967336&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617297.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967339&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617311.jpg" width="88" height="31" /></a></li>
                      <li><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=967342&amp;view=admin"  target="_blank"><img src="http://image.ccidnet.com/imagedir/ccid/1293617327.jpg" width="88" height="31" /></a></li>
              </ul>
            </div>
            <div class="h6-bottom"><img src="template/default/blog/style/images/h6-bg.jpg" width="714" height="5" /></div>
        </div>
        
        <div class="blog-main4-nei margin1">
      <div class="h12-cont1">
                <h2><span class="nleft"><img src="template/default/blog/style/images/h12.jpg" width="714" height="42" border="0" usemap="#Map7" />
                    <map name="Map7" id="Map7">
                      <area shape="rect" coords="9,6,133,35" href="#" />
                    </map>
                </span></h2>
                <div class="clear"></div>
            <ul>
               	<li><span><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1834779&amp;fromuid=2329746" target="_blank"><img src="upload/edithtml/wy.jpg" width="148" height="102" /></a></span>
  <h4><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1834779&amp;fromuid=2329746"  target="_blank">丁磊高薪养马当员工</a></h4></li>
                <li><span><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1840047&amp;fromuid=2329746" target="_blank"><img src="upload/edithtml/mozhi.jpg" width="148" height="102" /></a></span>
  <h4><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1840047&amp;fromuid=2329746"  target="_blank">墨汁除了写字还可制作粉条</a></h4></li>
                <li><span><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1840066&amp;fromuid=2329746" target="_blank"><img src="upload/edithtml/gz.jpg" width="148" height="102" /></a></span>
  <h4><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1840066&amp;fromuid=2329746"  target="_blank">广州初中生刀捅同学</a></h4></li>
                <li><span><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1840034&amp;fromuid=2329746" target="_blank"><img src="upload/edithtml/jn.jpg" width="148" height="102" /></a></span>
  <h4><a href="http://bbs.ccidnet.com/forum.php?mod=viewthread&amp;tid=1840034&amp;fromuid=2329746"  target="_blank">胶囊类药沾了水</a></h4></li>
            </ul>        </div>
            <div class="h6-bottom"><img src="template/default/blog/style/images/h6-bg.jpg" width="714" height="5" /></div>
        </div>
    </div>
    <div class="nright blog-main7-right">
    	<div class="fg3">
   		<h2><span class="nleft">博文排行榜</span></h2>
        <div class="clear"></div>
        <div class="bkph">
            <dt>
                <div id="portal_block_14" class="block move-span"><div id="portal_block_14_content" class="content"><dl>
<dt class="phfont phw1_1">1</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=821&do=blog&id=129294" title="538042" style="color: #0A6C9B;"  target="_blank">关于粤传媒的几个问题</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">2</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=3276&do=blog&id=159234" title="535519" style="color: #0A6C9B;"  target="_blank">关于直通车用户的几点</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">3</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=68879&do=blog&id=226221" title="533271" style="color: #0A6C9B;"  target="_blank">周新宁：重申对CN域名</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">4</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=18853&do=blog&id=52231" title="445359" style="color: #0A6C9B;"  target="_blank">泪告白</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">5</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=60117&do=blog&id=20618292" title="426756" style="color: #0A6C9B;"  target="_blank">诺基亚的发展仍然存在</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">6</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=64063&do=blog&id=397872" title="375959" style="color: #0A6C9B;"  target="_blank">为什么我们错过了盖茨</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">7</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=59435&do=blog&id=20618519" title="362070" style="color: #0A6C9B;"  target="_blank">云通信带动国际电话会</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">8</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=22055&do=blog&id=63507" title="361278" style="color: #0A6C9B;"  target="_blank">往期博客周刊</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">9</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=299771&do=blog&id=20619986" title="357724" style="color: #0A6C9B;"  target="_blank">阿里巴巴推手机志在支</a></dt>
 </dl><dl>
<dt class="phfont phw1_1">10</dt><dt class="phw2_2"><a href="home.php?mod=space&uid=43439&do=blog&id=629134" title="346166" style="color: #0A6C9B;"  target="_blank">Struts 2核心技术与Ja</a></dt>
 </dl></div></div>               
            </dt>
        </div>
        </div>
        <div class="clear"></div>
        <div class="fg4">
        <h2><span class="nleft">博主排行榜</span></h2>
        <div class="clear"></div>
        <ul class="hp-bq">
        	<li class="hp-bq-x1" id="bk1"><a href=""   class="hp-bq-hover">博客人气排行</a></li>
            <li id="bk2"><a href=""  class="">博客发帖排行</a></li>
        </ul>
        <div class="clear"></div>
        <div class="bkph1">
            <dt><div id="portal_block_16" class="block move-span"><div id="portal_block_16_content" class="content"><dl>
<dt class="phfont phw1">1</dt><dt class="phw2"><a href="home.php?mod=space&uid=40429" style="color: #0A6C9B;" title="刘兴亮"  target="_blank">刘兴亮</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=40429" style="color: #0A6C9B;" target="_blank">2576159</a></dt>
</dl><dl>
<dt class="phfont phw1">2</dt><dt class="phw2"><a href="home.php?mod=space&uid=39509" style="color: #0A6C9B;" title="项有建的科技博客--《冲出数字化》全面上架"  target="_blank">项有建的科技博客</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=39509" style="color: #0A6C9B;" target="_blank">2368059</a></dt>
</dl><dl>
<dt class="phfont phw1">3</dt><dt class="phw2"><a href="home.php?mod=space&uid=44460" style="color: #0A6C9B;" title="方圆e家"  target="_blank">方圆e家</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=44460" style="color: #0A6C9B;" target="_blank">2223784</a></dt>
</dl><dl>
<dt class="phfont phw1">4</dt><dt class="phw2"><a href="home.php?mod=space&uid=7390" style="color: #0A6C9B;" title="惶者生存之潜龙腾渊"  target="_blank">惶者生存之潜龙腾</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=7390" style="color: #0A6C9B;" target="_blank">2222558</a></dt>
</dl><dl>
<dt class="phfont phw1">5</dt><dt class="phw2"><a href="home.php?mod=space&uid=36931" style="color: #0A6C9B;" title="利纳克斯"  target="_blank">利纳克斯</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=36931" style="color: #0A6C9B;" target="_blank">2124882</a></dt>
</dl></div></div>            </dt>
        </div>
               <div class="bkph2">
            <dt><div id="portal_block_17" class="block move-span"><div id="portal_block_17_content" class="content"><dl>
<dt class="phfont phw1">1</dt><dt class="phw2"><a href="home.php?mod=space&uid=3388" style="color: #0A6C9B;" title="激情IT，浪子之家"  target="_blank">激情IT，浪子之家</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=3388" style="color: #0A6C9B;" target="_blank">1823</a></dt>
</dl><dl>
<dt class="phfont phw1">2</dt><dt class="phw2"><a href="home.php?mod=space&uid=44460" style="color: #0A6C9B;" title="方圆e家"  target="_blank">方圆e家</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=44460" style="color: #0A6C9B;" target="_blank">993</a></dt>
</dl><dl>
<dt class="phfont phw1">3</dt><dt class="phw2"><a href="home.php?mod=space&uid=244311" style="color: #0A6C9B;" title="康斯坦丁"  target="_blank">康斯坦丁</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=244311" style="color: #0A6C9B;" target="_blank">987</a></dt>
</dl><dl>
<dt class="phfont phw1">4</dt><dt class="phw2"><a href="home.php?mod=space&uid=57180" style="color: #0A6C9B;" title="IT爱好者的创作坊"  target="_blank">IT爱好者的创作坊</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=57180" style="color: #0A6C9B;" target="_blank">925</a></dt>
</dl><dl>
<dt class="phfont phw1">5</dt><dt class="phw2"><a href="home.php?mod=space&uid=12997" style="color: #0A6C9B;" title="靳生玺"  target="_blank">靳生玺</a></dt><dt class="phw3"><a href="home.php?mod=space&uid=12997" style="color: #0A6C9B;" target="_blank">946</a></dt>
</dl></div></div>            </dt>
        </div>
        </div>
        <div class="clear"></div>
        <div class="fg5">
        <h2><span class="nleft">博客帮助</span></h2>
        <div class="bkbz">
            <span>编辑：柠檬</span>
                        <br/>
           <span>Mail：lixue@staff.ccidnet.com</span>
        </div>        </div>
    </div>
</div>
<div class="clear"></div>
<div class="mrbk">
<div class="mrbk-top"><img src="template/default/blog/style/images/mrbk-di.jpg" width="950" height="36" /></div>
    <div class="mrbk-cont1">
        	<h2>总裁</h2>                         
        <p><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=490861&amp;view=admin"  target="_blank"> 李树翀</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=276520&amp;view=admin"  target="_blank">张&nbsp;&nbsp;鹤</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=282288&amp;view=admin"  target="_blank">孙淑园</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=313932&amp;view=admin"  target="_blank">李善友</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=355762&amp;view=admin"  target="_blank">古永锵</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=916107&amp;view=admin"  target="_blank">宋中杰</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=9507&amp;view=admin"  target="_blank">曹&nbsp;&nbsp;健</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=310868&amp;view=admin"  target="_blank">张向东</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=5759&amp;view=admin"  target="_blank">项立刚</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=923362&amp;view=admin"  target="_blank">罗清启</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=310683&amp;view=admin"  target="_blank">黄&nbsp;&nbsp;华</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=285248&amp;view=admin"  target="_blank">宋石男</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=274307&amp;view=admin"  target="_blank">卢轶男</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=84336&amp;view=admin"  target="_blank">胡才勇</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=947250&amp;view=admin"  target="_blank">张贯京</a><br><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=72940&amp;view=admin"  target="_blank">谭晨辉</a></p> 
        <div class="clear"></div> 


        <h2>互联网</h2>    

        <p><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=62265&amp;view=admin"  target="_blank">小刀马</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=681050&amp;view=admin"  target="_blank">姜洪军</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=927455&amp;view=admin"  target="_blank">陆新之</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=88995&amp;view=admin"  target="_blank">贾敬华</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=268582&amp;view=admin"  target="_blank">李思睿</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=50707&amp;view=admin"  target="_blank">瞬&nbsp;&nbsp;雨</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=46358&amp;view=admin"  target="_blank">赵玉勇</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=67711&amp;view=admin"  target="_blank">米晓彬</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60117&amp;view=admin"  target="_blank">于&nbsp;&nbsp;斌</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=14389&amp;view=admin"  target="_blank">方佳平</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=277976&amp;view=admin"  target="_blank">秦&nbsp;&nbsp;尘</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=285973&amp;view=admin"  target="_blank">谢&nbsp;&nbsp;青</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=308618&amp;view=admin"  target="_blank">安明明</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=95516&amp;view=admin"  target="_blank">赵福军</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=968908&amp;view=admin"  target="_blank">何&nbsp;&nbsp;玺</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=49440&amp;view=admin"  target="_blank">王&nbsp;&nbsp;斌</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=285656&amp;view=admin"  target="_blank">方礼勇</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=39509&amp;view=admin"  target="_blank">项有建</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=11711&amp;view=admin"  target="_blank">张书乐</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=349850&amp;view=admin"  target="_blank">朱&nbsp;&nbsp;帅</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=57180&amp;view=admin"  target="_blank">王易见</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=961617&amp;view=admin"  target="_blank">刘晶晶</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=295790&amp;view=admin"  target="_blank">朱&nbsp;&nbsp;翊</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=244311&amp;view=admin"  target="_blank">王&nbsp;&nbsp;鲲</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60630&amp;view=admin"  target="_blank">罗会祥</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=12997&amp;view=admin"  target="_blank">谨生玺</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60019&amp;view=admin"  target="_blank">金错刀</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=54269&amp;view=admin"  target="_blank">杨宇良</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=102306&amp;view=admin"  target="_blank">孙永杰</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=242481&amp;view=admin"  target="_blank">于&nbsp;&nbsp;明</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=50115&amp;view=admin"  target="_blank">张&nbsp;&nbsp;毅</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=58468&amp;view=admin"  target="_blank">李忠存</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=275047&amp;view=admin"  target="_blank">帅智军</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=321037&amp;view=admin"  target="_blank">董&nbsp;&nbsp;柱</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=923264&amp;view=admin"  target="_blank">曾会明</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=92312&amp;view=admin"  target="_blank">赵&nbsp;&nbsp;勇</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60630&amp;view=admin"  target="_blank">罗会祥</a><a href=" http://zhenghuafeng.blog.ccidnet.com"  target="_blank">郑化锋</a><a href="http://blog.ccidnet.com/?983029"  target="_blank">姜伯静</a><a href=" http://blog.ccidnet.com/?983311"  target="_blank">城&nbsp;&nbsp;宇</a><a href="http://blog.ccidnet.com/space-uid-1015152.html"  target="_blank">杨世界</a></p>

        
  <h2>通信</h2> 
        <p><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=365925&amp;view=admin"  target="_blank">梁宇亮</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=6062&amp;view=admin"  target="_blank">辛鹏骏</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=59435&amp;view=admin"  target="_blank">毛启盈</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=269701&amp;view=admin"  target="_blank">马继华</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=110023&amp;view=admin"  target="_blank">李瀛寰</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=83334&amp;view=admin"  target="_blank">计育青</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=604067&amp;view=admin"  target="_blank">饶品同</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=609552&amp;view=admin"  target="_blank">程德杰</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=299771&amp;view=admin"  target="_blank">曾高飞</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=269783&amp;view=admin"  target="_blank">周双阳</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=356078&amp;view=admin"  target="_blank">吴茂林</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=610728&amp;view=admin"  target="_blank">崔明轩</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=91966&amp;view=admin"  target="_blank">袁茂峰</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=299785&amp;view=admin"  target="_blank">李于兰</a><a href="http://blog.ccidnet.com/blog-htm-uid-501.html"  target="_blank">付&nbsp;&nbsp;亮</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=64063&amp;view=admin"  target="_blank">老&nbsp;&nbsp;杳</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=389160&amp;view=admin"  target="_blank">陈志刚</a><a href="http://chengjs.xiaoli.com.smt.jin.xin.blog.ccidnet.com/blog-htm-uid-134206.html"  target="_blank">程京生</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=947258&amp;view=admin"  target="_blank">陈威中</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=389020&amp;view=admin"  target="_blank">康国平</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=70224&amp;view=admin"  target="_blank">杨贤忠</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=784864&amp;view=admin"  target="_blank">梁振鹏</a><a href="http://blog.ccidnet.com/blog-htm-uid-5329.html"  target="_blank">曾剑秋</a><a href="http://blog.ccidnet.com/blog-htm-uid-3712.html"  target="_blank">毕朝晖</a></p>

    <h2>产经</h2> 
        <p><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=55361&amp;view=admin"  target="_blank">马红兵</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=57934&amp;view=admin"  target="_blank">鲁学军</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=10803&amp;view=admin"  target="_blank">王洪波</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=55331&amp;view=admin"  target="_blank">龚炳铮</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=55960&amp;view=admin"  target="_blank">王希跃</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=44828&amp;view=admin"  target="_blank">胥&nbsp;&nbsp;军</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=33694&amp;view=admin"  target="_blank">宋关福</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=55375&amp;view=admin"  target="_blank">孙洪林</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=44460&amp;view=admin"  target="_blank">程&nbsp;&nbsp;伟</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=55367&amp;view=admin"  target="_blank">潘成文</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=48794&amp;view=admin"  target="_blank">许延才</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=38221&amp;view=admin"  target="_blank">马伟国</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=3642&amp;view=admin"  target="_blank">高&nbsp;&nbsp;峰</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=21100&amp;view=admin"  target="_blank">王卫乡</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=43361&amp;view=admin"  target="_blank">王仰富</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=758583&amp;view=admin"  target="_blank">盛&nbsp;&nbsp;洪</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=18764&amp;view=admin"  target="_blank">余立斌</a><a href="http://blog.ccidnet.com/blog-htm-uid-825979.html"  target="_blank">赵文银</a><a href="http://blog.ccidnet.com/blog-htm-uid-754031.html"  target="_blank">陈季冰</a><a href="http://blog.ccidnet.com/blog-htm-uid-749709.html"  target="_blank">周庭锐</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=892726&amp;view=admin"  target="_blank">张红樱</a><a href="http://blog.ccidnet.com/blog-htm-uid-813601.html"  target="_blank">张志勇</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=758568&amp;view=admin"  target="_blank">茅于轼</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=758532&amp;view=admin"  target="_blank">段绍译</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=758554&amp;view=admin"  target="_blank">张曙光</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=857629&amp;view=admin"  target="_blank">林&nbsp;&nbsp;君</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=813682&amp;view=admin"  target="_blank">余德进</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=37806&amp;view=admin"  target="_blank">谢元泰</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=3444&amp;view=admin"  target="_blank">朱鹏举</a><a href="http://blog.ccidnet.com/blog-htm-uid-819800.html"  target="_blank">易&nbsp;&nbsp;鹏</a><br><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=53507&amp;view=admin"  target="_blank">徐祖哲</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=34408&amp;view=admin"  target="_blank">吴文钊</a><a href="http://blog.ccidnet.com/blog-htm-uid-699458.html"  target="_blank">夏&nbsp;&nbsp;斌</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=48787&amp;view=admin"  target="_blank">老&nbsp;&nbsp;柯</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=42967&amp;view=admin"  target="_blank">老&nbsp;&nbsp;邓</a></p>


   <h2>技术</h2>
        <p><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=40080&amp;view=admin"  target="_blank">李晓波</a><a href="http://lylsz.blog.ccidnet.com/blog-htm-uid-348450.html"  target="_blank">李英林</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=43623&amp;view=admin"  target="_blank">王&nbsp;&nbsp;勇</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=43439&amp;view=admin"  target="_blank">范立锋</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=923014&amp;view=admin"  target="_blank">郝宪玮</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=43492&amp;view=admin"  target="_blank">马千里</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=35280&amp;view=admin"  target="_blank">胡&nbsp;&nbsp;焦</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=10104&amp;view=admin"  target="_blank">顽&nbsp;&nbsp;童</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=6587&amp;view=admin"  target="_blank">张敬波</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=37839&amp;view=admin"  target="_blank">蒙圣光</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=62538&amp;view=admin"  target="_blank">张&nbsp;&nbsp;平</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=36931&amp;view=admin"  target="_blank">高铁柱</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=58605&amp;view=admin"  target="_blank">张庆华</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=3587&amp;view=admin"  target="_blank">田&nbsp;&nbsp;逸</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=45279&amp;view=admin"  target="_blank">石榴亭</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=60604&amp;view=admin"  target="_blank">胡&nbsp;&nbsp;静</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=62827&amp;view=admin"  target="_blank">刘&nbsp;&nbsp;雷</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=13174&amp;view=admin"  target="_blank">张宝林</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=20593&amp;view=admin"  target="_blank">大白鲨</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=34092&amp;view=admin"  target="_blank">老&nbsp;&nbsp;丁</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=61684&amp;view=admin"  target="_blank">黄相民</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=6980&amp;view=admin"  target="_blank">疯笑笑</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=12856&amp;view=admin"  target="_blank">杨延成</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=37742&amp;view=admin"  target="_blank">任甲林</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=12026&amp;view=admin"  target="_blank">赵文明</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=20630&amp;view=admin"  target="_blank">张新杰</a><a href="http://zhangxinzhou.blog.ccidnet.com/blog-htm-uid-36421.html"  target="_blank">张新周</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=5320&amp;view=admin"  target="_blank">迎&nbsp;&nbsp;风</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=38234&amp;view=admin"  target="_blank">张&nbsp;&nbsp;扬</a><a href="http://blog.ccidnet.com/home.php?mod=space&amp;uid=689&amp;view=admin"  target="_blank">风飞扬</a></p>

    </div>
    <div class="mrbk-bottom"><img src="template/default/blog/style/images/mrbk-bottom.jpg" width="950" height="5" /></div>
</div><style>
.table1 a{ font-size:12px; color:#000; line-height:23px;}
</style>
<table width="950" border="0" cellspacing="0" cellpadding="0" style="margin-top:10px; border:1px #e7e7e7 solid;">
  <tr>
    <td height="32" align="center" valign="bottom" style="border-left:1px solid #E4E4E4;border-right:1px solid #E4E4E4;border-bottom:1px #e7e7e7 solid;"><table width="17%" border="0" cellspacing="0" cellpadding="0">
      <tr>
        <td width="20%"></td>
        <td width="80%" height="28" align="left"><span class="a14"><strong>合作站点</strong></span></td>
      </tr>
    </table></td>
  </tr>
  <tr>
    <td bgcolor="F4F4F4" style="border-right:1px solid #E7E7E7;border-left:1px solid #E7E7E7;border-bottom:1px solid #E7E7E7;"><table width="100%" border="0" cellspacing="3" cellpadding="0">
      <tr>
        <td align="center" bgcolor="#FFFFFF" style="padding:7px;"><table width="98%" border="0" cellspacing="0" cellpadding="0" >
              <tr> 
                <td align="center" style="border-bottom:1px #999 dotted"><table width="100%" border="0" cellspacing="0" cellpadding="0" class="table1">
                    <tr> 
                      <td width="2%" align="center" valign="top" style="padding-top:7px;"></td>
                      <td width="98%" align="left"> <a href="http://blog.tom.com/" target="_blank" class="a9c_1"  style="color:#42495B">TOM博客</a> 
                        <a href="http://www.bokee.com/" target="_blank" class="a9c_1" style="color:#42495B">博客网</a> 
                        <a href="http://blog.hexun.com/" target="_blank" class="a9c_1" style="color:#42495B">和讯博客</a> 
                        <a href="http://blog.tianya.cn/ " target="_blank" class="a9c_1" style="color:#42495B">天涯博客</a> 
                        <a href="http://blog.techweb.com.cn/index.html" target="_blank" class="a9c_1" style="color:#42495B">TechWeb博客</a> 
                        <a href="http://blog.mop.com/" target="_blank" class="a9c_1" style="color:#42495B">猫扑博客</a> 
                        <a href="http://blog.china.com/" target="_blank" class="a9c_1" style="color:#42495B">中华网博客</a> 
                        <a href="http://blog.soufun.com/" target="_blank" class="a9c_1" style="color:#42495B">搜房博客</a> 
                        <a href="http://blog.yesky.com/index.html" target="_blank" class="a9c_1" style="color:#42495B">天极博客</a> 
                        <a href="http://blog.people.com.cn/blog/main.jspe" target="_blank" class="a9c_1" style="color:#42495B">强国博客</a> 
                        <a href="http://blog.csdn.net/default.html" target="_blank" class="a9c_1" style="color:#42495B">CSDN博客</a> 
                        <a href="http://www.blogchinese.com/" target="_blank"class="a9c_1" style="color:#42495B">博客中国人</a> 
                        <a href="http://blog.focus.cn/" target="_blank" class="a9c_1" style="color:#42495B">焦点博客</a> 
                        <a href="http://blog.scol.com.cn/" target="_blank" class="a9c_1" style="color:#42495B">天府博客 
                        </a> <a href="http://blog.chinaitlab.com/index.htm" target="_blank" class="a9c_1" style="color:#42495B">爱踢博客</a> 
                        <a href="http://blog.icxo.com/" target="_blank" class="a9c_1" style="color:#42495B">世界经理人博客</a> 
                        <a href="http://www.kejixun.com/" target="_blank" class="a9c_1" style="color:#42495B">科技讯</a> 
                      </td>
                    </tr>
                  </table></td>
              </tr>
              <tr> 
                <td align="center"><table width="99%" border="0" cellspacing="0" cellpadding="0">
                    <tr> 
                      <td height="1" background="http://blog.ccidnet.com/image/index/line2.gif"></td>
                    </tr>
                  </table></td>
              </tr>
              <tr> 
                <td style="border-bottom:1px #999 dotted;"><table width="100%" border="0" cellspacing="0" cellpadding="0" style="margin-top:5px;margin-bottom:5px;">
                    <tr> 
                      <td width="2%" align="center" valign="top" style="padding-top:5px;"></td>
                      <td width="98%" align="left"> <a href="http://blog.51cto.com " target="_blank" class="a9c_1">51CTO技术博客 
                        </a> <a href="http://blog.cnfol.com/" target="_blank" class="a9c_1">中金博客 
                        </a> <a href="http://blog.csai.cn" target="_blank" class="a9c_1">希赛博客</a> 
                        <a href="http://bbs.kooxoo.com/" target="_blank" class="a9c_1">酷讯社区 
                        </a> <a href="http://www.jsdo.org/" target="_blank" class="a9c_1">healthbook 
                        </a> <a href="http://www.zzchn.com" target="_blank" class="a9c_1">站长中国 
                        </a><a href="http://www.joohe.com/" target="_blank" class="a9c_1">聚合网
</a> 
                      <a href="http://blog.edu.cn/" target="_blank" class="a9c_1">中国教育人博客                      </a> <a href="http://www.yacol.com/" target="_blank"class="a9c_1">雅酷时空
                        </a><a href="http://www.52huodong.com/" target="_blank" class="a9c_1"> 我爱活动网
                   </a> <a href="http://www.boke.jqw.com/" target="_blank"class="a9c_1">金泉网商人博客                    </a> <a href="http://www.yoyv.com/" target="_blank"class="a9c_1">游鱼旅游博客
                      </a>  <a href="http://www.9ask.cn/" target="_blank" class="a9c_1">免费法律咨询</a> <a href="http://www.zhanpeng.org " target="_blank" class="a9c_1">网络营销博客</a>   </td>
                    </tr>
                  </table></td>
              </tr>
              <tr> 
                <td align="center"><table width="99%" border="0" cellspacing="0" cellpadding="0">
                    <tr> 
                      <td height="1" background="http://blog.ccidnet.com/image/index/line2.gif"></td>
                    </tr>
                  </table></td>
              </tr>
              <tr> 
                <td><table width="100%" border="0" cellspacing="0" cellpadding="0" style="margin-top:5px;">
                    <tr> 
                      <td width="2%" align="center" style="padding-top:5px;"></td>
                      <td width="98%" align="left" valign="top"> <a href="http://blog.cnmo.com/" target="_blank" class="a9c_1">科技博客</a>
 <a href="http://www.51kang.com/" target="_blank" class="a9c_1">无忧健康网</a>
<a href="http://www.quanke.net/" target="_blank" class="a9c_1">全客网</a>
 <a href="http://www.moabc.net/" target="_blank" class="a9c_1">摩客空间</a>
 <a href="http://www.prnasia.com/index.html" target="_blank" class="a9c_1">美通社（亚洲）</a>
 <a href="http://bj.snifast.com/news/" target="_blank" class="a9c_1">北京狙房地产</a>
<a href="http://blog.chinabyte.com/" target="_blank" class="a9c_1">比特博客</a> <a href="http://www.gamecheng.com/" target="_blank" class="a9c_1">Game城</a> <a href="http://name.renren.com " target="_blank" class="a9c_1">人人网姓名</a> <a href="http://blog.xmnn.cn/" target="_blank" class="a9c_1">厦门网海峡博客</a> <a href="http://column.iresearch.cn/" target="_blank" class="a9c_1">艾瑞专栏</a> <a href="http://www.78che.com/" target="_blank" class="a9c_1">汽博之家</a>
</td>
                             

                    </tr>
                  </table></td>
              </tr>
              <tr> 
                <td align="center"><table width="99%" border="0" cellspacing="0" cellpadding="0">
                    <tr> 
                      <td height="1" background="http://blog.ccidnet.com/image/index/line2.gif"></td>
                    </tr>
                  </table></td>
              </tr>
              <tr> 
                <td align="center"><table width="99%" border="0" cellspacing="0" cellpadding="0">
                    <tr> 
                      <td height="1" background="http://blog.ccidnet.com/image/index/line2.gif"></td>
                    </tr>
                  </table></td>
              </tr>
            </table>

          <table width="98%" border="0" cellspacing="0" cellpadding="0" style="margin-top:5px;">
            <tr>
              <td><div align="right"><a href="http://www.ccidnet.com/homepage/logo/blog.htm" target="_blank" class="color-o">友情链接 
                申请链接</a></div></td>
            </tr>
          </table></td>
      </tr>
    </table></td>
  </tr>
</table><!-- ccidnet -->
<script language="JavaScript" type="text/javascript">
 var s_domain = location.host;
 if(s_domain == "bbs.ccidnet.com" || s_domain == "bbs.tech.ccidnet.com"){
  var _PCSWebSite="100020";
 }else{
  var _PCSWebSite="100001";
 }
 var _PCSImage="countlogo1.gif";
</script>
<script src="http://stat.ccidnet.com/count/count.js" type="text/javascript" ></script>
<!-- ccidnet -->
<script src="http://image.ccidnet.com/nav/CCID_JS/ccidnet_art_footer.js" type="text/javascript"></script>
</body>
</html>