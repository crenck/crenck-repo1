#include "MainOption.h"

#include "Platform.h"

extern int mainP4v( int argc, char** argv );
extern int mainVersion( int argc, char** argv );

int main( int argc, char** argv )
{
    Platform::SetApplication(Platform::P4V_App);
    Platform::SetApplicationName("Helix P4V", "Helix Visual Client");

    MainOption::Main m = MainOption::getMain(argc, argv);

    switch (m)
    {
    case MainOption::MainVersion:
        return mainVersion(argc, argv);

    case MainOption::MainP4v:
    default:    // just in case.....
        try
        {
            return mainP4v(argc, argv);
        }
        catch (...)
        {
        }
    }
}


