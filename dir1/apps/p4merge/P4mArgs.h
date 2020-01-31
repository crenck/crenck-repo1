#ifndef __P4mArgs_H__
#define __P4mArgs_H__

#include <QString>

/*!
    \brief  This class parses and validades the command line arguments that are passed into P4Merge.
*/

class P4mArgs
{
public:
    P4mArgs();
    ~P4mArgs();
    bool parseArgs( QStringList args );
    bool hasErrorMessage();
    bool checkFiles();
    int numFiles();

    /*! \brief Returns the Version information to be displayed to the user */
    QString versionMessage() const;
    /*! \brief Returns the help message to be displayed to the user */
    QString helpMessage() const;
    /*! \brief Returns the error message to be displayed to the user */
    QString errorMessage() const { return mErrorMessage; };


public:

    QString files[4]; ///< Full path to the files
    QString names[4]; ///< Names to display for the files
/*!
    \brief White space option.<br>
    Valid options are:
    -# -db    Ignore line ending and whitespace length differences
    -# -dw    Ignore line ending and all whitespace differences
    -# -dl    Ignore line ending differences
    -# -da    Diff everything
*/
    int wsOption;
/*!
    \brief Grid option.<br>
    (currently undoc feature)
    Valid options are:
    -# no parameter (Optimal) Default
    -# -dg  (Guarded)
*/
    int gridOption;
/*!
    \brief Tab width value.<br>
    Valid options are integers greater then 0
*/
    int tabWidth;
/*!
    \brief Line ending option.<br>
    Valid options are:
    -# -unix  Option for unix and newer Macs
    -# -win   Option for Window systems
    -# -mac   Option for older Macs
*/
    int lineEnding;
    QString charSet;        ///< Charset to be used when decoding the file contents
    QString leftLabel;      ///< Label to display for the left file
    QString rightLabel;     ///< Label to display for the right file

/*!
    \brief MS Office Compare Web Service URL<br>
*/
    QString compareUrl;
/*!
    \brief MS Office Compare type option.<br>
    Valid values are:
    -# compare-parent-child
    -# compare-siblings
*/
    QString compareType;

    bool helpRequested;     ///< Indicates if help is being requested
    bool versionRequested;  ///< Indicates if version is being requested

private:
    QString mErrorMessage;
};

#endif // __P4mArgs_H__
