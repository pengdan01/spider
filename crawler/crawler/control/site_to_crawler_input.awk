BEGIN{FS="\t";}
{
  if ($2 <= 2) {
    next;
  }
  $2="";
# html
  $3=1;
# css
#  $3=2;
  $4=2;
  $5="";
  $6="";
  $7=0;
  OFS="\t";
  print $0
}END{}
