import os
import sys
import re

#http://search.suning.com/emall/search.do?keyword=iphone&ci=20006
regex = "<a href=[\"'](http://search.suning.com/emall/search.do\?keyword=[0-9a-zA-Z_%]+&ci=\d+)[\"']\starget="
regobj = re.compile(regex)

#http://search.suning.com/emall/search.do?keyword=iphone
regex2 = "<a href=[\"'](http://search.suning.com/emall/search.do\?keyword=[0-9a-zA-Z_%]+)[\"']\starget="
regobj2 = re.compile(regex2)

#http://search.suning.com/emall/pcd.do?ci=161615&tp=_
regex3 = "<a href=[\"'](http://search.suning.com/emall/pcd.do\?ci=\d+)&tp=_[\"']\starget="
regobj3 = re.compile(regex3)

#http://search.suning.com/emall/strd.do?ci=285519
regex4 = " href=[\"'](http://search.suning.com/emall/s?trd.do\?ci=\d+).*[\"']\starget="
regobj4 = re.compile(regex4)

hub_urls = []
file_content = ""
if os.path.exists(".suning_list_utf8.html"):
  f = open(".hub_urls.txt", "wb")
  with open(".suning_list_utf8.html", "rb") as list_file:
    for line in list_file:
      file_content += line
    sys.stdout.write(file_content)
    all = regobj.findall(file_content)
    all2 = regobj2.findall(file_content)
    all3 = regobj3.findall(file_content)
    all4 = regobj4.findall(file_content)

    print (len(all))
    print (len(all2))
    print (len(all3))
    print (len(all4))

    for i in all:
      if i:
        f.write(i + "\n");
    for i in all2:
      if i:
        f.write(i + "\n");
    for i in all3:
      if i:
        f.write(i + "\n");
    for i in all4:
      if i:
        f.write(i + "\n");
  list_file.close()
  f.close();
else:
  sys.stdout.write("./.suning_list_utf8.html not exist\n");

sys.exit(0)
