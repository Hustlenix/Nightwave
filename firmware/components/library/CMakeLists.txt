idf_component_register(SRCS "media_documents.cpp" "library_catalog.cpp" "catalog_lookup.cpp" INCLUDE_DIRS "include" REQUIRES heap decoder)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
