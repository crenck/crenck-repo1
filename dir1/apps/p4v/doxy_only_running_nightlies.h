/*! \page running_nighties Running Nightly Builds


\li \ref running_nightlies_mac
\li \ref running_nightlies_win_91
\li \ref running_nightlies_win_91dev
\li \ref running_nightlies_win_82


\section running_nightlies_mac Mac, all versions

P4Merge.app is in the P4V.dmg disk image. To run P4Merge, mount P4V.dmg then open P4Merge.app.

-# Open P4V.dmg image. The disk will mount and open.
-# Within that disk image, open P4V.app or P4Merge.app.
-# There is no step 3.

P4V.dmg locations vary by P4V version and CPU type:
\verbatim
Universal binaries, starting with 9.1
//depot/p09.1/p4-bin/bin.macosx104u/nightly/P4V.dmg

Intel
//depot/p08.2/p4-bin/bin.macosx104x86/nightly/P4V.dmg
//depot/r08.2/p4-bin/bin.macosx104x86/P4V.dmg
//depot/r08.1/p4-bin/bin.macosx104x86/P4V.dmg
//depot/r07.3/p4-bin/bin.macosx104x86/P4V.dmg
//depot/r07.2/p4-bin/bin.macosx104x86/P4V.dmg
//depot/r07.1/p4-bin/bin.macosx104x86/P4V.dmg

Motorola
//depot/p08.2/p4-bin/bin.macosx104ppc/nightly/P4V.dmg
//depot/r08.2/p4-bin/bin.macosx104ppc/P4V.dmg
//depot/r08.1/p4-bin/bin.macosx104ppc/P4V.dmg
//depot/r07.3/p4-bin/bin.macosx104ppc/P4V.dmg
//depot/r07.2/p4-bin/bin.macosx104ppc/P4V.dmg
//depot/r07.1/p4-bin/bin.macosx104ppc/P4V.dmg
\endverbatim

You do not need anything in the <tt>lib.macosx*</tt> directories.

\section running_nightlies_win_91 Windows, version 9.1

P4V and P4Merge require an installer. P4V and P4Merge require several DLLs and resource files. The installer supplies these files.

Version 9.1 does not permit running P4V or P4Merge directly out of the nightly folder.

-# Open p4vinst.exe installer.
-# Install p4v.
-# Run p4v.exe

\verbatim
//depot/p09.1/p4-bin/bin.ntx86/nightly/p4vinst.exe
\endverbatim

\section running_nightlies_win_91dev Windows, version 9.1, developer instructions

You <em>can</em> run P4V and P4Merge directly out of the nightly folder, with a bit of setup.

-# Build and copy images.rcc into p4-bin/nightly/
-# In your p4 client, plus-map lib.ntx86/... over bin.ntx86/nightly/...
-# p4 sync lib.ntx86/... and bin.ntx86/nightly/...
-# open p4v.exe or p4merge.exe.





\section running_nightlies_win_82 Windows, versions 8.2 and earlier

-# Open p4v.exe or p4merge.exe

p4v.exe locations vary by P4V version. There are no 64-bit builds of P4V or P4Merge: use the 32-bit builds.
\verbatim
//depot/p09.1/p4-bin/bin.ntx86/nightly/p4v.exe
//depot/p08.2/p4-bin/bin.ntx86/nightly/p4v.exe
//depot/r08.2/p4-bin/bin.ntx86/p4v.exe
//depot/r08.1/p4-bin/bin.ntx86/p4v.exe
//depot/r07.3/p4-bin/bin.ntx86/p4v.exe
//depot/r07.2/p4-bin/bin.ntx86/p4v.exe
//depot/r07.1/p4-bin/bin.ntx86/p4v.exe
\endverbatim


Starting with 9.1, you can no longer launch p4v.exe or p4merge.exe directly out of the bin.


*/

