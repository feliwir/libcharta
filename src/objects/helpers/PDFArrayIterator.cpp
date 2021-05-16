///////////////////////////////////////////////////////////////////////////////
// PDFArrayIterator.cpp    Stefan Woerthmueller 2014  StefanWoe@googlemail.com
///////////////////////////////////////////////////////////////////////////////

#include "PDFArrayIterator.h"
#include "PDFDictionaryIterator.h"

PDFDictionaryIterator PDFArrayIterator::DictAt(int ndx)
{
    if (mArray.GetPtr() == nullptr)
        return PDFDictionaryIterator(mParser);
    PDFObjectCastPtr<PDFIndirectObjectReference> foundReference(mArray->QueryObject(ndx));
    if (!foundReference)
        return PDFDictionaryIterator(mParser);

    PDFObjectCastPtr<PDFDictionary> catalog(mParser.ParseNewObject(foundReference->mObjectID));
    if (!catalog)
        return PDFDictionaryIterator(mParser);

    return PDFDictionaryIterator(mParser, catalog);
}

unsigned long PDFArrayIterator::GetLength()
{
    if (mArray.GetPtr() == nullptr)
        return 0;
    return mArray->GetLength();
}