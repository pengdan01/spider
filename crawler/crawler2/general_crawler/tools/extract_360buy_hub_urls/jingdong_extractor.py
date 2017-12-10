import os
import sys
import re

regex = "<a href=[\"']/?(products/\d+-\d+-\d+.html)[\"']>"
regobj = re.compile(regex)

hub_urls = []
file_content = ""
if os.path.exists(".jingdong_list_utf8.html"):
  f = open(".hub_urls.txt", "wb")
  with open(".jingdong_list_utf8.html", "rb") as list_file:
    for line in list_file:
      file_content += line
    sys.stdout.write(file_content)
    all = regobj.findall(file_content)

    print (len(all))

    for i in all:
      if i:
        hub_urls.append("http://www.360buy.com/" + i);
    for url in hub_urls:
      f.write(url + "\n");
  list_file.close()
  f.close();
else:
  sys.stdout.write("./.jingdong_list_utf8.html not exist\n");

sys.exit(0)
