cls
@echo OFF

:: combiling modbus library - uncomment only for recombiling dll library
gcc -I src/includes/ -o bin/libmodbus.dll -s -shared src/modbus.c src/modbus-rtu.c src/modbus-tcp.c src/modbus-data.c -lws2_32 -D DLLBUILD

:: combiling cJSON - JSON Parsing library -  - always recombileuncomment only for recombiling dll library
gcc -I src/includes/ -o bin/libcJSON.dll -s -shared src/cJSON.c

:: combiling library for error handling - always recombile
 gcc -I src/includes/ -o bin/liberror-modalo.dll -s -shared src/modaloError.c -D MODALO_EXPORT

:: combiling library for parsing - always recombile
gcc -L bin/ -I src/includes/ -o bin/libparse-modalo.dll -s -shared src/modaloParse.c -lcJSON -lerror-modalo -D CJSON_IMPORT_SYMBOLS -D MODALO_EXPORT

:: combiling main application modalo - always recombile
gcc -L bin/ -I src/includes/ -o bin/Modalo.exe -w -s src/main.c -lmodbus -lparse-modalo -lerror-modalo