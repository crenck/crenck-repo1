#include "MainOption.h"

#include <string.h>

MainOption::Main
MainOption::getMain(int argc, char **argv)
{
    if (argc < 2)
        return MainP4v;
    if(!strcmp(argv[1], "-V"))
        return MainVersion;
    return MainP4v;
}

