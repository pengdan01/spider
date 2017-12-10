import os
import sys
import re

#<a href="http://list.taobao.com/market/tbwt.htm?spm=1.151338.252575.18&atype=b&cat=150401&viewIndex=1&isnew=2&yp4p_page=0"
regex = "<a href=[\"'](http://list.taobao.com/market/[/\w_-]+.html?\?[^\s\"']+)[\"'\s]"
regobj = re.compile(regex)

#<a href="http://list.taobao.com/itemlist/sport2011a.htm?spm=1.151824.252637.12&q=&ex_q=&promoted_service4=4&olu=yes&cat=50010728&sort=commend&sd=0&viewIndex=1&yp4p_page=0&commend=0%2C2&user_type=0&atype=b&s=0&style=grid&path=50010388&isnew=2&navid=city&smc=1&rr=1#!promoted_service4=4&cat=50049230&user_type=0&sd=0&as=0&viewIndex=1&yp4p_page=0&atype=b&style=grid&tid=0&olu=yes&isnew=2&navid=city&smc=1&json=on&tid=0"
regex2 = "<a href=[\"'](http://list.taobao.com/itemlist/[/\w_-]+.htm\?[^\s\"']+)[\"'\s]"
regobj2 = re.compile(regex2)

hub_urls = []
file_content = ""
if os.path.exists(".list_utf8.html"):
  f = open(".hub_urls.txt", "ab")
  with open(".list_utf8.html", "rb") as list_file:
    for line in list_file:
      file_content += line
    #sys.stdout.write(file_content)
    all = regobj.findall(file_content)
    all2 = regobj2.findall(file_content)

    print (len(all))
    print (len(all2))

    for i in all:
      if i:
        f.write(i + "\n");
    for i in all2:
      if i:
        f.write(i + "\n");
  list_file.close()
  f.close();
else:
  sys.stdout.write("./.list_utf8.html not exist\n");

sys.exit(0)
