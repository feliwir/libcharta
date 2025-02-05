/*
   Source File : ImagesAndFormsForwardReferenceTest.cpp


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
#include "IndirectObjectsReferenceRegistry.h"
#include "ObjectsContext.h"
#include "PDFFormXObject.h"
#include "PDFImageXObject.h"
#include "PDFPage.h"
#include "PDFWriter.h"
#include "PageContentContext.h"
#include "ProcsetResourcesConstants.h"
#include "TestHelper.h"
#include "XObjectContentContext.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace charta;

TEST(PDFImages, ImagesAndFormsForwardReferenceTest)
{
    PDFWriter pdfWriter;
    EStatusCode status;

    status = pdfWriter.StartPDF(RelativeURLToLocalPath(PDFWRITE_BINARY_PATH, "ImagesAndFormsForwardReferenceTest.pdf"),
                                ePDFVersion13);
    ASSERT_EQ(status, charta::eSuccess);

    PDFPage page;
    page.SetMediaBox(charta::PagePresets::A4_Portrait);

    PageContentContext *pageContentContext = pdfWriter.StartPageContentContext(page);
    ASSERT_NE(pageContentContext, nullptr);

    // continue page drawing size the image to 500,400
    pageContentContext->q();
    pageContentContext->cm(500, 0, 0, 400, 0, 0);

    ObjectIDType imageXObjectID = pdfWriter.GetObjectsContext().GetInDirectObjectsRegistry().AllocateNewObjectID();
    pageContentContext->Do(page.GetResourcesDictionary().AddImageXObjectMapping(imageXObjectID));

    // optionally i can also add the necessary PDF Procsets. i'll just add all that might be relevant
    page.GetResourcesDictionary().AddProcsetResource(KProcsetImageB);
    page.GetResourcesDictionary().AddProcsetResource(KProcsetImageC);
    page.GetResourcesDictionary().AddProcsetResource(KProcsetImageI);

    pageContentContext->Q();

    // continue page drawing size the image to 500,400
    pageContentContext->q();
    pageContentContext->cm(1, 0, 0, 1, 0, 400);
    ObjectIDType formXObjectID = pdfWriter.GetObjectsContext().GetInDirectObjectsRegistry().AllocateNewObjectID();
    pageContentContext->Do(page.GetResourcesDictionary().AddFormXObjectMapping(formXObjectID));
    pageContentContext->Q();

#ifndef LIBCHARTA_NO_TIFF
    pageContentContext->q();
    ObjectIDType tiffFormXObjectID = pdfWriter.GetObjectsContext().GetInDirectObjectsRegistry().AllocateNewObjectID();
    pageContentContext->Do(page.GetResourcesDictionary().AddFormXObjectMapping(tiffFormXObjectID));
    pageContentContext->Q();
#endif

    pageContentContext->q();
    pageContentContext->cm(1, 0, 0, 1, 100, 500);
    ObjectIDType simpleFormXObjectID = pdfWriter.GetObjectsContext().GetInDirectObjectsRegistry().AllocateNewObjectID();
    pageContentContext->Do(page.GetResourcesDictionary().AddFormXObjectMapping(simpleFormXObjectID));
    pageContentContext->Q();

    status = pdfWriter.EndPageContentContext(pageContentContext);
    ASSERT_EQ(status, charta::eSuccess);

    status = pdfWriter.WritePage(page);
    ASSERT_EQ(status, charta::eSuccess);

    // Create image xobject
    PDFImageXObject *imageXObject = pdfWriter.CreateImageXObjectFromJPGFile(
        RelativeURLToLocalPath(PDFWRITE_SOURCE_PATH, "data/images/otherStage.JPG"), imageXObjectID);
    ASSERT_NE(imageXObject, nullptr);

    // now create form xobject
    PDFFormXObject *formXObject = pdfWriter.CreateFormXObjectFromJPGFile(
        RelativeURLToLocalPath(PDFWRITE_SOURCE_PATH, "data/images/otherStage.JPG"), formXObjectID);
    ASSERT_NE(formXObject, nullptr);

#ifndef LIBCHARTA_NO_TIFF
    PDFFormXObject *tiffFormXObject = pdfWriter.CreateFormXObjectFromTIFFFile(
        RelativeURLToLocalPath(PDFWRITE_SOURCE_PATH, "data/images/tiff/jim___ah.tif"), tiffFormXObjectID);
    ASSERT_NE(tiffFormXObject, nullptr);
    delete tiffFormXObject;
#endif

    delete imageXObject;
    delete formXObject;

    // define an xobject form to draw a 200X100 points red rectangle
    PDFFormXObject *xobjectForm = pdfWriter.StartFormXObject(PDFRectangle(0, 0, 200, 100), simpleFormXObjectID);

    XObjectContentContext *xobjectContentContext = xobjectForm->GetContentContext();
    xobjectContentContext->q();
    xobjectContentContext->k(0, 100, 100, 0);
    xobjectContentContext->re(0, 0, 200, 100);
    xobjectContentContext->f();
    xobjectContentContext->Q();

    status = pdfWriter.EndFormXObjectAndRelease(xobjectForm);
    ASSERT_EQ(status, charta::eSuccess);

    status = pdfWriter.EndPDF();
    ASSERT_EQ(status, charta::eSuccess);
}