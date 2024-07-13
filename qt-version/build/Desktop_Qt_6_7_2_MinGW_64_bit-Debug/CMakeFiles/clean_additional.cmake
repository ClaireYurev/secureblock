# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\SecureBlock_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\SecureBlock_autogen.dir\\ParseCache.txt"
  "SecureBlock_autogen"
  )
endif()
