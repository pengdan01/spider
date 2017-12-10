#include "log_analysis/common/url_util.h"

#include "base/testing/gtest.h"
#include "base/common/base.h"

TEST(UrlUtil, WorthlessUrlForSE) {
  struct Urls {
    const char* url;
    bool ret;
  } cases[] = {
    {"http://www.baidu.com", true},
    {"http://www.baidu.com/", true},
    {"http://www.baidu.com/man_made_dir/index.htm", true},
    {"http://www.outwx.com/", true},
    {"http://v.360.cn/dianying/list.php", true},

    {"http://www.baidu.com/?xx", false},  // 手工构造的 url
    {"http://www.baidu.com/xx?", false},
    {"http://www.baidu.com/#xx", false},
    {"http://www.baidu.com/s?wd=hello+world&rsv_bp=0&rsv_spt=3&inputT=1903", false},
    {"http://www.baidu.com/baidu.php?url=1nbK00a9xmNQ8rSC1gHU9zWrjbHf7lZkn0huE2znakkpx9deq1R3t7Fqk-NQCWmlCKv5Oi8CRPK8uJ7W1vpraUWxLII_cDCBf3K4PCyXaSFTLEEECvnL11YFCzNJ.7b_jCX17N6uG6S9S5Le3ArMxgbRojPakvNdtX-f.U1Yk0ZDq1tJeJ0KY5TO28l60pyYqnWcd0ZTq0ATqmhRYn0KdpHdBmy-bIfKspyfqnfKWpyfqn0KVIjYk0AVG5H00TMfqn1Dz0AN3TjY0uy-b5HDzrj-x", false},  // NOLINT
    {"http://shop58407719.taobao.com/search.htm?orderType=_hotsell&pageNum=2&scid=404786296&search=y&viewType=grid", false},  // NOLINT
    {"http://ylcwyp.tmall.com/?checkedRange=true&queryType=cat&scid=326102482&scname=0duyv8flveC7pMDt&search=y", false},  // NOLINT
    {"http://www.google.com.hk/#hl=en&newwindow=1&safe=active&site=&source=hp&q=%E4%BD%A0%E5%A5%BD&oq=%E4%BD%A0%E5%A5%BD&aq=f&aqi=&aql=&gs_l=hp.3...4091974.4095981.0.4096208.8.8.0.0.0.0.0.0..0.0...0.0.8FcCPwjGZUQ&bav=on.2,or.r_gc.r_pw.,cf.osb&fp=47a5bff63c5f11f8&biw=1920&bih=1096", false},  // NOLINT
    {"http://dl_dir.qq.com/qqfile/qd/QQ2012Beta1_QQProtect2.6.1.exe", false},
    {"http://dl_dir.qq.com/qqfile/qd/QQ2012Beta1_QQProtect2.6.1.exe..", true},
    {"http://cache.baidu.com/c?m=9f65cb4a8c8507ed4fece763105392230e54f76238d5805234c3933fc239045c5323befb712d0774a4d20f6c16ae394b9af4210147&p=8b2a9405cc934ea458bbc660474d&user=baidu&fm=sc&query=%BE%A9%B6%AB&qid=d80d981a2c55cb06&p1=1", false},  // NOLINT
    {"http://search.taobao.com/search?initiative_id=staobaoz_20120512&q=%B2%CF%B1%A6%B1%A6+%C6%B7%D6%D6&sort=sale-desc", false},  // NOLINT
    {"https://login.taobao.com/member/login.jhtml?spm=1.1000386.0.2&f=top&redirectURL=http%3A%2F%2Fwww.taobao.com%2F", false},  // NOLINT
    {"http://0077.taobao.com/?search=y&scid=188864266&scname=v%2BPX0w%3D%3D&checkedRange=true&queryType=cat", false},  // NOLINT
    {"http://360.70e.com/code/100top.asp?u__26754032___w__10000417___t__6___i__http%3A//www.qvod78.com/___o__null", false},  // NOLINT
    {"http://37.artemisbuzz.com/AdMonitor/customerPVAction.do?method=pvCount&flag=amonthly&url=http://www.unicef.cn/pledge/201206CDALPA?utm_source=163&utm_campaign=190*180homepage_M1&utm_medium=cda", false},  // NOLINT
    {"http://3c.paipai.com/search_list.shtml?type=238647&np=32480,14&searchtype=1&PTAG=32523.29.1", false},
    {"http://list.taobao.com/search_auction.htm?q=dd&commend=all&ssid=s5-e&search_type=item&atype=&filterFineness=&rr=1&style=grid&cat=11%2C1101%2C1201%2C14%2C1512%2C20%2C50007218%2C50008090%2C50018908%2C50018930%2C50018957%2C50019393%2C50018627%2C50040831%2C50041307%2C50047310%2C50035182%2C50076292%2C50051952%2", false}, // NOLINT
    {"http://3c.yiqisoo.com/index.php?action=search&type=life&classid=13&utm_campaign=yqs_baidu_sem_0001813&bd=310&page=3", false},  // NOLINT
    {"http://trade.taobao.com/trade/itemlist/listBoughtItems.htm?action=itemlist%2FQueryAction&event_submit_do_query=1&prePageNo=5&clickMore=0&pageNum=1", false},  // NOLINT
    {"http://autohome.adsame.com/c?z=autohome&la=27&si=1&cg=7&c=472&ci=49466&or=10661&l=101622&bg=50823&b=34588&u=http://vw.faw-vw.com/index.php/component/brand/?brand_id=6", false},  // NOLINT
    {"http://car.autohome.com.cn/Baoyang/list_1_0_0_50_20.html", false},
    {"http://img.ifeng.com/tres/html/pop_page.html?http://bc.ifeng.com/main/c?db=ifeng%26bid=5908,5693,1255%26cid=1630,46,1%26sid=13031%26advid=433%26camid=1340%26show=ignore%26url=http://chexian.pingan.com/campaign/IB/mttf.jsp?WT.mc_id=Wc03-fh-03${}http://y2.ifengimg.com/mappa/2012/05/28/bb88b9989b3e88e7c6ceb7d", false},  // NOLINT
    {"http://entry.mail.163.com/coremail/fcg/ntesdoor2?lightweight=1&verifycookie=1&language=-1&style=-1", false},  // NOLINT
    {"http://adsrich.qq.com/web/a.html?oid=1329413&cid=266580&type=&resource_url=http%3A%2F%2Fadsfile.qq.com%2F201205%2F28%2Ffkcp_QB_201205285103.swf&width=750&height=500&cover=true", false},  // NOLINT
    {"http://reg.163.com/login.jsp?type=1&url=http://entry.mail.163.com/coremail/fcg/ntesdoor2?lightweight%3D1%26verifycookie%3D1%26language%3D-1%26style%3D-1", false},  // NOLINT
    {"http://d1.sina.com.cn/d1images/pb/pbv4.html?http://sina.allyes.com/main/adfclick?db=sina&bid=343571,402780,408094&cid=0,0,0&sid=405375&advid=3406&camid=65333&show=ignore&url=http://gongyi.sina.com.cn/z/dream6/index.shtml${}jpg${}http://d5.sina.com.cn/201205/25/424666_er61-750x450-3.JPG", false},  // NOLINT
    {"http://qw2.dacais.com/erzs50_6001112040.htm?ths=8423&FromWebId=74502&DomainId=74584&ds8datastr=_czNDM1_AzMjdj_EzMjM5_IzODdj_czNDM1_gzNDdj_EzNTMy_kzNDM1", false},  // NOLINT
    {"http://tj.gamebean.com/hct.php?id=A002&sid=74502&FromWebId=74502&DomainId=74584&ds8datastr=_czNDM1_AzMjdj_Ez_Mw_YzMTdj_czNDM1_gzNDdj_EzNTMy_kzNDM1", false},  // NOLINT
    {"http://xybor.r.arpg2.com/xybor.htm?FromWebId=74502&DomainId=74584&ds8datastr=_czNDM1_AzMjdj_EzMjM5_IzODdj_czNDM1_gzNDdj_EzNTMy_kzNDM1", false}, // NOLINT
    {"http://ad.doubleclick.net/click;h=v2%7C3F32%7C0%7C0%7C%2a%7Ch;256339580;0-0;0;79249430;31-1%7C1;47781282%7C47796680%7C1;;%3fhttp:/www.intel.com/cn/ultrabook?dfaid=1&crtvid=%ecid!;", false},  // NOLINT
    {"http://buy.tmall.com/error.htm?error_code=ERROR_DEFAULT&error_msg=%B6%D4%B2%BB%C6%F0%A3%AC%CF%C2%B5%A5%CA%A7%B0%DC%A3%AC%C7%EB%C4%FA%D6%D8%D0%C2%CC%E1%BD%BB%B6%A9%B5%A5", false},  // NOLINT
    {"http://www.youku.com/v_olist/c_97_a_%E9%A6%99%E6%B8%AF_s__g__r__lg__im__st__mt__d_1_et_0_fv_0_fl__fc__fe__o_7.html", false},  // NOLINT
    {"http://union.76mi.com/fmt/cpf_dc.asp?114aid=3229&114uid=137&114url=http%3A//huhuzj.r.arpg2.com/huhuzj.htm&114uIP=", false},  // NOLINT
    {"http://list.iqiyi.com/www/1/-11---------0-2012-2-1-1-1---.html", false},
    {"http://v.360.cn/dianying/list.php?cat=107", false},
    {"http://v.360.cn/dianshi/index.php?cat=105&year=all&area=all&act=all", false},
    {"http://v.360.cn/dongman/index.php?cat=103&year=&area=all&rank=rankhot", false},
    {"http://al.myrice.com/adpolestar/wayl/;ad=57fd9ae3_79e4_da16_e490_7cfc39c3e6e7;ap=d7911eae_1de3_a578_1395_624a5e6dc001;pu=flashget;/?http://port.duoyi.com/rewrite/a.py?fromid=131001035", false},  // NOLINT
    {"http://pro.letv.com/adpolestar/wayl/;ad=2575F66B_E0AB_977B_EB43_3F07E8E1F615;ap=9807026C_E3EE_2382_475C_3D47B71B03F2;pu=letv;ext_key=;/?http://wop.360buy.com/p994.html", false}, // NOLINT
    {"http://so.pps.tv/search?q=%BD%F0%CC%AB%C0%C7%B5%C4%D0%D2%B8%A3%C9%FA%BB%EE&from=1", false},
    {"http://s.vancl.com/search?k=hello", false},
    {"http://so.tv.sohu.com/list_p12_p2_p3_u5185_u5730_p4-1_p5_p6_p7_p80_p9-1_p101_p11.html", false},
    {"http://www.youdao.com/search?q=%E5%A4%A7%E8%AF%9D2+%E7%AD%94%E9%A2%98%E5%99%A8", false},
    {"http://search.360buy.com/search?area=1&page=1", false},
    {"http://www.360buy.com/products/652-653-655-0-1270-0-0-0-0-0-1-1-1.html", false},
    {"http://search.china.alibaba.com/selloffer/offer_search.htm?categoryId=&categoryStyle=&button_click=top&n=y", false},  // NOLINT
    {"http://mobile.taobao.com/list.htm?spm=872.121097.152902.7&cat=1512&pidvid=20000%3A3261618%2C28247%3B30606%3A3271031%2C10285019%2C11397753%3B", false},  // NOLINT
    {"http://category.dangdang.com/all/?att=1%3A2011&category_id=4004279", false},
    {"http://e.dangdang.com/list_98.01.36.08.htm#ref=category-0-A", false},
    {"http://list.daquan.xunlei.com/movie/list_-1_-1_-1_-1_-1_-1_-1_0_2.html", false},
    {"http://movie.xunlei.com/type,genre,year/movie,Action,2009/", false},
    {"http://movie.xunlei.com/type/tv/", true},
    {"http://list.daquan.xunlei.com/movie", true},
    {"http://yinyue.xunlei.com/list/area/1/hits/1.html", false},
    {"http://sina.allyes.com/main/adfclick?db=sina&bid=402772,463689,468974&cid=0,0,0&sid=467361&advid=13912&camid=73083&show=ignore&url=http://ad-apac.doubleclick.net/clk;255640256;79296562;u", false},  // NOLINT
    {"http://tc.whlongda.com/2012gotbsvk3.php?uid=36&uksid=niuniu176&ukuid=36&ulinkstr=MjA0fDE5NDZ8bml1bml1MTc2fDF8aHR0cDovL3d3dy45NzN3ZWIuY29tL25vdGljZS9wb3AuaHRtbA%3D%3D", false},  // NOLINT
    {"http://movie.tudou.com/albumtop/c22t25v-1z-1a-1y-1h-1s0p1.html", false},
    {"http://tv.tudou.com/albumtop/c30t5v-1z-1a-1y-1h-1s0p1.html", false},
    {"http://zy.tudou.com/albumtop/c31t53v-1z-1a-1y-1h-1s0p1.html", false},
    {"http://www.tudou.com/top/gc22c220110a-1y-1o1p4h0p1.html", false},
    {"http://stat.simba.taobao.com/rd?b=zhl36&l=http%3A%2F%2Fs8.taobao.com%2Fsearch%3Fq%3D%25CF%25C4%25D7%25B0%26ex_q%3D%26filterFineness%3D%26atype%3D%26isprepay%3D1%26promoted_service4%3D4%26olu%3Dyes%26cat%3D16%26style%3Dgrid%26tab%3Dall%26cps%3Dyes%26p4p_str%3Dlo1%253D0%2526lo2%253D0%2526nt%253D1%26mode%3D63%26initia", false},  // NOLINT
    {"http://www.meilishuo.com/search?sortby=weight&page=3", false},
    {"http://so.tv.sohu.com/list_p11_p2_u98CE_u6708_u7247_p3_p4_p5_p6_p7_p8_p9.html", false},
    {"http://game.ad1111.com/lj36/39---2900b66a773079420850021b67d7d88a?from=kb50", false},
    {"http://aclick9.wisemediakcl.com/102401.html", false},
    {"http://www.soku.com/search_video/q_%E7%9B%B4%E5%88%B0%E4%B8%96%E7%95%8C%E5%B0%BD%E5%A4%B4_orderby_1_page_4", false},  // NOLINT
    {"http://s.weibo.com/weibo/BC%2520%25E7%259B%25B4%25E5%2588%25B0%25E4%25B8%2596%25E7%2595%258C%25E5%25B0%25BD%25E5%25A4%25B4&Refer=STopic_box", false},  // NOLINT
    {"http://www.kk.com/a.do", false},
    {"http://www.kk.com/a.do?xxx", false},
    {"http://www.kk.com/a.do#xxx", false},
    {"http://www.kk.com/a.does", true},
    {"http://cgi.music.soso.com/fcgi-bin/m.q?source=1&t=0&w=%D6%B1%B5%BD%CA%C0%BD%E7%B5%C4%BE%A1%CD%B7", false},  // NOLINT
    {"http://book.douban.com/people", true},
    {"http://book.douban.com/people?d", false},
    {"http://book.douban.com/tag", true},
    {"http://xx.douban.com/dfe?fe", false}, // 手工构造的case
  };

  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    bool ret = log_analysis::util::WorthlessUrlForSE(cases[i].url);
    EXPECT_EQ(ret, cases[i].ret) << cases[i].url;
  }
}

