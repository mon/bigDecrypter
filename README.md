# bigDecrypter
Decrypts Homeworld Remastered .big files into a usable format

##Download:
[Stable release](https://github.com/mon/bigDecrypter/releases)

##Usage:
Drag your .big file onto `dropBigHere.bat`.
A file ending with _decrypted will be created in the same directory as the original.

Or use the command line:
`bigDecrypter.exe inputBig.big outputBig.big`

The output big is identical in format to Homeworld 2 (it is the same engine, after all), so use any of the many existing tools to extract it. If you install the Remastered tools, Archive.exe will do it for you. Go to Steam, install the Homeworld Remastered Toolkit:

`<HOMEWORLD_REMASTERED_INSTALL>\GBXTools\WorkshopTool\Archive.exe -e C:\Destination\Directory -a decrypted.big`

##Compiling:

At console, run `make`.
I use mingw gcc under cygwin, pick your favourite C compiler.
