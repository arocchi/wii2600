/* $Id: getargs.h,v 1.2 2006-06-11 21:49:07 stephena Exp $
*/
#ifndef GETARGS_H
#define GETARGS_H

#ifdef __cplusplus
extern "C" {
#endif

extern int getargs(char *module,int argc,char *argv[],...);
extern int help_only;

#ifdef __cplusplus
}
#endif

#endif /* GETARGS_H */