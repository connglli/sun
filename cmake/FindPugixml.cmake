# Try to find pugixml
find_path(PUGIXML_INCLUDE_DIR pugixml.hpp
  PATHS /usr/include /usr/local/include
)

if(PUGIXML_INCLUDE_DIR)
  set(PUGIXML_FOUND TRUE)
  message(STATUS "Found pugixml: ${PUGIXML_INCLUDE_DIR}")
else()
  set(PUGIXML_FOUND FALSE)
  message(STATUS "pugixml not found, will fetch from GitHub")
endif()
