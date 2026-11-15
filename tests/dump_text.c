/* writes lines.txt, talk.txt, items.txt and names.txt (the built-in text of src/lines.h, src/talk.h and src/extitems.h) into a folder: gcc tests/dump_text.c -o dump_text && ./dump_text packaging
   Run it again after you change src/lines.h or src/talk.h to refresh the template files. (The game itself also writes them on first start when they are missing.) */
#include "../src/extlines.h"
#include "../src/extitems.h"
int main(int argc, char **argv) {
    char a[400], b[400], c[400], e[400]; const char *d = argc > 1 ? argv[1] : ".";
    snprintf(a, sizeof a, "%s/lines.txt", d); snprintf(b, sizeof b, "%s/talk.txt", d); snprintf(c, sizeof c, "%s/items.txt", d); snprintf(e, sizeof e, "%s/names.txt", d);
    ex_snapshot(); it_snapshot();
    if (!ex_dump(a, 0) || !ex_dump(b, 1) || !it_dump(c) || !nm_dump(e)) { printf("cannot write into %s\n", d); return 1; }
    printf("wrote %s, %s, %s and %s (%d texts, %d items)\n", a, b, c, e, TXT_N, IT_N); return 0;
}
