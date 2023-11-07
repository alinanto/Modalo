#! /bin/sh

clear

# combiling modbus library - uncomment only for recombiling dll library
#gcc -I includes/ -o bin/libmodbus.dll -s -shared src/modbus*.c -lws2_32 -Wl,--subsystem,windows

# combiling cJSON - JSON Parsing library -  - always recombileuncomment only for recombiling dll library
#gcc -I includes/ -o bin/libcJSON.dll -s -shared src/cJSON.c -Wl,--subsystem,windows

# combiling library for error handling - always recombile
gcc -I includes/ -o bin/liberror-modalo.dll -s -shared src/modaloError.c -D MODALO_EXPORT -Wl,--subsystem,windows

# combiling library for parsing - always recombile
gcc -L bin/ -I includes/ -o bin/libparse-modalo.dll -s -shared src/modaloParse.c -lcJSON -lerror-modalo -D CJSON_IMPORT_SYMBOLS -D MODALO_EXPORT -Wl,--subsystem,windows

# combiling main application modalo - always recombile
gcc -L bin/ -I includes/ -o bin/Modalo.exe -w -s src/main.c -lmodbus -lparse-modalo -lerror-modalo

#gcc -o bin/test.exe src/fileOut.c

cd bin/
./Modalo.exe
cd ../
