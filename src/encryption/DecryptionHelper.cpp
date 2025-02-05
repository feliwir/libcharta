/*
Source File : DecryptionHelper.cpp


Copyright 2016 Gal Kahana PDFWriter

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


*/
#include "encryption/DecryptionHelper.h"

#include "Trace.h"
#include "io/InputAESDecodeStream.h"
#include "io/InputRC4XcodeStream.h"
#include "io/InputStringStream.h"
#include "io/OutputStreamTraits.h"
#include "io/OutputStringBufferStream.h"
#include "objects/Deletable.h"
#include "objects/PDFArray.h"
#include "objects/PDFBoolean.h"
#include "objects/PDFDictionary.h"
#include "objects/PDFName.h"
#include "objects/PDFObject.h"
#include "objects/PDFObjectCast.h"
#include "objects/PDFStreamInput.h"
#include "objects/helpers/ParsedPrimitiveHelper.h"
#include "parsing/PDFParser.h"
#include <memory>
#include <utility>

using namespace std;
using namespace charta;

DecryptionHelper::DecryptionHelper()
{
    Reset();
}

void DecryptionHelper::Release()
{
    auto it = mXcrypts.begin();
    for (; it != mXcrypts.end(); ++it)
        delete it->second;
    mXcrypts.clear();
}

DecryptionHelper::~DecryptionHelper()
{
    Release();
}

void DecryptionHelper::Reset()
{
    mSupportsDecryption = false;
    mFailedPasswordVerification = false;
    mDidSucceedOwnerPasswordVerification = false;
    mIsEncrypted = false;
    mXcryptStreams = nullptr;
    mXcryptStrings = nullptr;
    mXcryptAuthentication = nullptr;
    mParser = nullptr;
    mDecryptionPauseLevel = 0;
    Release();
}

uint32_t ComputeLength(std::shared_ptr<charta::PDFObject> inLengthObject)
{
    ParsedPrimitiveHelper lengthHelper(std::move(inLengthObject));
    uint32_t value = lengthHelper.IsNumber() ? (uint32_t)lengthHelper.GetAsInteger() : 40;
    return value < 40 ? value : (value / 8); // this small check here is based on some errors i saw, where the length
                                             // was given in bytes instead of bits
}

XCryptionCommon *GetFilterForName(const StringToXCryptionCommonMap &inXcryptions, const string &inName)
{
    auto it = inXcryptions.find(inName);

    if (it == inXcryptions.end())
        return nullptr;
    return it->second;
}

static const string scStdCF = "StdCF";
EStatusCode DecryptionHelper::Setup(PDFParser *inParser, const string &inPassword)
{
    mSupportsDecryption = false;
    mFailedPasswordVerification = false;
    mDidSucceedOwnerPasswordVerification = false;
    mParser = inParser;

    // setup encrypted flag through the existance of encryption dict
    std::shared_ptr<charta::PDFDictionary> encryptionDictionary =
        std::static_pointer_cast<PDFDictionary>(inParser->QueryDictionaryObject(inParser->GetTrailer(), "Encrypt"));
    mIsEncrypted = encryptionDictionary != nullptr;

    do
    {
        if (!mIsEncrypted)
            break;

        PDFObjectCastPtr<charta::PDFName> filter(inParser->QueryDictionaryObject(encryptionDictionary, "Filter"));
        if (!filter || filter->GetValue() != "Standard")
        {
            // Supporting only standard filter
            if (!filter)
                TRACE_LOG("DecryptionHelper::Setup, no filter defined");
            else
                TRACE_LOG1("DecryptionHelper::Setup, Only Standard encryption filter is supported. Unsupported filter "
                           "encountered - %s",
                           filter->GetValue().substr(0, MAX_TRACE_SIZE - 200).c_str());
            break;
        }

        std::shared_ptr<charta::PDFObject> v(inParser->QueryDictionaryObject(encryptionDictionary, "V"));
        if (!v)
        {
            mV = 0;
        }
        else
        {
            ParsedPrimitiveHelper vHelper(v);
            if (!vHelper.IsNumber())
                break;
            mV = (uint32_t)vHelper.GetAsInteger();
        }

        // supporting versions 1,2 and 4
        if (mV != 1 && mV != 2 && mV != 4)
        {
            TRACE_LOG1(
                "DecryptionHelper::Setup, Only 1 and 2 are supported values for V. Unsupported filter encountered - %d",
                mV);
            break;
        }

        std::shared_ptr<charta::PDFObject> revision(inParser->QueryDictionaryObject(encryptionDictionary, "R"));
        if (!revision)
        {
            break;
        }
        else
        {
            ParsedPrimitiveHelper revisionHelper(revision);
            if (!revisionHelper.IsNumber())
                break;
            mRevision = (uint32_t)revisionHelper.GetAsInteger();
        }

        std::shared_ptr<charta::PDFObject> o(inParser->QueryDictionaryObject(encryptionDictionary, "O"));
        if (!o)
        {
            break;
        }
        else
        {
            ParsedPrimitiveHelper oHelper(o);
            mO = XCryptionCommon::stringToByteList(oHelper.ToString());
        }

        std::shared_ptr<charta::PDFObject> u(inParser->QueryDictionaryObject(encryptionDictionary, "U"));
        if (!u)
        {
            break;
        }
        else
        {
            ParsedPrimitiveHelper uHelper(u);
            mU = XCryptionCommon::stringToByteList(uHelper.ToString());
        }

        std::shared_ptr<charta::PDFObject> p(inParser->QueryDictionaryObject(encryptionDictionary, "P"));
        if (!p)
        {
            break;
        }
        else
        {
            ParsedPrimitiveHelper pHelper(p);
            if (!pHelper.IsNumber())
                break;
            mP = pHelper.GetAsInteger();
        }

        PDFObjectCastPtr<charta::PDFBoolean> encryptMetadata(
            inParser->QueryDictionaryObject(encryptionDictionary, "EncryptMetadata"));
        if (!encryptMetadata)
        {
            mEncryptMetaData = true;
        }
        else
        {
            mEncryptMetaData = encryptMetadata->GetValue();
        }

        // grab file ID from trailer
        mFileIDPart1 = ByteList();
        PDFObjectCastPtr<charta::PDFArray> idArray(inParser->QueryDictionaryObject(inParser->GetTrailer(), "ID"));
        if (!!idArray && idArray->GetLength() > 0)
        {
            std::shared_ptr<charta::PDFObject> idPart1Object(inParser->QueryArrayObject(idArray, 0));
            if (!!idPart1Object)
            {
                ParsedPrimitiveHelper idPart1ObjectHelper(idPart1Object);
                mFileIDPart1 = XCryptionCommon::stringToByteList(idPart1ObjectHelper.ToString());
            }
        }

        std::shared_ptr<charta::PDFObject> length(inParser->QueryDictionaryObject(encryptionDictionary, "Length"));
        if (!length)
        {
            mLength = 40 / 8;
        }
        else
        {
            mLength = ComputeLength(length);
        }

        // Setup crypt filters, or a default filter
        if (mV == 4)
        {
            // multiple xcryptions. read crypt filters, determine which does what
            PDFObjectCastPtr<charta::PDFDictionary> cryptFilters(
                inParser->QueryDictionaryObject(encryptionDictionary, "CF"));
            if (!!cryptFilters)
            {
                auto cryptFiltersIt = cryptFilters->GetIterator();

                // read crypt filters
                while (cryptFiltersIt.MoveNext())
                {
                    PDFObjectCastPtr<charta::PDFDictionary> cryptFilter;
                    // A little caveat of those smart ptrs need to be handled here
                    // make sure to pass the pointer after init...otherwise cast wont do addref
                    // and object will be released
                    cryptFilter = cryptFiltersIt.GetValue();
                    if (!!cryptFilter)
                    {
                        PDFObjectCastPtr<charta::PDFName> cfmName(inParser->QueryDictionaryObject(cryptFilter, "CFM"));
                        std::shared_ptr<charta::PDFObject> lengthObject(
                            inParser->QueryDictionaryObject(cryptFilter, "Length"));
                        uint32_t length = !lengthObject ? mLength : ComputeLength(lengthObject);

                        auto *encryption = new XCryptionCommon();
                        encryption->Setup(cfmName->GetValue() == "AESV2"); // singe xcryptions are always RC4
                        encryption->SetupInitialEncryptionKey(inPassword, mRevision, length, mO, mP, mFileIDPart1,
                                                              mEncryptMetaData);
                        mXcrypts.insert(
                            StringToXCryptionCommonMap::value_type(cryptFiltersIt.GetKey()->GetValue(), encryption));
                    }
                }

                PDFObjectCastPtr<charta::PDFName> streamsFilterName(
                    inParser->QueryDictionaryObject(encryptionDictionary, "StmF"));
                PDFObjectCastPtr<charta::PDFName> stringsFilterName(
                    inParser->QueryDictionaryObject(encryptionDictionary, "StrF"));
                mXcryptStreams =
                    GetFilterForName(mXcrypts, !streamsFilterName ? "Identity" : streamsFilterName->GetValue());
                mXcryptStrings =
                    GetFilterForName(mXcrypts, !stringsFilterName ? "Identity" : stringsFilterName->GetValue());
                mXcryptAuthentication = GetFilterForName(mXcrypts, scStdCF);
            }
        }
        else
        {
            // single xcryption, use as the single encryption source
            auto *defaultEncryption = new XCryptionCommon();
            defaultEncryption->Setup(false); // single xcryptions are always RC4
            defaultEncryption->SetupInitialEncryptionKey(inPassword, mRevision, mLength, mO, mP, mFileIDPart1,
                                                         mEncryptMetaData);

            mXcrypts.insert(StringToXCryptionCommonMap::value_type(scStdCF, defaultEncryption));
            mXcryptStreams = defaultEncryption;
            mXcryptStrings = defaultEncryption;
            mXcryptAuthentication = defaultEncryption;
        }

        // authenticate password, try to determine if user or owner
        ByteList password = XCryptionCommon::stringToByteList(inPassword);
        mDidSucceedOwnerPasswordVerification = AuthenticateOwnerPassword(password);
        mFailedPasswordVerification = !mDidSucceedOwnerPasswordVerification && !AuthenticateUserPassword(password);

        mSupportsDecryption = true;
    } while (false);

    return eSuccess;
}

bool DecryptionHelper::IsEncrypted() const
{
    return mIsEncrypted;
}

bool DecryptionHelper::SupportsDecryption() const
{
    return mSupportsDecryption;
}

bool DecryptionHelper::CanDecryptDocument() const
{
    return mSupportsDecryption && !mFailedPasswordVerification;
}

bool DecryptionHelper::DidFailPasswordVerification() const
{
    return mFailedPasswordVerification;
}

bool DecryptionHelper::DidSucceedOwnerPasswordVerification() const
{
    return mDidSucceedOwnerPasswordVerification;
}

static const string scEcnryptionKeyMetadataKey = "DecryptionHelper.EncryptionKey";

bool HasCryptFilterDefinition(PDFParser *inParser, const std::shared_ptr<charta::PDFStreamInput> &inStream)
{
    std::shared_ptr<charta::PDFDictionary> streamDictionary(inStream->QueryStreamDictionary());

    // check if stream has a crypt filter
    std::shared_ptr<charta::PDFObject> filterObject(inParser->QueryDictionaryObject(streamDictionary, "Filter"));
    if (!filterObject)
    {
        // no filter, so stop here
        return false;
    }

    if (filterObject->GetType() == PDFObject::ePDFObjectArray)
    {
        auto filterObjectArray = std::static_pointer_cast<charta::PDFArray>(filterObject);
        bool foundCrypt = false;
        for (unsigned long i = 0; i < filterObjectArray->GetLength() && !foundCrypt; ++i)
        {
            PDFObjectCastPtr<charta::PDFName> filterObjectItem(filterObjectArray->QueryObject(i));
            if (!filterObjectItem)
            {
                // error
                break;
            }
            foundCrypt = filterObjectItem->GetValue() == "Crypt";
        }
        return foundCrypt;
    }
    if (filterObject->GetType() == charta::PDFObject::ePDFObjectName)
    {
        return std::static_pointer_cast<charta::PDFName>(filterObject)->GetValue() == "Crypt";
    }
    return false; //???
}

charta::IByteReader *DecryptionHelper::CreateDefaultDecryptionFilterForStream(
    const std::shared_ptr<charta::PDFStreamInput> &inStream, charta::IByteReader *inToWrapStream)
{
    // This will create a decryption filter for streams that dont have their own defined crypt filters. null for no
    // decryption filter
    if (!IsEncrypted() || !CanDecryptDocument() || HasCryptFilterDefinition(mParser, inStream) ||
        (mXcryptStreams == nullptr))
        return nullptr;

    charta::IDeletable *savedEcnryptionKey = inStream->GetMetadata(scEcnryptionKeyMetadataKey);
    if (savedEcnryptionKey != nullptr)
    {
        return CreateDecryptionReader(inToWrapStream, *(((Deletable<ByteList> *)savedEcnryptionKey)->GetPtr()),
                                      mXcryptStreams->IsUsingAES());
    }
    return nullptr;
}

charta::IByteReader *DecryptionHelper::CreateDecryptionFilterForStream(
    const std::shared_ptr<charta::PDFStreamInput> &inStream, charta::IByteReader *inToWrapStream,
    const std::string &inCryptName)
{
    // note that here the original stream is returned instead of null
    if (!IsEncrypted() || !CanDecryptDocument())
        return inToWrapStream;

    charta::IDeletable *savedEcnryptionKey = inStream->GetMetadata(scEcnryptionKeyMetadataKey);
    if (savedEcnryptionKey == nullptr)
    {
        // sign for no encryption here
        return inToWrapStream;
    }
    XCryptionCommon *xcryption = GetFilterForName(mXcrypts, inCryptName);

    if ((xcryption != nullptr) && (savedEcnryptionKey != nullptr))
    {
        return CreateDecryptionReader(inToWrapStream, *(((Deletable<ByteList> *)savedEcnryptionKey)->GetPtr()),
                                      xcryption->IsUsingAES());
    }
    return inToWrapStream;
}

bool DecryptionHelper::IsDecrypting() const
{
    return IsEncrypted() && CanDecryptDocument() && mDecryptionPauseLevel == 0;
}

std::string DecryptionHelper::DecryptString(const std::string &inStringToDecrypt)
{
    if (!IsDecrypting() || (mXcryptStrings == nullptr))
        return inStringToDecrypt;

    charta::IByteReader *decryptStream = CreateDecryptionReader(
        new InputStringStream(inStringToDecrypt), mXcryptStrings->GetCurrentObjectKey(), mXcryptStrings->IsUsingAES());
    if (decryptStream != nullptr)
    {
        OutputStringBufferStream outputStream;
        OutputStreamTraits traits(&outputStream);
        traits.CopyToOutputStream(decryptStream);

        delete decryptStream;
        return outputStream.ToString();
    }
    return inStringToDecrypt;
}

void DecryptionHelper::OnObjectStart(long long inObjectID, long long inGenerationNumber)
{
    auto it = mXcrypts.begin();
    for (; it != mXcrypts.end(); ++it)
    {
        it->second->OnObjectStart(inObjectID, inGenerationNumber);
    }
}

XCryptionCommon *DecryptionHelper::GetCryptForStream(const std::shared_ptr<charta::PDFStreamInput> &inStream)
{
    // Get crypt for stream will return the right crypt filter thats supposed to be used for stream
    // whether its the default stream encryption or a specific filter defined in the stream
    // not the assumption (well, one that's all over) that if the name is not found in the CF dict, it
    // will be "identity" which is the same as providing NULL as the xcryptioncommon return value

    if (HasCryptFilterDefinition(mParser, inStream))
    {
        // find position of crypt filter, and get the name of the crypt filter from the decodeParams
        std::shared_ptr<charta::PDFDictionary> streamDictionary(inStream->QueryStreamDictionary());

        std::shared_ptr<charta::PDFObject> filterObject(mParser->QueryDictionaryObject(streamDictionary, "Filter"));
        if (filterObject->GetType() == PDFObject::ePDFObjectArray)
        {
            auto filterObjectArray = std::static_pointer_cast<charta::PDFArray>(filterObject);
            unsigned long i = 0;
            for (; i < filterObjectArray->GetLength(); ++i)
            {
                PDFObjectCastPtr<charta::PDFName> filterObjectItem(filterObjectArray->QueryObject(i));
                if (filterObjectItem->GetValue() == "Crypt")
                    break;
            }
            if (i < filterObjectArray->GetLength())
            {
                PDFObjectCastPtr<charta::PDFArray> decodeParams(
                    mParser->QueryDictionaryObject(streamDictionary, "DecodeParms"));
                if (!decodeParams)
                    return mXcryptStreams;
                // got index, look for the name in the decode params array
                PDFObjectCastPtr<charta::PDFDictionary> decodeParamsItem((mParser->QueryArrayObject(decodeParams, i)));
                if (!decodeParamsItem)
                    return mXcryptStreams;

                PDFObjectCastPtr<charta::PDFName> cryptFilterName(
                    mParser->QueryDictionaryObject(decodeParamsItem, "Name"));
                return GetFilterForName(mXcrypts, cryptFilterName->GetValue());
            }
            return mXcryptStreams; // this shouldn't realy happen
        }
        if (filterObject->GetType() == PDFObject::ePDFObjectName)
        {
            // has to be crypt filter, look for the name in decode params
            PDFObjectCastPtr<charta::PDFDictionary> decodeParamsItem(
                (mParser->QueryDictionaryObject(streamDictionary, "DecodeParms")));
            if (!decodeParamsItem)
                return mXcryptStreams;

            PDFObjectCastPtr<charta::PDFName> cryptFilterName(mParser->QueryDictionaryObject(decodeParamsItem, "Name"));
            return GetFilterForName(mXcrypts, cryptFilterName->GetValue());
        }
        return mXcryptStreams; // ???
    }

    return mXcryptStreams;
}

void DecryptionHelper::OnObjectEnd(const std::shared_ptr<charta::PDFObject> &inObject)
{
    if (inObject == nullptr)
        return;

    // for streams, retain the encryption key with them, so i can later decrypt them when needed
    if ((inObject->GetType() == PDFObject::ePDFObjectStream) && IsDecrypting())
    {
        XCryptionCommon *streamCryptFilter =
            GetCryptForStream(std::static_pointer_cast<charta::PDFStreamInput>(inObject));
        if (streamCryptFilter != nullptr)
        {
            auto *savedKey = new ByteList(streamCryptFilter->GetCurrentObjectKey());
            inObject->SetMetadata(scEcnryptionKeyMetadataKey, new Deletable<ByteList>(savedKey));
        }
    }

    auto it = mXcrypts.begin();
    for (; it != mXcrypts.end(); ++it)
    {
        it->second->OnObjectEnd();
    }
}

charta::IByteReader *DecryptionHelper::CreateDecryptionReader(charta::IByteReader *inSourceStream,
                                                              const ByteList &inEncryptionKey, bool inIsUsingAES)
{
    if (inIsUsingAES)
        return new InputAESDecodeStream(inSourceStream, inEncryptionKey);
    return new InputRC4XcodeStream(inSourceStream, inEncryptionKey);
}

bool DecryptionHelper::AuthenticateUserPassword(const ByteList &inPassword)
{
    if (mXcryptAuthentication == nullptr)
        return true;
    return mXcryptAuthentication->algorithm3_6(mRevision, mLength, inPassword, mO, mP, mFileIDPart1, mEncryptMetaData,
                                               mU);
}

bool DecryptionHelper::AuthenticateOwnerPassword(const ByteList &inPassword)
{
    if (mXcryptAuthentication == nullptr)
        return true;

    return mXcryptAuthentication->algorithm3_7(mRevision, mLength, inPassword, mO, mP, mFileIDPart1, mEncryptMetaData,
                                               mU);
}

uint32_t DecryptionHelper::GetLength() const
{
    return mLength;
}

uint32_t DecryptionHelper::GetV() const
{
    return mV;
}

uint32_t DecryptionHelper::GetRevision() const
{
    return mRevision;
}

long long DecryptionHelper::GetP() const
{
    return mP;
}

bool DecryptionHelper::GetEncryptMetaData() const
{
    return mEncryptMetaData;
}

const ByteList &DecryptionHelper::GetFileIDPart1() const
{
    return mFileIDPart1;
}

const ByteList &DecryptionHelper::GetO() const
{
    return mO;
}

const ByteList &DecryptionHelper::GetU() const
{
    return mU;
}

const ByteList &DecryptionHelper::GetInitialEncryptionKey() const
{
    return mXcryptAuthentication->GetInitialEncryptionKey();
}

void DecryptionHelper::PauseDecryption()
{
    ++mDecryptionPauseLevel;
}

void DecryptionHelper::ReleaseDecryption()
{
    --mDecryptionPauseLevel;
}

const StringToXCryptionCommonMap &DecryptionHelper::GetXcrypts() const
{
    return mXcrypts;
}

XCryptionCommon *DecryptionHelper::GetStreamXcrypt() const
{
    return mXcryptStreams;
}

XCryptionCommon *DecryptionHelper::GetStringXcrypt() const
{
    return mXcryptStrings;
}

XCryptionCommon *DecryptionHelper::GetAuthenticationXcrypt() const
{
    return mXcryptAuthentication;
}
