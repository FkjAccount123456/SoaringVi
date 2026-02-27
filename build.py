import os
import sys

# 有其他办法最好
if sys.platform == 'win32':
    with open("wcwidth/wcwidth.h", "w") as f:
        f.write("""#ifndef WCWIDTH_H_INCLUDED
#define WCWIDTH_H_INCLUDED

#include <stdlib.h>

int wcwidth(wchar_t ucs);

#endif
""")

all_src = ["wcwidth/wcwidth.c"]
for i in os.listdir('.'):
    if i.endswith('.c') and i != 'main.c':
        all_src.append(i)

os.system('gcc ' + ' '.join(all_src) + ' -o smallest')

if sys.platform == 'win32':
    with open("wcwidth/wcwidth.h", "w") as f:
        f.write("""#ifndef WCWIDTH_H_INCLUDED
#define WCWIDTH_H_INCLUDED

#include <stdlib.h>

__BEGIN_DECLS

int wcwidth(wchar_t ucs);

__END_DECLS

#endif
""")
