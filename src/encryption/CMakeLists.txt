target_sources(libcharta PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MD5Generator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MD5Generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RC4.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RC4.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DecryptionHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EncryptionHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EncryptionOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XCryptionCommon.cpp
)