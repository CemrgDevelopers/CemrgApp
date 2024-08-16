/*=========================================================================BSD 3-Clause License

Copyright (c) 2020, OpenHeartDevelopers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

/*=========================================================================
*
* CemrgApp Plugin - Cemrg Atrial Strain Motion
*
* Cardiac Electromechanics Research Group
* http://www.cemrgapp.com
* info@cemrgapp.com 
*
* This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
* 
=========================================================================*/

#include "kcl_cemrgapp_atrialstrainmotion_Activator.h"
#include "AtrialStrainMotionView.h"

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

//Micro services
#include <usModuleRegistry.h>
#ifdef _WIN32
// _WIN32 = we're in windows
#include <winsock2.h>
#else
// or linux/mac
#include <arpa/inet.h>
#endif

//VTK
#include <vtkFieldData.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkBooleanOperationPolyDataFilter.h>
#include <vtkClipPolyData.h>
#include <vtkDecimatePro.h>

//ITK
#include <itkAddImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>
#include "itkLabelObject.h"
#include "itkLabelMap.h"
#include "itkLabelImageToLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"
#include "itkLabelSelectionLabelMapFilter.h"

// Qt
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDirIterator>
#include <QDate>
#include <QFile>

// mitk
#include <QmitkIOUtil.h>
#include <mitkImage.h>
#include <mitkLog.h>
#include <mitkProgressBar.h>
#include <mitkIDataStorageService.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkDataStorageEditorInput.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkBoundingObject.h>
#include <mitkCuboid.h>
#include <mitkAffineImageCropperInteractor.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkUnstructuredGrid.h>
#include <mitkManualSegmentationToSurfaceFilter.h>

//CemrgAppModule
#include <CemrgCommonUtils.h>
#include <CemrgCommandLine.h>
#include <CemrgMeasure.h>
#include <CemrgScar3D.h>


const std::string AtrialStrainMotionView::VIEW_ID = "org.mitk.views.atrialstrainmotionview";

void AtrialStrainMotionView::SetFocus()
{
  m_Controls.segment_extract->setFocus();
}

void AtrialStrainMotionView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  // Step 1: Segment and Extract
  connect(m_Controls.segment_extract, SIGNAL(clicked()), this, SLOT(SegmentExtract()));


  // Set default variables
  atrium = std::unique_ptr<CemrgAtrialTools>(new CemrgAtrialTools());
}

void AtrialStrainMotionView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                                const QList<mitk::DataNode::Pointer> &nodes)
{
  // iterate all selected objects, adjust warning visibility
  foreach (mitk::DataNode::Pointer node, nodes)
  {
    if (node.IsNotNull() && dynamic_cast<mitk::Image *>(node->GetData()))
    {
      m_Controls.labelWarning->setVisible(false);
      m_Controls.segment_extract->setEnabled(true);
      return;
    }
  }

  m_Controls.labelWarning->setVisible(true);
  m_Controls.segment_extract->setEnabled(false);
}

void AtrialStrainMotionView::DoImageProcessing()
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  mitk::DataNode *node = nodes.front();

  if (!node)
  {
    // Nothing selected. Inform the user and return
    QMessageBox::information(nullptr, "Template", "Please load and select an image before starting image processing.");
    return;
  }

  // here we have a valid mitk::DataNode

  // a node itself is not very useful, we need its data item (the image)
  mitk::BaseData *data = node->GetData();
  if (data)
  {
    // test if this data item is an image or not (could also be a surface or something totally different)
    mitk::Image *image = dynamic_cast<mitk::Image *>(data);
    if (image)
    {
      std::stringstream message;
      std::string name;
      message << "Performing image processing for image ";
      if (node->GetName(name))
      {
        // a property called "name" was found for this DataNode
        message << "'" << name << "'";
      }
      message << ".";
      MITK_INFO << message.str();

      // actually do something here...
    }
  }
}

bool AtrialStrainMotionView::RequestProjectDirectoryFromUser() {

  bool succesfulAssignment = true;

  //Ask the user for a dir to store data
  if (directory.isEmpty()) {

    MITK_INFO << "Directory is empty. Requesting user for directory.";
    directory = QFileDialog::getExistingDirectory( NULL, "Open Project Directory",
        mitk::IOUtil::GetProgramPath().c_str(),QFileDialog::ShowDirsOnly|QFileDialog::DontUseNativeDialog);

    MITK_INFO << ("Directory selected:" + directory).toStdString();
    atrium->SetWorkingDirectory(directory);

    if (directory.isEmpty() || directory.simplified().contains(" ")) {
      MITK_WARN << "Please select a project directory with no spaces in the path!";
      QMessageBox::warning(NULL, "Attention", "Please select a project directory with no spaces in the path!");
      directory = QString();
      succesfulAssignment = false;
    }//_if

    if (succesfulAssignment){
      QString now = QDate::currentDate().toString(Qt::ISODate);
      QString logfilename = directory + "/afib_log" + now + ".log";
      std::string logfname_str = logfilename.toStdString();

      mitk::LoggingBackend::SetLogFile(logfname_str.c_str());
      MITK_INFO << ("Changed logfile location to: " + logfilename).toStdString();
    }

  } else {
    MITK_INFO << ("Project directory already set: " + directory).toStdString();
  }//_if


  return succesfulAssignment;
}

void AtrialStrainMotionView::SegmentExtract() {

    MITK_INFO << "[AnalysisChoice]";
    bool testRpdfu = RequestProjectDirectoryFromUser();
    MITK_INFO(testRpdfu) << "Should continue";
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.
    MITK_INFO << "After directory selection checkup";

    // TODO: select the first image and segment it
    QString nifti_dir = directory + "/nifti/";
    QString input_file = nifti_dir + "dcm-0.nii";
    std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());

    MITK_INFO(QFile::copy(input_file, directory + "/dcm-0.nii")) << "QFile::Copy successful";
    // MITK_INFO << successful;

    // get pathToNifti file!
    QFileInfo file(input_file);

    cmd->SetUseDockerContainers(true);
    QString pathToSegmentation = cmd->DockerCctaMultilabelSegmentation(directory, directory + "/dcm-0.nii", false);
    MITK_INFO << pathToSegmentation;
    // TODO: build a docker image for extracting LA




}