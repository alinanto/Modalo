#ifndef H_ERROR_MODALO
#define H_ERROR_MODALO

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
  /* You should define MODALO_EXPORT *only* when building the DLL. */
  #ifdef MODALO_EXPORT
    #define MODALO_API __declspec(dllexport)
  #else
    #define MODALO_API __declspec(dllimport)
  #endif
  /* Define calling convention in one place, for convenience. */
  #define MODALO_CALL __stdcall
#else /* _WIN32 not defined. */
  /* Define with no value on non-Windows OSes. */
  #define MODALO_EXPORT
  #define MODALO_CALL
#endif

#define MODALO_ERROR_MAXLENGTH 256

#define EMODBUS_INIT 1
#define EPARSE_CONFIG_STRING 2
#define EPARSE_CONFIG_FILE 3
#define EVALIDATE_PARAMETER 4
#define EFILE_BUFFER 5
#define EPARSE_CJSON_FILE 6
#define EPARSE_CJSON_STRING 7
#define ELOG_FILE 8
#define EMODBUS_READ 9

// API functions (not accessible by including header and linking library)
MODALO_API void MODALO_CALL modaloSetLastError(int errorCode, char * LastError);
MODALO_API void MODALO_CALL modaloPrintLastError();


// non API functions
char* modaloGetErrorType();

#ifdef __cplusplus
}
#endif


#endif
