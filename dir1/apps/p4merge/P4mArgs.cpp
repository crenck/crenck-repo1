#include "P4mArgs.h"

#include "Platform.h"

#include <QStringList>
#include <QFileInfo>
#include <QCoreApplication>

#if defined (Q_OS_OSX) || defined (Q_OS_WIN)
#define GUI_APPLICATION
#endif


/*!
    \class  P4mArgs

When P4Merge is run from the command line, several parameters are allowed to be
passed in.  This class parses and verifies the validity of the parameters.
If requested, it displays the Version or Help information.
It also checks to see if the correct number of files is passed in, and it
returns an error if they are not.
It also checks and displays errors for any other incorrect values that
are passed for white space option, line ending option, tab width option
and charset option.

Here's a code sample using P4mArgs:
      \code
    P4mArgs args;
    args.parseArgs( appArgs );
    if ( args.helpRequested )
        printf( args.helpMessage() );
     \endcode

*/

P4mArgs::P4mArgs()
{
    wsOption           = -1;
    gridOption         = -1;
    tabWidth           = -1;
    lineEnding         = -1;

    helpRequested      = false;
    versionRequested   = false;

}

QString
P4mArgs::helpMessage() const
{
    return QCoreApplication::translate("P4mArgs", QT_TRANSLATE_NOOP("P4mArgs",
"     Usage:\n"
"\n"
"     p4merge [options] left right\n"
"     p4merge [options] [base] left right\n"
"     p4merge [options] [base] left right [merge]\n"
"     p4merge -h\tPrint this message and exit\n"
"     p4merge -V\tPrint the version and exit\n"
"\n"
"    Options:\n"
"\n"
"    The following three options are mutually exclusive:\n"
"      -db    Ignore line ending and whitespace length differences\n"
"      -dw    Ignore line ending and all whitespace differences\n"
"      -dl    Ignore line ending differences\n"
"      -da    Diff everything\n"
"\n"
"      -tw <tab width>\n"
"      -le <line endings>\n"
"             'mac'\n"
"             'win'\n"
"             'unix'\n"
"\n"
"      -C <character set>\n"
"\n"
"           System:\n"
"             'none'            (System character set)\n"
"\n"
"           Unicode:\n"
"             'utf8'\n"
"             'utf8-bom'        (UTF-8 with a Byte-Order Mark [BOM])\n"
"             'utf16'           (UTF-16 with the default byte-order with a BOM)\n"
"             'utf16-nobom'     (UTF-16 default byte-order )\n"
"             'utf16be'         (UTF-16 big-endian byte-order)\n"
"             'utf16be-bom'     (UTF-16 big-endian byte-order with BOM)\n"
"             'utf16le'         (UTF-16 little-endian byte-order)\n"
"             'utf16le-bom'     (UTF-16 little-endian byte-order with a BOM)\n"
"             'utf32'           (UTF-32 with the default byte-order with a BOM)\n"
"             'utf32-nobom'     (UTF-32 default byte-order )\n"
"             'utf32be'         (UTF-32 big-endian byte-order)\n"
"             'utf32be-bom'     (UTF-32 big-endian byte-order with BOM)\n"
"             'utf32le'         (UTF-32 little-endian byte-order)\n"
"             'utf32le-bom'     (UTF-32 little-endian byte-order with a BOM)\n""\n"
"           Western European:\n"
"             'iso8859-1'\n"
"             'winansi'         (Windows code page 1252)\n"
"             'macosroman'\n"
"             'iso8859-15'\n"
"             'cp850'           (Windows code page 850)\n"
"\n"
"           Japanese:\n"
"             'shiftjis'\n"
"             'eucjp'\n"
"\n"
"           Cyrillic:\n"
"             'iso8859-5'\n"
"             'koi8-r'\n"
"             'cp1251'          (Windows code page 1251)\n"
"\n"
"           Korean:\n"
"             'cp949'           (Windows code page 949)\n"
"\n"
"           Chinese:\n"
"             'cp936'           (Simplified Chinese GBK)\n"
"             'cp950'           (Traditional Chinese Big5)\n"
"\n"
"           Greek:\n"
"             'cp1253'\n"
"             'iso8859-7'\n"
"\n"
"      -url <P4Combine Web Service URL>\n"
"      -ct <compare type>\n"
"             'compare-parent-child'\n"
"             'compare-siblings'\n"
"\n"
"      -nb  <label for base file>\n"
"      -nl  <label for left file>\n"
"      -nr  <label for right file>\n"
"      -nm  <label for merged file>\n"
));

}

QString
P4mArgs::versionMessage() const
{
    return QCoreApplication::translate("P4mArgs", QT_TRANSLATE_NOOP("P4mArgs", "%5 - The Perforce Visual Merge Tool\nCopyright 1995-%1 Perforce Software.\nAll rights reserved.\nRev. %5/%2/%3/%4\n"))
        .arg(ID_Y).arg(ID_OS ).arg( ID_REL ).arg( ID_PATCH).arg(Platform::ApplicationName());
}

P4mArgs::~P4mArgs()
{
}

/// \brief Parses and verifies arguments passed from the command line.
///     Additionally, it stores the files paths and files names being passed in
///     in the files[] and names[] arrays respectively.
///
/// \param              args = a QString list of arguments
/// \returns            A bool indicating if parsing succeeded or not
///
bool
P4mArgs::parseArgs( QStringList args )
{
    enum { Unset = -1, Line = 0, DashL, DashB, DashW };
    enum { Optimal = 0, Guarded };
    enum { Mac = 0, Windows, Unix };

    bool parseSuccess = true;

    enum State { Normal,
                 BaseName,
                 TheirsName,
                 YoursName,
                 MergeName,
                 LeftLabel,
                 RightLabel,
                 TabWidth,
                 LineEnding,
                 CharEncoding,
                 P4CharSet,
                 CompareUrl,
                 CompareType,
                 Files,
                 PostFiles,
                 Text } state = Normal;

    QString BASE = "-nb";
    QString THEIRS = "-nl";
    QString YOURS = "-nr";
    QString MERGE = "-nm";

    QString LEFTLABEL = "-ll";
    QString RIGHTLABEL = "-rl";

    QString IGNORE_WS = "-db";
    QString IGNORE_ALL_WS = "-dw";
    QString IGNORE_LE = "-dl";
    QString IGNORE_NOTHING = "-da";

    QString GUARDED = "-dg"; // Undoc feature

    QString TAB_WIDTH = "-tw";
    QString LINE_ENDING = "-le";

    QString COMPARE_URL = "-url";
    QString COMPARE_TYPE = "-ct";

    // The -text argument is not used anymore, but we leave it here for
    // backwards compatibility with 7.2 and older versions of P4V
    // that might pass this to P4Merge
    QString TEXT = "-text";

    // NOTES ON ENCODINGS:
    //
    // Character Encoding is the flag and set of charsets supported by
    // the 5.1 version of p4merge. P4V passed these to the merge tool. The
    // -ce flag was undocumented.
    //
    // For 5.2, the -C flag is published and will only take p4-style codepage
    // names. The -ce flag is still parsed and it reads QT charset names for backwards
    // compatibility, but it should *never* be published.
    //
    QString CHARACTER_ENCODING = "-ce"; // NEVER PUBLISH
    QString CHARACTER_SET = "-C";  // Supported flag for perforce charsets

    QString HELP = "-h";
    QString VERSION = "-V";

    int numFiles = 0;
    int exclusiveArgs = 0;

    // First arg is the executable name, so we start at 1 instead of 0
    for ( int i = 1; i < args.size(); ++i)
    {
        QString arg = args.at(i);

        switch ( state )
        {
            case Normal:
                if ( arg == BASE )
                    state = BaseName;
                else if ( arg == THEIRS)
                    state = TheirsName;
                else if ( arg == YOURS)
                    state = YoursName;
                else if ( arg == MERGE)
                    state = MergeName;
                else if ( arg == LEFTLABEL)
                    state = LeftLabel;
                else if ( arg == RIGHTLABEL)
                    state = RightLabel;
                else if ( arg == TAB_WIDTH)
                    state = TabWidth;
                else if ( arg == LINE_ENDING)
                    state = LineEnding;
                else if ( arg == CHARACTER_ENCODING)
                    state = CharEncoding;
                else if ( arg == CHARACTER_SET)
                    state = P4CharSet;
                else if ( arg == COMPARE_URL)
                    state = CompareUrl;
                else if ( arg == COMPARE_TYPE)
                    state = CompareType;
                else if ( arg == TEXT)
                    state = Normal;
                else if ( arg == GUARDED)
                    gridOption = Guarded;
                else if ( arg == IGNORE_WS)
                {
                    wsOption = DashB;
                    exclusiveArgs++;
                }
                else if ( arg == IGNORE_ALL_WS)
                {
                    wsOption = DashW;
                    exclusiveArgs++;
                }
                else if ( arg == IGNORE_LE)
                {
                    wsOption = DashL;
                    exclusiveArgs++;
                }
                else if ( arg == IGNORE_NOTHING)
                {
                    wsOption = Line;
                    exclusiveArgs++;
                }
                else if ( arg == HELP)
                    helpRequested = true;
                else if ( arg == VERSION)
                    versionRequested = true;
                else
                {
                    if ( arg.startsWith("-") )
                    {
                        mErrorMessage.append( "'" );
                        mErrorMessage.append( arg );
                        mErrorMessage.append( "' is not a valid option.\n" );
                        parseSuccess = false;
                    }
                    else
                    {
                        files[numFiles++] = arg;
                        state = Files;
                    }

                }
            break;

            case BaseName:
                names[0] = arg;
                state = Normal;
            break;

            case TheirsName:
                names[1] = arg ;
                state = Normal;
            break;

            case YoursName:
                names[2] = arg ;
                state = Normal;
            break;

            case MergeName:
                names[3] = arg ;
                state = Normal;
            break;

            case LeftLabel:
                leftLabel = arg ;
                state = Normal;
            break;

            case RightLabel:
                rightLabel = arg ;
                state = Normal;
            break;

            case CompareUrl:
                compareUrl = arg ;
                state = Normal;
            break;

            case CompareType:
                compareType = arg ;
                state = Normal;
            break;

            case TabWidth:
            {
                tabWidth = arg.toInt();
                if ( tabWidth < 0 )
                {
                    mErrorMessage.append( "'" );
                    mErrorMessage.append( arg );
                    mErrorMessage.append( "' is not a valid tab spacing.\n" );
                    tabWidth = -1;
                    parseSuccess = false;
                }
                state = Normal;
            }
            break;

            case LineEnding:
            {
                //
                // MAKE SURE THIS MATCHES THE ENUM IN P4MPREFS!!!!
                //
                if ( arg == "mac")
                    lineEnding = 1;
                else if ( arg == "win")
                    lineEnding = 2;
                else if ( arg == "unix")
                    lineEnding = 0;
                else
                {
                    mErrorMessage.append( "'" );
                    mErrorMessage.append( arg );
                    mErrorMessage.append( "' is not a valid line ending.\n" );
                    parseSuccess = false;
                }
                state = Normal;
            }
            break;

            case CharEncoding:
                if (( arg != "none") &&
                    ( arg != "utf8") &&
                    ( arg != "utf16") &&

                    ( arg != "ISO8859-1") &&
                    ( arg != "CP1252") &&
                    ( arg != "Apple Roman") &&
                    ( arg != "ISO8859-15") &&

                    ( arg != "Shift-JIS") &&
                    ( arg != "eucJP") &&

                    ( arg != "ISO8859-5") &&
                    ( arg != "ISO8859-7") &&
                    ( arg != "KOI8-R") &&
                    ( arg != "CP1251") &&
                     ( arg != "CP1253") )
                {
                    mErrorMessage.append( "'" );
                    mErrorMessage.append( arg );
                    mErrorMessage.append( QString("' is not a QT character encoding %1 supports.\n").arg(Platform::ApplicationName()) );
                    parseSuccess = false;
                }
                charSet = arg;
                state = Normal;
            break;


            case P4CharSet:
                if (( arg != "none") &&
                    ( arg != "utf8") &&
                    ( arg != "utf8-bom") &&
                    ( arg != "utf16-nobom") &&
                    ( arg != "utf16")  &&
                    ( arg != "utf16be")  &&
                    ( arg != "utf16be-bom")  &&
                    ( arg != "utf16le")  &&
                    ( arg != "utf16le-bom")  &&

                    ( arg != "utf32-nobom") &&
                    ( arg != "utf32")  &&
                    ( arg != "utf32be")  &&
                    ( arg != "utf32be-bom")  &&
                    ( arg != "utf32le")  &&
                    ( arg != "utf32le-bom")  &&

                    ( arg != "iso8859-1")  &&
                    ( arg != "winansi")  &&
                    ( arg != "macosroman")  &&
                    ( arg != "iso8859-15")  &&
                    ( arg != "cp850")  &&
                    ( arg != "cp858")  &&

                    ( arg != "shiftjis") &&
                    ( arg != "eucjp")  &&

                    ( arg != "iso8859-5")  &&
                    ( arg != "koi8-r")  &&
                    ( arg != "cp1251")  &&
                    ( arg != "cp949")  &&
                    ( arg != "cp936")  &&
                    ( arg != "cp950")  &&
                    ( arg != "cp1253")  &&
                    ( arg != "iso8859-7") )
                {
                    mErrorMessage.append( "'" );
                    mErrorMessage.append( arg );
                    mErrorMessage.append( "' is not a valid Perforce character encoding.\n" );
                    parseSuccess = false;
                }
                charSet = arg;
                state = Normal;
            break;


            case Files:
            {

                files[numFiles++] = arg;
                if ( numFiles >= 4 )
                    state = PostFiles;
            }
            break;

            //
            // NO BREAK STATEMENT. We want it to pass to the next state.
            //

            case PostFiles:
            {
                mErrorMessage.append( "Too many files: You may enter a maximum of 4 files.\n" );
                parseSuccess = false;
            }
            break;

            case Text:
                break;
        }
    }

if ( exclusiveArgs > 1 )
    {
        mErrorMessage.append( IGNORE_WS );
        mErrorMessage.append( ", " );
        mErrorMessage.append( IGNORE_ALL_WS );
        mErrorMessage.append( " and " );
        mErrorMessage.append( IGNORE_LE );
        mErrorMessage.append( " are mutually exclusive. You may enter only one.\n" );
        parseSuccess = false;
    }

#ifndef Q_OS_OSX
    if ( numFiles < 2 )
    {
        mErrorMessage.append("At least two files are needed.\n");
        parseSuccess = false;
    }
#endif

    // If we only have two files, we want them in positions 1 and 2 or our array,
    // so we copy them there and null out position 0 which is the base
    if ( numFiles == 2 )
    {
        files[2] = files[1];
        files[1] = files[0];
        files[0] = QString();
    }

    return parseSuccess;
}

/// \brief      Checks if there is an error message to display.
///
/// \returns    A bool if there is an error message to display
///
bool
P4mArgs::hasErrorMessage()
{
    bool empty = false;
    if ( !mErrorMessage.isEmpty() )
        empty = true;
    return empty;
}

/// \brief      Checks if the files passed in exist and are valid.
///
/// \returns    A bool indicating if files are valid
///
bool
P4mArgs::checkFiles()
{
    bool b = true;

    if ( !files[0].isEmpty() && files[1].isEmpty() )
    {
        mErrorMessage.append( QString("Cannot open only one file. %1 needs 0, 2, or 3 files.\n").arg(Platform::ApplicationName()) );
        return false;
    }

    for ( int i = 0; i <= 3; i++ )
    {
        if ( files[i].isEmpty() )
            continue;

        files[i] = files[i].trimmed();
        QFileInfo fi(files[i]);
        // Check to see if the file is valid
        //
        // It doesn't exist, the file it links to doesn't exist
        // or it is a directory.
        //
        if ( fi.exists() && !fi.isDir() )
            continue;

        mErrorMessage.append( "'" );
        mErrorMessage.append( files[i] );
        mErrorMessage.append( "' is (or points to) an invalid file.\n" );
        b = false;
    }
    return b;
}

/// \brief Calculates the number of files passed in.
///
/// \returns            number of files passed in
/// \retval 0           if no files were passed
///
int
P4mArgs::numFiles()
{
    int numFiles = 0;
    for ( int i = 0; i < 3; i++ )
    {
        if (!files[i].isEmpty())
            numFiles++;
    }

    return numFiles;
}
