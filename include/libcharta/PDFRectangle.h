/*
   Source File : PDFRectangle.h


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
#pragma once

class PDFRectangle
{
  public:
    PDFRectangle() = default;
    PDFRectangle(const double inLowerLeftX, const double inLowerLeftY, const double inUpperRightX,
                 const double inUpperRightY);
    PDFRectangle(const PDFRectangle &inOther);

    bool operator==(const PDFRectangle &inOther) const;
    bool operator!=(const PDFRectangle &inOther) const;

    double LowerLeftX = 0;
    double LowerLeftY = 0;
    double UpperRightX = 0;
    double UpperRightY = 0;
};
