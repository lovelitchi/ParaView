/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContour.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVContour.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkPVSourceInterface.h"
#include "vtkContourFilter.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabel.h"
#include "vtkPVData.h"
#include "vtkPVApplication.h"

int vtkPVContourCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContour::vtkPVContour()
{
  this->CommandFunction = vtkPVContourCommand;
  
  this->ContourValuesLabel = vtkKWLabel::New();
  this->ContourValuesList = vtkKWListBox::New();
  this->NewValueFrame = vtkKWWidget::New();
  this->NewValueLabel = vtkKWLabel::New();
  this->NewValueEntry = vtkKWEntry::New();
  this->AddValueButton = vtkKWPushButton::New();
  this->DeleteValueButton = vtkKWPushButton::New();
  this->ComputeNormalsCheck = vtkPVLabeledToggle::New();
  this->ComputeGradientsCheck = vtkPVLabeledToggle::New();
  this->ComputeScalarsCheck = vtkPVLabeledToggle::New();
  
  this->ScalarRangeLabel = vtkKWLabel::New();

  this->ReplaceInputOff();

}

//----------------------------------------------------------------------------
vtkPVContour::~vtkPVContour()
{
  this->ContourValuesLabel->Delete();
  this->ContourValuesLabel = NULL;
  this->ContourValuesList->Delete();
  this->ContourValuesList = NULL;
  this->NewValueLabel->Delete();
  this->NewValueLabel = NULL;
  this->NewValueEntry->Delete();
  this->NewValueEntry = NULL;
  this->AddValueButton->Delete();
  this->AddValueButton = NULL;
  this->NewValueFrame->Delete();
  this->NewValueFrame = NULL;
  this->DeleteValueButton->Delete();
  this->DeleteValueButton = NULL;
  this->ComputeNormalsCheck->Delete();
  this->ComputeNormalsCheck = NULL;
  this->ComputeGradientsCheck->Delete();
  this->ComputeGradientsCheck = NULL;
  this->ComputeScalarsCheck->Delete();
  this->ComputeScalarsCheck = NULL;
  this->ScalarRangeLabel->Delete();
  this->ScalarRangeLabel = NULL;
}

//----------------------------------------------------------------------------
vtkPVContour* vtkPVContour::New()
{
  return new vtkPVContour();
}

//----------------------------------------------------------------------------
void vtkPVContour::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
  
  this->ContourValuesLabel->SetParent(this->GetParameterFrame()->GetFrame());
  this->ContourValuesLabel->Create(pvApp, "");
  this->ContourValuesLabel->SetLabel("Contour Values");
  
  this->ContourValuesList->SetParent(this->GetParameterFrame()->GetFrame());
  this->ContourValuesList->Create(pvApp, "");
  this->ContourValuesList->SetHeight(5);
  this->ContourValuesList->SetBalloonHelpString("List of the current contour values");
  this->Script("bind %s <Delete> {%s DeleteValueCallback}",
               this->ContourValuesList->GetWidgetName(),
               this->GetTclName());
  // We need focus for delete binding.
  this->Script("bind %s <Enter> {focus %s}",
               this->ContourValuesList->GetWidgetName(),
               this->ContourValuesList->GetWidgetName());

  this->AcceptCommands->AddString("%s ContourValuesAcceptCallback",
                                  this->GetTclName());
  this->ResetCommands->AddString("%s ContourValuesResetCallback",
                                 this->GetTclName());
  
  this->NewValueFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->NewValueFrame->Create(pvApp, "frame", "");
  
  this->ScalarRangeLabel->SetParent(this->GetParameterFrame()->GetFrame());
  this->ScalarRangeLabel->Create(pvApp, "");
  
  this->Script("pack %s %s %s %s", this->ScalarRangeLabel->GetWidgetName(),
               this->ContourValuesLabel->GetWidgetName(),
               this->ContourValuesList->GetWidgetName(),
               this->NewValueFrame->GetWidgetName());
  
  this->NewValueLabel->SetParent(this->NewValueFrame);
  this->NewValueLabel->Create(pvApp, "");
  this->NewValueLabel->SetLabel("New Value:");
  this->NewValueLabel->SetBalloonHelpString("Enter a new contour value");
  
  this->NewValueEntry->SetParent(this->NewValueFrame);
  this->NewValueEntry->Create(pvApp, "");
  this->NewValueEntry->SetValue("");
  this->NewValueEntry->SetBalloonHelpString("Enter a new contour value");
  this->Script("bind %s <KeyPress-Return> {%s AddValueCallback}",
               this->NewValueEntry->GetWidgetName(),
               this->GetTclName());

  
  this->AddValueButton->SetParent(this->NewValueFrame);
  this->AddValueButton->Create(pvApp, "-text {Add Value}");
  this->AddValueButton->SetCommand(this, "AddValueCallback");
  this->AddValueButton->SetBalloonHelpString("Add the new contour value to the contour values list");
  
  this->Script("pack %s %s %s -side left",
               this->NewValueLabel->GetWidgetName(),
               this->NewValueEntry->GetWidgetName(),
               this->AddValueButton->GetWidgetName());
  
  this->DeleteValueButton->SetParent(this->GetParameterFrame()->GetFrame());
  this->DeleteValueButton->Create(pvApp, "-text {Delete Value}");
  this->DeleteValueButton->SetCommand(this, "DeleteValueCallback");
  this->DeleteValueButton->SetBalloonHelpString("Remove the currently selected contour value from the list");
  
  this->ComputeNormalsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->ComputeNormalsCheck->SetPVSource(this);
  this->ComputeNormalsCheck->Create(pvApp, "Compute Normals",
                                    "SetComputeNormals", "GetComputeNormals",
                                    "Select whether to compute normals",
                                    this->GetVTKSourceTclName());
  this->ComputeNormalsCheck->SetState(1);
  this->Widgets->AddItem(this->ComputeNormalsCheck);

  this->ComputeGradientsCheck->SetParent(this->GetParameterFrame()->GetFrame());

  this->ComputeGradientsCheck->SetPVSource(this);
  this->ComputeGradientsCheck->Create(pvApp, "Compute Gradients",
                                      "SetComputeGradients",
                                      "GetComputeGradients",
                                      "Select whether to compute gradients",
                                      this->GetVTKSourceTclName());
  this->ComputeGradientsCheck->SetState(0);
  this->Widgets->AddItem(this->ComputeGradientsCheck);
  
  this->ComputeScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());

  this->ComputeScalarsCheck->SetPVSource(this);
  this->ComputeScalarsCheck->Create(pvApp, "ComputeScalars",
                                    "SetComputeScalars", "GetComputeScalars",
                                    "Select whether to compute scalars",
                                    this->GetVTKSourceTclName());
  this->ComputeScalarsCheck->SetState(1);
  this->Widgets->AddItem(this->ComputeScalarsCheck);

  this->Script("pack %s %s %s %s -anchor w -padx 10",
               this->DeleteValueButton->GetWidgetName(),
               this->ComputeNormalsCheck->GetWidgetName(),
               this->ComputeGradientsCheck->GetWidgetName(),
               this->ComputeScalarsCheck->GetWidgetName());
}

void vtkPVContour::AddValueCallback()
{
  if (strcmp(this->NewValueEntry->GetValue(), "") == 0)
    {
    return;
    }

  this->ContourValuesList->AppendUnique(this->NewValueEntry->GetValue());
  this->NewValueEntry->SetValue("");
  this->ChangeAcceptButtonColor();
}

void vtkPVContour::DeleteValueCallback()
{
  int index;
  
  index = this->ContourValuesList->GetSelectionIndex();
  this->ContourValuesList->DeleteRange(index, index);
  this->ChangeAcceptButtonColor();
}

void vtkPVContour::AddValue(char *val)
{
  this->ContourValuesList->AppendUnique(val);
}

void vtkPVContour::DeleteAllValues()
{
  this->ContourValuesList->DeleteAll();
}

void vtkPVContour::ContourValuesAcceptCallback()
{
  int i;
  float value;
  int numContours = this->ContourValuesList->GetNumberOfItems();
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetNumberOfContours %d",
                         this->GetVTKSourceTclName(), numContours);
  
  for (i = 0; i < numContours; i++)
    {
    value = atof(this->ContourValuesList->GetItem(i));
    pvApp->BroadcastScript("%s SetValue %d %f",
                           this->GetVTKSourceTclName(),
                           i, value);
    }
}

void vtkPVContour::ContourValuesResetCallback()
{
  int i;
  vtkContourFilter* contour =
    (vtkContourFilter*)this->GetVTKSource();
  int numContours = contour->GetNumberOfContours();
  char newValue[128];
  
  this->ContourValuesList->DeleteAll();
  
  for (i = 0; i < numContours; i++)
    {
    sprintf(newValue, "%f", contour->GetValue(i));
    this->ContourValuesList->AppendUnique(newValue);
    }
}

void vtkPVContour::UpdateScalars()
{
  float range[2];
  char label[100];
  // call the superclass method
  this->vtkPVSource::UpdateScalars();
  
  this->GetDataArrayRange(range);
  if (range[0] == 1.0 && range[1] == 0.0)
    {
    sprintf(label, "Invalid Data Range");
    }
  else
    {
    sprintf(label, "Data Range: %f to %f", range[0], range[1]);
    }
  this->ScalarRangeLabel->SetLabel(label);
}

void vtkPVContour::GetDataArrayRange(float range[2])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int id, num;
  vtkDataArray *array;
  float temp[2];
  
  range[0] = 1.0;
  range[1] = 0.0;
  if (! this->DefaultScalarsName)
    {
    return;
    }
  
  pvApp->BroadcastScript("Application SendDataArrayRange %s %s",
                         this->GetNthPVInput(0)->GetVTKDataTclName(),
                         this->DefaultScalarsName);
  array = this->GetNthPVInput(0)->GetVTKData()->GetPointData()->
                  GetArray(this->DefaultScalarsName);

  if (array != NULL)
    {
    array->GetRange(range, 0);
    }
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; id++)
    {
    controller->Receive(temp, 2, id, 1976);
    // try to protect against invalid ranges.
    if (range[0] > range[1])
      {
      range[0] = temp[0];
      range[1] = temp[1];
      }
    else if (temp[0] < temp[1])
      {
      if (temp[0] < range[0])
        {
        range[0] = temp[0];
        }
      if (temp[1] > range[1])
        {
        range[1] = temp[1];
        }
      }
    }
}

void vtkPVContour::SaveInTclScript(ofstream* file)
{
  char* tempName;
  int i;
  vtkContourFilter *source =
    (vtkContourFilter*)this->GetVTKSource();
  vtkPVSourceInterface *pvsInterface =
    this->GetNthPVInput(0)->GetPVSource()->GetInterface();
  
  if (this->DefaultScalarsName)
    {
    *file << "vtkFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n\t"
          << this->ChangeScalarsFilterTclName << " SetInput [";
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      delete [] dataName;
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    *file << this->ChangeScalarsFilterTclName
          << " SetInputFieldToPointDataField\n";
    *file << this->ChangeScalarsFilterTclName
          << " SetOutputAttributeDataToPointData\n";
    *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 0 "
          << this->DefaultScalarsName << " 0\n\n";
    }

  *file << this->VTKSource->GetClassName() << " "
        << this->VTKSourceTclName << "\n";

  *file << "\t" << this->VTKSourceTclName << " SetInput [";

  if (!this->DefaultScalarsName)
    {
    if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                               "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetNthPVInput(0)->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetNthPVInput(0)->GetVTKDataTclName());
      
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      delete [] dataName;
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    }
  else
    {
    *file << this->ChangeScalarsFilterTclName << " GetOutput]\n\t";
    }
  
  *file << this->VTKSourceTclName << " SetNumberOfContours "
        << source->GetNumberOfContours() << "\n\t";
  
  for (i = 0; i < source->GetNumberOfContours(); i++)
    {
    *file << this->VTKSourceTclName << " SetValue " << i << " "
          << source->GetValue(i) << "\n\t";
    }

  *file << this->VTKSourceTclName << " SetComputeNormals "
        << this->ComputeNormalsCheck->GetState() << "\n\t"
        << this->VTKSourceTclName << " SetComputeGradients "
        << this->ComputeGradientsCheck->GetState() << "\n\t"
        << this->VTKSourceTclName << " SetComputeScalars "
        << this->ComputeScalarsCheck->GetState() << "\n\n";
  
  this->GetPVOutput(0)->SaveInTclScript(file, this->VTKSourceTclName);
}
