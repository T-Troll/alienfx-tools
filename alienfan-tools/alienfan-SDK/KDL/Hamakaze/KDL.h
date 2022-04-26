#pragma once
#include <wtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef KDL_EXPORTS
#define KDL_LIB_FUNCTION __declspec(dllexport)
#else
#define KDL_LIB_FUNCTION __declspec(dllimport)
#endif

	KDL_LIB_FUNCTION
		BOOLEAN
		APIENTRY
		LoadKernelDriver(
			LPWSTR DriverName,
			LPWSTR DriverDevice
		);

#ifdef __cplusplus
}
#endif