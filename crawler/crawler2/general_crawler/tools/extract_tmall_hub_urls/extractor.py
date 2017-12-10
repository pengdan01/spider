import os
import sys
import re

#http://list.tmall.com/search_product.htm?spm=3.356818.294515.21&q=&start_price=&end_price=&catName=&search_condition=16&cat=50026473&sort=s&style=g&vmarket=0&active=1"
regex = "<a href=[\"'](http://list.tmall.com/search_product.htm\?[^\s\"']+)[\"'\s]"
regobj = re.compile(regex)

#http://list.tmall.com/50037443/g-d-----40-0--50037443-x.htm?spm=3.356818.294515.151&TBG=19623.15483.68"
regex2 = "<a href=[\"'](http://list.tmall.com/\d+/[\w-]+.htm\?[^\s\"']+)[\"'\s]"
regobj2 = re.compile(regex2)

hub_urls = []
file_content = ""
if os.path.exists(".list_utf8.html"):
  f = open(".hub_urls.txt", "wb")
  with open(".list_utf8.html", "rb") as list_file:
    for line in list_file:
      file_content += line
    sys.stdout.write(file_content)
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
