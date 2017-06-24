#ifndef _ERR_
#define _ERR_

/* Wypisuje informacje o blednym zakonczeniu funkcji systemowej
i konczy dzialanie programu. */
extern void syserr(const char *fmt, ...);

/* Wypisuje informacje o bledzie i konczy dzialanie programu. */
extern void fatal(const char *fmt, ...);

#endif
