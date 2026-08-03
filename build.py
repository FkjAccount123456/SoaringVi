import os
import sys

fdata = hdata = ""

def proc_wcwidth():
    global fdata, hdata
    with open("wcwidth/wcwidth.h", "r") as f:
        hdata = f.read()
    with open("wcwidth/wcwidth.h", "w") as f:
        f.write("""#ifndef WCWIDTH_H_INCLUDED
#define WCWIDTH_H_INCLUDED

#include <stdlib.h>

int wcwidth(int ucs);

#endif
""")
    with open("wcwidth/wcwidth.c", "r") as f:
        fdata = f.read()
    with open("wcwidth/wcwidth.c", "w") as f:
        f.write(fdata.replace("wchar_t", "int"))


def postproc_wcwidth():
    global fdata, hdata
    with open("wcwidth/wcwidth.h", "w") as f:
        f.write(hdata)
    with open("wcwidth/wcwidth.c", "w") as f:
        f.write(fdata)


# 有其他办法最好
if sys.platform == "win32":
    proc_wcwidth()

all_src = ["wcwidth/wcwidth.c"]
for i in os.listdir("."):
    if i.endswith(".c") and i != "smallest.c":
        all_src.append(i)

opts = []
if "--debug" in sys.argv:
    opts.append("-O0 -g")
elif "--check" in sys.argv:
    opts.append("-fsyntax-only")
elif "--procwcwidth" in sys.argv:
    exit(0)
else:
    opts.append("-O3")

os.system("gcc " + " ".join(opts) + " " + " ".join(all_src) + " -o srvi")

if sys.platform == "win32":
    postproc_wcwidth()
