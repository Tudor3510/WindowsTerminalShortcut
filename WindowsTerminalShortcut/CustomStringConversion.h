#pragma once
#include <Windows.h>


// Returns a null terminating PSTR(char *) string
// The argument should be a null terminated PWSTR(wchar_t*) string
// The string buffer is allocated dynamically on the heap. In order to free memory allocated by this function you should call delete[].
// The length of the allocated buffer is exactly the length of the given string + 1(in order to put the null-terminating string)
// This functions assumes that the argument does not contain any non-ASCII characters.
PSTR wcharToChar(PWSTR stringToBeConverted);


// Returns a null terminating PWSTR(wchar_t *) string
// The argument should be a null terminated PSTR(char_t*) string
// The string buffer is allocated dynamically on the heap. In order to free memory allocated by this function you should call delete[].
// The length of the allocated buffer is exactly the length of the given string + 1(in order to put the null-terminating string)
PWSTR charToWchar(PSTR stringToBeConverted);