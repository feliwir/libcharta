target_sources(libcharta PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFArray.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFBoolean.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFDictionary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFHexString.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFIndirectObjectReference.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFInteger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFLiteralString.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFName.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFNull.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFReal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFStreamInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFSymbol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PDFPageInput.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/ParsedPrimitiveHelper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/ParsedPrimitiveHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/PDFArrayIterator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/PDFArrayIterator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/PDFDictionaryIterator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/PDFDictionaryIterator.cpp
)