import os
import sys

# 有其他办法最好
if sys.platform == "win32":
    with open("wcwidth/wcwidth.h", "w") as f:
        f.write("""#ifndef WCWIDTH_H_INCLUDED
#define WCWIDTH_H_INCLUDED

#include <stdlib.h>

int wcwidth(wchar_t ucs);

#endif
""")

all_src = ["wcwidth/wcwidth.c"]
for i in os.listdir("."):
    if i.endswith(".c") and i != "smallest.c":
        all_src.append(i)

opts = []
if "--debug" in sys.argv:
    opts.append("-O0 -g")
else:
    opts.append("-O3")

os.system("gcc " + " ".join(opts) + " " + " ".join(all_src) + " -o srvi")

if sys.platform == "win32":
    with open("wcwidth/wcwidth.h", "w") as f:
        f.write("""#ifndef WCWIDTH_H_INCLUDED
#define WCWIDTH_H_INCLUDED

#include <stdlib.h>

__BEGIN_DECLS

int wcwidth(wchar_t ucs);

__END_DECLS

#endif
""")
