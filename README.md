# bigDecrypter
Decrypts Homeworld Remastered .big files into a usable format

##Download:
[Stable release (1.1)](https://github.com/mon/bigDecrypter/releases/tag/v1.1)

##Usage:
`bigDecrypter.exe inputBig.big outputBig.big`

The output big is identical in format to Homeworld 2 (it is the same engine, after all), so use any of the many existing tools to extract it. If you install the Remastered tools, Archive.exe will do it for you. Go to Steam, install the Homeworld Remastered Toolkit:

`<HOMEWORLD_REMASTERED_INSTALL>\GBXTools\WorkshopTool\Archive.exe -e C:\Destination\Directory -a decrypted.big`

##Compiling:
Nothing special here. I use mingw gcc, pick your favourite C compiler.

`gcc -Wall bigDecrypter.c -o bigDecrypter.exe`
