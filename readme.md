# REVISION NOTES 

## V1.0 
* Support for solis invertV1.1. 
* Added editable config file for portability
* Added parsing of config file
* Added support for U16 and U32 registers.
## V1.2 
* Added support for detailed error handling & error module
* Added support for parsing json file (beta)
* Added support for other endien (big/little) register
* Added support for reading register map from json (beta)
* Shifted core functionality of modbus to dll file for version control. 
## V1.3 
* Bug fix while reading little endien register
* Bug fix while parsing incomplete config file
changed time format to UTC with Z terminator
* Bug fix in file interval - Active throughout the valid minute
## V1.4 
* Added plant code to config file.
* Added plant code to logfileName
* Cleaned and optimised token validation function for json parser
* Bug fix while printing parity to console window
* Recombiled for standard UCRT windows runtime library. removed support for windows 8 and below.
## V2.0 
* Added functionality for polling multiple devices
* Added preprocessor directives for hidding nonAPI function from dll export (demotivate dll injection attack)
* Removed asset ID from config file and use slave ID instead as device Identification.
## V2.1 
* Added verbose mode by using debug mode in modbus library. divert stderr to logfile.
* Modified printModaloConfig, printModalo & printOnly function to print to given FILE DESCRIPTOR
* Recombiled libmodbus.dll with DLL_EXPORT defined to remove unused functions from exporting.
* Add float register type to map.json (U32,U16, IEEE-float32) maintaining backward compatibility with older versions of config and map files
## VX.X (pending)
* WINAPI main for hiding console window and using window UI
preprocessor directives for supporting verbose mode
* Implement log file name as dynamically allocated and only pointer part of device config
* Support for IEEE float32. 
* UI editor for configuration files and map files
