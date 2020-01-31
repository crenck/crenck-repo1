                         Readme for P4Thumb,
                    The Helix Thumbnail Generator
                           Version 2017.3

Introduction

	P4Thumb, the Helix Thumbnail Generator, creates thumbnails of graphic 
	files managed by Helix Core and stores them in the Helix Versioning 
	Engine,	also called the Helix server. A thumbnail is a 160x160 pixel 
	PNG representation of the image. P4V displays the thumbnails in the 
	right pane when you choose View > Show Files As > (Large/Medium/Small)
	Thumbnail. P4V does not	communicate directly with P4Thumb; instead, 
	it obtains thumbnails from the server.
	
	By default, P4V displays thumbnails for image files located in the 
	workspace. If you install and configure P4Thumb, P4V can also display 
	thumbnails for any revisions of graphic files stored in the server.
    
    Before running P4Thumb, you need to create a dedicated client 
	(workspace). This client only maps directories containing image files
	that require thumbnails. A thumbnail is stored as a Revision attribute
	named "thumb". P4V recognizes this attribute. You can read this 
	attribute with the fstat function (for example: 
	p4 fstat -Oae -A thumb //depot/images/luftballon.jpg#1). 
    
	When you start P4Thumb against a server for the first time, it creates
	thumbnails for all graphic file versions in its client view and then
	adds thumbnails for any new image versions as they get added to the 
	depot. 
	
	Note: Make sure to start only one P4Thumb server. Starting 2 
	servers that map the same directories in their respective clients 
	would result in two competing P4Thumb servers managing the same	set
	of attributes.

--------------------------------------------------------------------------

Supported File Extension Formats
	
    If P4Thumb does not find a customer-provided conversion, it only 
    generates a thumbnail if the file extension is of a format 
	supported by Qt.
	
	Qt supports the following formats:
        * bmp
        * cur
        * gif
        * icns
        * ico
        * jpeg
        * jpg
        * pbm
        * pgm
        * png
        * ppm
        * svg
        * svgz
        * tga
        * tif
        * tiff
        * wbmp
        * webp
        * xbm
        * xpm

--------------------------------------------------------------------------

Configuring P4Thumb

    P4Thumb supports the following modes:

    * Using only the built-in conversion of Qt supported image formats.
 
      To support this mode, you need to provide a configuration file that defines
      all settings needed to run P4Thumb.

      The following code snippet is a configuration template for supporting 
      built-in formats only:
	
	  ===== from here
	 {
	 	 "settings": {
	 		 "connection": {
	 			 "client": “<workspace name>“,
				 "port": “<server:port>“,
				 "user": “<thumbnail user>”
			 },
			 "logFile": “<path to log file>”,
			 "changeListRange": {
				 "from": <lowest changelist #>,
				 "to": <highest changelist #>
			 },
			 "nativeSize": {
				 "height": <number of pixels default=160>,
				 "width": <number of pixels default=160>
			 },
			 "pollInterval": <number of seconds default=30>,
			 "maxFileSize" : <size in bytes default no-limit>
		 }
         }
	   ===== to here

        This configuration only includes the mandatory 'settings' block.

        You find an explanation of the settings after the following example of a configuration 
        file that adds customized conversions using 3rd party tools of your choice.

    * Overwriting or extending the built-in formats by adding your own conversions. 

	To create thumbnails, you can use external tools, such as ImageMagick,
	XnConvert, TinyPng, Imagine, or Image Tuner, provided the generated 
	thumbnails are in a format supported by Qt and stored as PNG files in
	the Helix Versioning Engine. 
	
	Using an external tool requires that you register a 'conversion' for
	a set of file extensions. A 'conversion' defines the executable to be
	called and the parameters required to perform the 
	transformation/resizing. If a revision's file extension matches a 
	registered file extension, P4Thumb uses the 'conversion' to generate 
	a thumbnail.
	   
	P4Thumb delivers the following example configuration files:
		* thumbconfig.json: Uses ImageMacgick and 	Ghostscript to extend 
		  P4Thumb for choosen extensions on Linux 
		* winconfig.json: Uses XnView on Windows. 
	
	You can also use your own tool to do the conversion for you. The only 
	requirement is that you provide $FILEIN and $FILEOUT as parameters.
	P4Thumb provides the value for $FILEIN (revision) and $FILEOUT (thumb).  

	The following code snippet is a configuration template:
	
	  ===== from here
	{
		"settings": {
			"connection": {
				"client": “<workspace name>“,
				"port": “<server:port>“,
				"user": “<thumbnail user>”
			},
			"logFile": “<path to log file>”,
			"changeListRange": {
				"from": <lowest changelist #>,
				"to": <highest changelist #>
			},
			"nativeSize": {
				"height": <number of pixels default=160>,
				"width": <number of pixels default=160>
			},
			"pollInterval": <number of seconds default=30>,
			"maxFileSize" : <size in bytes default no-limit>
		},
		"conversions": [
			{
				"extensions": [
					<file extension>,
					<file extension>
				],
				"execPath": "<CONVERSION EXECUTABLE>",
				"arguments": [
					“<argument>”,
					“<argument>“,
					"$FILEIN",
					"$FILEOUT"
				],
				"convertToPng": <true/false default=true>,
				"thumbnailExtension": <file extension>
			}
		]
	}
	  ===== to here

	A configuration includes the following blocks:
		1. Settings (mandatory block)
		2. Conversions (optional block, only to be used when using P4Thumb
           for non-Qt formats)

	settings::connection
		The Perforce connection and client used by P4Thumb.
	settings::logFile (optional)
		The log file used to output P4Thumb operations.
	settings::changeListRange (optional)
		Limits thumbnail generation to the range provided.
	settings::nativeSize (optional)
		Specifies the size of thumbnails. By default (if not set), the 
		thumbnail size of Qt formats is 160x160 pixels.
	settings::pollinterval
		Specifies how often P4Thumb polls the server for changes. By
		default (if not set), polling happens every 30 seconds.
	settings::maxFileSize
		If set, P4Thumb only creates thumbnails for revisions below the 
		specified maximum size (in bytes). If not set, P4Thumb creates 
		a thumbnail for every file, assuming it presents a supported format.

	conversions:extensions
		The file extensions handled by this conversion.
	conversions::execPath
		The executable being called.
	conversion::arguments
		The arguments needed. $FILEIN and $FILEOUT are required. P4Thumb
		uses them as variables.
	conversion::convertToPng (optional)
		If set to false (default), the thumbail extension is ".png". If 
		set to true because the conversion program cannot convert to .png,
		P4Thumb uses the thumbnail extension specified. 
	conversion::thumbnailExtension (optional)
		When convertToPng is set to true and this parameter is not set, the
		thumbnail extension is the source extension. Otherwise, if this
		parameter is set, the thumbnail extension is used. The generated 
		%FILEOUT% now has the specified extension, such as ".jpg".
		NOTE: The thumbnail extension needs to be a format supported by Qt.
		P4Thumb loads the file, converts it to ".png", and saves it as a 
		".png" thumbnail.

--------------------------------------------------------------------------
	
Starting P4Thumb

    Run one instance of P4Thumb for each server in which you want to 
	enable thumbnails. Do not run multiple instances of P4Thumb using the 
	same client view and server. (You might improve performance by running
	multiple instances with differing client views, for example one 
	instance for each large project.) 

    The user associated with P4Thumb:
		* Does not need to be dedicated to P4Thumb, but the workspace must
          be dedicated for use only	by P4Thumb. 
		* Must have 'admin' privileges on the server to run the daemon.

    Note: The first time you start P4Thumb for a specified Helix server, 
	it creates thumbnails for all images in its client view, which is 
	likely to affect server performance. After the initial thumbnails are 
	created, P4Thumb does not have a significant effect on server 
	performance.

    To start P4Thumb, issue the following command specifying the server 
    containing the images files, the workspace for P4Thumb to use, and 
    the Helix Core username associated with P4Thumb activity. 

	p4thumb  -c <config files> [options]

    Options

    -v              : Writes verbose status output to console; default is
					  silent
    -d              : Deletes thumbnails from server; must use with -n (a 
					  ChangeListRange in the configuration file)
    -f              : Forces to always write thumbnail; default skips if 
					  set
    -V              : Prints application version and exits
    -test <testFile>: Tests thumbnail conversion of testFile and exits

--------------------------------------------------------------------------
   
Stopping P4Thumb

    In the command window used to start P4Thumb, type CTRL-C. By default,
	when you restart P4Thumb, it resumes processing starting with the last
	changelist that was processed. P4Thumb maintains a counter for this 
	purpose on the specified server. Starting P4Thumb from a specified 
	changelist using the "-n" flag resets this counter if it exceeds the 
	specified value. Deleting thumbnails does not affect the counter.
