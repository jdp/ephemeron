#ifndef UTIL_H
#define UTIL_H

/* convenience macros */

#define NOTDEADYET printf("-- STILL OK IN %s AT %d --\n", __FILE__, __LINE__);

#define ERROR(FMT, ARGS...) error(stderr, 0, "ERROR: " #FMT "\n", ##ARGS);

#define FATAL(FMT, ARGS...) error(stderr, 1, "FATAL ERROR: " #FMT "\n", ##ARGS);

#define DEBUG(FMT, ARGS...) error(stderr, 0, "DEBUG %s:%d: " #FMT "\n", __FILE__, __LINE__, ##ARGS)

/* function prototypes */

void
error(FILE *, int, const char *, ...);

#endif
