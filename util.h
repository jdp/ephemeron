#ifndef UTIL_H
#define UTIL_H

#define NOTDEADYET printf("-- STILL OK IN %s AT %d --\n", __FILE__, __LINE__);

#define ERROR(...) fprintf(stderr, __VA_ARGS__);

#endif
