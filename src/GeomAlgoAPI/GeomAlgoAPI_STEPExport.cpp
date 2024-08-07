// Copyright (C) 2014-2025  CEA, EDF
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

#include <GeomAlgoAPI_STEPExport.h>

#include "GeomAlgoAPI_Tools.h"

#include <GeomAPI_Shape.h>
#include <ModelAPI_ResultBody.h>

#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <StepData_StepModel.hxx>
#include <UnitsMethods.hxx>
#include <Interface_Static.hxx>

#include <Basics_OCCTVersion.hxx>

bool STEPExport(const std::string& theFileName,
                const std::list<std::shared_ptr<GeomAPI_Shape> >& theShapes,
                const std::list<std::shared_ptr<ModelAPI_Result> >& theResults,
                std::string& theError)
{
  theError = "";
  // prepare XCAF document to store the whole data structure there
  Handle(TDocStd_Document) aDoc = new TDocStd_Document("BinXCAF");

  GeomAlgoAPI_Tools::AttributeExport_Tools::GeomAlgoAPI_Attributes anAttrs(aDoc->Main());

  std::list<std::shared_ptr<GeomAPI_Shape> >::const_iterator aShape = theShapes.cbegin();
  std::list<std::shared_ptr<ModelAPI_Result> >::const_iterator aResult = theResults.cbegin();
  for (; aShape != theShapes.cend(); aShape++, aResult++) {
    TDF_Label aNullLab;
    if (aResult->get() && !(*aShape)->isSame((*aResult)->shape()))
    { // simple sub-shape
      GeomAlgoAPI_Tools::AttributeExport_Tools::getAttributes(*aResult, anAttrs);
      GeomAlgoAPI_Tools::AttributeExport_Tools::exportShape(*aShape, anAttrs, GeomShapePtr(), aNullLab);
    }
    else if (aResult->get()) { // whole result selection
      GeomAlgoAPI_Tools::AttributeExport_Tools::putResult(*aResult, GeomShapePtr(), aNullLab, anAttrs);
    }
    else { // feature selection => export simple shape
      GeomAlgoAPI_Tools::AttributeExport_Tools::exportShape(*aShape, anAttrs, GeomShapePtr(), aNullLab);
    }
  }
  // store the XCAF document to STEP file
  try {
    GeomAlgoAPI_Tools::Localizer aLocalizer; // Set "C" numeric locale to save numbers correctly

#if OCC_VERSION_LARGE < 0x07070000
    STEPCAFControl_Writer aWriter;
    Interface_Static::SetCVal("xstep.cascade.unit", "M");
    Interface_Static::SetIVal("write.step.nonmanifold", 0); // 1 don't allow to export assemly tree
    Interface_Static::SetCVal("write.step.unit", "M");
#elif OCC_VERSION_LARGE < 0x07080000
    STEPCAFControl_Writer aWriterTmp;
    Interface_Static::SetCVal("xstep.cascade.unit", "M");
    Interface_Static::SetIVal("write.step.nonmanifold", 0); // 1 don't allow to export assemly tree
    Interface_Static::SetCVal("write.step.unit", "M");
    STEPCAFControl_Writer aWriter;
#else
    STEPCAFControl_Writer aWriter;
    Interface_Static::SetCVal("xstep.cascade.unit", "M");
    Interface_Static::SetIVal("write.step.nonmanifold", 0); // 1 don't allow to export assemly tree
    Interface_Static::SetCVal("write.step.unit", "M");
    Handle(StepData_StepModel) aModel = aWriter.ChangeWriter().Model();
    aModel->InternalParameters.InitFromStatic();
    Standard_Integer aWriteUnitInt = Interface_Static::IVal("write.step.unit");
    Standard_Real aWriteUnitReal = UnitsMethods::GetLengthFactorValue(aWriteUnitInt);
    aModel->SetWriteLengthUnit(aWriteUnitReal);
#endif

    auto aStatus = aWriter.Transfer(aDoc, STEPControl_AsIs);
    if (aStatus == IFSelect_RetDone)
      aStatus = aWriter.Write(theFileName.c_str());
    if (aStatus != IFSelect_RetDone)
      theError = "STEP Export failed";
  }
  catch (Standard_Failure&) {
    theError = "Exception catched in STEPExport";
  }
  return theError.empty();
}
