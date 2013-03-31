/*
	This module defines the return codes and some useful macros used across all other modules in the library.
	Most functions also set errno accordingly when returning an error.
*/

#ifndef __MACLEARN_H__
#define __MACLEARN_H__

#include <errno.h>

/* Return codes used by all functions */
#define ML_OK					0				/*			No Error */
#define ML_WARN_EOF				1				/*			End of file reached */
#define ML_ERR_OUTOFMEMORY		-1				/* ENOMEM	System ran Out of Memory */
#define ML_ERR_FILENOTFOUND		-2				/* ENOENT	File not found or error opening the file */
#define ML_ERR_FILE				-3				/* EIO		Error while reading/writing to the file */
#define ML_ERR_PARAM			-4				/* EINVAL	Invalid parameter(s) passed to the function */
#define ML_ERR_NOTIMPLEMENTED	-5				/* ENOSYS	Function not implemented. Most likely caused by calling a function on an "abstract" structure */

/* Define "PRIVATE", "PROTECTED" and "PUBLIC" tags. They don't DO anything, they are just informative */
#define PRIVATE
#define PROTECTED
#define PUBLIC

/* Defines utility max and min macros, if not yet defined */
#if !defined(max) && !defined(WIN32)
	#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

#if !defined(min) && !defined(WIN32)
	#define min(x,y) ((x) < (y) ? (x) : (y))
#endif

/* Define TRUE and FALSE */
#ifndef TRUE
	#define TRUE		1
	#define FALSE		0
#endif


#endif