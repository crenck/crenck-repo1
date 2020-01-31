#include <iostream>
#include "p4vLauncher.h"
#include "option_parser.h"

int main(int argc, char *argv[])
{
    // Parse options for our message
    p4vc::OptionParser optParse;
    if(optParse.parse(argc, argv))
    {
        if(optParse.mpMessage == NULL)
            return -1;

        p4vc::P4VLauncher launcher(argc, argv, optParse.mpMessage);
        return launcher.exec();
    }

    return -1;
}
