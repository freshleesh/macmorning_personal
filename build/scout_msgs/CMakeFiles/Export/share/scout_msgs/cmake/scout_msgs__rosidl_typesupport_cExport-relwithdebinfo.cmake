#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "scout_msgs::scout_msgs__rosidl_typesupport_c" for configuration "RelWithDebInfo"
set_property(TARGET scout_msgs::scout_msgs__rosidl_typesupport_c APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(scout_msgs::scout_msgs__rosidl_typesupport_c PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libscout_msgs__rosidl_typesupport_c.so"
  IMPORTED_SONAME_RELWITHDEBINFO "libscout_msgs__rosidl_typesupport_c.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS scout_msgs::scout_msgs__rosidl_typesupport_c )
list(APPEND _IMPORT_CHECK_FILES_FOR_scout_msgs::scout_msgs__rosidl_typesupport_c "${_IMPORT_PREFIX}/lib/libscout_msgs__rosidl_typesupport_c.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
