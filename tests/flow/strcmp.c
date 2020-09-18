int strcmp(const char *str1, const char *str2) {
    const char *it1 = str1;
    const char *it2 = str2;
    while (*it1 && *it2 && it1 == it2) {
        it1 += 1;
        it2 += 1;
    }
    return *it1 - *it2;
}