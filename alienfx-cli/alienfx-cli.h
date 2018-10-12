#pragma once

#include <string>  // std::wstring
#include <utility> // std::pair

// common definitions

// result type:
// first: true => success, false => failure
// second: status message (optional)
typedef std::pair<bool, std::wstring> ResultT;
