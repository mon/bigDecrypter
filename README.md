# bigDecrypter
Decrypts Homeworld Remastered .big files into a usable format

##Usage:
`bigDecrypter.exe inputBig outputBig`

The output big is identical in format to Homeworld 2 (it is the same engine, after all), so use any of the many existing tools to extract it. If you install the Remastered tools, Archive.exe will do it for you.

##Compiling:
Nothing special here. I use gcc, pick your favourite C compiler.

`gcc -Wall bigDecrypter.c -o bigDecrypter.exe`
