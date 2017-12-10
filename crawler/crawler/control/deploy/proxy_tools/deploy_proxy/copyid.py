#!/usr/bin/python

import os
import sys
import task_runner

def source():
  for line in open('servers.txt'):
    yield line
def run(line):
  line = line.strip()
  print line
  os.system('ssh-copy-id root@"%s"' % line)
  os.system('ssh root@%s \"echo ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAwO7gPX/P/I2kBer40Eblkmzrshy/orLCH6DT2fC46yV7HPNeWsdipaXavMeSMA+1UgfDjzua2GmLrZdvSV90YR8HkeXJeOk452b4Ulz4vkdRk3nuHdixvjOSqUlDQkK+9KXXCnFgWau4pMTOkC/0yi0WaGcT1CSU3Rfb55LcZCWLf48AFGB7STmQBbLo3wWfNv0A1ooy78731EwImC5bfAVgnEcTKUU05OhCLprphGWREY/PIwijLYT2Z55RCgAvTZn/PhJF4kXLQj8YYLaM6JyR97PxTWzfg+pe4uVO/AuIB6mba3AZvX0G9stUGenvEJoklqi8dCKXFeec72Zm1Q== zhubotao@m07.dong.shgt.qihoo.net >>.ssh/authorized_keys\"' % line)
  os.system('ssh root@%s \"echo ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA7AoGaLdL6QeI/Fj+dPxfk3duvces5piB0DkyeQlKpH/Loej1l79rAkZ6+nGRnDxfVvFbYe7LJepEALeWviIrNOKMznVwPWa1zDgdqIqdQDegIGaJUFd5pYywgeG3A/Rr3TsxasWuTvJiueOhlkrz1OWe616LJD4UBGfGnEyb/lnQKbea8PoB8goihM9pix/NefQ2ssxC0MYvx+9hnGGPs8PLFeHH58MKkMNqTorqKxNe54I007VztQbByJGfHO8cIQc36JSdb+mncNBC9V/yeRDZcfvStr8vu+czMWDhVxfaHSH3st1WLeO7wtBPpFiQL7kEJgHycuQPxUEG0nMWnQ== zhubotao@h59>>.ssh/authorized_keys\"' % line)
task_runner.map_task(source(), run, 1)


