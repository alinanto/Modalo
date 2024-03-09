#include "modaloError.h"
#include <string.h> // for strcpy,memset
#include <stdio.h> // for printf

char modaloLastError[MODALO_ERROR_MAXLENGTH] = ""; // buffer to hold error string
int modaloErrorCode = 0;

char *modaloGetErrorType()
{
  switch(modaloErrorCode)
  {
    case EMODBUS_INIT: return "MODBUS INITIALSATION ERROR";
    case EPARSE_CONFIG_STRING: return "CONFIG PARSE ERROR";
    case EPARSE_CONFIG_FILE: return "CONFIG FILE ERROR";
    case EVALIDATE_PARAMETER: return "CONFIG FILE VALIDATION ERROR";
    case EFILE_BUFFER: return "FILE BUFFER ERROR";
    case EPARSE_CJSON_FILE: return "CJSON FILE ERROR";
    case EPARSE_CJSON_STRING: return "CJSON PARSE ERROR";
    case ELOG_FILE: return "LOG FILE ERROR";
    case EMODBUS_READ: return "MODBUS READ ERROR";

    default: return NULL;
  }
}

MODALO_API void MODALO_CALL modaloSetLastError(int errorCode, char * LastError)
{
  if(modaloGetErrorType!=NULL) modaloErrorCode=errorCode; //if valid error code store it
  memset((void*)modaloLastError,'\0',sizeof(char)*MODALO_ERROR_MAXLENGTH); // clear buffer
  if(strlen(LastError)<(MODALO_ERROR_MAXLENGTH-1)) //input less than buffer length?
    strcpy(modaloLastError,LastError); // copy error to buffer
  else strcpy(modaloLastError,"Error Buffer Overflowed!"); //default error
}

MODALO_API void MODALO_CALL modaloPrintLastError()
{
  printf("%s: %s\n",modaloGetErrorType(),modaloLastError);
  fprintf(stderr,"%s: %s\n",modaloGetErrorType(),modaloLastError);
}
