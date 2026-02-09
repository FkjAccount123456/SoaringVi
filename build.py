import os

all_src = ["wcwidth/wcwidth.c"]
for i in os.listdir('.'):
    if i.endswith('.c') and i != 'main.c':
        all_src.append(i)

os.system('gcc ' + ' '.join(all_src) + ' -o smallest')
