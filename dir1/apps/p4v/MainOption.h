#ifndef INCLUDED_MAINOPTION
#define INCLUDED_MAINOPTION

class MainOption
{
public:
    enum Main {
        MainP4v,
        MainVersion
    };
    static Main getMain(int argc, char **argv);
};

#endif // INCLUDED_MAINOPTION
