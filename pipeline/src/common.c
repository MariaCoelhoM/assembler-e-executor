#include "../include/common.h"

void str_upper(char *dst, const char *src) {
    int i = 0;
    while (src[i]) { dst[i] = (char)toupper((unsigned char)src[i]); i++; }
    dst[i] = '\0';
}

int str_is_empty(const char *s) {
    while (*s) { if (!isspace((unsigned char)*s)) return 0; s++; }
    return 1;
}

void ltrim(char *s) {
    int i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i > 0) memmove(s, s + i, strlen(s) - i + 1);
}

void rtrim(char *s) {
    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) len--;
    s[len] = '\0';
}

void trim(char *s) { ltrim(s); rtrim(s); }
