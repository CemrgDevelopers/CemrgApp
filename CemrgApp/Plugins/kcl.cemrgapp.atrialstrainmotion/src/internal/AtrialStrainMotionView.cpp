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
#include "AtrialFibresClipperView.h"

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
#include "CemrgMultilabelSegmentationUtils.h"


const std::string AtrialStrainMotionView::VIEW_ID = "org.mitk.views.atrialstrainmotionview";

void AtrialStrainMotionView::SetFocus()
{
  m_Controls.segment_extract->setFocus();
}

void AtrialStrainMotionView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  // 1: Segment and Extract
  connect(m_Controls.segment_extract, SIGNAL(clicked()), this, SLOT(SegmentExtract()));
  m_Controls.segment_extract->setStyleSheet("Text-align:left");
  // 2: Create Surface mesh and smooth it
  connect(m_Controls.surface_mesh_smooth, SIGNAL(clicked()), this, SLOT(SurfaceMeshSmooth()));
  m_Controls.surface_mesh_smooth->setStyleSheet("Text-align:left");
  // 3. UAC pipeline, starting from step 3
  connect(m_Controls.button_3_imanalysis, SIGNAL(clicked()), this, SLOT(AnalysisChoice()));
  m_Controls.button_3_imanalysis->setStyleSheet("Text-align:left");
  // 4. Post processing
  // m_Controls.button_man4_2_postproc->setVisible(false);
  connect(m_Controls.button_man4_2_postproc, SIGNAL(clicked()), this, SLOT(SegmentationPostprocessing()));
  m_Controls.button_man4_2_postproc->setStyleSheet("Text-align:left");
  // 5. Identify PVs
  connect(m_Controls.button_man5_idPV, SIGNAL(clicked()), this, SLOT(IdentifyPV()));
  m_Controls.button_man5_idPV->setStyleSheet("Text-align:left");
  // 6. Create Labelled Mesh
  connect(m_Controls.button_man6_labelmesh, SIGNAL(clicked()), this, SLOT(CreateLabelledMesh()));
  m_Controls.button_man6_labelmesh->setStyleSheet("Text-align:left");
  // 7. Mesh Preprocessing
  connect(m_Controls.button_auto4_meshpreproc, SIGNAL(clicked()), this, SLOT(MeshPreprocessing()));
  m_Controls.button_auto4_meshpreproc->setStyleSheet("Text-align:left");
  // 8. Verify Labels
  connect(m_Controls.button_0_3_checklabels, SIGNAL(clicked()), this, SLOT(UacCalculationVerifyLabels()));
  m_Controls.button_0_3_checklabels->setStyleSheet("Text-align:left");
  // 9. Clip PV
  connect(m_Controls.button_man8_clipPV, SIGNAL(clicked()), this, SLOT(ClipperPV()));
  m_Controls.button_man8_clipPV->setStyleSheet("Text-align:left");



  // Set default variables
  atrium = std::unique_ptr<CemrgAtrialTools>(new CemrgAtrialTools());
  tagName = "Labelled";
  uiSelector_pipeline = 0;
  uiSelector_imgauto_skipCemrgNet = false;
  uiSelector_imgauto_skipLabel = false;
  uiSelector_img_scar = false;
  uiSelector_man_useCemrgNet = false;

}

void AtrialStrainMotionView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                                const QList<mitk::DataNode::Pointer>& /*nodes*/)
{
  // iterate all selected objects, adjust warning visibility
  /*
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
  m_Controls.segment_extract->setEnabled(true);

   */
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

bool AtrialStrainMotionView::GetUserAnalysisSelectorInputs(){
    MITK_INFO << "[GetUserAnalysisSelectorInputs]";
    QString metadata = Path("UAC_CT/prodMetadata.txt");
    bool userInputAccepted=false;

    if(QFile::exists(metadata)){
        std::ifstream fi(metadata.toStdString());
        fi >> uiSelector_pipeline;
        fi >> uiSelector_imgauto_skipCemrgNet;
        fi >> uiSelector_imgauto_skipLabel;
        fi >> uiSelector_man_useCemrgNet;
        fi >> uiSelector_img_scar;
        fi.close();

        userInputAccepted=true;

        MITK_INFO(LoadSurfaceChecks()) << ("Loaded surface" + tagName).toStdString();

    }

    if(!userInputAccepted){
        uiSelector_img_scar = 0;
        uiSelector_pipeline = 2;
        uiSelector_img_scar = false;

        std::ofstream fo(metadata.toStdString());
        fo << uiSelector_pipeline << std::endl;
        fo << uiSelector_imgauto_skipCemrgNet << std::endl;
        fo << uiSelector_imgauto_skipLabel << std::endl;
        fo << uiSelector_man_useCemrgNet << std::endl;
        fo << uiSelector_img_scar << std::endl;
        fo.close();

        userInputAccepted=true;

    }
    SetLgeAnalysis(uiSelector_img_scar);

    return userInputAccepted;
}

int AtrialStrainMotionView::Ask(std::string title, std::string msg){
    return QMessageBox::question(NULL, title.c_str(), msg.c_str(), QMessageBox::Yes, QMessageBox::No);
}

bool AtrialStrainMotionView::LoadSurfaceChecks(){
    bool success = true;
    UserLoadSurface();
	QString prodPath = directory + "/UAC_CT/" + tagName + ".vtk";
    MITK_INFO << ("[LoadSurfaceChecks] Loading surface: " + prodPath).toStdString();

    return success;
}

void AtrialStrainMotionView::SetLgeAnalysis(bool b){
    analysisOnLge = b;
    // m_Controls.button_z_scar->setEnabled(b);
}

void AtrialStrainMotionView::CheckLoadedMeshQuality(){
    QString prodPath = directory + "/UAC_CT/";
    QString meshinput = prodPath + tagName + ".vtk";

    mitk::Surface::Pointer surface = mitk::IOUtil::Load<mitk::Surface>(meshinput.toStdString());

    vtkSmartPointer<vtkPolyDataConnectivityFilter> cf = vtkSmartPointer<vtkPolyDataConnectivityFilter>::New();
    cf->ScalarConnectivityOff();
    cf->SetInputData(surface->GetVtkPolyData());
    cf->SetExtractionModeToAllRegions();
    cf->Update();

    int numRegions = cf->GetNumberOfExtractedRegions();

    if(numRegions>1){
        resurfaceMesh = true;
        MITK_INFO << ("Number of regions in mesh: " + QString::number(numRegions)).toStdString();
        cf->SetExtractionModeToSpecifiedRegions();
        cf->Modified();
        cf->Update();

        for (int ix = 0; ix < numRegions; ix++) {

            cf->InitializeSpecifiedRegionList();
            cf->AddSpecifiedRegion(ix);
            cf->Modified();
            cf->Update();

            vtkSmartPointer<vtkPolyData> temp = cf->GetOutput();
            surface->SetVtkPolyData(cf->GetOutput());
            QString nameExt = "connectivityTest_"+QString::number(ix)+".vtk";
            mitk::IOUtil::Save(surface, (prodPath+nameExt).toStdString());
        }
    }
}

void AtrialStrainMotionView::SetAutomaticModeButtons(bool b){
    m_Controls.button_auto4_meshpreproc->setVisible(b);
    // m_Controls.button_auto5_clipPV->setVisible(b);

    if(b){
        // m_Controls.button_0_landmarks->setText("    Step6: Select Landmarks");
        // m_Controls.button_0_calculateUac->setText("    Step7: Calculate UAC");
        // m_Controls.button_0_fibreMapUac->setText("    Step8: Fibre Mapping");
    }
}

void AtrialStrainMotionView::UserLoadSurface(){
    QString newpath = directory + "/UAC_CT/" + tagName + ".vtk";
    SetTagNameFromPath(newpath);
    CheckLoadedMeshQuality();
}

QString AtrialStrainMotionView::GetFilePath(QString nameSubstring, QString extension){
    QString result = "";
    QDirIterator searchit(directory, QDirIterator::Subdirectories);
    while(searchit.hasNext()) {
        QFileInfo searchfinfo(searchit.next());
        if (searchfinfo.fileName().contains(extension, Qt::CaseSensitive)) {
            if (searchfinfo.fileName().contains((nameSubstring), Qt::CaseSensitive))
                result = searchfinfo.absoluteFilePath();
        }
    }//_while

    return result;
}

void AtrialStrainMotionView::SetTagNameFromPath(QString path){
    QFileInfo fi(path);
    tagName = fi.baseName();
}

void AtrialStrainMotionView::AutomaticAnalysis(){
    MITK_INFO << "TIMELOG|AutomaticAnalysis| Start";
    QString prodPath = directory + "/";
    if(cnnPath.isEmpty()){
        if(uiSelector_imgauto_skipCemrgNet){
            cnnPath = QFileDialog::getOpenFileName(NULL, "Open Automatic Segmentation file",
            directory.toStdString().c_str(), QmitkIOUtil::GetFileOpenFilterString());
            MITK_INFO << ("Loaded file: " + cnnPath).toStdString();
        } else{
            QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
            if (nodes.size() > 1) {
                QMessageBox::warning(NULL, "Attention",
                "Please select the CEMRA image from the Data Manager to segment!");
                return;
            }

            QString mraPath = prodPath + "test.nii";
            if(nodes.size() == 0){
                MITK_INFO << "Searching MRA files";

                QString imagePath = QFileDialog::getOpenFileName(NULL, "Open MRA image file",
                directory.toStdString().c_str(), QmitkIOUtil::GetFileOpenFilterString());

                if(!QFile::exists(mraPath)){
                    if(!QFile::copy(imagePath, mraPath)){
                        MITK_ERROR << "MRA could not be prepared for automatic segmentation.";
                        return;
                    }
                }
            } else if(nodes.size() == 1){
                mitk::BaseData::Pointer data = nodes.at(0)->GetData();
                bool successfulImageLoading = false;
                if (data) {
                    mitk::Image::Pointer image = dynamic_cast<mitk::Image*>(data.GetPointer());
                    if (image) {
                        mitk::IOUtil::Save(image, mraPath.toStdString());
                        successfulImageLoading = true;
                    }
                }

                if(!successfulImageLoading){
                    MITK_ERROR << "MRA image could not be loaded";
                    return;
                }
            }

            MITK_INFO << "[AUTOMATIC_PIPELINE] CemrgNet (auto segmentation)";
            std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
            cmd->SetUseDockerContainers(true);
            cnnPath = cmd->DockerCemrgNetPrediction(mraPath);
            remove(mraPath.toStdString().c_str());
        }
    }
    CemrgCommonUtils::RoundPixelValues(cnnPath);
    QFileInfo fi(cnnPath);
    if(directory.compare(fi.absolutePath())==0){
        MITK_INFO << ("[ATTENTION] Changing working directory to: " + fi.absolutePath());
        directory = fi.absolutePath();
    }
    MITK_INFO << ("Directory: "+ directory + " segmentation name: " + fi.baseName()).toStdString();

    MITK_INFO << "[AUTOMATIC_PIPELINE] Clean Segmentation";
    ImageType::Pointer segImage = atrium->CleanAutomaticSegmentation(directory, (fi.baseName()+".nii"), "prodClean.nii");
    cnnPath = prodPath + "LA.nii";
    mitk::IOUtil::Save(mitk::ImportItkImage(segImage), cnnPath.toStdString());
    cnnPath = UserIncludeLgeAnalysis(cnnPath, segImage);

    mitk::IOUtil::Load(cnnPath.toStdString(), *this->GetDataStorage());

    MITK_INFO << "[AUTOMATIC_PIPELINE] Assign PV labels automatically";
    ImageType::Pointer tagAtriumAuto = atrium->AssignAutomaticLabels(segImage, directory);

    MITK_INFO << "[AUTOMATIC_PIPELINE] Create Mesh";
    //Ask for user input to set the parameters
    bool userInputsAccepted = GetUserMeshingInputs();

    if(userInputsAccepted){
        std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
        cmd->SetUseDockerContainers(true);

        cmd->ExecuteSurf(directory, Path("prodClean.nii"), "close", uiMesh_iter, uiMesh_th, uiMesh_bl, uiMesh_smth);
        atrium->ProjectTagsOnExistingSurface(tagAtriumAuto, directory, tagName+".vtk");

        MITK_INFO << "[AUTOMATIC_PIPELINE] Add the mesh to storage";
        QString path = prodPath + tagName + ".vtk";

        std::cout << "Path to load: " << path.toStdString() <<'\n';
        std::cout << "tagName: " << tagName.toStdString() << '\n';
        mitk::Surface::Pointer surface = mitk::IOUtil::Load<mitk::Surface>(path.toStdString());

        std::string meshName = tagName.toStdString() + "-Mesh";
        CemrgCommonUtils::AddToStorage(surface, meshName, this->GetDataStorage());

        std::string msg = "Created labelled mesh - Complete\n\n";
        msg += "Automatic labels were assigned to the mesh. \n";
        msg += "To set default labels: \n\n";
        msg += "+ Identify the LAA and PVs in Step4: Mesh Preprocessing, and set the clippers.\n";
        msg += "+ Click Step5: Clip PVs and MV (this step will set the correct labels)";
        QMessageBox::information(NULL, "Automatic Labelling complete", msg.c_str());
    }
    MITK_INFO << "TIMELOG|AutomaticAnalysis| End";
}

QString AtrialStrainMotionView::UserIncludeLgeAnalysis(QString segPath, ImageType::Pointer segImage){
    // uiSelector_img_scar
    QString prodPath = directory + "/";
    QString resultString = segPath;

    // int replyLAreg = Ask("Register to LGE", "Move to LGE space and prepare for scar projection?");
    if(uiSelector_img_scar){
        QFileInfo fi(segPath);
        QString segRegPath = prodPath + fi.baseName() + "-reg." + fi.suffix();

        MITK_INFO << "search for lge and mra";
        QString lgePath = GetFilePath("dcm-LGE", ".nii");
        QString mraPath = GetFilePath("dcm-MRA", ".nii");

        if(lgePath.isEmpty()){
            lgePath = QFileDialog::getOpenFileName(NULL, "Open LGE image file",
            directory.toStdString().c_str(), QmitkIOUtil::GetFileOpenFilterString());
        }
        if(mraPath.isEmpty()){
            mraPath = QFileDialog::getOpenFileName(NULL, "Open MRA image file",
            directory.toStdString().c_str(), QmitkIOUtil::GetFileOpenFilterString());
        }
        // register and transform
        std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
        cmd->ExecuteRegistration(directory, lgePath, mraPath); // rigid.dof is the default name
        cmd->ExecuteTransformation(directory, segPath, segRegPath);

        segImage = atrium->LoadImage(segRegPath);

        resultString = segRegPath;
        tagName += "-reg";

        SetLgeAnalysis(true);
    }

    return resultString;
}

bool AtrialStrainMotionView::GetUserMeshingInputs(){
    bool userInputAccepted=false;

    if(userHasSetMeshingParams){
        QString msg = "The parameters have been set already, change them?\n";
        msg += "close iter= " + QString::number(uiMesh_iter) + '\n';
        msg += "threshold= " + QString::number(uiMesh_th) + '\n';
        msg += "blur= " + QString::number(uiMesh_bl) + '\n';
        msg += "smooth iter= " + QString::number(uiMesh_smth);

        if(Ask("Question", msg.toStdString())==QMessageBox::Yes){
            userHasSetMeshingParams=false;
            return GetUserMeshingInputs();
        } else{
            userInputAccepted=true;
        }
    } else{
        QDialog* inputs = new QDialog(0, Qt::WindowFlags());
        m_UIMeshing.setupUi(inputs);
        connect(m_UIMeshing.buttonBox, SIGNAL(accepted()), inputs, SLOT(accept()));
        connect(m_UIMeshing.buttonBox, SIGNAL(rejected()), inputs, SLOT(reject()));
        int dialogCode = inputs->exec();

        //Act on dialog return code
        if (dialogCode == QDialog::Accepted) {

            bool ok1, ok2, ok3, ok4;
            uiMesh_th= m_UIMeshing.lineEdit_1->text().toDouble(&ok1);
            uiMesh_bl= m_UIMeshing.lineEdit_2->text().toDouble(&ok2);
            uiMesh_smth= m_UIMeshing.lineEdit_3->text().toDouble(&ok3);
            uiMesh_iter= m_UIMeshing.lineEdit_4->text().toDouble(&ok4);

            //Set default values
            if (!ok1 || !ok2 || !ok3 || !ok4)
            QMessageBox::warning(NULL, "Attention", "Reverting to default parameters!");
            if (!ok1) uiMesh_th=0.5;
            if (!ok2) uiMesh_bl=0.0;
            if (!ok3) uiMesh_smth=10;
            if (!ok4) uiMesh_iter=0;

            inputs->deleteLater();
            userInputAccepted=true;
            userHasSetMeshingParams=true;

        } else if (dialogCode == QDialog::Rejected) {
            inputs->close();
            inputs->deleteLater();
        }//_if
    }

    return userInputAccepted;
}

bool AtrialStrainMotionView::GetUserUacOptionsInputs(bool enableFullUiOptions){
    QString metadata = Path("UAC_CT/prodUacMetadata.txt");
    bool userInputAccepted=false;

    if(uiUac_fibreFile.size() == 0){
        uiUac_fibreFile << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "A" << "L";
        uiUac_whichAtrium << "LA" << "RA";
        uiUac_surftype << "Endo" << "Epi" << "Bilayer";
    }

    if(enableFullUiOptions && QFile::exists(metadata)){
        int reply0 = Ask("Metadata Found", "Load previous analysis metadata found?");
        if(reply0==QMessageBox::Yes){
            std::ifstream fi(metadata.toStdString());
            fi >> uiUac_meshtype_labelled;
            fi >> uiUac_whichAtriumIndex;
            fi >> uiUac_fibreFileIndex;
            fi >> uiUac_surftypeIndex;
            fi.close();

            userInputAccepted=true;
        }
    }

    if(!userInputAccepted){
        QDialog* inputs = new QDialog(0, Qt::WindowFlags());
        m_UIUac.setupUi(inputs);
        connect(m_UIUac.buttonBox, SIGNAL(accepted()), inputs, SLOT(accept()));
        connect(m_UIUac.buttonBox, SIGNAL(rejected()), inputs, SLOT(reject()));

        // enable or disable parts that might not be used
        m_UIUac.label_3->setVisible(enableFullUiOptions);
        m_UIUac.combo_uac_surftype->setVisible(enableFullUiOptions);
        m_UIUac.label->setVisible(enableFullUiOptions);
        m_UIUac.combo_uac_fibrefile->setVisible(enableFullUiOptions);
        m_UIUac.check_uac_meshtype_labelled->setVisible(enableFullUiOptions);

        int dialogCode = inputs->exec();

        //Act on dialog return code
        if (dialogCode == QDialog::Accepted) {
            userInputAccepted = true;
            uiUac_meshtype_labelled = m_UIUac.check_uac_meshtype_labelled->isChecked();
            uiUac_whichAtriumIndex = m_UIUac.combo_uac_whichAtrium->currentIndex();
            uiUac_fibreFileIndex = m_UIUac.combo_uac_fibrefile->currentIndex();
            uiUac_surftypeIndex = m_UIUac.combo_uac_surftype->currentIndex();

            if (enableFullUiOptions){
                std::ofstream fo(metadata.toStdString());
                fo << uiUac_meshtype_labelled << std::endl;
                fo << uiUac_whichAtriumIndex << std::endl;
                fo << uiUac_fibreFileIndex << std::endl;
                fo << uiUac_surftypeIndex << std::endl;
                fo.close();
            }

        } else if (dialogCode == QDialog::Rejected) {
            inputs->close();
            inputs->deleteLater();
        }//_if
    }

    return userInputAccepted;
}

bool AtrialStrainMotionView::GetUserEditLabelsInputs(){
    bool userInputAccepted=false;

    if(!userInputAccepted){
        QDialog* inputs = new QDialog(0, Qt::WindowFlags());
        m_UIEditLabels.setupUi(inputs);
        connect(m_UIEditLabels.buttonBox, SIGNAL(accepted()), inputs, SLOT(accept()));
        connect(m_UIEditLabels.buttonBox, SIGNAL(rejected()), inputs, SLOT(reject()));
        std::cout << "WHICH ATRIUM: " << uac_whichAtrium.toStdString() << '\n';
        bool isLeftAtrium = (uac_whichAtrium.compare("LA", Qt::CaseInsensitive)==0);

        m_UIEditLabels.lineEdit_LA->setVisible(isLeftAtrium);
        m_UIEditLabels.lineEdit_LAA->setVisible(isLeftAtrium);
        m_UIEditLabels.lineEdit_LSPV->setVisible(isLeftAtrium);
        m_UIEditLabels.lineEdit_LIPV->setVisible(isLeftAtrium);
        m_UIEditLabels.lineEdit_RSPV->setVisible(isLeftAtrium);
        m_UIEditLabels.lineEdit_RIPV->setVisible(isLeftAtrium);
        m_UIEditLabels.lineEdit_RA->setVisible(!isLeftAtrium);
        m_UIEditLabels.lineEdit_RAA->setVisible(!isLeftAtrium);
        m_UIEditLabels.lineEdit_RA_SVC->setVisible(!isLeftAtrium);
        m_UIEditLabels.lineEdit_RA_IVC->setVisible(!isLeftAtrium);
        m_UIEditLabels.lineEdit_RA_CS->setVisible(!isLeftAtrium);

        int dialogCode = inputs->exec();
        //Act on dialog return code
        if (dialogCode == QDialog::Accepted) {
            userInputAccepted = true;
            bool ok1, ok2, ok3, ok4, ok5, ok6;
            if(isLeftAtrium){
                int la, laa, ls, li, rs, ri;

                la = m_UIEditLabels.lineEdit_LA->text().toInt(&ok1);
                laa = m_UIEditLabels.lineEdit_LAA->text().toInt(&ok2);
                ls = m_UIEditLabels.lineEdit_LSPV->text().toInt(&ok3);
                li = m_UIEditLabels.lineEdit_LIPV->text().toInt(&ok4);
                rs = m_UIEditLabels.lineEdit_RSPV->text().toInt(&ok5);
                ri = m_UIEditLabels.lineEdit_RIPV->text().toInt(&ok6);

                if (!ok1) la = 1;
                if (!ok2) laa = 19;
                if (!ok3) ls = 11;
                if (!ok4) li = 13;
                if (!ok5) rs = 15;
                if (!ok6) ri = 17;

                uiLabels.clear();
                uiLabels << QString::number(la);
                uiLabels << QString::number(laa);
                uiLabels << QString::number(ls);
                uiLabels << QString::number(li);
                uiLabels << QString::number(rs);
                uiLabels << QString::number(ri);
            } else{
                int ra, raa, svc, ivc, cs;

                ra = m_UIEditLabels.lineEdit_RA->text().toInt(&ok1);
                svc = m_UIEditLabels.lineEdit_RA_SVC->text().toInt(&ok3);
                ivc = m_UIEditLabels.lineEdit_RA_IVC->text().toInt(&ok4);
                cs = m_UIEditLabels.lineEdit_RA_CS->text().toInt(&ok5);
                raa = m_UIEditLabels.lineEdit_RAA->text().toInt(&ok2);

                if (!ok1) ra = 1;
                if (!ok3) svc = 6;
                if (!ok4) ivc = 7;
                if (!ok5) cs = 5;
                if (!ok2) raa = 2;

                uiLabels.clear();
                uiLabels << QString::number(ra);
                uiLabels << QString::number(svc);
                uiLabels << QString::number(ivc);
                uiLabels << QString::number(cs);
                uiLabels << QString::number(raa);
            }

        } else if (dialogCode == QDialog::Rejected) {
            inputs->close();
            inputs->deleteLater();
        }//_if
    }

    return userInputAccepted;
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
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.

    // copy the first image and segment it
    QDir nifti_dir = QDir(directory + "/nifti/");
    QString input_file_name = nifti_dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)[0];
    QString input_file_path = directory + "/" + input_file_name;
    MITK_INFO << input_file_path.toStdString();

    MITK_INFO(QFile::copy(nifti_dir.absolutePath() + "/" + input_file_name, input_file_path)) << "Copied " << input_file_name;

    std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
    cmd->SetUseDockerContainers(true);
    QString pathToSegmentation = cmd->DockerCctaMultilabelSegmentation(directory, input_file_path, false);
    MITK_INFO << pathToSegmentation;

    // extract LA
    mitk::Image::Pointer segmentation = mitk::IOUtil::Load<mitk::Image>(pathToSegmentation.toStdString());

    if (!segmentation) return ;

    // create CemrgMultilabelSegmntation (cmls)
    std::unique_ptr<CemrgMultilabelSegmentationUtils> cmls(new CemrgMultilabelSegmentationUtils());
    segmentation = cmls->ReplaceLabel(segmentation, 8, 4);
    segmentation = cmls->ReplaceLabel(segmentation, 9, 4);
    segmentation = cmls->ReplaceLabel(segmentation, 10, 4);
    for (int i = 1; i <= 7; i++) {
        if (i != 4)
            segmentation = cmls->ReplaceLabel(segmentation, i, 0);
    }

    QString la_path = directory + "/LA.nii";
    mitk::IOUtil::Save(segmentation, la_path.toStdString());

    // CemrgCommonUtils::AddToStorage(segmentation, "LA", this->GetDataStorage());

}

void AtrialStrainMotionView::SurfaceMeshSmooth() {

    if (!RequestProjectDirectoryFromUser()) return;

    QString la_path = directory + "/LA.nii";
    QString la_msh_path = directory + "/LA_msh.vtk";
    QString la_msh_smth_path = directory + "/LA_msh_smth.vtk";

    std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
    // cmd->ExecuteSurf(directory, directory + "/LA.nii", "close", 0, 0.5, 0, 100);
    cmd->ExecuteExtractSurface(directory, la_path, la_msh_path, 0.5, 0);
    // cmd->ExecuteSmoothSurface(directory, la_msh_path, la_msh_smth_path, 100);

    // mitk::Surface::Pointer la_msh = mitk::IOUtil::Load<mitk::Surface>(la_msh_path.toStdString());
    // CemrgCommonUtils::AddToStorage(la_msh, "la_msh", this->GetDataStorage());

    QDir q_directory(directory);
    q_directory.mkdir("UAC_CT") ? MITK_INFO << "UAC_CT created" : MITK_INFO << "UAC_CT exists";
    q_directory.mkdir("UAC_CT_aligned") ? MITK_INFO << "UAC_CT_aligned created" : MITK_INFO << "UAC_CT_aligned exists";

    MITK_INFO(QFile::copy(la_msh_path, directory + "/UAC_CT" + "/LA_msh.vtk")) << "Copied LA_msh.vtk";

    // ~/Software/CemrgApp_v2.1/bin/MLib/extract-surface ${basePath}/S-0046/nifti/LA.nii ${basePath}/S-0046/nifti/LA-msh.vtk -isovalue 0.5 -blur 0 -ascii -verbose 3
    // ~/Software/CemrgApp_v2.1/bin/MLib/smooth-surface ${basePath}/S-0046/nifti/LA-msh.vtk ${basePath}/S-0046/nifti/LA-msh-smth.vtk -iterations 100
}

void AtrialStrainMotionView::AnalysisChoice(){
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.

    bool userInputsAccepted = GetUserAnalysisSelectorInputs();
    uiSelector_pipeline = 2;
    if(userInputsAccepted){
        // uiSelector_pipeline; // =0 (imgAuto), =1 (imgManual), =2 (surf)
        SetAutomaticPipeline(uiSelector_pipeline==0); // Image Pipeline (automatic)
        MITK_INFO << "uiSelector_pipeline " << uiSelector_pipeline;

        MITK_INFO << "[AnalysisChoice] Analysis starting from surface";
        SetAutomaticPipeline(false);

        // Load surface mesh: LA_msh.vtk
		tagName = "LA_msh";
        if(!LoadSurfaceChecks()) return;

        // Create fake segmentation image for labelling
        double origin[3] = {0, 0, 0};
        double spacing[3] = {1, 1, 1};
		MITK_INFO << tagName;
        CemrgCommonUtils::SaveImageFromSurfaceMesh(Path("UAC_CT/" + tagName + ".vtk"), origin, spacing);
        CemrgCommonUtils::SavePadImageWithConstant(Path("UAC_CT/" + tagName + ".nii"));

        mitk::Image::Pointer im = CemrgCommonUtils::ReturnBinarised(mitk::IOUtil::Load<mitk::Image>(StdStringPath("UAC_CT/" + tagName+".nii")));

        mitk::IOUtil::Save(im, StdStringPath("UAC_CT/" + tagName+".nii"));
        CemrgCommonUtils::AddToStorage(im, tagName.toStdString(), this->GetDataStorage());

    }
}

void AtrialStrainMotionView::SegmentationPostprocessing(){
    this->GetSite()->GetPage()->ShowView("org.mitk.views.segmentationutilities");
}


void AtrialStrainMotionView::IdentifyPV(){
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.
    QString path = Path("UAC_CT/") + "segmentation.vtk";

	userHasSetMeshingParams = false;

    if(!QFileInfo::exists(path)){
        // Select segmentation from data manager
        QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
        if (nodes.size() != 1) {
            QMessageBox::warning(NULL, "Attention",
                "Please select the loaded or created segmentation to create a surface!");
            return;
        }

        //Find the selected node
        mitk::DataNode::Pointer segNode = nodes.at(0);
        mitk::BaseData::Pointer data = segNode->GetData();
        if (data) {
            mitk::Image::Pointer image = dynamic_cast<mitk::Image*>(data.GetPointer());
            if (image) {

                MITK_INFO << "Mesh and save as 'segmentation.vtk'";
                bool userInputsAccepted = GetUserMeshingInputs();
                if(userInputsAccepted){

                    MITK_INFO << "[IdentifyPV] Create clean segmentation";
                    mitk::IOUtil::Save(image, StdStringPath("UAC_CT/prodClean.nii"));

                    MITK_INFO << "[IdentifyPV] Create surface file and projecting tags";
                    std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
                    cmd->SetUseDockerContainers(true);

                    cmd->ExecuteSurf(Path("UAC_CT"), Path("UAC_CT/prodClean.nii"), "close", uiMesh_iter, uiMesh_th, uiMesh_bl, uiMesh_smth);

                    //Add the mesh to storage
                    std::string meshName = segNode->GetName() + "-Mesh";
                    CemrgCommonUtils::AddToStorage(
                        CemrgCommonUtils::LoadVTKMesh(path.toStdString()), meshName, this->GetDataStorage());
                }
            }
        }
    }

    MITK_INFO(QFile::copy(Path("UAC_CT/segmentation.vtk"), Path("UAC_CT_aligned/segmentation.vtk"))) << "Copied: " << "segmentation.vtk";

    MITK_INFO << "Loading org.mitk.views.atrialfibresclipperview";
    this->GetSite()->GetPage()->ResetPerspective();
    this->GetSite()->GetPage()->ShowView("org.mitk.views.atrialfibresclipperview");
}

void AtrialStrainMotionView::CreateLabelledMesh() {
    MITK_INFO << "TIMELOG|CreateLabelledMesh| Start";
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.

    if(!analysisOnLge){
        QString segRegPath = GetFilePath("LA-reg", ".nii");
        if(!segRegPath.isEmpty()){
            int reply = Ask("Registered segmentation found", "Consider a Scar Projection analysis?");
            SetLgeAnalysis(reply==QMessageBox::Yes);
        }
        tagName += analysisOnLge ? "-reg" : "";
    }

    QString prodPath =  directory + "/UAC_CT/";

    ImageType::Pointer pveins = atrium->LoadImage(prodPath+"PVeinsLabelled.nii");
    if(!tagName.contains("Labelled")){
        std::string msg = "Changing working name from " + tagName.toStdString() + " to 'Labelled'";
        QMessageBox::information(NULL, "Attention", msg.c_str());
        tagName = "Labelled";
    }
    pveins = atrium->AssignOstiaLabelsToVeins(pveins, directory, tagName);

    MITK_INFO << "[CreateLabelledMesh] Create Mesh";
    //Ask for user input to set the parameters
    bool userInputsAccepted = GetUserMeshingInputs();

    if(userInputsAccepted){
        MITK_INFO << "[CreateLabelledMesh] Create clean segmentation";
        mitk::Image::Pointer segIm = mitk::Image::New();
        mitk::CastToMitkImage(pveins, segIm);
        segIm = CemrgCommonUtils::ReturnBinarised(segIm);
        mitk::IOUtil::Save(segIm, StdStringPath("UAC_CT/prodClean.nii"));

        MITK_INFO << "[CreateLabelledMesh] Create surface file and projecting tags";
        std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
        cmd->SetUseDockerContainers(true);

        cmd->ExecuteSurf(directory + "/UAC_CT", Path("UAC_CT/prodClean.nii"), "close", uiMesh_iter, uiMesh_th, uiMesh_bl, uiMesh_smth);
        atrium->ProjectTagsOnExistingSurface(pveins, directory + "/UAC_CT", tagName+".vtk");

        MITK_INFO << "Add the mesh to storage";
        QString path = prodPath + tagName + ".vtk";

        std::cout << "Path to load: " << path.toStdString() <<'\n';
        std::cout << "tagName: " << tagName.toStdString() << '\n';
        mitk::Surface::Pointer surface = mitk::IOUtil::Load<mitk::Surface>(path.toStdString());

        std::string meshName = tagName.toStdString() + "-Mesh";
        CemrgCommonUtils::AddToStorage(surface, meshName, this->GetDataStorage());
    }
    MITK_INFO << "TIMELOG|CreateLabelledMesh| End";
}

void AtrialStrainMotionView::MeshPreprocessing(){
    MITK_INFO << "TIMELOG|MeshPreprocessing| Start";
    MITK_INFO << "[MeshPreprocessing] ";
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.
    if (!LoadSurfaceChecks()) return;

    //Show the plugin
    this->GetSite()->GetPage()->ResetPerspective();
    AtrialFibresClipperView::output_directory(); // directory is empty
    // AtrialFibresClipperView::SetDirectoryFile(directory, tagName+".vtk", true);
    this->GetSite()->GetPage()->ShowView("org.mitk.views.atrialfibresclipperview");
    AtrialFibresClipperView::output_directory(); // directory is empty
}


void AtrialStrainMotionView::ClipperPV(){

	MITK_INFO << "TIMELOG|MeshPreprocessing| End";
    MITK_INFO << "TIMELOG|ClipperPV| Start";
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.#
    if (!LoadSurfaceChecks()) return; // Surface was not loaded and user could not find file.

    QString prodPath = Path("UAC_CT/");

    MITK_INFO << "[ClipperPV] clipping PVs.";

	tagName = "Labelled";
    QString path = prodPath + tagName + ".vtk";

	MITK_INFO << path.toStdString();

    mitk::Surface::Pointer surface = mitk::IOUtil::Load<mitk::Surface>(path.toStdString());
    path = prodPath + "prodClipperIDsAndRadii.txt";
    if(QFile::exists(path)){

        MITK_INFO << "[ClipperPV] Reading in centre IDs and radii";
        std::ifstream fi;
        fi.open((path.toStdString()));

        int numPts;
        fi >> numPts;

        for (int ix = 0; ix < numPts; ix++) {
            int ptId;
            double radius, x_c, y_c, z_c;

            fi >> ptId >> x_c >> y_c >> z_c >> radius;

            QString spherePath = prodPath + "pvClipper_" + QString::number(ptId) + ".vtk";
            std::cout << "Read points: ID:" << ptId << " C=[" << x_c << " " << y_c << " " << z_c << "] R=" << radius <<'\n';

            surface = CemrgCommonUtils::ClipWithSphere(surface, x_c, y_c, z_c, radius, spherePath);
        }
        fi.close();

		// mitk::Surface::Pointer la_msh = mitk::IOUtil::Load<mitk::Surface>(la_msh_path.toStdString());
        CemrgCommonUtils::AddToStorage(surface, "clipped", this->GetDataStorage());

        // save surface
        path = prodPath + tagName + ".vtk";
        mitk::IOUtil::Save(surface, path.toStdString());
        atrium->SetSurface(path);

        SetTagNameFromPath(path);
        if(automaticPipeline){
            ClipperMV();

            QString correctLabels = prodPath + "prodSeedLabels.txt";
            QString naiveLabels = prodPath + "prodNaiveSeedLabels.txt";

            QMessageBox::information(NULL, "Attention", "Attempting to correct automatic labels to default ones");
            atrium->SetSurfaceLabels(correctLabels, naiveLabels);
            atrium->SaveSurface(path.toStdString());
        }

        QMessageBox::information(NULL, "Attention", "Clipping of PV and MV finished");

    } else{
        QMessageBox::warning(NULL, "Warning", "Radii file not found");
        return;
    }

}

void AtrialStrainMotionView::ClipperMV(){

   MITK_INFO << "[ClipperMV] Clipping mitral valve";
   atrium->ClipMitralValveAuto(directory + "/UAC_CT", "prodMVI.nii", tagName+".vtk");
}

void AtrialStrainMotionView::UacCalculationVerifyLabels(){
	tagName = "Labelled";

	MITK_INFO << tagName;
    MITK_INFO << "TIMELOG|VerifyLabels| Start";
    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.
    if(GetUserUacOptionsInputs(false)){
        uac_whichAtrium = uiUac_whichAtrium.at(uiUac_whichAtriumIndex);
        MITK_INFO << ("[UacCalculationVerifyLabels] Seleted ["+uac_whichAtrium+"] analysis").toStdString();
    } else{
        MITK_INFO << "User cancelled selection of LA/RA selection";
        return;
    }
    if(!GetUserEditLabelsInputs()){
        MITK_INFO << "labels not checked. Stopping";
        return;
    }
	MITK_INFO << tagName;

    MITK_INFO(LoadSurfaceChecks()) << ("Loaded surface" + tagName).toStdString();
    std::string title, msg;
    mitk::Surface::Pointer surface = mitk::IOUtil::Load<mitk::Surface>(StdStringPath("UAC_CT/" + tagName + ".vtk"));
    std::vector<int> incorrectLabels;
    if(atrium->CheckLabelConnectivity(surface, uiLabels, incorrectLabels)){
        title = "Label connectivity verification - failure";
        msg = "Make sure label connectivity is correct.\n\n";
        msg += "Do you want to try automatic fixing? ";

        int reply_auto_fix = Ask(title, msg);
        if(reply_auto_fix == QMessageBox::Yes){
            MITK_INFO << "Solving labelling inconsistencies";
            for (unsigned int ix = 0; ix < incorrectLabels.size(); ix++) {
                atrium->FixSingleLabelConnectivityInSurface(surface, incorrectLabels.at(ix));
            }
            mitk::IOUtil::Save(surface, StdStringPath("UAC_CT/" + tagName + ".vtk"));

            title = "Labels fixed";
            msg = "Labels fixed - saved file: \n";
            msg += StdStringPath("UAC_CT/" + tagName + ".vtk");
            QMessageBox::information(NULL, title.c_str(), msg.c_str());

        } else{
            title = "Opening Mesh Preprocessing";
            msg = "Use the Fix labelling button to fix labelling connectivity";
            QMessageBox::information(NULL, title.c_str(), msg.c_str());

            //Show the plugin
            this->GetSite()->GetPage()->ResetPerspective();
            // AtrialFibresClipperView::SetDirectoryFile(directory, tagName+".vtk", true);
            this->GetSite()->GetPage()->ShowView("org.mitk.views.atrialfibresclipperview");
        }

        return;
    } else{
        title = "Label connectivity verification - success";
        msg = "No connectivity problems found on labels";

        QMessageBox::information(NULL, title.c_str(), msg.c_str());
        MITK_INFO << msg;
    }
    MITK_INFO << "TIMELOG|VerifyLabels| End";
}


