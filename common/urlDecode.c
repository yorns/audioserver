// from https://stackoverflow.com/questions/2673207/c-c-url-decode-library/19826808

#include <stdio.h>
#include <ctype.h>

int decodeURIComponent (char *sSource, char *sDest) {
    int nLength;
    for (nLength = 0; *sSource; nLength++) {
        if (*sSource == '%' && sSource[1] && sSource[2] && isxdigit(sSource[1]) && isxdigit(sSource[2])) {
            sSource[1] -= sSource[1] <= '9' ? '0' : (sSource[1] <= 'F' ? 'A' : 'a')-10;
            sSource[2] -= sSource[2] <= '9' ? '0' : (sSource[2] <= 'F' ? 'A' : 'a')-10;
            sDest[nLength] = (char)(16 * sSource[1] + sSource[2]);
            sSource += 3;
            continue;
        }
        sDest[nLength] = *sSource++;
    }
    sDest[nLength] = '\0';
    return nLength;
}

#define implodeURIComponent(url) decodeURIComponent(url, url)

