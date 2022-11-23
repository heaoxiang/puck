import sys
import random

if len(sys.argv) != 2:
    print >> sys.stderr, "parameter error !"
    sys.exit(-1)
bsnum = int(sys.argv[1])
idx = 0
for line in sys.stdin:
    if not line:
        print >> sys.stderr, 'not line'
        break
    idx += 1
    fields = line.strip().split("\t")
    if len(fields) < 3:
         print >> sys.stderr, 'reporter:counter:%s,%s,%s' % ("puck", "merge", "1")
         continue
    cellid = fields[0]
    bsid = 0
    if bsnum > 1:
        bsid = random.randint(1, bsnum) % bsnum
    print "%s %s\t%s" % (bsid, cellid, line.strip())
