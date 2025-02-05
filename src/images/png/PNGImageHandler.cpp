/*
   Source File : PNGImageHandler.cpp


   Copyright 2011 Gal Kahana PDFWriter

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
#ifndef LIBCHARTA_NO_PNG

#include "images/png/PNGImageHandler.h"
#include "DictionaryContext.h"
#include "DocumentContext.h"
#include "EStatusCode.h"
#include "ObjectsContext.h"
#include "PDFFormXObject.h"
#include "PDFImageXObject.h"
#include "PDFStream.h"
#include "ProcsetResourcesConstants.h"
#include "SafeBufferMacrosDefs.h"
#include "Trace.h"
#include "XObjectContentContext.h"
#include "io/InputStringBufferStream.h"
#include "io/OutputStreamTraits.h"
#include "io/OutputStringBufferStream.h"
#include <png.h>

#include <list>
#include <stdlib.h>

using PDFImageXObjectList = std::list<PDFImageXObject *>;

charta::PNGImageHandler::PNGImageHandler()
{
    mObjectsContext = nullptr;
    mDocumentContext = nullptr;
}

void charta::PNGImageHandler::SetOperationsContexts(DocumentContext *inDocumentContext,
                                                    ObjectsContext *inObjectsContext)
{
    mObjectsContext = inObjectsContext;
    mDocumentContext = inDocumentContext;
}

static const std::string scType = "Type";
static const std::string scXObject = "XObject";
static const std::string scSubType = "Subtype";
static const std::string scImage = "Image";
static const std::string scWidth = "Width";
static const std::string scHeight = "Height";
static const std::string scColorSpace = "ColorSpace";
static const std::string scDeviceGray = "DeviceGray";
static const std::string scDeviceRGB = "DeviceRGB";
static const std::string scBitsPerComponent = "BitsPerComponent";
static const std::string scSMask = "SMask";
PDFImageXObject *CreateImageXObjectForData(png_structp png_ptr, png_infop info_ptr, png_bytep row,
                                           ObjectsContext *inObjectsContext)
{
    PDFImageXObjectList listOfImages;
    PDFImageXObject *imageXObject = nullptr;
    std::shared_ptr<PDFStream> imageStream = nullptr;
    charta::EStatusCode status = charta::eSuccess;

    do
    {

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            // reset failure pointer
            status = charta::eFailure;
            break;
        }

        // get info
        png_uint_32 transformed_width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 transformed_height = png_get_image_height(png_ptr, info_ptr);
        png_byte transformed_color_type = png_get_color_type(png_ptr, info_ptr);
        png_byte transformed_bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        png_byte channels_count = png_get_channels(png_ptr, info_ptr);
        ObjectIDType imageXObjectObjectId = inObjectsContext->GetInDirectObjectsRegistry().AllocateNewObjectID();
        // this image has only the color components, use to flag to determine what to ignore
        bool isAlpha = (transformed_color_type & PNG_COLOR_MASK_ALPHA) != 0;
        png_byte colorComponents = isAlpha ? (channels_count - 1) : channels_count;
        ObjectIDType imageMaskObjectId =
            isAlpha ? inObjectsContext->GetInDirectObjectsRegistry().AllocateNewObjectID() : 0;
        MyStringBuf alphaComponentsData;

        inObjectsContext->StartNewIndirectObject(imageXObjectObjectId);
        DictionaryContext *imageContext = inObjectsContext->StartDictionary();

        // type
        imageContext->WriteKey(scType);
        imageContext->WriteNameValue(scXObject);

        // subtype
        imageContext->WriteKey(scSubType);
        imageContext->WriteNameValue(scImage);

        // Width
        imageContext->WriteKey(scWidth);
        imageContext->WriteIntegerValue(transformed_width);

        // Height
        imageContext->WriteKey(scHeight);
        imageContext->WriteIntegerValue(transformed_height);

        // Bits Per Component
        imageContext->WriteKey(scBitsPerComponent);
        imageContext->WriteIntegerValue(transformed_bit_depth);

        // Color Space
        imageContext->WriteKey(scColorSpace);
        imageContext->WriteNameValue(1 == colorComponents ? scDeviceGray : scDeviceRGB);

        // Mask in case of Alpha
        if (isAlpha)
        {
            imageContext->WriteKey(scSMask);
            imageContext->WriteNewObjectReferenceValue(imageMaskObjectId);
        }

        // now for the image
        imageStream = inObjectsContext->StartPDFStream(imageContext);
        charta::IByteWriter *writerStream = imageStream->GetWriteStream();

        png_uint_32 y = transformed_height;

        if (isAlpha)
        {
            charta::OutputStringBufferStream alphaWriteStream(&alphaComponentsData);

            while (y-- > 0)
            {
                // read (using "rectangle" method)
                png_read_row(png_ptr, nullptr, row);
                // write. iterate per sample, splitting color components and alpha
                for (png_uint_32 i = 0; i < transformed_width; ++i)
                {

                    // note that we're writing by color components, but multiply by channel...that's casue we're
                    // skipping alpha
                    writerStream->Write((uint8_t *)(row + i * channels_count), colorComponents);

                    // write out to alpha stream as well (hummfff i don't like this...but i like less to decode the png
                    // again.... alpha is the last byte, so offset by color components
                    alphaWriteStream.Write((uint8_t *)(row + i * channels_count + colorComponents), 1);
                }
            }
        }
        else
        {
            while (y-- > 0)
            {
                // read
                png_read_row(png_ptr, row, nullptr);
                // write
                writerStream->Write((uint8_t *)(row), transformed_width * colorComponents);
            }
        }

        inObjectsContext->EndPDFStream(imageStream);

        // if there's a soft mask, write it now
        if (isAlpha)
        {
            inObjectsContext->StartNewIndirectObject(imageMaskObjectId);
            DictionaryContext *imageMaskContext = inObjectsContext->StartDictionary();

            // type
            imageMaskContext->WriteKey(scType);
            imageMaskContext->WriteNameValue(scXObject);

            // subtype
            imageMaskContext->WriteKey(scSubType);
            imageMaskContext->WriteNameValue(scImage);

            // Width
            imageMaskContext->WriteKey(scWidth);
            imageMaskContext->WriteIntegerValue(transformed_width);

            // Height
            imageMaskContext->WriteKey(scHeight);
            imageMaskContext->WriteIntegerValue(transformed_height);

            // Bits Per Component
            imageMaskContext->WriteKey(scBitsPerComponent);
            imageMaskContext->WriteIntegerValue(transformed_bit_depth);

            // Color Space
            imageMaskContext->WriteKey(scColorSpace);
            imageMaskContext->WriteNameValue(scDeviceGray);

            std::shared_ptr<PDFStream> imageMaskStream = inObjectsContext->StartPDFStream(imageMaskContext);
            charta::IByteWriter *writerMaskStream = imageMaskStream->GetWriteStream();

            // write the alpha samples
            charta::InputStringBufferStream alphaWriteStream(&alphaComponentsData);
            charta::OutputStreamTraits traits(writerMaskStream);
            traits.CopyToOutputStream(&alphaWriteStream);

            inObjectsContext->EndPDFStream(imageMaskStream);
        }

        imageXObject =
            new PDFImageXObject(imageXObjectObjectId, 1 == colorComponents ? KProcsetImageB : KProcsetImageC);
    } while (false);

    if (status == charta::eFailure)
    {
        delete imageXObject;
        imageXObject = nullptr;
    }

    return imageXObject;
}

PDFFormXObject *CreateImageFormXObjectFromImageXObject(const PDFImageXObjectList &inImageXObject,
                                                       ObjectIDType inFormXObjectID, png_uint_32 transformed_width,
                                                       png_uint_32 transformed_height,
                                                       charta::DocumentContext *inDocumentContext)
{
    PDFFormXObject *formXObject = nullptr;
    do
    {

        formXObject = inDocumentContext->StartFormXObject(PDFRectangle(0, 0, transformed_width, transformed_height),
                                                          inFormXObjectID);
        XObjectContentContext *xobjectContentContext = formXObject->GetContentContext();

        // iterate the images in the list and place one on top of each other
        auto it = inImageXObject.begin();
        for (; it != inImageXObject.end(); ++it)
        {
            xobjectContentContext->q();
            xobjectContentContext->cm(transformed_width, 0, 0, transformed_height, 0, 0);
            xobjectContentContext->Do(formXObject->GetResourcesDictionary().AddImageXObjectMapping(*it));
            xobjectContentContext->Q();
        }

        auto status = inDocumentContext->EndFormXObjectNoRelease(formXObject);
        if (status != charta::eSuccess)
        {
            TRACE_LOG("PNGImageHandler::CreateImageFormXObjectFromImageXObject. Unexpected Error, could not create "
                      "form XObject for image");
            delete formXObject;
            formXObject = nullptr;
            break;
        }

    } while (false);
    return formXObject;
}

void ReadDataFromStream(png_structp png_ptr, png_bytep data, png_size_t length)
{
    if (png_ptr == nullptr)
        return;

    auto *reader = (charta::IByteReaderWithPosition *)png_get_io_ptr(png_ptr);
    long long readBytes = reader->Read((uint8_t *)(data), length);

    if (readBytes != (long long)length)
        png_error(png_ptr, "Read Error");
}

void HandlePngError(png_structp png_ptr, png_const_charp error_message)
{
    {
        if (error_message != nullptr)
            TRACE_LOG1("LibPNG Error: %s", error_message);
    }
    png_longjmp(png_ptr, 1);
}

void HandlePngWarning(png_structp /*png_ptr*/, png_const_charp warning_message)
{
    if (warning_message != nullptr)
        TRACE_LOG1("LibPNG Warning: %s", warning_message);
}

PDFFormXObject *CreateFormXObjectForPNGStream(charta::IByteReaderWithPosition *inPNGStream,
                                              charta::DocumentContext *inDocumentContext,
                                              ObjectsContext *inObjectsContext, ObjectIDType inFormXObjectID)
{
    // Start reading image to get dimension. we'll then create the form, and then the image
    PDFFormXObject *formXObject = nullptr;
    PDFImageXObject *imageXObject = nullptr;
    PDFImageXObjectList listOfImages;
    charta::EStatusCode status = charta::eSuccess;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;
    png_bytep row = nullptr;

    do
    {
        // init structs and prep
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, HandlePngError, HandlePngWarning);
        if (png_ptr == nullptr)
        {
            break;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            status = charta::eFailure;
            break;
        }

        // pair png with custom IO (dont bother with writing)
        png_set_read_fn(png_ptr, (png_voidp)inPNGStream, ReadDataFromStream);

        // create info struct
        info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == nullptr)
        {
            png_error(png_ptr, "OOM allocating info structure");
        }

        // Gal: is this important?
        png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_ALWAYS, nullptr, 0);

        // read info from png
        png_read_info(png_ptr, info_ptr);

        png_byte color_type = png_get_color_type(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        // Let's setup some default transformations, and then reprint the png info that will
        // now adapt to post-translation

        // all them set_expand option, to bring us to a common 8 bits per component as a minimum
        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0u)
            png_set_tRNS_to_alpha(png_ptr);

        // now let's also avoid 16 bits for now, to stay always at 8 bits per component
        if (bit_depth == 16)
            png_set_strip_16(png_ptr);

        // and let's deal with random < 8 packing, so we're surely in 8 bits now
        if (bit_depth < 8)
            png_set_packing(png_ptr);

        // setup for potential interlace
        int passes = png_set_interlace_handling(png_ptr);

        // let's update info so now it fits the post transform data
        png_read_update_info(png_ptr, info_ptr);

        // grab updated info
        png_size_t transformed_rowbytes = png_get_rowbytes(png_ptr, info_ptr);
        png_uint_32 transformed_width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 transformed_height = png_get_image_height(png_ptr, info_ptr);

        // allocate reading info
        row = (png_bytep)malloc(transformed_rowbytes);

        if (row == nullptr)
            png_error(png_ptr, "OOM allocating row buffers");

        // K. time to start outputting something. do 1 for each pass, just in case we get

        // for each pass, create an image xobject.
        while (passes > 1)
        {
            // Gal: actually no.i jist need the last image.so skip the rest [till i find out otherwise...so i'm keeping
            // the rest of the code intact]
            png_uint_32 y = transformed_height;

            while (y-- > 0)
            {
                // read (using "rectangle" method)
                png_read_row(png_ptr, nullptr, row);
            }

            --passes;
        }

        while (passes-- > 0)
        {
            imageXObject = CreateImageXObjectForData(png_ptr, info_ptr, row, inObjectsContext);
            if (imageXObject == nullptr)
            {
                status = charta::eFailure;
                break;
            }

            listOfImages.push_back(imageXObject);
        }

        if (status == charta::eFailure)
        {
            break;
        }

        // finish reading image...no longer needed
        png_read_end(png_ptr, nullptr);

        // now let's get to the form, which should just place the image and be done
        formXObject = CreateImageFormXObjectFromImageXObject(listOfImages, inFormXObjectID, transformed_width,
                                                             transformed_height, inDocumentContext);
        if (formXObject == nullptr)
        {
            status = charta::eFailure;
        }
    } while (false);

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    if (row != nullptr)
        free(row);
    auto it = listOfImages.begin();
    for (; it != listOfImages.end(); ++it)
        delete *it;
    listOfImages.clear();
    if (status != charta::eSuccess)
    {
        delete formXObject;
        formXObject = nullptr;
    }

    return formXObject;
}

PDFFormXObject *charta::PNGImageHandler::CreateFormXObjectFromPNGStream(charta::IByteReaderWithPosition *inPNGStream,
                                                                        ObjectIDType inFormXObjectID)
{
    PDFFormXObject *imageFormXObject = nullptr;

    do
    {
        if ((mObjectsContext == nullptr) || (mObjectsContext == nullptr))
        {
            TRACE_LOG("PNGImageHandler::CreateFormXObjectFromPNGFile. Unexpected Error, mDocumentContex or "
                      "mObjectsContext not initialized");
            break;
        }

        imageFormXObject =
            CreateFormXObjectForPNGStream(inPNGStream, mDocumentContext, mObjectsContext, inFormXObjectID);
    } while (false);

    return imageFormXObject;
}

std::pair<double, double> charta::PNGImageHandler::ReadImageDimensions(charta::IByteReaderWithPosition *inPNGStream)
{
    return ReadImageInfo(inPNGStream).dimensions;
}

charta::PNGImageHandler::PNGImageInfo charta::PNGImageHandler::ReadImageInfo(
    charta::IByteReaderWithPosition *inPNGStream)
{
    // reading as is set by internal reader (meaning, post transformations)

    EStatusCode status = eSuccess;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;
    PNGImageHandler::PNGImageInfo data;

    do
    {
        // init structs and prep
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, HandlePngError, HandlePngWarning);
        if (png_ptr == nullptr)
        {
            break;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            status = eFailure;
            break;
        }

        // pair png with custom IO (dont bother with writing)
        png_set_read_fn(png_ptr, (png_voidp)inPNGStream, ReadDataFromStream);

        // create info struct
        info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == nullptr)
        {
            png_error(png_ptr, "OOM allocating info structure");
        }

        // Gal: is this important?
        png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_ALWAYS, nullptr, 0);

        // read info from png
        png_read_info(png_ptr, info_ptr);

        png_byte color_type = png_get_color_type(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        // Let's setup some default transformations, and then reprint the png info that will
        // now adapt to post-translation

        // all them set_expand option, to bring us to a common 8 bits per component as a minimum
        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0u)
            png_set_tRNS_to_alpha(png_ptr);

        // now let's also avoid 16 bits for now, to stay always at 8 bits per component
        if (bit_depth == 16)
            png_set_strip_16(png_ptr);

        // and let's deal with random < 8 packing, so we're surely in 8 bits now
        if (bit_depth < 8)
            png_set_packing(png_ptr);

        // let's update info so now it fits the post transform data
        png_read_update_info(png_ptr, info_ptr);

        // grab updated info
        png_uint_32 transformed_width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 transformed_height = png_get_image_height(png_ptr, info_ptr);
        png_byte transformed_color_type = png_get_color_type(png_ptr, info_ptr);
        png_byte channels_count = png_get_channels(png_ptr, info_ptr);
        bool isAlpha = (transformed_color_type & PNG_COLOR_MASK_ALPHA) != 0;
        png_byte colorComponents = isAlpha ? (channels_count - 1) : channels_count;

        data.colorComponents = colorComponents;
        data.hasAlpha = isAlpha;
        data.dimensions.first = transformed_width;
        data.dimensions.second = transformed_height;

    } while (false);

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return data;
}

#endif