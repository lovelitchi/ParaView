/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParaView.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkObject.h"
#include "vtkMultiProcessController.h"
#include "vtkPVApplication.h"
#include "vtkTclUtil.h"
#include "vtkToolkits.h"

// external global variable.
vtkMultiProcessController *VTK_PV_UI_CONTROLLER = NULL;


struct vtkPVArgs
{
  int argc;
  char **argv;
  int* RetVal;
};



void vtkPVSlaveScript(void *localArg, void *remoteArg, int remoteArgLength,
		      int remoteProcessId)
{
  vtkPVApplication *self = (vtkPVApplication *)(localArg);

  //cerr << " ++++ SlaveScript: " << ((char*)remoteArg) << endl;
  
  self->SimpleScript((char*)remoteArg);
}


// Each process starts with this method.  One process is designated as "master" 
// and starts the application.  The other processes are slaves to the application.
void Process_Init(vtkMultiProcessController *controller, void *arg )
{
  vtkPVArgs *pvArgs = (vtkPVArgs *)arg;
  int myId, numProcs;
  
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  if (myId ==  0)
    { // The last process is for UI.

    // We need to pass the local controller to the UI process.
    Tcl_Interp *interp = vtkPVApplication::InitializeTcl(pvArgs->argc,pvArgs->argv);
    
    // To bypass vtkKWApplicaion assigning vtkKWApplicationCommand
    // to the tcl command, create the application from tcl.
    if (Tcl_Eval(interp, "vtkPVApplication Application") != TCL_OK)
      {
      cerr << "Error returned from tcl script.\n" << interp->result << endl;
      }
    int    error;
    vtkPVApplication *app = (vtkPVApplication *)(
      vtkTclGetPointerFromObject("Application","vtkPVApplication",
				 interp,error));
    if (app == NULL)
      {
      vtkGenericWarningMacro("Could not get application pointer.");
      return;
      }  
    
    app->SetController(controller);
    app->Script("wm withdraw .");
    app->Start(pvArgs->argc,pvArgs->argv);
    int status = app->GetExitStatus();
    if (Tcl_Eval(interp, "Application Delete") != TCL_OK)
      {
      cerr << "Could not delete application.\n";
      *(pvArgs->RetVal) = -1;
      }
    else
      {
      *(pvArgs->RetVal) = status;
      }
    }
  else
    {
    // The slaves try to connect.  In the future, we may not want to initialize Tk.
    //putenv("DISPLAY=:0.0");

    vtkKWApplication::SetWidgetVisibility(0);
    Tcl_Interp *interp = vtkPVApplication::InitializeTcl(pvArgs->argc,pvArgs->argv);
    
    // We should use the application tcl name in the future.
    // All object in the satellite processes must be created through tcl.
    // (To assign the correct name).
    if (Tcl_Eval(interp, "vtkPVApplication Application") != TCL_OK)
      {
      cerr << "Error returned from tcl script.\n" << interp->result << endl;
      }
    int    error;
    vtkPVApplication *app = (vtkPVApplication *)(
      vtkTclGetPointerFromObject("Application","vtkPVApplication",
				 interp,error));
    if (app == NULL)
      {
      vtkGenericWarningMacro("Could not get application pointer.");
      return;
      }  
    app->SetController(controller);
    controller->AddRMI(vtkPVSlaveScript, (void *)(app), VTK_PV_SLAVE_SCRIPT_RMI_TAG);
    controller->ProcessRMIs();
    }
}

#ifdef _WIN32
#include <windows.h>

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nShowCmd)
{
  int argc, retVal=0;
  char **argv;
  vtkPVArgs pvArgs;

  unsigned int i;
  int j;
  // parse a few of the command line arguments
  // a space delimites an argument except when it is inside a quote

  argc = 1;
  int pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        i++;
        pos++;
        }
      argc++;
      pos = 0;
      }
    }

  argv = (char**)malloc(sizeof(char*)* (argc+1));

  argv[0] = (char*)malloc(1024);
  ::GetModuleFileName(0, argv[0],1024);

  for(j=1; j<argc; j++)
    {
    argv[j] = (char*)malloc(strlen(lpCmdLine)+10);
    }
  argv[argc] = 0;

  argc = 1;
  pos = 0;
  for (i = 0; i < strlen(lpCmdLine); i++)
    {
    while (lpCmdLine[i] == ' ' && i < strlen(lpCmdLine))
      {
      i++;
      }
    if (lpCmdLine[i] == '\"')
      {
      i++;
      while (lpCmdLine[i] != '\"' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    else
      {
      while (lpCmdLine[i] != ' ' && i < strlen(lpCmdLine))
        {
        argv[argc][pos] = lpCmdLine[i];
        i++;
        pos++;
        }
      argv[argc][pos] = '\0';
      argc++;
      pos = 0;
      }
    }
  argv[argc] = 0;

  vtkMultiProcessController *controller = vtkMultiProcessController::New();
  controller->Initialize(&argc, (char***)(&argv));

#ifndef VTK_USE_MPI
  controller->SetNumberOfProcesses(1);
#endif
  
  if (controller->GetNumberOfProcesses() > 1)
    {
    controller->CreateOutputWindow();
    }

  pvArgs.argc = argc;
  pvArgs.argv = argv;
  pvArgs.RetVal = &retVal;
  controller->SetSingleMethod(Process_Init, (void *)(&pvArgs));
  controller->SingleMethodExecute();

  controller->Finalize();
  controller->Delete();
  
  for(j=0; j<argc; j++)
    {
    free(argv[j]);
    }
  free(argv);

  Tcl_Finalize();
  return retVal;;
}
#else
int main(int argc, char *argv[])
{

  vtkMultiProcessController *controller = vtkMultiProcessController::New();  
  controller->Initialize(&argc, &argv);

  int retVal = 0;
  // New processes need these args to initialize.
  vtkPVArgs pvArgs;
  pvArgs.argc = argc;
  pvArgs.argv = argv;
  pvArgs.RetVal = &retVal;


#ifdef VTK_USE_MPI

  controller->CreateOutputWindow();


  controller->SetSingleMethod(Process_Init, (void *)(&pvArgs));
  controller->SingleMethodExecute();
  
  controller->Finalize();
  controller->Delete();

#else
  controller->SetNumberOfProcesses(1);
  vtkMultiProcessController::SetGlobalController(controller);
  Process_Init(controller, (void *)(&pvArgs)); 
  controller->Delete();
#endif
  
  return retVal;
}
#endif
