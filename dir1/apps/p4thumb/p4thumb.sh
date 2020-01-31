#!/bin/sh

# When one sets the environment varaible P4VHOST, its value will be prepended
# on the command running the executable.
# This will allow customers or support to run the executable in the enviroment
# provided by this script from 'ldd', 'gdb', 'strace' etc .....
# example : $export P4VHOST=ldd

getrealfullprogname()
{
    # If possible, handle the case that someone has created a symlink in
    # /usr/local/bin back to this script in its original unpacked
    # distribution directory.
    thisfile=`{ readlink -f "$1" \
                || { ls -ld "$1" | sed -n -e 's/.* -> //p'; }
              } 2> /dev/null`
    case $thisfile in
        '' ) thisfile=$1 ;;
    esac

    echo "$thisfile"
}

topdir()
{
    progdir=`dirname "$1"`
    case $progdir in
        . | '' | "$1" ) progdir=`pwd` ;;
    esac

    case $progdir in
        */bin ) topdir=`dirname "$progdir"` ;;
        *     ) topdir=$progdir ;;
    esac

    echo "$topdir"
}

main()
{
    realfullprogname=`getrealfullprogname "$0"`
            progname=`basename "$realfullprogname"`
              prefix=`topdir   "$realfullprogname"`

    DISPLAY=:0.0
    QT5DIR=$prefix/lib/qt5
    LD_LIBRARY_PATH=$QT5DIR/lib:$prefix/lib/openssl:$prefix/lib/icu${LD_LIBRARY_PATH+:$LD_LIBRARY_PATH}
    PATH=$prefix/bin:$PATH
    export LD_LIBRARY_PATH PATH DISPLAY

    exec $P4VHOST "$prefix/bin/$progname.bin" "$@" || exit $?
}

main "$@"

# eof
