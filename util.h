#ifndef UTIL_H
#define UTIL_H

#define NOTDEADYET printf("-- STILL OK IN %s AT %d --\n", __FILE__, __LINE__);

#define ERROR(F, ...) fprintf(stderr, "ERROR %s:%d: " F "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif
