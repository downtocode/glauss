#ifndef PHYSENGINE_PARS
#define PHYSENGINE_PARS

int preparser();
int parser(data** object, char filename[200]);
char* readshader(const char* filename);

#endif
