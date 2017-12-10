#include "crawler2/general_crawler/util/url_util.h"

#include "base/common/base.h"
#include "base/testing/gtest.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/file/file_util.h"
#include "base/encoding/url_encode.h"
#include "crawler2/general_crawler/url.pb.h"
#include "extend/regexp/re3/re3.h"

TEST(crawler_selector_util, WillFilterAccordingRulesStrict) {
  static const struct {
    const char * url;
    bool filter_not_strict;  // NOT USED
    bool filter_strict;
  } cases[] = {
    // Black Host
    {"http://ptlogin2.qq.com/qqmail?ptlang=2052&Fun=clientread&uin=835953157&k=EF63127AD989F214B69DA16AFBB8F4795F9D9ACF36EE0DCB468BA342BC41D5DF&mailid=LP0025-ekqVHEgq3A1jS2couWk4V23&httptype=0&ADUIN=835953157&ADSESSION=1332628828&ADTAG=CLIENT.QQ.4333_.0", true, true},  // NOLINT
    {"http://img03.taobaocdn.com/imgextra/i3/350639611/T2mrpnXotXXXXXXXXX_!!350639611.gif", true, true},
    {"http://cache.baidu.com/c?fm=sc&m=9f65cb4a8c8507ed4fece7631016d5651b4380146d94944e3dc3933fc239045c043ffeb025231603b9d87c6503a44b5bf7f134713c0423b190df883d87fdcd763bcd7a742613913162c468dcdc362ed656e34de8df0e97bbe74294b9a3d8c82325dd52756df0fa9c2c7403ba6be7653bf4a7e95f652b07cbe62715f84e032c&p=9e60c64ad5c51db908e2977f0f", true, true}, // NOLINT
    // Check Path && Schema
    {"http://www.sohu.com/", false, false},
    {"http://www.sohu.com#", false, false},
    {"http://www.sohu.com/", false, false},
    {"javascript:://www.sohu.com/", true, true},
    {"JavascrIpt:://www.sohu.com/", true, true},
    {"mailto://www.sohu.com/", true, true},
    // Check Path postpix
    // {"http://www.baidu.com/search/error.html", true, true},
    {"http://www.sohu.com/a.exe", true, true},
    {"http://www.sohu.com/a.zip", true, true},
    {"http://81.duote.org:8080/matschool.zip", true, true},
    {"http://81.duote.org:8080/matschool.zip  ", true, true},
    // {"http://www.baidu.com/forbiddenip/forbidden.html", true, true},
    {"http://m61.mail.qq.com/cgi-bin/download?c=ne", false, false},
    {"http://k2b-bulk.ebay.com/ws/eBayISAPI.dll?MfcISAPICommand=ListingConsole&"
      "currentpage=LCActive&pageNumber=5&searchValues=&searchField=ItemTitle&Sto"
          "reCategory=944752010&Status=All&Format=All&matchCriteria=and&srcFld_1=Tit"
          "le&searchOpDd_1=CONTAINS&srcVal_1=&lrowcount=1&srcType=0&searchSubmit=Search&goToPage=", true, true},  // NOLINT
    // Check Query
    {"http://bbs.unpcn.com/attachment.aspx?attachmentid=628036", true, true},
    {"http://rate.taobao.com/remark_buyer.jhtml?tradeID=155306048563593", true, true},
    {"http://rate.taobao.com/remark_buyer.jhtml?trade_Id=155306048563593", true, true},
    {"http://my.cn.china.cn/admin.php?op=LoginSh&dc", true, true},
    {"http://s537.hz.7.qq.com/dynasty/swf/login.jhtml?cdnUrl="
      "cdn.dl.7.qq.com&PcacheTime=1332155929", true, true},
    {"http://www.m18.com/app/AdEntrance.aspx?from=E001&targetURL="
      "http://list.m18.com/N4-N4-12-60-1-578-1-N-N-0-0.htm", true, true},
    {"http://dmtracking.alibaba.com/b.jpg?cD0yJnU9ey9zZWFyY2guY2hpbmEuYWxpYmFiYS5"
      "jb20vc2VsbG9mZmVyLy0zMjMwMzEzMkI0QkFCQ0JFRDBDMikJGRUUtMTA0NTAyNS5odG1sP3Nob"
          "3dTdHlsZT1zaG9wd2luZG93JmlzUG9wPXRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04"
          "NDk3ODYzODElN0MlNUU4JTdDMTAlN0MxNiU3QzE5JTdDNTkHlsZT1zaG9wd2luZG93JmlzUG9wP"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "XRydWUmc3BySWQ9NTAxMDMmaW1hZ2VGZWF0dXJlcz04NDk3ODYzODElN0MlNUU4JTdDMTAlN0Mx"
          "zEyJTNBJUM2JUQ1JUNEJUE4JUJGJUVFJTT9&time=1333078739", true, true},
    {"http://www.baidu.com/baidu.php?url=7TcK000OyU1aAzYst8Qjk89_34Og4xjSLM3", true, true},
    // Check Login page
    {"http://110.taobao.com/account/rebind_phone_result.htm?tag=18858031120", true, true},
    {"http://56.china.alibaba.com/order/evaluate/evaluate.htm?orderId=1938752", true, true},
    {"http://aq.qq.com/cn2/findpsw/findpsw_index?source_id=1048&aquin=1501402297", true, true},
    {"http://archive.taobao.com/auction/goods/item_detail.htm?itemID=9517587441", true, true},
    {"http://wuliu.taobao.com/user/order_detail_new.htm?trade_id=155853403047474", true, true},
    {"http://athena.china.alibaba.com/favorites/add_to_favorites.htm?conpe=CO&t_id=11401412", true, true},
    {"http://buy.taobao.com/auction/auctionlist/buying_item_list.htm?nekot=6NbX07ajtqMxMTE=1337", true, true},
    {"http://buy.tmall.com/detail/orderDetail.htm?bizOrderId=127300939445274", true, true},
    {"http://cashier.alipay.com/standard/payment/cashier.htm?bizIdentity=trad"
      "e10001&orderId=113e1e1f1023eeb3384345740587c987&outBizNo=2012032971996447", true, true},
    {"http://china.alibaba.com/member/signin.htm?Done=http%3A%2F%2Fcjs8999.cn.alibaba.com%2F", true, true},
    {"http://china.alibaba.com/offer/post/fill_product_info.htm?operator=edit&offer_id=1142438", true, true},
    {"http://cid-e0ae43a120f46963.profile.live.com/?wlexpid=C103F9D0C02C3A4CC4FAB&wlrefapp=2", true, true},
    // The following is the search resule page of Search Engine
    // Baidu
    {"http://www.baidu.com/s?wd=flower&f=12&rsp=0&oq=flowler&tn=baiduhome_pg", false, true},
    {"http://www.baidu.com/cpro.php?AncK00acDJ45v6nUu-bYsIJ-ip4dTt99xTELTsyX"
      "QxLR72_FOiJ62R1R1u1CVYAe04JV_l3IhYW0p-KSwHMGp-UjyVH2LLoB2gyv0e_1W1lCV"
          "hHuS6E-DPikN7ppe2lOLrOexlZ-G94THDPMZ5lOQqxtH5vulSEexlZ-ZPjRkgdeXjDkud9"
          "hi1fxqaynhSz1_LeqLgZwmtjLkkClqE0s_HAFQPZgwYT_rrumuCyr5u3q5m0.IgF_5y9YI", true, true},
    {"http://zhidao.baidu.com/q?ct=17&pn=0&tn=ikaslist&rn=10&word=%E4%BA%AC%E"
      "4%B8%9C%E6%89%8B%E6%9C%BA%E4%B8%8B%E5%8D%95&fr=wwwt", false, true},
    {"http://map.baidu.com/?newmap=1&ie=utf-8&s=s%26wd%3D%E6%B1%9F%E8%8B%8F%E"
      "7%9C%81%E6%BB%A8%E6%B5%B7%E7%BB%8F%E6%B5%8E%E5%BC%80%E5%8F%91%E5%8C%BA", true, true},
    {"http://map.baidu.com", false, false},
    {"http://news.baidu.com/ns?cl=2&rn=20&tn=news&word=%BA%AB%BE%E7%C3%C0%C0"
      "%F6%B5%C4%C8%D5%D7%D3", false, true},
    {"http://passport.baidu.com/?reg", true, true},
    {"http://passport.baidu.com/", false, false},
    // Google
    {"http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&q=+%E9%B2%9C"
      "%E8%8A%B1&oq=+%E9%B2%9C%E8%8A%B1&aq=f&aqi=&aql=&gs_sm=12&gs_upl=3937l12"
          "233l0l13863l16l12l0l0l0l0l0l0ll0l0&gs_l=serp.12...3937l12233l0l13864l16l"
          "12l0l0l0l0l0l0ll0l0&bav=on.2,or.r_gc.r_pw.,cf.osb&fp=d31cc13f798c362e&biw=1920&bih=1008", false, true}, // NOLINT
    {"http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&site=&q=%E8%AF%BE%E6%9C%AC"
      "&oq=%E8%AF%BE%E6%9C%AC&aq=f&aqi=&aql=&gs_sm=3&gs_upl=5543l5543l0l5729l1l1l0l0l0l0l0l0l"
          "l0l0&gs_l=hp.3...5543l5543l0l5729l1l1l0l0l0l0l0l0ll0l0&bav=on.2,or.r_gc.r_pw.,cf.osb&f"
          "p=6995e23aa793edfd&biw=1920&bih=1038", false, true},
    // Sogou
    {"http://www.sogou.com/web?query=%CF%CA%BB%A8&_asf=www.sogou.com&_ast=1330661325&w=01019900"
      "&p=40040100&sut=3595&sst0=1330661325101", false, true},
    {"http://v.sogou.com/mlist/3i1w5m2c0b0a4a_5l_0_.html", true, true},
    // Bing
    {"http://cn.bing.com/search?q=%E5%A5%A5%E8%BF%90%E4%BC%9A&qs=AS&form=QBLH&pq=aoy&sc=8-3&sp="
      "7&sk=AS6", false, true},
    // Soso
    {"http://www.soso.com/q?ie=utf-8&w=%E8%85%BE%E8%AE%AF&pid=sb.idx&ch=sb.c.idx&cid=s.ix.smb", false, true},
    {"http://www.soso.com/ie=utf-8&w=%E8%85%BE%E8%AE%AF&pid=sb.idx&ch=sb.c.idx&cid=s.ix.smb", false, false},
    {"www.soso.com/q?ie=utf-8&w=%E8%85%BE%E8%AE%AF&pid=sb.idx&ch=sb.c.idx&cid=s.ix.smb", false, true},
    {"http://wenwen.soso.com/z/Search.e?sp=S%E4%BC%9A%E8%AE%A1%E6%80%8E%E6%A0%B7%E5%81%9A%E"
      "5%A5%BD%E8%AE%B0%E8%B4%A6%E5%87%AD%E8%AF%81&sp=0&ch=search.lishi", false, true},
    {"http://map.soso.com/?ie=utf-8&mp;pid=sobar.map&mp;w=", true, true},
    {"http://map.soso.com/", false, false},
    // Yahoo
    {"http://search.yahoo.com/search;_ylt=A0oGdWytSFBPu1kA4AtXNyoA;_ylc=X1MDMjc2NjY3OQRfcgMyBGF"
      "vA2FvBGNzcmNwdmlkA3BTT3A1MG9HZFRBcGhONjlUcWZBYkFrallCRnRYVTlRU09JQUROaWcEZnIDeWZwLXQtNzAx"
          "BGZyMgNzYnRuBG5fZ3BzAzEwBG9yaWdpbgNzcnAEcHFzdHIDCBHF1ZXJ5A6gRzYW8DMQR2dGVzdGlkA1lTNzA-?p="
          "%E6%83%85%E4%BA%BA%E8%8A%82&fr2=s", false, true},
    // Taobao
    {"http://s.taobao.com/search?q=%CE%C0%D2%C2+%C5%AE+%BA%AB%B0%E6+%CC%D7%D7%B0&from=rs&navlog"
      "=rs-2-q-%CE%C0%D2%C2+%C5%AE+%BA%AB%B0%E6+%CC%D7%D7%B0&rels_k=97&initiative_id=staobaoz_20"
          "120301", false, true},
    {"http://s8.taobao.com/search?cat=162104&commend=all&s=0&sort=coefp&n=40&ratesum=1%2C2%"
      "2C3%2C4%2C5%2C6%2C7%2C8%2C9%2C10%2C11%2C12%2C13%2C14%2C15%2C16%2C17%2C18%2C19%2C20&q=%"
          "CC%A8%CD%E5%B9%DD&tab=coefp&tk_rate=%5B150%2C5000%5D&pid=mm_11095527_0_0&mode=23", false, true},
    {"http://s.click.taobao.com/t_3?&p=mm_11095527_0_0&n=23&l=http%3A%2F%2Fs8.taobao.com%2F"
      "browse%2F162104%2Fn-g%2Corvv64tborsvwmjvgawdkmbqgboq---g%2Czsum3znz3u-------------1%2"
          "C2%2C3%2C4%2C5%2C6%2C7%2C8%2C9%2C10%2C11%2C12%2C13%2C14%2C15%2C16%2C17%2C18%2C19%2C20"
          "---40--coefp-0-all-162104.htm?pid=mm_11095527_0_0", true, true},
    {"http://search8.taobao.com/search?q=%CE%C4%D0%D8%BC%D3%B3%A4%B1%B3%BF%DB+&unid=0&mod"
      "e=63&pid=mm_29741267_2555346_9677412&p4p_str=lo1%3D36%26lo2%3D36%26nt%3D1&s=160", false, true},
    // Sina
    {"http://iask.sina.com.cn/question/ask_new_2.php?key=&tag=0&title=%CC%A9%B0%B2%C2%B7&c"
      "lassid=0&type=0&gjss=0&page=0", true, false},
    {"http://iask.sina.com.cn/search_engine/search_knowledge_engine.php?key=style%3D%22co"
      "lor%3A%23C03%22%3E%D2%BD%D4%BA&tag=0&title=&classid=0&type=0&gjss=0&page=0", true, true},
    {"http://ishare.iask.sina.com.cn/search_engine/search_knowledge_engine.php?key=style%3"
      "D%22color%3A%23C03%22%3E%D2%BD%D4%BA&classid=0&type=3&ps=2130770168&pf=2131425521&g_"
          "time=&tag=&site=5", true, true},
    {"http://video.sina.com.cn/search/index.php?k=%E6%8C%81%E8%AF%81", true, true},
    // Uqude
    {"http://www.uqude.com/search/quiz;jsessionid=2136C181D305559240BFFB81C444FF82?keyword"
      "s=%25E5%25B0%258F%25E6%25B0%2594%25E8%25B4%25A2%25E7%25A5%259E", true, false},
    {"http://www.uqude.com/content/getSolr.action?keywords=%E6%B7%98%E5%AE%9D&typeSolr=0", true, true},
    {"http://www.uqude.com/content/getSolr.action", true, true},
    {"http://www.uqude.com/search?keywords=%25E7%2599%25BE%25E5%25BA%25A6", true, false},
    // Link173
    {"http://link.admin173.com/index.php?bd=5&wl=5&qz=4&act=byprGD", true, true},
    {"http://link.admin173.com/index.php", false, false},
    {"http://link.admin173.com", false, false},
    // Kaixin
    {"http://www.kaixin001.com/login/?flag=1&url=%2F!file%2Findex.php%3Fuid%3D57735461", true, true},
    // 168dushi
    {"http://www.168dushi.com.cn/czfy/?11-5-0-8-4-10-2-0-0-0", true, true},
    // Autohome
    {"http://car.autohome.com.cn/price/list-15_20-0-0-0-0-0-0-0-59-0-0-0-0-0-0-1.html", true, true},
    // Sohu
    {"http://db.auto.sohu.com/searchterm.sip?paixu=0&item=bid:158|bodyWork:6|engineSiz"
      "e:2.1-2.6|gearType:AMT|peizhi:10011110110101110|price:100#condi", true, true},
    {"http://db.auto.sohu.com/", false, false},
    {"http://product.it.sohu.com/search/subcategoryid=314&manuid%5B%5D=143&manuid%5"
      "B%5D=25&manuid%5B%5D=84&manuid%5B%5D=24&manuid%5B%5D=566&param_12113%5B%5D=li"
          "ke_LED%B5%E7%CA%D3&param_12113%5B%5D=like_%D4%C6%B5%E7%CA%D3&param_4019%5B%5D"
          "=between_47_47.999&param_4019%5B%5D=between_32_32.999&price%5B%5D=between", true, true},
    // 360buy
    {"http://search.360buy.com/search?keyword=%E6%98%A5%E8%A3%85&cid=1354&ev=137653702"
      "59520@37503654500979@&area=6", false, true},
    {"http://search.360buy.com/", false, false},
    // Hao123
    {"http://tv.hao123.com/index/dq-taiguo-nf-2007-yy-kouzhenhai17df", true, true},
    {"http://www.hao123.com/indexmk.html/game/haoserver/tools/haosever/redian/hao"
      "server/soft/qq/tianqi.htm", true, true},
    {"http://tv.hao123.com/", false, false},
    {"http://tv.hao123.net/index/dq-taiguo-nf-2007-yy-kouzhenhai17df", true, true},
    // 51job
    {"http://search.51job.com/list/0902,0000,2303,00,9,99,%2B,2,1.html?lang=c&stype=2"
      "&postchannel=0000&workyear=3&cotype=07&degreefrom=99&jobterm=0&lonlat=0%2C0&radi"
          "us=-1&ord_field=0&list_type=0&confirmdate=4&fromType=40&fromType=22", true, true},
    // Xunlei
    {"http://movie.xunlei.com/person/search,area,initial,constellation/%E5%BE%90%E5%B"
      "0%91%E5%BC%BA%2C8%2Cz,6", true, true},
    // Ifeng
    {"http://bbs.ifeng.com/pm.php?action=send&uid=3120718", true, true},
    // Huilitongxie
    {"http://huilitongxie.com.cn/?gallery-41-s3,30_4,0_5,1_s1,80-0--1--index.html", true, true},
    // Enet
    {"http://product.enet.com.cn/price/plist23_2289_s34542-1694_7_0_0_0p1.shtml", true, true},
    // 52dep
    {"http://www.52dpe.com/?gallery--p,0_tp,2_2,10_4,2_14,3_5,1_3,12-0--1-15-grid.html", true, true},
    // Pctowap
    {"http://old.pctowap.com/dir/asdf", true, true},
    {"http://www.pctowap.com/dir/asdf", true, true},
    // 5173
    {"http://trading.5173.com/search/dd1d2af96d444386951de92efdf96e07.shtml?cate="
      "-1&ga=fff64756459842ed8a3edc258f7863d7", true, true},
    //    // ChinaDaily
    {"http://chinadaily.chinadaily.com.cn/2012-02/13/2012-01/dfpd/2012-03/07/2011-12/"
      "05/2012-03/language_tips/auvideo/2012-03/22/content_14891788.htm", true, true},
    {"http://travel.chinadaily.cn/2012-02/13/2012-01/dfpd/2012-03/07/2011-12/"
      "05/2012-03/language_tips/auvideo/2012-03/22/content_14891788.htm", true, true},
    {"http://www.chinadaily.com.cn", false, false},
    {"http://www.chinadaily.cn", false, false},
    // Whnews
    {"http://news.whnews.cn/11zhuanti/news/node/2012-03/02/news/weihai/weihai/node/2012"
      "-03/21/11zhuanti/news/data/attachement/jpg/site2/20120322/0022159ceeb710d5911228.jpg", true, true},
    {"http://whnews.cn/11zhuanti/news/node/2012-03/02/news/weihai/weihai/node/2012"
      "-03/21/0022159ceeb710d5911228.htm", true, true},
    {"http://whccr.com/member/weihai/node/2012-02/23/wh_public/html/news/node/2012-03/03"
      "/weihai/news/node/2012-03/14/jiaju/wh_public/html/news/data/1939.files/home_map.gif", true, true},
    {"http://www.whnews.cn", false, false},
    //    // ItcpnZjol
    {"http://dgvan.zjol.com.cn/058763/036484/907672/323618/gf/404373/394186151423b.shtml", true, true},
    // Search360
    {"http://v.360.cn/dianshi/index.php?cat=103&year=all&area=13&act=%E8%83%A1%E6%AD%8C", true, true},
    {"http://v.360.cn", false, false},
    // Baixing
    {"http://www.newegg.com.cn/Search.aspx?N=800000465", true, true},
    // Mail 163
    {"http://twebmail.mail.163.com/js4/main.jsp?sid=qAuaxinOnNpEKPTyOXOOPXOILQdUFqvq", true, true},
    {"http://mail.163.com/?sid=qAuaxinOnNpEKPTyOXOOPXOILQdUFqvq", true, true},
    {"http://mail.163.com/", false, false},
    // Tencent Weibo
    {"http://t.qq.com/p/t/116595118596153", true, true},
    // Google webcahe and translate
    {"http://webcache.googleusercontent.com/search?q=cache:bErgbMzY12EJ:http://www.codeproject.com/Articles/316068/Restful-WCF-EF-POCOtory-MEF-1-o%2Bwcf%2Bef&hl=zh-TW&ct=clnk", true, true},  // NOLINT
    {"http://translate.google.com.hk/?q=yang+ming&um=1&ie=UTF-8&hl=zh-TW&sa=N&tab=wT ", true, true},
    {"http://translate.google.com.hk/a/?q=yang+ming&um=1&ie=UTF-8&hl=zh-TW&sa=N&tab=wT ", true, true},
    {"http://translate.google.com.hk/", false, false},
    // Baidu 推广
    {"http://e.baidu.com/?id=1", true, true},
    {"http://e.baidu.com/", false, false},
    // Bing 翻译 & 快照
    {"http://www.microsofttranslator.com/bv.aspx?ref=SERP&br=ro&mkt=en-ww&dl=en&lp=ZH-CHS_EN&a=http%3a%2f%2fpassport.baihe.com%2flogin.jsp", true, true},  // NOLINT
    {"http://cc.bingj.com/cache.aspx?q=%e7%99%be%e5%90%88&d=492356561099209&mkt=en-ww&setlang=en-US&w=666508ac,f1759f35", true, true},  // NOLINT
    // Sogou 快照 {"http://www.sogou.com/websnapshot?&url=http%3A%2F%2Fzhidao.baidu.com%2Fquestion%2F302185926&did=60d2f4fe0275d790-7b87d5c4474ee9b4-d90580eb05c7f73dcb767d929f3a9838&k=4c7f14ba5699a7f23e29fd8ce31268de&encodedQuery=%B0%D9%BA%CF&query=%B0%D9%BA%CF&&p=40040100&dp=1&w=01020400&m=0", true, true},  // NOLINT
    // Soso 快照
    {"http://snapshot.soso.com/snap.cgi?d=134875789409415390&w=%B0%D9%BA%CF&u=http://www.hudong.com/wiki/%e7%99%be%e5%90%88", true, true},  // NOLINT
    // 百度知道问题分类页 (非内容页)
    {"http://zhidao.baidu.com/browse/1031/?lm=2", true, true},
    {"http://zhidao.baidu.com/browse/?lm=2", true, true},
    // Baidu news 快照
    {"http://newscache.baidu.com/c?m=9d78d513d9d431db4f9e9e697c16c0106e43f1612ba4a6013894cd47c9221d03506790a63a7343449294263c5dfc5e5c9da16b2d2a5573eace95cf579deccd7f73de74692541c15c12d11aafc94027c4219a51eeaa19f0ba8768d5f18c&p=8e7cc316d9c54afe08e296651c42", true, true},  // NOLINT
    {"http://passport.baidu.com/?business&aid=6&un=%20%D4%D9%BC%FB1%C7%E0%B4%BA%20#2", true, true},
    // Sogou 广告
    {"http://www.sogou.com/bill_search?p=hFczn161m@x5$bNblllxP@qExkln7@D5lY0ksXtipxBNllllJwyvlt@3mfklllll&q=cGlkPXNvZ291JnI9MSZoPTE2ODU0NDI4OSZ2PTEmYz1MJnB2aWQ9MGJhMmM4MjFlOWRkM2UxMjAwMDAwMDAyNDY2NzcyNjMmcmVmPWh0dHA6Ly93d3cuc29nb3UuY29tLyZzPTEwMTAwJmVyPTEmY2Q9MjQ2Njc3MjUwJmd0PTAmcG49MTM1MDIwJmJ2PTEA&url=http://www.Xian", true, true},  // NOLINT
    // Google 广告
    {"http://www.google.com.hk/aclk?sa=l&ai=CchHygripT5CbDfGriQf8tJGfCvOEjLYBx4zokQXA7ZGKARABILZUKAdQlZyG-v7_____AWCdudCBkAWgAf-1q_4DyAEBqQLFVr_1WkeFPqoEF0_QqiW2TVagH-E5Ath_WUAF35bonFmtgAWQTg&num=4&sig=AOD64_3cw9rYOjP5GhgWU4rdVWJVYWINXg&ved=0CCcQ0Qw&adurl=http://www.China-Gift.com", true, true}, // NOLINT
    // Bing 广告
    {"http://adredir.adcenter.bing.com.cn/redir?params=93gD:sH8i:AQ:6bKc6Iqx&url=d3d3LmVkaWJsZWFycmFuZ2VtZW50cy5jb20uY24&form=QBRE&rid=2gunawYZnUGO4z0c1R8V8g", true, true},  // NOLINT
    {"http://www.content4ads.com/live.php?url=d_mK00josJ_CXCFVMJXrQoSZW63-skVZiKTyEsl-vZ95vfhM1r4Q-L1J4djB9r_uyYtwS_CyWiiM20ihApot7Gy59QTc4dWfD08VrQfNRsy3RLJCpb2E5PEUqeuK.7b_aoKsFx_ovUYQDknhlOKwCtoPxQWdQjPakvU2th4f.U1Yz0ZDq1tJeJQUbV8f0IjL5zo8CkxJLC6KGUHYznWf0I1Y0u1dsTvwYnfKdpHY0TA-b5HD0mv-b5H00Ugfqn0KopHYs0ZFY5i", true, true},  // NOLINT
    // Soso 广告
    {"http://jzclick.soso.com/click?vid=SXhG4EOdjEW+BO9bpzU6y/yIs/PyejVizUWpQurWqlkxKhsZicOMMzfnFECM+1qDt5TfI10eJILhmlIlYk0ltrX29ZODePZM0+uDJaLEwy5iTKQxp2GI9zNr41q4FBSLB5ajHFG7HT91PLsNHOYJOLb4LNjW4nqS+h6G8Y/EKwMiz6sAjfR5i7EAzoYp9l60O9wIw1yUkM8gOJtGz74tsQhLx+juQ1TjFKNf/kmz2f2kuN4p8iE7++tSPkpw3GYlQiPItmQjTaa0", true, true},  // NOLINT
    // Yougdao 广告 & 快照
    {"http://clkservice.youdao.com/clk/request.s?d=http%3A%2F%2Fwww.jy2scar.com&k=jyYMnPlawQHzhvwkIMsXFlkphEZHjwkT%2FnUigLHRR27HLe47L4amVnPaiMMALxJO41wNIVOuZkJ7XBbTondFUQTjqJMrEhnN9XrqmJA7yBynI4gmKRCM7xwj%2FHfxqNR4bgqm1o4W65Wz4wFrww%2FuQah5guPAvtAU9KkpHa5PL%2FnXxo%2BoRxcJpjjAgKuViCqv18aPqEcXCaY4wICrlYgqr9fGj6hHFwmmOMC", true, true},  // NOLINT
    {"http://www.youdao.com/cache?docid=9DCFA2EFA5D1C7F7&dc=00000077&eterms=YHY8ZJxAh00%3D&q=%E9%B2%9C%E8%8A%B1&url=http%3A%2F%2Fwww.vnasi.com%2F&", true, true},  // NOLINT

    // Filter for bad format
    {"http://%20%3C/body%3E%20%3C/html%3E", true, true},
    {"http://../04/4-0-main.htm", true, true},
    {"http://++baidu.com/", true, true},
  };
  std::string reason;
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(crawler3::WillFilterAccordingRules(cases[i].url, &reason), cases[i].filter_strict);
  }
}

TEST(selector, IsGeneralSearchFirstNPage) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    // Google
    {"http://www.google.com.hk/search?q=123&hl=zh-CN&newwindow=1&safe=strict&prmd=imvns&ei=r2CnT7nTN6utiQeNy6CvAw&sa=N&biw=1920&bih=1012", true},  // NOLINT
    {"http://www.google.com.hk/search?q=123&hl=zh-CN&newwindow=1&safe=strict&prmd=imvns&ei=r2CnT7nTN6utiQeNy6CvAw&start=0&sa=N&biw=1920&bih=1012", true},  // NOLINT
    {"http://www.google.com.hk/search?q=123&hl=zh-CN&newwindow=1&safe=strict&prmd=imvns&ei=r2CnT7nTN6utiQeNy6CvAw&start=20&sa=N&biw=1920&bih=1012", true},  // NOLINT
    {"http://www.google.com.hk/search?q=123&hl=zh-CN&newwindow=1&safe=strict&prmd=imvns&ei=r2CnT7nTN6utiQeNy6CvAw&start=30&sa=N&biw=1920&bih=1012", false},  // NOLINT
    // Baidu
    {"http://www.baidu.com/s?wd=%B4%F3%D1%A7&usm=2", true},
    {"http://www.baidu.com/s?wd=%B4%F3%D1%A7&pn=&usm=2", true},
    {"http://www.baidu.com/s?wd=%B4%F3%D1%A7&pn=10&usm=2", true},
    {"http://www.baidu.com/s?wd=%CF%CA%BB%A8&pn=20&tn=monline_dg&usm=3", true},
    {"http://www.baidu.com/s?wd=%CF%CA%BB%A8&pn=30&tn=monline_dg&usm=3", false},
    // Bing
    {"http://www.bing.com/search?q=%E7%AC%AC%E4%B8%89%E6%96%B9&qs=n&pq=%E7%AC%AC%E4%B8%89%E6%96%B9&sc=0-0&sp=-1&sk=&first=31&FORM=PERE3", false},  // NOLINT
    {"http://www.bing.com/search?q=%E7%AC%AC%E4%B8%89%E6%96%B9&qs=n&pq=%E7%AC%AC%E4%B8%89%E6%96%B9&sc=0-0&sp=-1&sk=&first=1&FORM=PERE3", true},  // NOLINT
    {"http://www.bing.com/search?q=%E7%AC%AC%E4%B8%89%E6%96%B9&qs=n&pq=%E7%AC%AC%E4%B8%89%E6%96%B9&sc=0-0&sp=-1&sk=&first=21&FORM=PERE3", true},  // NOLINT
    {"http://www.bing.com/search?q=%E7%AC%AC%E4%B8%89%E6%96%B9&qs=n&pq=%E7%AC%AC%E4%B8%89%E6%96%B9&sc=0-0&sp=-1&sk=&FORM=PERE3", true},  // NOLINT
    // Sougo
    {"http://www.sogou.com/sogou?query=%B7%AD%D0%C2%CB%AE%CF%B4%CD%C1%B6%B9%D2%D7%D6%D0%B6%BE&pid=sogou-site-5dec707028b05bcb+1&search_select=engine_36&page=2&duppid=1", true},  // NOLINT
    {"http://www.sogou.com/sogou?query=%B7%AD%D0%C2%CB%AE%CF%B4%CD%C1%B6%B9%D2%D7%D6%D0%B6%BE&pid=sogou-site-5dec707028b05bcb+1&search_select=engine_36&page=9&duppid=1", false},  // NOLINT
    // Soso
    {"http://www.soso.com/q?w=%CF%CA%BB%A8&lr=&sc=web&ch=w.p&num=10&gid=&cin=&site=&sf=0&sd=0&nf=0&pg=1", true},  // NOLINT
    {"http://www.soso.com/q?w=%CF%CA%BB%A8&lr=&sc=web&ch=w.p&num=10&gid=&cin=&site=&sf=0&sd=0&nf=0&pg=4", false},  // NOLINT
    /*
    // Yahoo
    {"http://search.yahoo.com/search;_ylt=A0oGdWDbbKdPNRsA5VpXNyoA?p=%E9%B2%9C%E8%8A%B1&fr=yfp-t-701&fr2=sb-top&xargs=0&pstart=1&b=31&xa=03xuF0uVurHRhK5yQz7.yA--,1336458843", false},  // NOLINT
    {"http://search.yahoo.com/search;_ylt=A0oGdWDbbKdPNRsA5VpXNyoA?p=%E9%B2%9C%E8%8A%B1&fr=yfp-t-701&fr2=sb-top&xargs=0&pstart=1&b=1&xa=03xuF0uVurHRhK5yQz7.yA--,1336458843", true},  // NOLINT
    */
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(crawler3::IsGeneralSearchFirstNPage(cases[i].url, 3), cases[i].result);
  }
}

TEST(selector, IsVerticalSearchFirstNPage) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    // Baidu news(每页有 20 条结果)
    {"http://news.baidu.com/ns?bt=0&et=0&si=&rn=20&tn=newsA&ie=gb2312&ct=1&word=%BF%A8%C2%DE%C0%AD&pn=40&cl=2", true},  // NOLINT
    {"http://news.baidu.com/ns?bt=0&et=0&si=&rn=20&tn=newsA&ie=gb2312&ct=1&word=%BF%A8%C2%DE%C0%AD&pn=60&cl=2", false},  // NOLINT
    // Baidu zhidao
    {"http://zhidao.baidu.com/q?ct=17&tn=ikaslist&rn=10&word=%C9%F9%C0%D6%B8%E8%C7%FA%20%D3%D0%C3%B5%B9%E5%CA%B2%C3%B4%A3%BF&lm=0&pn=10", true},  // NOLINT
    {"http://zhidao.baidu.com/q?ct=17&tn=ikaslist&rn=10&word=%C9%F9%C0%D6%B8%E8%C7%FA%20%D3%D0%C3%B5%B9%E5%CA%B2%C3%B4%A3%BF&lm=0&pn=30", false},  // NOLINT
    // Baidu baike
    {"http://baike.baidu.com/w?ct=17&word=%C0%EE%D0%A1%E8%B4&tn=baiduWikiSearch&rn=10&pn=20", true},  // NOLINT
    {"http://baike.baidu.com/w?ct=17&word=%C0%EE%D0%A1%E8%B4&tn=baiduWikiSearch&rn=10&pn=30", false},  // NOLINT
    // Baidu wenku
    {"http://wenku.baidu.com/search?word=%BB%FA%C6%F7%C8%CB%B5%C4%B9%A6%C4%DC&lm=0&od=0&pn=10", true},  // NOLINT
    {"http://wenku.baidu.com/search?word=%BB%FA%C6%F7%C8%CB%B5%C4%B9%A6%C4%DC&lm=0&od=0&pn=40", false},  // NOLINT
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(crawler3::IsVerticalSearchFirstNPage(cases[i].url, 3), cases[i].result);
  }
}

TEST(selector, IsBlackHostLink) {
  static struct {
    const char *url;
    const char *parent_url;
    bool result;
  } cases[] = {
    {"http://edu.360.cn/edu/?channel=zd&city=%B9%FE%B6%FB%B1%F5&famous=%B2%BB%CF%DE&grade=%b8%df%d2%bb&prices=500-999&nav_red=&cp=&page=&order=", "http://edu.360.cn/?channel=zd&city=%CE%E4%BA%BA&famous=%B2%BB%CF%DE&grade=%b8%df%d2%bb&prices=500-999", true},  // NOLINT
    {"http://edu.360.cn", "http://edu.360.cn/?channel=zd&city=%CE%E4%BA%BA&grade=%b8%df%d2%bb&prices=500-999", false},  // NOLINT
    {"http://edu.360.cn/edu/?channel=zd&city=%B9%FE%B6%FB%B1%F5&famous=%B2%BB%CF%DE&grade=%b8%df%d2%bb&prices=500-999&nav_red=&cp=&page=&order=", "http://edu.360.cn/", false},  // NOLINT
    // NOT in BlackHost Dict
    {"http://a.b.cn/edu/?channel=zd&city=%B9%FE%B6%FB%B1%F5&famous=%B2%BB%CF%DE&grade=%b8%df%d2%bb&prices=500-999&nav_red=&cp=&page=&order=", "http://a.b.cn/?channel=zd&city=%CE%E4%BA%BA&famous=%B2%BB%CF%DE&grade=%b8%df%d2%bb&prices=500-999", false},  // NOLINT
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    GURL gurl(cases[i].url);
    GURL parent_gurl(cases[i].parent_url);
    EXPECT_EQ(crawler3::IsBlackHoleLink(&gurl, &parent_gurl), cases[i].result);
  }
}

TEST(url_util, RewriteTaobaoItemDetail) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"http://item.taobao.com/item.htm?id=17103383391&ali_refid=a3_420999_1007:1102425886:7:662a55ef3a5f3fd1f0a150eaaa84293d:a10c55face9295c49d396f11d4aad5ec&ali_trackid=1_a10c55face9295c49d396f11d4aad5ec", true},  // NOLINT
    {"http://item.taobao.com/item.htm?id=17103383391", true},
    {"http://detail.tmall.com/item.htm?spm=a2106.m874.1000384.d11.5tKQ06&id=16118170707&source=dou&scm=1029.newlist-0.bts1.50008898&ppath=", true},  // NOLINT
    {"http://dail.tmall.com/item.htm?th=", false}
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string tmp(cases[i].url);
    EXPECT_EQ(crawler3::RewriteTaobaoItemDetail(&tmp), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "after rewrite, url: " << tmp;
    }
  }
}


TEST(url_util, RewriteTaobaoItemList) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"http://list.taobao.com/itemlist/nvzhuang2011a.htm?spm=a2106.m874.152316.110.8TkpR1&cat=16&isprepay=1&random=false&viewIndex=1&yp4p_page=0&commend=all&atype=b&style=grid&olu=yes&user_type=0&isnew=2&smc=1&fl=Shangpml#!cat=50108382&user_type=0&isprepay=1&sd=0&random=false&as=0&viewIndex=1&yp4p_page=0&commend=all&atype=b&fl=Shangpml&style=grid&ppath=3014869%3A136847%3B3014869%3A136848&tid=0&olu=yes&isnew=2&smc=1&json=on&tid=0", true},  // NOLINT
    {"http://list.taobao.com/itemlist/nvzhuang2011a.htm?spm=874.266289.265759.17.6NXRQF&cat=50102538&viewIndex=1&yp4p_page=0&commend=all&atype=b&style=grid&q=%C3%AB%C1%EC&isnew=2&msp=1#!cat=50102538&msp=1&viewIndex=1&as=0&yp4p_page=0&commend=all&atype=b&style=grid&q=%E6%AF%9B%E9%A2%86&tid=0&isnew=2&json=on&tid=0", true},  // NOLINT
    {"http://list.taobao.com/market/nvxieshichang2011a.htm?spm=1.124404.152499.30&cat=50029743&isprepay=1&viewIndex=1&yp4p_page=0&commend=all&atype=b&style=grid&olu=yes&isnew=2&mSelect=false", true},  // NOLINT
    {"http://list.taobao.com/market/1/searchAuction.htm?spm=874.266289.152316.24.ptZr29&fl=Xiazxpg#!http://list.taobao.com/market/1/nvzhuang2011a.htm?spm=874.266289.265759.1&&atype=b&cat=16&fl=Xiazxpg&user_type=0&viewIndex=11&isnew=2&hpsort=new&scm=1029.xinpin-1.2.16&json=on&_input_charset=utf-8", true},  // NOLINT
    {"http://nvzhuang.taobao.com/market/nvzhuang/yangrong.php?spm=541.217289.152316.17.8KMS7G&ad_id=&am_id=1301033454c88ef546a3&cm_id=&pm_id=&fl=Yangr", true},  // NOLINT
    {"http://list.taobao.com/itemlist/nvzhuang2011a.htm?spm=874.266289.152316.18.eIhOPu&cat=16&isprepay=1&random=false&viewIndex=1&yp4p_page=0&commend=all&atype=b&style=grid&olu=yes&user_type=0&isnew=2&smc=1&fl=Miany#!cat=16&user_type=0&isprepay=1&random=false&as=0&viewIndex=1&yp4p_page=0&commend=all&atype=b&fl=Miany&s=48&style=grid&tid=0&olu=yes&isnew=2&smc=1&json=on&tid=0", true},  // NOLINT
    {"http://dail.tmall.com/item.htm?th=", false},
    {"http://list.taobao.com/itemlist/beauty.htm?cat=50047291&sort=coefp&promoted_service2=tag&viewIndex=1&as=1&yp4p_page=0&commend=all&atype=b&style=grid&same_info=1&tid=0&isnew=2&smc=1&mSelect=false&json=on&tid=0&_input_charset=utf-8&json=on&_ksTS=1361433948666_207", true}, // NOLINT
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string tmp(cases[i].url);
    EXPECT_EQ(crawler3::RewriteTaobaoItemList(&tmp), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "after rewrite, url: " << tmp;
    }
  }
}

TEST(url_util, ParseJsonFormatPage) {
  static struct {
    const char *file;
    bool result;
  } cases[] = {
    {"crawler2/general_crawler/tests/data/taobao_json_1.txt", true},
    {"crawler2/general_crawler/tests/data/taobao_json_2.txt", true},
    {"crawler2/general_crawler/tests/data/taobao_json_bad.txt", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string filename(cases[i].file);
    std::string content;
    int page_num;
    CHECK(base::file_util::ReadFileToString(base::FilePath(filename), &content));
    std::vector<crawler3::url::TaobaoProductInfo> items;
    EXPECT_EQ(crawler3::ParseJsonFormatPage(content, &items, &page_num), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "item size: " << items.size();
      for (auto it = items.begin(); it != items.end(); ++it) {
        LOG(ERROR) << (*it).title() << ", page num: " << page_num;
      }
    }
  }
}

TEST(url_util, GetPageNumFromJinDongListPage) {
  static struct {
    const char *file;
    bool result;
  } cases[] = {
    {"crawler2/general_crawler/tests/data/jingdong_list_1.txt", true},
    {"crawler2/general_crawler/tests/data/jingdong_list_2.txt", true},
    {"crawler2/general_crawler/tests/data/jingdong_list_bad.txt", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string filename(cases[i].file);
    std::string content;
    int page_num;
    CHECK(base::file_util::ReadFileToString(base::FilePath(filename), &content));
    EXPECT_EQ(crawler3::GetPageNumFromJingDongListPage(content, &page_num), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "page num: " << page_num;
    }
  }
}


TEST(url_util, GetJinDongNextUrl) {
  const std::string url("http://www.360buy.com/products/1315-1343-1358.html");
  std::vector<std::string> urls;
  int n = 9;
  crawler3::GetJingDongNextUrl(url, n, &urls);
  LOG(ERROR) << "next n(" << n << ") url is: ";
  for (int i = 0; i < n; ++i) {
    LOG(ERROR) << urls[i];
  }
  //
  const std::string large_tv_url("http://www.360buy.com/products/737-794-798.html");
  std::vector<std::string> urls2;
  int n2 = 9;
  crawler3::GetJingDongNextUrl(large_tv_url, n2, &urls2);
  LOG(ERROR) << "大家电 next n(" << n2 << ") url is: ";
  for (int i = 0; i < n2; ++i) {
    LOG(ERROR) << urls2[i];
  }
}

TEST(url_util, ConvertTaobaoUrlFromPCToMobile) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"http://item.taobao.com/item.htm?id=17103383391&ali_refid=a3_420999_1007:1102425886:7:662a55ef3a5f3fd1f0a150eaaa84293d:a10c55face9295c49d396f11d4aad5ec&ali_trackid=1_a10c55face9295c49d396f11d4aad5ec", true}, // NOLINT
    {"http://item.taobao.com/item.htm?id=17103383391", true},
    {"http://detail.tmall.com/item.htm?spm=a2106.m874.1000384.d11.5tKQ06&id=16118170707&source=dou&scm=1029.newlist-0.bts1.50008898&ppath=", true},  // NOLINT
    {"http://detail.tmall.com/venus/spu_detail.htm?spm=a220m.1000858.1000725.9.qkQIVO&spu_id=211358243&cat_id=50024400&entryNum=0&loc=&mallstItemId=16659873114&rn=dde595adb417e565f6cf97d58cc48a78&rewcatid=50024400&from=&disp=g&active=1", true},  // NOLINT
    {"http://dail.tmall.com/item.htm?th=", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string tmp(cases[i].url);
    std::string out;
    EXPECT_EQ(crawler3::ConvertTaobaoUrlFromPCToMobile(tmp, &out), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "after rewrite, url: " << out;
    }
  }
}

TEST(url_util, BuildJingDongPriceImageLink) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"http://www.360buy.com/product/1005692938.html", true},
    {"http://www.360buy.com/product/1005692938.html#comments-list", false},
    {"http://www.360buy.com/products/1315-1343-1354-0-0-0-0-0-0-0-1-1-3.html", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string out;
    EXPECT_EQ(crawler3::BuildJingDongPriceImageLink(cases[i].url, &out), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "jingdong price image url: " << out;
    }
  }
}

TEST(url_util, BuildJingDongCommentLink) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"http://www.360buy.com/product/1005692938.html", true},
    {"http://www.360buy.com/product/1005692938.html#comments-list", false},
    {"http://www.360buy.com/products/1315-1343-1354-0-0-0-0-0-0-0-1-1-3.html", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string out;
    EXPECT_EQ(crawler3::BuildJingDongCommentLink(cases[i].url, &out), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "jingdong comment url: " << out;
    }
  }
}

TEST(url_util, GetPageNumFromSuNingListPage) {
  static struct {
    const char *file;
    bool result;
  } cases[] = {
    {"crawler2/general_crawler/tests/data/suning_list_1.txt", true},
    {"crawler2/general_crawler/tests/data/suning_list_2.txt", true},
    {"crawler2/general_crawler/tests/data/suning_list_bad.txt", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string filename(cases[i].file);
    std::string content;
    int page_num;
    CHECK(base::file_util::ReadFileToString(base::FilePath(filename), &content));
    EXPECT_EQ(crawler3::GetPageNumFromSuNingListPage(content, &page_num), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "page num: " << page_num;
    }
  }
}

TEST(url_util, GetPageNumFromTmallListPage) {
  static struct {
    const char *file;
    bool result;
  } cases[] = {
    {"crawler2/general_crawler/tests/data/tmall_list_1.txt", true},
    {"crawler2/general_crawler/tests/data/tmall_list_2.txt", true},
    {"crawler2/general_crawler/tests/data/tmall_list_bad.txt", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string filename(cases[i].file);
    std::string content;
    int page_num;
    CHECK(base::file_util::ReadFileToString(base::FilePath(filename), &content));
    EXPECT_EQ(crawler3::GetPageNumFromTmallListPage(content, &page_num), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "page num: " << page_num;
    }
  }
}

TEST(url_util, GetSuNingNextUrl) {
  // const std::string url("http://search.suning.com/emall/strd.do?ci=289505&cityId=9173");
  // const std::string url("http://search.suning.com/emall/strd.do?ci=289505");
  // const std::string url("http://search.suning.com/emall/pcd.do?ci=256003");
  const std::string url("http://search.suning.com/emall/search.do?keyword=%E6%88%B7%E5%A4%96%E9%9E%8B");
  std::vector<std::string> urls;
  int n = 9;
  crawler3::GetSuNingNextUrl(url, n, &urls);
  LOG(ERROR) << "next n(" << n << ") url is: ";
  for (int i = 0; i < n; ++i) {
    LOG(ERROR) << urls[i];
  }
}

TEST(url_util, BuildSuNingPriceLink) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"http://www.suning.com/emall/prd_10052_10051_-7_3435507_.html", true},
    {"http://www.suning.com/emall/prd_10052_10051_-7_3435507_.html#comments-list", false},
    {"http://search.suning.com/emall/strd.do?ci=289505&cityId=9173", false},
    {"http://www.suning.com/emall/prd_10052_10051_-7_2783729_.html", true},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string out;
    EXPECT_EQ(crawler3::BuildSuNingPriceLink(cases[i].url, &out), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "suning price url: " << out;
    }
  }
}

TEST(url_util, BuildSuNingCommentLink) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"http://www.suning.com/emall/prd_10052_10051_-7_3435507_.html", true},
    {"http://www.suning.com/emall/prd_10052_10051_-7_3435507_.html#comments-list", false},
    {"http://search.suning.com/emall/strd.do?ci=289505&cityId=9173", false},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string out;
    EXPECT_EQ(crawler3::BuildSuNingCommentLink(cases[i].url, &out), cases[i].result);
    if (cases[i].result) {
      LOG(ERROR) << "suning comment url: " << out;
    }
  }
}

TEST(url_util, BuildTaobaoCommentLink) {
  static struct {
    const char *url;
    bool result;
    const char *target_url;
  } cases[] = {
    {"http://a.m.taobao.com/i18178811690.htm", true, "http://a.m.taobao.com/ajax/rate_list.do?item_id=18178811690&p=1&ps=150"},  // NOLINT
    {"http://a.m.taobao.com/i18178811690.html", true, "http://a.m.taobao.com/ajax/rate_list.do?item_id=18178811690&p=1&ps=150"},  // NOLINT
    {"http://a.m.taobao.com/i18s178811690.htm", false, ""},
    {"http://search.suning.com/emall/strd.do?ci=289505&cityId=9173", false, ""},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string out;
    EXPECT_EQ(crawler3::BuildTaobaoCommentLink(cases[i].url, &out), cases[i].result);
    if (cases[i].result) {
      EXPECT_EQ(out, cases[i].target_url);
    }
  }
}

TEST(url_util, BuildTaobaoProductDetailLink) {
  static struct {
    const char *url;
    bool result;
    const char *target_url;
  } cases[] = {
    {"http://a.m.taobao.com/i18178811690.htm", true, "http://a.m.taobao.com/ajax/desc_list.do?item_id=18178811690"},  // NOLINT
    {"http://a.m.taobao.com/i18s178811690.htm", false, ""},
    {"http://search.suning.com/emall/strd.do?ci=289505&cityId=9173", false, ""},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string out;
    EXPECT_EQ(crawler3::BuildTaobaoProductDetailLink(cases[i].url, &out), cases[i].result);
    if (cases[i].result) {
      EXPECT_EQ(out, cases[i].target_url);
    }
  }
}

TEST(url_util, BuildTaobaoProductParamLink) {
  static struct {
    const char *url;
    bool result;
    const char *target_url;
  } cases[] = {
    {"http://a.m.taobao.com/i18178811690.htm", true, "http://a.m.taobao.com/ajax/param.do?item_id=18178811690"},  // NOLINT
    {"http://a.m.taobao.com/i18s178811690.htm", false, ""},
    {"http://search.suning.com/emall/strd.do?ci=289505&cityId=9173", false, ""},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string out;
    EXPECT_EQ(crawler3::BuildTaobaoProductParamLink(cases[i].url, &out), cases[i].result);
    if (cases[i].result) {
      EXPECT_EQ(out, cases[i].target_url);
    }
  }
}

TEST(url_util, BuildNextNUrl) {
  const std::string url("http://list.taobao.com/itemlist/nvzhuang2011a.htm?_input_charset=utf-8&as=0&atype=b&cat=16&commend=all&fl=Miany&isnew=2&isprepay=1&json=on&olu=yes&random=false&s=48&smc=1&style=grid&tid=0&user_type=0&viewIndex=1&yp4p_page=0");  // NOLINT
  std::vector<std::string> urls;
  int n = 9;
  crawler3::BuildNextNUrl(url, n, &urls);
  LOG(ERROR) << "next n(" << n << ") url is: ";
  for (int i = 0; i < n; ++i) {
    LOG(ERROR) << urls[i];
  }

  const std::string url2("http://list.tmall.com/search_product.htm?spm=a220m.1000858.1000724.9.gllmIY&active=1&from=sn_1_pro&area_code=330100&search_condition=20&vmarket=72&style=g&sort=p&prop=20573:3227476&n=60&s=0&cat=50024400#J_Filter");  // NOLINT
  std::vector<std::string> urls2;
  crawler3::BuildNextNUrl(url2, n, &urls2, 60);
  LOG(ERROR) << "next n(" << n << ") url is: ";
  for (int i = 0; i < n; ++i) {
    LOG(ERROR) << urls2[i];
  }
}

TEST(url_util, BuildTmallDifferentSortType) {
  static struct {
    const char *url;
  } cases[] = {
    {"http://list.tmall.com/search_product.htm?spm=3.356818.294515.48&cat=50023743&scm=1003.5.08005.0&acm=515.1026.0.771.50023743_8"}, // NOLINT
    {"http://list.tmall.com/search_product.htm?spm=3.356818.294515.41&from=sn_1_cat&area_code=330100&vmarket=0&wwonline=1&style=g&sort=s&start_price=20&n=60&s=0&cat=50031564#J_crumbs"}, // NOLINT
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::vector<std::string> out;
    crawler3::BuildTmallDifferentSortType(cases[i].url, &out);
    for (int i = 0; i < (int)out.size(); ++i) {
      LOG(ERROR) << out[i];
    }
  }
}

TEST(url_util, BuildTaobaoDifferentSortType) {
  static struct {
    const char *url;
  } cases[] = {
    {"http://list.taobao.com/itemlist/nvzhuang2011a.htm?spm=a2106.m874.152316.24.gZXO0E&cat=16&isprepay=1&random=false&viewIndex=1&yp4p_page=0&commend=all&atype=b&style=grid&olu=yes&user_type=0&isnew=2&smc=1&fl=Shangpml#!cat=16&user_type=0&isprepay=1&random=false&as=0&viewIndex=1&yp4p_page=0&commend=all&at"},  // NOLINT
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::vector<std::string> out;
    std::string url(cases[i].url);
    CHECK(crawler3::RewriteTaobaoItemList(&url));
    crawler3::BuildTaobaoDifferentSortType(url, &out);
    for (int i = 0; i < (int)out.size(); ++i) {
      LOG(ERROR) << out[i];
    }
  }
}

TEST(url_util, ConvertTaobaoImageUrl) {
  static struct {
    const char *url;
    const char *new_url;
  } cases[] = {
    {"http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.jpg_70x70.jpg", "http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.jpg"}, // NOLINT
    {"http://q.i01.wimg.taobao.com/bao/uploaded/i1/11524018669188556/T1DhUlXlhXXXXXXXXX_!!0-item_pic.jpg_b.jpg", "http://q.i01.wimg.taobao.com/bao/uploaded/i1/11524018669188556/T1DhUlXlhXXXXXXXXX_!!0-item_pic.jpg"}, // NOLINT
    {"http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524", "http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524"}, // NOLINT
    {"http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.png", "http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.png"}, // NOLINT
    {"http://q.i01.wimg.taobao.com/bao/uploaded/i4/920655852/T2DzqZXo8XXXXXXXXX_!!920655852.png_70x70.jpg", "http://q.i01.wimg.taobao.com/bao/uploaded/i4/920655852/T2DzqZXo8XXXXXXXXX_!!920655852.png"}, // NOLINT
    {"http://q.i04.wimg.taobao.com/bao/uploaded/i1/692282020/T21CuDXfxbXXXXXXXX_!!692282020.gif_70x70.jpg", "http://q.i04.wimg.taobao.com/bao/uploaded/i1/692282020/T21CuDXfxbXXXXXXXX_!!692282020.gif"}, // NOLINT
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string url;
    crawler3::ConvertTaobaoImageUrl(cases[i].url, &url);
    CHECK_EQ(url, std::string(cases[i].new_url));
  }
}

TEST(url_util, TestOnly) {
  // 普通的 . 从文件中读取只需要作一次转义; 但是从 c++ string 中这需要进行两次转义
  static const extend::re3::Re3 suning_product_url_pattern("http://detail.tmall.com/.+");
  // std::string url("http://detail.tmall.com/item.htm?spm=a220m.1000858.1000725.1.oVlJKL&id=16600551819&is_b=1&cat_id=50025829&q=");
  std::string url("http://detail.tmall.com/venus/spu_detail.htm?spm=a220m.1000858.1000725.9.qkQIVO&spu_id=211358243&cat_id=50024400&entryNum=0&loc=&mallstItemId=16659873114&rn=dde595adb417e565f6cf97d58cc48a78&rewcatid=50024400&from=&disp=g&active=1"); // NOLINT
  if (extend::re3::Re3::FullMatch(url, suning_product_url_pattern)) {
    LOG(ERROR) << "Matched: ";
  }
  // std::string url("http://search.suning.com/emall/search.do?keyword=%E6%88%B7%E5%A4%96%E9%9E%8B&cp=6&il=0&si=5&st=14&iy=-1&cp=8&il=0&si=5&st=14&iy=-1&cp=7&il=0&si=5&st=14&iy=-1&cp=15&il=0&si=5&st=14&iy=-1&cp=1&il=0&si=5&st=14&iy=-1&cp=9&il=0&si=5&st=14&iy=-1&cp=14&il=0&si=5&st=14&iy=-1&cp=15&il=0&si=5&st=14&iy=-1&cp=3&il");
  // GURL gurl(url);
  // const std::string &query = gurl.query();
  // LOG(ERROR) << "query: " << query;
  // std::string::size_type pos = query.find("&il=0&si=5&st=14&iy=-1");
  // if (base::MatchPattern(url, "http://search.suning.com/emall/*.do*")) {
  //   LOG(ERROR) << "match: ";
  // } else {
  //   LOG(ERROR) << "not match: ";
  // }
  // if (pos == std::string::npos) {
  //   LOG(ERROR) << "not find: ";
  // } else {
  //   LOG(ERROR) << "find: ";
  // }
}
