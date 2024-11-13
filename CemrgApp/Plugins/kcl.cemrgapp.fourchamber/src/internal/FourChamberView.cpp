/*=========================================================================
BSD 3-Clause License

Copyright (c) 2020, CemrgAppDevelopers
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

//Micro services
#include <usModuleRegistry.h>
#ifdef _WIN32
// _WIN32 = we're in windows
#include <winsock2.h>
#else
// or linux/mac
#include <arpa/inet.h>
#endif

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <berryIWorkbenchPage.h>
#include <berryFileEditorInput.h>

//Qmitk
#include <mitkImage.h>
#include <mitkLog.h>
#include <QmitkIOUtil.h>
#include <mitkProgressBar.h>
#include <mitkIDataStorageService.h>
// #include <mitkNodePredicateNot.h>
// #include <mitkNodePredicateProperty.h>
#include <mitkDataStorageEditorInput.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkLabelSetImage.h>

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
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

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

// plugin classes
#include "kcl_cemrgapp_fourchamber_Activator.h"
#include "FourChamberView.h"
#include "FourChamberLandmarksView.h"

// Qt
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDirIterator>
#include <QSignalMapper>
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>

//CemrgAppModule
#include <CemrgCommonUtils.h>
#include <CemrgCommandLine.h>
#include <CemrgFourChamberCmd.h>
#include <CemrgAtrialModellingToolCmd.h>

const std::string FourChamberView::VIEW_ID = "org.mitk.views.fourchamberheart";

// JSON pre-determined filenames
const QString FourChamberView::POINTS_FILE = "physical_points.json";
const QString FourChamberView::POINTS_FILE_INDEX = "points.json";
const QString FourChamberView::GEOMETRY_FILE = "geometry.json";
const QStringList FourChamberView::SEGMENTATION_LIST = {"MIXED_LABEL",  "BACKGROUND",  "LV_BP",  "LV_myo",  "RV_BP",  "LA_BP",  "RA_BP",  "Ao_BP",  "PArt_BP",  "LPV1",  "LPV2",  "RPV1",  "RPV2",  "LAA",  "SVC",  "IVC",  
                                                        "LV_neck",  "RV_myo",  "LA_myo",  "RA_myo",  "Ao_wall",  "PArt_wall",  "MV",  "TV",  "AV",  "PV",  
                                                        "plane_LPV1",  "plane_LPV2",  "plane_RPV1",  "plane_RPV2",  "plane_LAA",  "plane_SVC",  "plane_IVC",  
                                                        "LPV1_ring",  "LPV2_ring",  "RPV1_ring",  "RPV2_ring",  "LAA_ring",  "SVC_ring",  "IVC_ring",  
                                                        "DELETE"};
const QString FourChamberView::LABELS_FILE = "segmentation_labels.json";

std::vector<LabelsType> LabelsTypeRange(LabelsType begin, LabelsType end) {
    std::vector<LabelsType> result;
    for (LabelsType value = begin; value <= end; value = static_cast<LabelsType>(static_cast<int>(value) + 1)) {
        result.push_back(value);
    }
    return result;
}


void FourChamberView::SetFocus() {
    m_Controls.button_setfolder->setFocus();
}

void FourChamberView::CreateQtPartControl(QWidget *parent){
    // Link the slots to the buttons in the GUI here
    m_Controls.setupUi(parent);
    connect(m_Controls.button_setfolder, SIGNAL(clicked()), this, SLOT(SetWorkingFolder()));
    connect(m_Controls.button_prepseg, SIGNAL(clicked()), this, SLOT(PrepareSegmentation()));
    connect(m_Controls.button_corrections, SIGNAL(clicked()), this, SLOT(Corrections()));
    connect(m_Controls.button_smooth_segmentation, SIGNAL(clicked()), this, SLOT(SmoothSegmentation()));
    connect(m_Controls.button_clean_segmentation, SIGNAL(clicked()), this, SLOT(CleanMultilabelSegmentation()));
    connect(m_Controls.button_meshing, SIGNAL(clicked()), this, SLOT(Meshing()));
    connect(m_Controls.button_uvcs, SIGNAL(clicked()), this, SLOT(UVCs()));
    connect(m_Controls.button_ventfibres, SIGNAL(clicked()), this, SLOT(VentricularFibres()));
    connect(m_Controls.button_atrfibres, SIGNAL(clicked()), this, SLOT(AtrialFibres()));
    connect(m_Controls.button_surf2vol, SIGNAL(clicked()), this, SLOT(SurfaceToVolume()));
    connect(m_Controls.button_definetags, SIGNAL(clicked()), this, SLOT(DefineTags()));
    connect(m_Controls.button_simset, SIGNAL(clicked()), this, SLOT(SimulationSetup()));

    connect(m_Controls.button_loaddicom, SIGNAL(clicked()), this, SLOT(LoadDICOM()));
    connect(m_Controls.button_origin_spacing, SIGNAL(clicked()), this, SLOT(GetOriginSpacing()));
    connect(m_Controls.button_segment_image, SIGNAL(clicked()), this, SLOT(SegmentImgs()));
    connect(m_Controls.button_define_labels, SIGNAL(clicked()), this, SLOT(UserDefineLabels()));
    connect(m_Controls.button_corrections_split, SIGNAL(clicked()), this, SLOT(CorrectionGetLabels()));
    connect(m_Controls.button_confirm_labels, SIGNAL(clicked()), this, SLOT(CorrectionConfirmLabels()));
    connect(m_Controls.button_select_pts, SIGNAL(clicked()), this, SLOT(SelectPoints()));
    connect(m_Controls.button_select_pts_a, SIGNAL(clicked()), this, SLOT(SelectPointsCylinders()));
    connect(m_Controls.button_select_pts_b, SIGNAL(clicked()), this, SLOT(SelectPointsSlicers()));
    connect(m_Controls.button_select_pts_c, SIGNAL(clicked()), this, SLOT(SelectPointsValvePlains()));
    connect(m_Controls.button_select_pts_check, SIGNAL(clicked()), this, SLOT(SelectPointsCheck()));
    connect(m_Controls.button_select_pts_reset, SIGNAL(clicked()), this, SLOT(SelectPointsReset()));

    connect(m_Controls.button_extract_myo, SIGNAL(clicked()), this, SLOT(ExtractMyocardium()));
    connect(m_Controls.button_extractsurfs, SIGNAL(clicked()), this, SLOT(ExtractSurfaces()));
    connect(m_Controls.button_uvclandmarks, SIGNAL(clicked()), this, SLOT(SelectLARALandmarks()));
    connect(m_Controls.button_calcuvcs, SIGNAL(clicked()), this, SLOT(CalculateUVCS()));

    // Set default variables and initialise objects
    m_Controls.button_loaddicom->setVisible(false);
    m_Controls.button_origin_spacing->setVisible(false);
    m_Controls.button_segment_image->setVisible(false);
    m_Controls.button_define_labels->setVisible(false);

    m_Controls.button_select_pts->setVisible(false);
    m_Controls.button_select_pts_a->setVisible(false);
    m_Controls.button_select_pts_b->setVisible(false);
    m_Controls.button_select_pts_c->setVisible(false);
    m_Controls.button_select_pts_check->setVisible(false);
    m_Controls.button_select_pts_reset->setVisible(false);
    m_Controls.button_corrections_split->setVisible(false);
    m_Controls.button_confirm_labels->setVisible(false);
    m_Controls.button_confirm_labels->setEnabled(false);
    m_Controls.combo_corrections_id->setVisible(false);

    m_Controls.button_extract_myo->setVisible(false);
    m_Controls.button_extractsurfs->setVisible(false);
    m_Controls.button_uvclandmarks->setVisible(false);
    m_Controls.button_calcuvcs->setVisible(false);

    SetButtonsEnableToOff();
    InitialiseJsonObjects();
    carpless = true;
    geometry_set = false;

    fourchTools = std::unique_ptr<CemrgFourChamberTools>(new CemrgFourChamberTools());
    meshing_parameters = M3DParameters();
    vfibres_parameters = VentricularFibresParams();
}

void FourChamberView::OnSelectionChanged(
        berry::IWorkbenchPart::Pointer /*source*/, const QList<mitk::DataNode::Pointer>& /*nodes*/) {
}

// SLOTS
void FourChamberView::SetWorkingFolder(){
    if (!RequestProjectDirectoryFromUser()){
        MITK_WARN << "Folder not set. Check LOGFILE.";
        return;
    }

    // folder set successfully
    MITK_INFO << "Creating folder structure.";
    QStringList qsl = SDIR.Subdirectories();
    for (int ix=0; ix < qsl.size(); ix++) {
        MITK_INFO(QDir().mkpath(Path(qsl.at(ix)))) << ("Folder created: [" + qsl.at(ix) + "]");
    }
    fourchTools->SetSegDir(StdStringPath(SDIR.SEG));
    QDir().mkpath(QString::fromStdString(fourchTools->GetDebugDir()));

    int replyCarpless = Ask("Question", "Do you have CARP installed?");
    if (replyCarpless == QMessageBox::Yes && RequestCarpDirectoryFromUser()) {
        carpless = !CemrgFourChamberCmd::CheckCarpDirectory(carp_directory);
    }

    if (carpless) {
        Warn("CARP not found", "CARP not found. Some functionality will be disabled.");
        carp_directory = "";
    } else {
        std::string msg = "CARP found \n[" + carp_directory.toStdString() + "]\nAll functionality will be enabled.";
        Inform("CARP found", msg);
    }

    bool load_geometry_file = CheckForExistingFile(directory, FourChamberView::GEOMETRY_FILE);
    if (load_geometry_file) {
        MITK_INFO << "Loading geometry.json file";
        json_geometry = CemrgCommonUtils::ReadJSONFile(directory, FourChamberView::GEOMETRY_FILE);
        if (json_geometry["origin"].isUndefined() || json_geometry["spacing"].isUndefined()) {
            MITK_INFO << "Origin and/or spacing not defined in geometry.json file";
            m_Controls.button_origin_spacing->setEnabled(true);
        } else {
            ParseJsonArray(json_geometry, "origin", origin);
            ParseJsonArray(json_geometry, "spacing", spacing);
            ParseJsonArray(json_geometry, "dimensions", dimensions);
            std::cout << ArrayToString(origin, 3, "Loaded Origin").toStdString();
            std::cout << ArrayToString(spacing, 3, "Loaded Spacing").toStdString();
            std::cout << ArrayToString(dimensions, 3, "Loaded Dimensions").toStdString();
            m_Controls.button_origin_spacing->setEnabled(false);
            geometry_set = true;
        }
    }

    SetButtonsEnableToOn();
    DisableCarpButtons();
    m_Controls.button_loaddicom->setVisible(true);
    m_Controls.button_origin_spacing->setVisible(true);
    m_Controls.button_segment_image->setVisible(true);
    m_Controls.button_define_labels->setVisible(true);

    // m_Controls.button_uvcs->setEnabled(!carpless);
    // m_Controls.button_ventfibres->setEnabled(!carpless);
    // m_Controls.button_simset->setEnabled(!carpless);
}

void FourChamberView::LoadDICOM() {
    
    int replyNii = Ask("Question: Data type", "Do you have a Nifti file to load? \n(NO = DICOMs)"); 
    if (replyNii == QMessageBox::Yes) {
        QString niiFile = QFileDialog::getOpenFileName(NULL, "Open Nifti file.", StdStringPath(SDIR.SEG).c_str(), tr("Nifti file (*.nii, *.nii.gz)"));
        QFileInfo fnii(niiFile);
        if (fnii.exists()) {
            std::string key = "dicom.series.SeriesDescription";
            mitk::DataStorage::SetOfObjects::Pointer set = mitk::IOUtil::Load(niiFile.toStdString(), *this->GetDataStorage());
            set->Begin().Value()->GetData()->GetPropertyList()->SetStringProperty(key.c_str(), fnii.baseName().toStdString().c_str());

            mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
            return;
        }
    }

    int reply1 = QMessageBox::No;
#if defined(__APPLE__)
    MITK_INFO << "Ask user about alternative DICOM reader";
    reply1 = Ask("Question", "Use alternative DICOM reader?");
#endif

    if (reply1 == QMessageBox::Yes) {

        QString dicomFolder = QFileDialog::getExistingDirectory(NULL, "Open folder with DICOMs.", StdStringPath(SDIR.SEG).c_str(), QFileDialog::ShowDirsOnly|QFileDialog::DontUseNativeDialog);
        std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
        QString tmpNiftiFolder = cmd->DockerDicom2Nifti(dicomFolder);

        if (tmpNiftiFolder.compare("ERROR_IN_PROCESSING") != 0) {

            // add results in NIIs folder to Data Manager
            MITK_INFO << ("Conversion successful. Intermediate NII folder: " + tmpNiftiFolder).toStdString();
            Inform("Information", "Conversion successful, press the Process Images button to continue.");
            QDir niftiFolder(tmpNiftiFolder);
            QStringList niftiFiles = niftiFolder.entryList();

            if (niftiFiles.size()>0) {

                QString thisFile, path;
                for(int ix=0; ix<niftiFiles.size(); ix++) {

                    // load here files
                    thisFile = niftiFiles.at(ix);
                    if (thisFile.contains(".nii", Qt::CaseSensitive)) {
                        if (thisFile.contains("lge", Qt::CaseInsensitive) ||  thisFile.contains("mra", Qt::CaseInsensitive)) {

                            path = niftiFolder.absolutePath() + "/" + thisFile;
                            mitk::Image::Pointer image = mitk::IOUtil::Load<mitk::Image>(path.toStdString());
                            std::string key = "dicom.series.SeriesDescription";
                            mitk::DataStorage::SetOfObjects::Pointer set = mitk::IOUtil::Load(path.toStdString(), *this->GetDataStorage());
                            set->Begin().Value()->GetData()->GetPropertyList()->SetStringProperty(key.c_str(), thisFile.left(thisFile.length()-4).toStdString().c_str());

                        }//_if
                    }//_if
                }//_for

            } else {

                MITK_WARN << "Problem with conversion.";
                QMessageBox::warning(NULL, "Attention", "Problem with alternative conversion. Try MITK Dicom editor?");
                return;

            }//_if
        }//_if

    } else {

        MITK_INFO << "Using MITK DICOM editor";
        QString editor_id = "org.mitk.editors.dicomeditor";
        berry::IEditorInput::Pointer input(new berry::FileEditorInput(QString()));
        this->GetSite()->GetPage()->OpenEditor(input, editor_id);

    }//_if
}

void FourChamberView::GetOriginSpacing() {
    //Check for selection of images
    QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
    if (nodes.size() != 1) {
        Warn("Attention", "Please load and select only the image from the Data Manager to convert!");
        return;
    }//_if

    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.

    mitk::BaseData::Pointer data = nodes[0]->GetData();
    if (data) {
        mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (image) {
            image->GetGeometry()->GetOrigin().ToArray(origin);
            image->GetGeometry()->GetSpacing().ToArray(spacing);

            for (int ix=0; ix<3; ix++) {
                dimensions[ix] = image->GetDimension(ix);
            }

            QString origin_str = QString::number(origin[0]) + "," + QString::number(origin[1]) + "," + QString::number(origin[2]);
            QString spacing_str = QString::number(spacing[0]) + "," + QString::number(spacing[1]) + "," + QString::number(spacing[2]);
            QString dimensions_str = QString::number(dimensions[0]) + "," + QString::number(dimensions[1]) + "," + QString::number(dimensions[2]);

            QJsonObject geom_json = CemrgCommonUtils::CreateJSONObject({"origin", "spacing", "dimensions"}, {origin_str, spacing_str, dimensions_str}, {"array", "array", "array"});
            CemrgCommonUtils::WriteJSONFile(geom_json, directory, FourChamberView::GEOMETRY_FILE);

            MITK_INFO << ("Origin: (" + origin_str + ")").toStdString();
            MITK_INFO << ("Spacing: (" + spacing_str + ")").toStdString();
            MITK_INFO << ("Dimensions: (" + dimensions_str + ")").toStdString();

            geometry_set = true;
        }
    }
}

void FourChamberView::SegmentImgs() {
    if (!geometry_set) { 
        Warn("Geometry not set", "Please set the origin and spacing before segmenting images.");
        return;
    }

    int reply_load = Ask("Question", "Do you have a segmentation to load?");
    QString path = "";
    if (reply_load == QMessageBox::Yes) {
        path = QFileDialog::getOpenFileName(NULL, "Open Segmentation File", StdStringPath(SDIR.SEG).c_str(), QmitkIOUtil::GetFileOpenFilterString()); 

    } else if (reply_load == QMessageBox::No) {
        //Check for selection of images
        QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
        if (nodes.size() != 1) {
            Warn("Attention", "Please load and select only the image from the Data Manager to convert!");
            return;
        }//_if

        mitk::BaseData::Pointer data = nodes[0]->GetData();
        if (!data) {
            return; 
        }
        mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (!image) {
            return;
        }

        QString imagePath = Path(SDIR.SEG + "/ct.nii");
        mitk::IOUtil::Save(image, imagePath.toStdString());

        int reply_saveas = Ask("Question", "Do you want to save the DICOM as NIFTI?");
        std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
        cmd->SetUseDockerContainers(true);
        path = cmd->DockerCctaMultilabelSegmentation(Path(SDIR.SEG), imagePath, (reply_saveas == QMessageBox::Yes));
    }

    if (path.isEmpty()) return;


    mitk::Image::Pointer segmentation = mitk::IOUtil::Load<mitk::Image>(path.toStdString());

    double seg_origin[3], seg_spacing[3];
    segmentation->GetVtkImageData()->GetSpacing(seg_spacing);
    segmentation->GetGeometry()->GetOrigin().ToArray(seg_origin);

    mitk::Point3D mitk_origin;
    mitk_origin[0] = origin[0];
    mitk_origin[1] = origin[1];
    mitk_origin[2] = origin[2];
        
    if (!ArrayEqual(seg_origin, origin, 3)) {
        segmentation->SetOrigin(mitk_origin);
    }

    mitk::Vector3D mitk_spacing;
    mitk_spacing[0] = spacing[0];
    mitk_spacing[1] = spacing[1];
    mitk_spacing[2] = spacing[2];

    if (!ArrayEqual(seg_spacing, spacing, 3)){
        segmentation->SetSpacing(mitk_spacing);
    }  

    QFileInfo fi(path);
    try {
        mitk::LabelSetImage::Pointer mlseg = mitk::LabelSetImage::New();
        mlseg->InitializeByLabeledImage(segmentation);
        mlseg->SetGeometry(segmentation->GetGeometry());

        CemrgCommonUtils::AddToStorage(mlseg, fi.baseName().toStdString(), this->GetDataStorage());
        mlseg->Modified();
    } catch (mitk::Exception &e) {
        MITK_ERROR << "Exception caught: " << e.GetDescription();
        CemrgCommonUtils::AddToStorage(segmentation, fi.baseName().toStdString(), this->GetDataStorage());
    }

}

void FourChamberView::UserDefineLabels() {
    if (!RequestProjectDirectoryFromUser()) return ;

    bool userInputAccepted = UserSelectDefineLabels();

    if (userInputAccepted) {
        Inform("Information", "Labels defined. Press OK to continue.");
    }

}

void FourChamberView::ExtractMyocardium() {
    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    QStringList arguments = QStringList();
    QString wmsh = meshing_parameters.working_mesh;
    QString wdir = meshing_parameters.out_dir;
    QString mytags = segmentationLabels.ExtractMeshingLabels();

    if (wmsh.isEmpty()) {
        MITK_INFO << "No working mesh found. Setting default value [myocardium].";
        wmsh = "myocardium";
        meshing_parameters.working_mesh = wmsh;
    }

    arguments << "-msh=" + meshing_parameters.out_name << "-tags=" + mytags << "-submsh=" + wmsh << "-ifmt=carp_txt";
    QString extract_mesh = fourch_cmd->DockerMeshtoolGeneric(wdir, "extract", "mesh", arguments, wmsh + ".pts");

    MeshingLabels mshLabels;
    MITK_INFO << ("Updating file: " + wdir + "/" + wmsh + ".elem, with mesh labels").toStdString();
    mshLabels.RelabelMesh(wdir + "/" + wmsh + ".elem", segmentationLabels);
    
    ParfilesNamesStruct pfn;
    pfn.SetDirectory(Path(SDIR.PAR));
    mshLabels.SaveToJson(pfn.VentFibres());
    pfn.ResetApexSeptumTemplates();

    if (extract_mesh == "ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in extract myocardium");
        return;
    }

    arguments.clear();
    
    arguments << "-imsh=" + wmsh << "-omsh=" + wmsh << "-ifmt=carp_txt" << "-ofmt=vtk";
    QString convert_mesh = fourch_cmd->DockerMeshtoolGeneric(wdir, "convert", "", arguments, wmsh + ".vtk");

    if (convert_mesh == "ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in convert myocardium");
        return;
    }

    arguments.clear();

    // simplify topology
    std::string msg = "Select YES if you want to use the [simplify_topology] standalone";
    msg += "\nThe binary is often in the CARP directory."; 
    int reply_local_meshtool = Ask("Do you have meshtool locally?", msg);

    if (reply_local_meshtool == QMessageBox::Yes) {
        QString whichDir = (carpless) ? directory : carp_directory;
        QString simplify_topology_bin = QFileDialog::getOpenFileName(NULL, "Open meshtool binary", whichDir.toStdString().c_str(), tr("Meshtool binary (*)"));
        if (simplify_topology_bin.isEmpty() || !QFile::exists(simplify_topology_bin) || !simplify_topology_bin.contains("simplify_tag_topology")) {
            Warn("Error in processing", "Simplify topology binary not found");
            return;
        }

        arguments << "-msh=" + wdir + "/" + wmsh << "-outmsh=" + wdir + "/" + wmsh + "_clean" << "-neigh=50" << "-ifmt=carp_txt" << "-ofmt=carp_txt";
        bool simplify_topology = fourch_cmd->ExecuteCommand(simplify_topology_bin, arguments, wdir + "/" + wmsh + "_clean.pts", true);

        if (!simplify_topology) {
            Warn("Error in processing", "Error in simplify topology");
        } else {
            wmsh = wmsh + "_clean";
        }

        arguments.clear();
    }

    arguments << "-msh=" + wmsh << "-tags=" + mytags << "-smth=0.15" << "-outmsh=" + wmsh + "_smooth" << "-ifmt=carp_txt" << "-ofmt=carp_txt";
    wmsh += "_smooth";
    QString smooth_mesh = fourch_cmd->DockerMeshtoolGeneric(wdir, "smooth", "mesh", arguments, wmsh + ".pts");

    if (smooth_mesh == "ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in smooth myocardium");
        return;
    }
    meshing_parameters.working_mesh = wmsh;
    PrintMeshingParameters(Path(SDIR.MESH) + "/heart_mesh_data.par");

    arguments.clear();

    arguments << "-msh=" + wmsh << "-surf=whole_surface" << "-ofmt=vtk";
    QString extract_surface = fourch_cmd->DockerMeshtoolGeneric(wdir, "extract", "surface", arguments, "whole_surface.surfmesh.vtk");

    if (extract_surface == "ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in extract surface");
        return;
    }

    arguments.clear();
    arguments << "-msh=whole_surface.surfmesh.vtk" << "-submsh=whole_surface_CC" << "-ofmt=vtk";
    QString extract_unreachable = fourch_cmd->DockerMeshtoolGeneric(wdir, "extract", "unreachable", arguments, "whole_surface_CC.vtk");

    arguments.clear();

    arguments << "-imsh=" + wmsh << "-omsh=" + wmsh << "-ifmt=carp_txt" << "-ofmt=vtk";
    convert_mesh = fourch_cmd->DockerMeshtoolGeneric(wdir, "convert", "", arguments, wmsh + ".vtk");
}

bool FourChamberView::cp(QString src, QString dst) {
    bool exists = false;
    bool overwrite = false;
    bool success = false;

    if (exists) {
        int reply = Ask("Question", "File already exists. Overwrite?");
        overwrite = (reply == QMessageBox::Yes);
        if (!overwrite) {
            MITK_INFO << ("Using existing file" + dst).toStdString();
            return true;
        }
        QFile::rename(dst, dst + ".bak");
        QFile::remove(dst);
    } 

    success = QFile::copy(src, dst);

    if (exists) {
        if (success){
            QFile::remove(dst + ".bak");
        } else {
            QFile::rename(dst + ".bak", dst);
        }
    }

    return success;
}



void FourChamberView::PrepareSegmentation()
{

    if (!RequestProjectDirectoryFromUser()) return;

    if (m_Controls.button_select_pts->isVisible()){

        m_Controls.button_corrections_split->setVisible(false);
        m_Controls.button_select_pts->setVisible(false);
        m_Controls.button_select_pts_reset->setVisible(false);

    } else {
        m_Controls.button_corrections_split->setVisible(true);
        m_Controls.button_select_pts->setVisible(true);
        m_Controls.button_select_pts_reset->setVisible(true);
    }
}

void FourChamberView::Meshing(){

    if (!RequestProjectDirectoryFromUser()) return;

    QString seg_dir = Path(SDIR.SEG);
    QString mesh_dir = Path(SDIR.MESH);

    if (!QDir().mkpath(mesh_dir)) {
        Warn("Problem with subfolder", "Problem with meshing subfolder"); 
        return;
    }

    QString mesh_path = "";
    int reply_load = Ask("Question", "Do you have a mesh to load?");
    if (reply_load == QMessageBox::Yes) {
        mesh_path = QFileDialog::getOpenFileName(NULL, "Open Meshtools3d parameter file (.json)", mesh_dir.toStdString().c_str(), tr("M3d Parameter file (*.json)"));
        QFileInfo fi(mesh_path);
        LoadMeshingParametersFromJson(fi.absolutePath(), fi.fileName());

        QFileInfo testHeartMesh(meshing_parameters.out_dir + "/" + meshing_parameters.out_name + ".pts");

        if (!testHeartMesh.exists()) {
            Warn("Mesh file not found", "Paths can be wrong if a different computer was used. \nSelect the meshes from the correct paths");
            bool success_m3d_params = UserSelectMeshtools3DParameters(Path(SDIR.SEG));
            MITK_INFO(success_m3d_params) << "Amended meshing folders";
        }
       
    } else if (reply_load == QMessageBox::No) {
        QString basename = "seg_final_smooth";
        QString segname = basename + ".nrrd";
        QString path = seg_dir + "/" + segname;
        bool ask_to_load = true;
        if (!QFile::exists(path)) {
            // retrieve the path of the segmentation after user has selected it from the UI
            path = QFileDialog::getOpenFileName(NULL, "Open Segmentation file", seg_dir.toStdString().c_str(), QmitkIOUtil::GetFileOpenFilterString());

            if (path.isEmpty()){
                Warn("No segmentation found", "No segmentation found [seg_final_smooth.nrrd]. Use the Smoothing button to create it.");
                return;
            }

            QFileInfo fi(path);
            seg_dir = fi.absolutePath();
            segname = fi.fileName();
            basename = fi.baseName();
            MITK_INFO << ("Segmentation path: " + path).toStdString();
        } 

        // create cmd object (CemrgCommandLine) outputs to directory/meshing
        std::unique_ptr<CemrgCommandLine> cmd_object(new CemrgCommandLine());

        QString inr_path = CemrgCommonUtils::ConvertToInr(seg_dir, segname, true, basename + ".inr");
        if (inr_path.isEmpty()) {
            Warn("Failed to convert segmentation", "Error converting segmentation to INR format");
            return; 
        }
        QString parpath = mesh_dir + "/heart_mesh_data.par";
        bool success_m3d_params = UserSelectMeshtools3DParameters(inr_path);
        PrintMeshingParameters(parpath);
        if (success_m3d_params) {
            QString out_ext = meshing_parameters.out_carp ? "pts" : "vtk";
            QFileInfo finr(inr_path);
            Inform("Attention", "This operation takes a few minutes");
            QDir home(directory);
            mesh_path = cmd_object->ExecuteCreateCGALMesh(directory, meshing_parameters.out_name, parpath, SDIR.SEG + "/" + finr.fileName(), home.relativeFilePath(meshing_parameters.out_dir), out_ext);
        } else {
            return;
        }
    }

    MITK_INFO << ("Mesh path: " + mesh_path).toStdString();
    if (mesh_path.isEmpty()) return;

    QMessageBox::information(NULL, "Attention", "Meshing complete. Press OK to continue.");

    m_Controls.button_extract_myo->setVisible(true);
    m_Controls.button_extract_myo->setEnabled(true);
}

void FourChamberView::SelectLARALandmarks(){
    if (!RequestProjectDirectoryFromUser()) return;

    QString laSubdir = SDIR.UVC_LA + "/la";
    QString raSubdir = SDIR.UVC_RA + "/ra";
    QString laName = "la";
    QString raName = "ra";

    this->GetSite()->GetPage()->ResetPerspective();
    FourChamberLandmarksView::SetDirectoryFile(directory, SDIR.UVC_LA+"/la", laName + ".vtk", SDIR.UVC_RA+"/ra", raName + ".vtk");
    this->GetSite()->GetPage()->ShowView("org.mitk.views.fourchamberlandmarksview");
}

void FourChamberView::ExtractSurfaces(){
    if (!RequestProjectDirectoryFromUser()) return;

    int reply_load = Ask("Question", "Do you have a mesh to load?");
    if (meshing_parameters.working_mesh.isEmpty()) {
        if (reply_load == QMessageBox::Yes) {
            QString meshInfoPath = QFileDialog::getOpenFileName(NULL, "Open Meshtools3d parameter file (.json)", StdStringPath(SDIR.MESH).c_str(), tr("M3d Parameter file (*.json)"));
            QFileInfo fi(meshInfoPath);
            LoadMeshingParametersFromJson(fi.absolutePath(), fi.fileName());

            if (meshing_parameters.working_mesh.isEmpty()) {
                Warn("Mesh file not found", "Paths can be wrong if a different computer was used. \nSelect the meshes from the correct paths");
                return;
            }
        }
    } 

    QString laDir = SDIR.UVC_LA + "/la";
    QString raDir = SDIR.UVC_RA + "/ra";
    QString outLa = laDir + "/la.vtk";
    QString outRa = raDir + "/ra.vtk";

    QDir().mkpath(laDir);
    QDir().mkpath(raDir);

    ParfilesNamesStruct pfn;
    pfn.SetDirectory(Path(SDIR.PAR));
    QDir().mkpath(pfn.ApexSeptumTemplates());
    pfn.ResetApexSeptumTemplates();
    QFileInfo fi(pfn.VentFibres());
    if (!fi.exists()) {
        Inform("Information", "Ventricular Fibres parfiles not found. Creating with default tags. Press OK to continue.");
        MeshingLabels mshLabels;
        mshLabels.SaveToJson(pfn.VentFibres());
    }

    bool recalculateOutputs = true;
    if (QFile::exists(outLa) && QFile::exists(outRa)) {
        int reply = Ask("Question", "LA and RA surfaces already exist. Overwrite?");
        recalculateOutputs = (reply == QMessageBox::Yes);
    }

    if (recalculateOutputs) {
        QString inputTagsFilename = pfn.tagsVentFibres;
        QString apexSeptumFolder = pfn.dirApexSeptumTemplates;

        QString meshname = meshing_parameters.out_dir + "/" + meshing_parameters.working_mesh;
        MITK_INFO << ("Mesh name: " + meshname).toStdString();

        std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
        fourch_cmd->SetCarpDirectory(carp_directory);
        fourch_cmd->SetCarpless(carpless);

        QString output = fourch_cmd->DockerExtractSurfaces(directory, SDIR.PAR, inputTagsFilename, apexSeptumFolder, meshname);
        if (output=="ERROR_IN_PROCESSING") {
            Warn("Error in processing", "Error in extract surfaces");
            return;
        }
    }

}

void FourChamberView::SimulationSetup(){
    if (!RequestProjectDirectoryFromUser()) return;

    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);
    
    // call to cemrg:4ch presim 
    Inform("Information", "Running Presimulation Code");

    // finalise presim with CARP calls
    Inform("Calls to CARP", "Presim code");

    // call to cemrg:4ch fec
    Inform("Information", "Running Fec Code");
}

void FourChamberView::AtrialFibres(){
    if (!RequestProjectDirectoryFromUser()) return;

    QString afibFolder = Path(SDIR.AFIB);
    QString uacFolder = afibFolder + "/UAC";
    QString landmarksFolder = afibFolder + "/" + "Landmarks";
    QStringList expectedOutputs = {
        landmarksFolder + "/LA/prodRaLandmarks.txt",
        landmarksFolder + "/LA/prodRaRegion.txt",
        landmarksFolder + "/RA/prodRaLandmarks.txt",
        landmarksFolder + "/RA/prodRaRegion.txt"
    };

    bool prerquiredFilesExist = true;
    for (QString output : expectedOutputs) {
        if (!QFile::exists(output)) {
            prerquiredFilesExist = false;
            break;
        }
    }

    if (!prerquiredFilesExist) {
        Inform("Information", "Running Landmarks Selection Code");
        std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
        fourch_cmd->SetCarpDirectory(carp_directory); 
        fourch_cmd->SetBaseDirectory(directory);

        QString meshname = Path(SDIR.UVC + "/myocardium_bayer_60_-60");
        QString inputTags = "tags_atrial_fibres.json";
        QString raaApex = "raa_apex.json";
        fourch_cmd->DockerLandmarks(directory, meshname, "endo", SDIR.PAR, inputTags, raaApex, afibFolder);
    } 

    std::unique_ptr<CemrgAtrialModellingToolCmd> atmk_cmd(new CemrgAtrialModellingToolCmd());
    QString dockerMountVolume = uacFolder + "/LA_endo";
    QString meshname = "LA_only";
    atmk_cmd->SetVolume(dockerMountVolume);
    atmk_cmd->SetDockerTag("3.0-beta");
    atmk_cmd->SetAtriumToLA();
    // "ENDO"
    // "-Copy landmark files from example dir to [$DATA/LA_$l]"
    cp(landmarksFolder + "/LA/prodRaLandmarks.txt", dockerMountVolume + "/Landmarks.txt");
    cp(landmarksFolder + "/LA/prodRaRegion.txt", dockerMountVolume + "/Region.txt");

    // "-UAC Stage 1"
    QStringList landmarksList = {dockerMountVolume + "/Landmarks.txt", dockerMountVolume + "/Region.txt"};
    QString uacStage1 = atmk_cmd->UacStage1("endo", "", meshname, QStringList(), landmarksList, true, false, 1000);

    // "-Laplace solves (1)"
    // "--Copying parameter files (LS, PA)"
    QString param_ls = atmk_cmd->GetParfile(AtrialToolkitParamfiles::LS_PARAM, meshname);
    QString param_pa = atmk_cmd->GetParfile(AtrialToolkitParamfiles::PA_PARAM, meshname);

    // "--openCARP"
    std::unique_ptr<CemrgCommandLine> cmd(new CemrgCommandLine());
    QString lapsolve_ls = cmd->OpenCarpDocker(dockerMountVolume, param_ls, "LR_UAC_N2");
    QString lapsolve_pa = cmd->OpenCarpDocker(dockerMountVolume, param_pa, "PA_UAC_N2");

    // "-UAC Stage 2a"e layer EPI - Labarthe"
    QString fibre_map_epi_labarthe = atmk_cmd->FibreMapping("epi", "l", meshname, true, "Fibre", true, "");
    atmk_cmd->UacStage2a("endo", "", meshname, QStringList(), landmarksList, true, false, 1000);

    // "-Laplace solves (2)"
    // "--Copying parameter files (LR_P, LR_A, UD_P, UD_A)"
    QString param_lr_p = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_LR_P_PARAM, meshname);
    QString param_lr_a = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_LR_A_PARAM, meshname);
    QString param_ud_p = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_UD_P_PARAM, meshname);
    QString param_ud_a = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_UD_A_PARAM, meshname);

    // "--openCARP"
    QString lapsolve_lr_p = cmd->OpenCarpDocker(dockerMountVolume, param_lr_p, "LR_Post_UAC");
    QString lapsolve_lr_a = cmd->OpenCarpDocker(dockerMountVolume, param_lr_a, "LR_Ant_UAC");
    QString lapsolve_ud_p = cmd->OpenCarpDocker(dockerMountVolume, param_ud_p, "UD_Post_UAC");
    QString lapsolve_ud_a = cmd->OpenCarpDocker(dockerMountVolume, param_ud_a, "UD_Ant_UAC");

    // "-UAC Stage 2b"
    atmk_cmd->UacStage2b("endo", "", meshname, QStringList(), landmarksList, true, false, 1000);
   
    // "-Scalar Mapping"
    QString scalarmapping = atmk_cmd->ScalarMapping(meshname, true, AtrialToolkitScalarMap::BB);

    // "-Fibre Mapping - single layer ENDO - Labarthe"
    QString fibre_map_endo_labarthe = atmk_cmd->FibreMapping("endo", "l", meshname, true, "Fibre", true, "");
    fibre_map_endo_labarthe = atmk_cmd->FibreMapping("epi", "l", meshname, true, "Fibre", true, "");

    dockerMountVolume = uacFolder + "/RA_endo";
    meshname = "RA_only";
    atmk_cmd->SetVolume(dockerMountVolume);
    atmk_cmd->SetDockerTag("3.0-beta");
    atmk_cmd->SetAtriumToRA();

    landmarksList.clear();
    landmarksList << dockerMountVolume + "/Landmarks.txt" <<  dockerMountVolume + "/Region.txt";
    atmk_cmd->Labels("1", true, 0.6, meshname, landmarksList, 1000);

    QString param_raa = atmk_cmd->GetParfile(AtrialToolkitParamfiles::RAA_PARAM);
    QString param_laa = atmk_cmd->GetParfile(AtrialToolkitParamfiles::LAA_PARAM);

    QString lapsolve_mv_laa = cmd->OpenCarpDocker(dockerMountVolume, param_raa, "MV_LAA");
    
    atmk_cmd->Labels("2", true, 0.6, meshname, landmarksList, 1000);

    meshname+="_RAA";
    atmk_cmd->UacStage1("endo", "", meshname, QStringList(), landmarksList, true, false, 1000);

    param_ls = atmk_cmd->GetParfile(AtrialToolkitParamfiles::LS_PARAM, meshname);
    param_pa = atmk_cmd->GetParfile(AtrialToolkitParamfiles::PA_PARAM, meshname);

    lapsolve_ls = cmd->OpenCarpDocker(dockerMountVolume, param_ls, "LR_UAC_N2");
    lapsolve_pa = cmd->OpenCarpDocker(dockerMountVolume, param_pa, "PA_UAC_N2");

    atmk_cmd->UacStage2a("endo", "", meshname, QStringList(), landmarksList, true, false, 1000);

    param_lr_p = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_LR_P_PARAM, meshname);
    param_lr_a = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_LR_A_PARAM, meshname);
    param_ud_p = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_UD_P_PARAM, meshname);
    param_ud_a = atmk_cmd->GetParfile(AtrialToolkitParamfiles::single_UD_A_PARAM, meshname);

    lapsolve_lr_p = cmd->OpenCarpDocker(dockerMountVolume, param_lr_p, "LR_Post_UAC");
    lapsolve_lr_a = cmd->OpenCarpDocker(dockerMountVolume, param_lr_a, "LR_Ant_UAC");
    lapsolve_ud_p = cmd->OpenCarpDocker(dockerMountVolume, param_ud_p, "UD_Post_UAC");
    lapsolve_ud_a = cmd->OpenCarpDocker(dockerMountVolume, param_ud_a, "UD_Ant_UAC");

    atmk_cmd->UacStage2b("endo", "", meshname, QStringList(), landmarksList, true, false, 1000);

    scalarmapping = atmk_cmd->ScalarMapping(meshname, true, AtrialToolkitScalarMap::BB);
    scalarmapping = atmk_cmd->ScalarMapping(meshname, false, AtrialToolkitScalarMap::SAN);
    scalarmapping = atmk_cmd->ScalarMapping(meshname, false, AtrialToolkitScalarMap::CT);
    scalarmapping = atmk_cmd->ScalarMapping(meshname, false, AtrialToolkitScalarMap::PM);

    fibre_map_endo_labarthe = atmk_cmd->FibreMapping("endo", "l", meshname, true, "Fibre", true, "");
    fibre_map_endo_labarthe = atmk_cmd->FibreMapping("epi", "l", meshname, true, "Fibre", true, "");

}

bool FourChamberView::MainLaplaceProcess(QString uacFolder, QString atrium){
    QString surfEndo = atrium + "_endo";
    QString surfEpi = atrium + "_epi";

    QString endoSurfPath = uacFolder + "/" + atrium + "/" + surfEndo;
    QString epiSurfPath = uacFolder + "/" + atrium + "/" + surfEpi;
    QStringList stimFiles = {endoSurfPath, epiSurfPath};
    QString outSuffix = "endo_epi";
    QString lapOutSuffix = "laplace_" + outSuffix;
    QString outDir = uacFolder + "/" + atrium + "/" + outSuffix;
    QString parfile = "carpf_" + lapOutSuffix +".par";

    QStringList nList = {"phie.dat"};
    QString modelName = atrium + "/" + atrium;
    bool trimNames = true;

    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);

    QString output = fourch_cmd->DockerLaplacePrep(directory, atrium, SDIR.AFIB, surfEndo, surfEpi);

    // carp.pt:
    bool success = fourch_cmd->ExecuteCarp_Pt(directory, meshing_parameters.out_name, SDIR.PAR, parfile, stimFiles, outDir);

    // igbextract
    success = fourch_cmd->ExecuteIgbextract(directory, SDIR.AFIB + "/UAC/" + outSuffix, 0.0, 0.0, nList.at(0), "phie.igb");

    // GlVTKConvert
    success = fourch_cmd->ExecuteGlVTKConvert(uacFolder, modelName, nList, lapOutSuffix, trimNames);

    return success;
}

bool FourChamberView::MainSurfToVolumeProcess(QString uacFolderSuffix, QString atrium) {
    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);

    QString success = fourch_cmd->DockerSurfaceToVolume(directory, atrium, "Fibre_endo_l", "Fibre_epi_l", uacFolderSuffix);

    return (success=="ERROR_IN_PROCESSING");
}

bool FourChamberView::MeshtoolInsert(QString uacFolder, QString expOutputName, QString subMsh, QString msh) {
    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);

    QStringList argsinsert;
    QString expOutput = uacFolder + "/" + expOutputName;
    argsinsert << "-submsh=" + uacFolder + "/" + subMsh;
    argsinsert << "-msh=" + uacFolder + "/" + msh;
    argsinsert << "-ofmt=carp_txt";
    argsinsert << "-outmsh=" + expOutput;

    QString insrtOutput = fourch_cmd->DockerMeshtoolGeneric(directory, "insert", "submesh", argsinsert, expOutput + ".elem");

    return (insrtOutput == "ERROR_IN_PROCESSING");
}

bool FourChamberView::MeshtoolConvert(QString uacFolder, QString expOutputName, QString inputName) {
    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);

    QString expOutput = uacFolder + "/" + expOutputName;
    if (inputName.isEmpty()) inputName = expOutputName;

    QStringList argsconvert;
    argsconvert << "-imsh=" + inputName;
    argsconvert << "-omsh=" + expOutput + ".vtk";
    QString convrtOutput = fourch_cmd->DockerMeshtoolGeneric(directory, "convert", "", argsconvert, expOutput + ".elem");

    return (convrtOutput == "ERROR_IN_PROCESSING");
}

void FourChamberView::SurfaceToVolume() {
    // uac_folder = f"{directory}/{mesh_path}"
    QString uacFolderSuffix = SDIR.AFIB + "/UAC";
    QString uacFolder = Path(uacFolderSuffix);

    bool success = MainLaplaceProcess(uacFolder, "la");
    if (!success) return;

    success = MainLaplaceProcess(uacFolder, "ra");
    if (!success) return;

    // === surface to volume - la ===
    success = MainSurfToVolumeProcess(uacFolderSuffix, "la");
    if (!success) return;

    // === surface to volume - ra ===
    success = MainSurfToVolumeProcess(uacFolderSuffix, "ra");
    if (!success) return;

    // === Mapping to biatrial mesh ===
    QStringList atria = {"la", "ra"};
    QStringList suffix = {".nod", ".eidx"};
    QString fibrsSheet = "_fibres_l_sheet";

    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);
    for (unsigned int ix = 0; ix < atria.size(); ix++) {
        QString atr = atria.at(ix);
        for (unsigned int jx = 0; jx < suffix.size(); jx++) {
            QString srcPath = uacFolder + "/" + atr + "/" + atr + suffix.at(jx);
            QString dstPath = uacFolder + "/" + atr + "/" + atr + fibrsSheet + suffix.at(jx);
            if (!cp(srcPath, dstPath)) return;
        }
    }

    QString biatrPre = "biatrial/biatrial";
    if (!MeshtoolInsert(uacFolder, biatrPre + "_la_fibres", "la/la" + fibrsSheet, biatrPre)) return ;
    if (!MeshtoolConvert(uacFolder, biatrPre + "_la_fibres")) return ;

    if (!MeshtoolInsert(uacFolder, biatrPre + "_fibres_l", "ra/ra" + fibrsSheet, biatrPre + "_la_fibres")) return ;
    if (!MeshtoolConvert(uacFolder, biatrPre + "_fibres_l")) return ;

    // === Mapping to four chamber mesh ===
    for (unsigned int jx = 0; jx < suffix.size(); jx++) {
        QString srcPath = uacFolder + "/" + biatrPre + suffix.at(jx);
        QString dstPath = uacFolder + "/" + biatrPre + fibrsSheet + suffix.at(jx);
        if (!cp(srcPath, dstPath)) return;
    }

    if (!MeshtoolInsert(directory, SDIR.AFIB + "/myocardium_fibres_l", uacFolder + "/" + biatrPre + "_fibres_l", SDIR.UVC + meshing_parameters.out_name)) return ;
    if (!MeshtoolConvert(directory, SDIR.AFIB + "/myocardium_fibres_l")) return ;
}

void FourChamberView::DefineTags() {
    if (!RequestProjectDirectoryFromUser()) return;

    QString meshname = meshing_parameters.out_name;
    QString tagsname = meshing_parameters.out_dir + "/" + meshing_parameters.out_name + "_tags";
    QString inputTagsFilename = "tags_presim.json";
    QString bbSettingsFilename = "bachmann_bundle_fec_settings.json";

    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);
    //                                       baseDirectory, dataSubdir, atriaSubdir, meshname, parfolder, inputTagsFilename, bbSettingsFilename
    QString output = fourch_cmd->DockerDefineTags(directory, SDIR.UVC, SDIR.AFIB, meshname, SDIR.PAR, inputTagsFilename, bbSettingsFilename);
    if (output=="ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in define tags");
        return;
    }
}

void FourChamberView::VentricularFibres(){
    if (!RequestProjectDirectoryFromUser()) return;

    QString meshPath = Path(SDIR.UVC + "/BiV");

    QDir().mkpath(meshPath);

    bool success_vfib_params = UserSelectVFibresParameters(meshPath);
    if (success_vfib_params) {
        QStringList keys, values, types;
        vfibres_parameters.KeysAndValues(keys, values, types);
        for (unsigned int ix = 0; ix < keys.size(); ix++) {
            std::cout << "[" << keys.at(ix).toStdString() << "] " << values.at(ix).toStdString() << '\n';
        }

        QJsonObject json = CemrgCommonUtils::CreateJSONObject(keys, values, types);
        CemrgCommonUtils::WriteJSONFile(json, meshPath, "vfibres_parameters.json");
    } else {
        return;
    }

    std::unique_ptr<CemrgFourChamberCmd> fourch_cmd(new CemrgFourChamberCmd());
    fourch_cmd->SetCarpDirectory(carp_directory);
    fourch_cmd->SetCarpless(carpless);

    bool success = fourch_cmd->ExecuteGlRuleFibers(vfibres_parameters);
    MITK_INFO(success) << "GlRuleFibers executed successfully";

    if (carpless) {
        Warn("CARPless mode", "The command will be printed in the LOG file but not executed.");
        return;
    }

    if (!cp(vfibres_parameters.output(), Path(SDIR.UVC + "/BiV.lon"))) return;

    MITK_INFO << "Substituted biv.lon with new fibres";
    success = fourch_cmd->ExecuteGlElemCenters(Path(SDIR.UVC + "/BiV"), Path(SDIR.UVC + "/BiV_elem_centres.pts"));

    QString correctedFibresPath = fourch_cmd->DockerCorrectFibres(directory, SDIR.UVC + "/BiV/BiV");

    if (!cp(Path(SDIR.UVC + "/BiV/BiV_corrected.lon"), Path(SDIR.UVC + "/BiV/BiV.lon"))) return;
    MITK_INFO << "Substituted biv.lon with corrected fibres";

    MITK_INFO << "Converting to VTK for visualisation with Paraview";
    QStringList arguments;
    QDir home(directory);
    arguments << "-imsh=" + home.relativeFilePath(Path(SDIR.UVC + "/BiV/BiV"));
    arguments << "-ifmt=carp_txt";
    arguments << "-ofmt=carp_txt";
    arguments << "-surf=" + home.relativeFilePath(Path(SDIR.UVC + "/BiV/BiV_fibres"));
    QString expectedOutput = home.absoluteFilePath(Path(SDIR.UVC + "/BiV/BiV_fibres.vtk"));
    QString mshtlConvert = fourch_cmd->DockerMeshtoolGeneric(directory, "convert", "", arguments, expectedOutput);

    if (mshtlConvert=="ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in meshtool convert");
        return;
    }

    arguments.clear();

    MITK_INFO << "Copying myocardium mesh to surfaces folder";
    // find and copy all files 
    if (!cp(Path(SDIR.MESH + "/myocardium_OUT/myocardium.*"), Path(SDIR.UVC + "/"))) return;

    MITK_INFO << "Inserting in submesh";
    arguments << "-msh=" + home.relativeFilePath(Path(SDIR.UVC + "/myocardium"));
    arguments << "-op=2";
    arguments << "-outmsh=" + home.relativeFilePath(Path(SDIR.UVC + "/myocardium"));
    expectedOutput = home.absoluteFilePath(Path(SDIR.UVC + "/myocardium.lon"));
    QString mshtlInsert = fourch_cmd->DockerMeshtoolGeneric(directory, "generate", "fibres", arguments, expectedOutput);

    if (mshtlInsert=="ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in meshtool generate");
        return;
    }

    arguments.clear();

    arguments << "-submsh="+home.relativeFilePath(Path(SDIR.UVC + "/BiV/BiV"));
    arguments << "-msh="+home.relativeFilePath(Path(SDIR.UVC + "/myocardium"));
    arguments << "-outmsh=" + home.relativeFilePath(vfibres_parameters.output());
    arguments << "-ofmt=carp_txt";
    expectedOutput = home.absoluteFilePath(vfibres_parameters.output() + ".pts");
    QString mshtlSubmesh = fourch_cmd->DockerMeshtoolGeneric(directory, "insert", "submesh", arguments, expectedOutput);

    arguments.clear();

    if (mshtlSubmesh=="ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in meshtool insert");
        return;
    }

    arguments << "-imsh="+home.relativeFilePath(vfibres_parameters.output());
    arguments << "-omsh="+home.relativeFilePath(vfibres_parameters.output());
    arguments << "-ofmt=vtk_bin";
    expectedOutput = home.absoluteFilePath(vfibres_parameters.output() + ".vtk");
    QString mshtlConvert2 = fourch_cmd->DockerMeshtoolGeneric(directory, "convert", "", arguments, expectedOutput);

    if (mshtlConvert2=="ERROR_IN_PROCESSING") {
        Warn("Error in processing", "Error in meshtool convert");
    }
}

void FourChamberView::UVCs(){
    
    if (m_Controls.button_extractsurfs->isVisible()) {
        m_Controls.button_extractsurfs->setVisible(true);
    } else {
        m_Controls.button_extractsurfs->setVisible(true);
    }

    if (m_Controls.button_uvclandmarks->isVisible()) {
        m_Controls.button_uvclandmarks->setVisible(true);
    } else {
        m_Controls.button_uvclandmarks->setVisible(true);
    }

    if (m_Controls.button_calcuvcs->isVisible()) {
        m_Controls.button_calcuvcs->setVisible(true);
    } else {
        m_Controls.button_calcuvcs->setVisible(true);
    }
}

void FourChamberView::CalculateUVCs(){
    int reply = Ask("Question", "Placeholder");
    if(reply==QMessageBox::Yes){
        Inform("Answer", "OK!");
    }
}

void FourChamberView::Corrections(){

    if (!RequestProjectDirectoryFromUser()) return; // if the path was chosen incorrectly -> returns.
   
    std::string msg = "Loading Multilabel Segmentation tools.\n\n";
    msg += "Make additional corrections to segmentation";
    
    Inform("Attention", msg.c_str());

    this->GetSite()->GetPage()->ShowView("org.mitk.views.multilabelsegmentation");
}

void FourChamberView::SmoothSegmentation() {
    //Check for selection of images
    QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
    if (nodes.size() != 1) {
        Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
        return;
    }//_if


    mitk::BaseData::Pointer data = nodes[0]->GetData();
    std::string nodeName = nodes[0]->GetName();
    if (data) {
        mitk::Image::Pointer seg = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (seg) {
            // bool doResample = true, doReorient = false, isBinary = true;
            // mitk::Image::Pointer seg_smooth = CemrgCommonUtils::IsoImageResampleReorient(seg, doResample, doReorient, isBinary, std::vector<double>(3, 0.15));
            std::unique_ptr<CemrgMultilabelSegmentationUtils> multilabelUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
            mitk::Image::Pointer seg_smooth = multilabelUtils->ResampleSmoothLabel(seg, std::vector<double>(3, 0.15), 0.5);
            mitk::IOUtil::Save(seg_smooth, StdStringPath(SDIR.SEG + "/seg_final_smooth.nrrd"));
        }
    }
}

void FourChamberView::CleanMultilabelSegmentation() {
    //Check for selection of images
    QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
    if (nodes.size() != 1) {
        Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
        return;
    }//_if

    mitk::BaseData::Pointer data = nodes[0]->GetData();
    std::string nodeName = nodes[0]->GetName();
    if (data) {
        mitk::Image::Pointer seg = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (seg) {
            std::unique_ptr<CemrgMultilabelSegmentationUtils> multilabelUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
            mitk::Image::Pointer seg_clean = multilabelUtils->CleanMultilabelSegmentation(seg);

            QString fileName = QFileDialog::getSaveFileName(NULL, "Save Segmentation", StdStringPath(SDIR.SEG).c_str(), "NRRD Files (*.nrrd)");
            if (fileName.isEmpty()) return;

            mitk::IOUtil::Save(seg_clean, fileName.toStdString());
        }
    }
}

void FourChamberView::CorrectionGetLabels() {
    //Check for selection of images
    QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
    if (nodes.size() != 1) {
        Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
        return;
    }//_if


    mitk::BaseData::Pointer data = nodes[0]->GetData();
    std::string nodeName = nodes[0]->GetName();
    if (data) {
        mitk::Image::Pointer seg = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (seg) {
            // find which labels need splitting
            std::unique_ptr<CemrgMultilabelSegmentationUtils> multilabelUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
            multilabelUtils->GetLabels(seg, labelsInSegmentation);
            foreach (int label, labelsInSegmentation) {
                QString cbox_label = QString::number(label);
                m_Controls.combo_corrections_id->addItem(QString::number(label));
            }
           
            bool segmentation_labels_loaded = CheckForExistingFile(Path(SDIR.SEG), FourChamberView::LABELS_FILE);
            if (segmentation_labels_loaded) {
                int replySegLabels = Ask("Segmentation labels found", "Do you want to load the segmentation labels?");
                if (replySegLabels == QMessageBox::Yes) {
                    MITK_INFO << "Loading segmentation labels";
                    json_segmentation = CemrgCommonUtils::ReadJSONFile(Path(SDIR.SEG), FourChamberView::LABELS_FILE);
                    userLabels.LoadJsonObject(json_segmentation);
                    UpdateDataManager(seg, nodeName, nodes[0], true);
                    m_Controls.combo_corrections_id->setEnabled(false);
                }
            }
            m_Controls.combo_corrections_id->setVisible(true);
            m_Controls.button_confirm_labels->setVisible(true);
            connect(m_Controls.combo_corrections_id, SIGNAL(activated(int)), this, SLOT(CorrectionIdLabels(int)));
        } // _if_image
    } // _if_data
}

void FourChamberView::CorrectionConfirmLabels() {
    //Check for selection of images
    QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
    if (nodes.size() != 1) {
        Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
        return;
    }//_if

    mitk::BaseData::Pointer data = nodes[0]->GetData();
    mitk::Image::Pointer seg = mitk::Image::New();
    if (data) {
        seg = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (seg) {
            MITK_INFO << "Confirming segmentation";
        } else {
            return;
        }
    }
    else {
        return;
    }

    bool confirmLabels = false;

    std::unique_ptr<CemrgMultilabelSegmentationUtils> multilabelUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
    if (labelsToSplit.size() == 0) { // All labels have been selected - applying to segmentation
        Inform("Confirm Labels", "User-selected labels will be applied to the segmentation");
        std::vector<LabelsType> labelsToReprocess;
        for (LabelsType ltt : LabelsTypeRange(LabelsType::BACKGROUND, LabelsType::IVC)) {
            int usrLabel = userLabels.Get(ltt);
            int segLabel = segmentationLabels.Get(ltt);

            std::cout << "User label: " << usrLabel << " Segmentation label: " << segLabel << '\n';
            if (usrLabel != segLabel) {
                std::cout << "Labels are different\n";
                LabelsType ltt2;
                if (segmentationLabels.LabelExists(usrLabel, ltt2)) {
                    std::cout << "Label :" << usrLabel << " already exists in segmentation\n";
                    int auxLabel = segmentationLabels.GenerateNewLabel();

                    MITK_INFO << "Swapping labels: (" + QString::number(usrLabel) + "->" + QString::number(auxLabel) + "), then (" + QString::number(segLabel) + "->" + QString::number(usrLabel) + ")\n";
                    seg = multilabelUtils->ReplaceLabel(seg, usrLabel, auxLabel);
                    seg = multilabelUtils->ReplaceLabel(seg, segLabel, usrLabel);

                    // find the LabelsType that already exists in segmentationLabels

                    segmentationLabels.Set(ltt2, auxLabel);
                    labelsToReprocess.push_back(ltt2);
                } else {
                    MITK_INFO << "Replacing labels (" + QString::number(segLabel) + "->" + QString::number(usrLabel) + ")\n";
                    seg = multilabelUtils->ReplaceLabel(seg, segLabel, usrLabel);
                }
            } // _if
        } // _for

        for (LabelsType ltt : labelsToReprocess) {
            int usrLabel = userLabels.Get(ltt);
            int segLabel = segmentationLabels.Get(ltt);
            MITK_INFO << "Reprocessing auxiliary labels: (" + QString::number(segLabel) + "->" + QString::number(usrLabel) + ")\n";
            seg = multilabelUtils->ReplaceLabel(seg, segLabel, usrLabel);
        }

        confirmLabels = true;

        m_Controls.combo_corrections_id->setEnabled(false);
        m_Controls.button_confirm_labels->setEnabled(false);

        QStringList labelKeys, labelsValues, labelsTypes;
        userLabels.LabelInfoLists(labelKeys, labelsValues, labelsTypes);
        json_segmentation = CemrgCommonUtils::CreateJSONObject(labelKeys, labelsValues, labelsTypes);

        CemrgCommonUtils::WriteJSONFile(json_segmentation, Path(SDIR.SEG), FourChamberView::LABELS_FILE);

    } else {
        Inform ("Attention", "Splitting labels. This may take a while.");
    
        MITK_INFO << "Splitting labels";
        foreach (int label, labelsToSplit) {
            int radius = 5;
            MITK_INFO << ("Splitting label: " + QString::number(label)).toStdString();
            seg = multilabelUtils->SplitLabelsOnRepeat(seg, label, radius);
        }
        // update labelsInSegmentation with new labels in aux 
        std::vector<int> auxLabelsInSeg;
        multilabelUtils->GetLabels(seg, auxLabelsInSeg);
        foreach (int label, auxLabelsInSeg) {
            auto test =  std::find(labelsInSegmentation.begin(), labelsInSegmentation.end(), label);
            if (test == labelsInSegmentation.end()) { // label not found
                labelsInSegmentation.push_back(label);
                m_Controls.combo_corrections_id->addItem(QString::number(label));
            } // _if
        }
        labelsToSplit.clear();
    }

    std::string name = "seg_corrected";
    QString path = Path(SDIR.SEG + "/" + QString::fromStdString(name) + ".nrrd" );

    mitk::IOUtil::Save(seg, path.toStdString());

    UpdateDataManager(seg, name, nodes[0], confirmLabels);
}

void FourChamberView::CorrectionIdLabels(int index) {
    if (m_Controls.combo_corrections_id->count() == 0)
        return;

    // Check for selection of images
    QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
    if (nodes.size() != 1) {
        Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
        return;
    }//_if

    QString cbox_label = m_Controls.combo_corrections_id->itemText(index);
    unsigned int label = cbox_label.split(":")[0].toInt();
    mitk::BaseData::Pointer data = nodes[0]->GetData();
    if (data) {
        mitk::LabelSetImage::Pointer mlseg = dynamic_cast<mitk::LabelSetImage *>(data.GetPointer());
        if (mlseg) {
            std::string nodeName = nodes[0]->GetName();
            std::vector<double> labelCog;
            std::unique_ptr<CemrgMultilabelSegmentationUtils> multilabelUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
            multilabelUtils->GetLabelCentreOfMass((mitk::Image::Pointer) mlseg, label, labelCog);
            mitk::Point3D cog;
            cog[0] = labelCog[0];
            cog[1] = labelCog[1];
            cog[2] = labelCog[2];

            // set the crosshair to the centre of mass
            this->GetRenderWindowPart()->SetSelectedPosition(cog);
            mitk::RenderingManager::GetInstance()->RequestUpdateAll();

            QColor qcolor = MitkColorToQColor(mlseg->GetLabel(label)->GetColor());
            bool userInputsAccepted = UserSelectIdentifyLabels(index, label, qcolor);

            if (userInputsAccepted) {
                if (!m_Controls.button_confirm_labels->isEnabled()){
                    m_Controls.button_confirm_labels->setEnabled(true);
                }

                if (splitCurrentLabel) {
                    if (labelsToSplit.size() == 0) {
                        Inform("Attention", "Keep selecting labels to split until you are done.\nThen use Confirm button");
                    }

                    labelsToSplit.push_back(label);
                    m_Controls.combo_corrections_id->removeItem(index);

                } else if (deleteCurrentLabel) {
                    m_Controls.combo_corrections_id->removeItem(index);
                    multilabelUtils->RemoveLabel((mitk::Image::Pointer) mlseg, label);
                    mitk::IOUtil::Save(mlseg, StdStringPath(SDIR.SEG + "/seg_corrected.nrrd"));

                    CemrgCommonUtils::UpdateFromStorage(mlseg, nodes[0]->GetName(), this->GetDataStorage());

                } else {
                    QString itemText = QString::number(label);
                    itemText += (pickedLabelName.isEmpty()) ? "" : ": " + pickedLabelName;
                    m_Controls.combo_corrections_id->setItemText(index, itemText);

                    MITK_INFO << ("[" + QString::number(index) + "] Label: " + QString::number(userLabel) + " Name: " + pickedLabelName).toStdString();

                    segmentationLabels.SetLabelFromString(pickedLabelName.toStdString(), imageLabel);
                    userLabels.SetLabelFromString(pickedLabelName.toStdString(), userLabel);

                    mlseg->GetLabel(label)->SetName(pickedLabelName.toStdString());
                    mitk::IOUtil::Save(mlseg, StdStringPath(SDIR.SEG + "/seg_corrected.nrrd"));
                }
            }
            MITK_INFO(userInputsAccepted) << "User inputs accepted";
        } // _if_image
    }
}

void FourChamberView::SelectPoints() {

    if (!RequestProjectDirectoryFromUser()) return;

    points_file_loaded = CheckForExistingFile(directory, FourChamberView::POINTS_FILE);
    if (points_file_loaded) {
        MITK_INFO << "Loading points.json file";
        json_points = CemrgCommonUtils::ReadJSONFile(directory, FourChamberView::POINTS_FILE);
        // iterate over json file keys. Unlock buttons if keys are all zeros
    } else {
        int replyIndexFileAvailable = Ask("Question", "Do you have a points index file?");
        if (replyIndexFileAvailable == QMessageBox::Yes) {
            QString path = QFileDialog::getOpenFileName(NULL, "Open Points Index File", StdStringPath().c_str(), tr("Points index file (*.json)"));
            QFileInfo fi(path);

            if (!fi.exists()) return;

            QJsonObject json_index = CemrgCommonUtils::ReadJSONFile(fi.absolutePath(), fi.fileName());
            SetJsonPointsFromIndex(json_index); // updates json_points
            CemrgCommonUtils::WriteJSONFile(json_points, directory, FourChamberView::POINTS_FILE);

            points_file_loaded = true;

        } else {
            MITK_INFO << "Creating points.json file";
            CemrgCommonUtils::WriteJSONFile(json_points, directory, FourChamberView::POINTS_FILE);   
        }
    }

    if (!m_Controls.button_select_pts_a->isVisible()) {
        m_Controls.button_select_pts_a->setVisible(true);
        m_Controls.button_select_pts_b->setVisible(true);
        m_Controls.button_select_pts_c->setVisible(true);
        m_Controls.button_select_pts_check->setVisible(true);
    } else {
        m_Controls.button_select_pts_a->setVisible(false);
        m_Controls.button_select_pts_b->setVisible(false);
        m_Controls.button_select_pts_c->setVisible(false);
        m_Controls.button_select_pts_check->setVisible(false);
        return; 
    }

    std::string pset_name = "four_chamber_point_set";

    // Create pointset‘input_tags_parfile’
    pset = mitk::PointSet::New();
    CemrgCommonUtils::AddToStorage(pset, pset_name, this->GetDataStorage(), false);

    Inform("Controls", "SHIFT + Left Click to add points");

    CemrgDataInteractor::Pointer m_interactor = CemrgDataInteractor::New();
    m_interactor->Initialise(directory + "/" + FourChamberView::POINTS_FILE);
    m_interactor->LoadStateMachine("PointSet.xml");
    m_interactor->SetEventConfig("PointSetConfig.xml");
    m_interactor->SetDataNode(this->GetDataStorage()->GetNamedNode(pset_name.c_str()));

    if (points_file_loaded) { 
        // load points from json file
        QStringList allKeys = json_points.keys();
        mitk::PointSet::PointIdentifier id = 0;
        foreach (QString Key, allKeys) {
            double arr[3] = {0,0,0};
            ParseJsonArray(json_points, Key, arr);
            pset->InsertPoint(id, mitk::Point3D(arr));
            id++;
        }
    }
}

void FourChamberView::SelectPointsCylinders() {
    MITK_INFO << "SelectPointsCylinders";
    if (m_Controls.button_select_pts_a->text().contains("Check Cylinder")) {
        if (!CheckPointsInJsonFile(ManualPoints::CYLINDERS)) {
            Warn("Attention - Missing Points", "All points need to be defined before running the script.");
            SelectPointsCheck();
            return;
        }

        fourchTools->SetCylinders(json_points);
        UpdatePointsIndexFile(json_points);
        m_Controls.button_select_pts_a->setText("Run Scripts for Cylinders");
    } else {
        
        QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
        if (nodes.size() != 1) {
            Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
            return;
        }//_if

        mitk::BaseData::Pointer data = nodes[0]->GetData();
        if (!data) return; 

        mitk::Image::Pointer seg = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (!seg) return;
        
        if (!fourchTools->CylindersSet()) {
            fourchTools->SetCylinders(json_points);
        }
        // Run cylinders script
        MITK_INFO << "Running Scripts for Cylinders";
        
        // create fourchCmd object, define bits and run container from method
        std::unique_ptr<CemrgFourChamberCmd> fourchCmd = std::unique_ptr<CemrgFourChamberCmd>(new CemrgFourChamberCmd());
        QString heartFolder = Path(SDIR.SEG);

        cp(Path(FourChamberView::POINTS_FILE_INDEX), heartFolder + "/" + FourChamberView::POINTS_FILE_INDEX);
        cp(Path(FourChamberView::GEOMETRY_FILE), heartFolder + "/" + FourChamberView::GEOMETRY_FILE);

        if (!QFile::exists(heartFolder + "/" + FourChamberView::LABELS_FILE)) {
            UserSelectDefineLabels();
        }

        fourchCmd->SetBaseDirectory(heartFolder);
        fourchCmd->SetPointsFile(FourChamberView::POINTS_FILE_INDEX);
        fourchCmd->SetOriginSpacingFile(FourChamberView::GEOMETRY_FILE);
        fourchCmd->SetLabelsFile(FourChamberView::LABELS_FILE);

        QString seg_corrected = "seg_corrected.nrrd";
        QString seg_end = "seg_s2a.nrrd";


        QString cylindersOutput = fourchCmd->DockerCreateCylinders(seg_corrected);
        if (cylindersOutput=="ERROR_IN_PROCESSING") {
            Warn("Error in processing", "Error in create cylinders");
            return;
        }

        bool process = true;
        if (QFile::exists(heartFolder + "/" + seg_end)) {
            int reply = Ask("Question", "Segmentation [" +seg_end.toStdString()+ "] already exists.\n\n Process again?");
            process = (reply == QMessageBox::Yes);
        }

        QString nextSegmentation = "ERROR_IN_PROCESSING";
        if (process) {
            nextSegmentation = fourchCmd->DockerCreateSvcIvc(seg_corrected);
            if (nextSegmentation=="ERROR_IN_PROCESSING") {
                Warn("Error in processing", "Error in create SVC/IVC");
                return;
            }
        } else {
            nextSegmentation = heartFolder + "/" + seg_end; // best guess
        }

        MITK_INFO << "Loading segmentation ("+seg_end.toStdString()+") from file";
        mitk::Image::Pointer nextSeg = mitk::IOUtil::Load<mitk::Image>(nextSegmentation.toStdString());
        
        UpdateDataManager(nextSeg, seg_end.toStdString(), nodes[0]);

        m_Controls.button_select_pts_a->setEnabled(false);
    }
}

void FourChamberView::SelectPointsSlicers() {
    MITK_INFO << "SelectPointsSlicers";   
    if (m_Controls.button_select_pts_b->text().contains("Check Slicer")) {
        if (!CheckPointsInJsonFile(ManualPoints::SLICERS)) {
            Warn("Attention - Missing Points", "All points need to be defined before running the script.");
            SelectPointsCheck();
            return;
        }

        fourchTools->SetSlicers(json_points);
        UpdatePointsIndexFile(json_points);
        m_Controls.button_select_pts_b->setText("Run Scripts for Slicers");
    } else {
        
        QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
        if (nodes.size() != 1) {
            Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
            return;
        }//_if

        mitk::BaseData::Pointer data = nodes[0]->GetData();
        if (!data) return; 

        mitk::Image::Pointer seg = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (!seg) return;
        
        if (!fourchTools->SlicersSet()) {
            fourchTools->SetSlicers(json_points);
        }
        // Run Slicers script
        MITK_INFO << "Running Scripts for Slicers";

        // create fourchCmd object, define bits and run container from method
        std::unique_ptr<CemrgFourChamberCmd> fourchCmd = std::unique_ptr<CemrgFourChamberCmd>(new CemrgFourChamberCmd());
        QString heartFolder = Path(SDIR.SEG);

        cp(Path(FourChamberView::POINTS_FILE_INDEX), heartFolder + "/" + FourChamberView::POINTS_FILE_INDEX);
        cp(Path(FourChamberView::GEOMETRY_FILE), heartFolder + "/" + FourChamberView::GEOMETRY_FILE);

        if (!QFile::exists(heartFolder + "/" + FourChamberView::LABELS_FILE)) {
            UserSelectDefineLabels();
        }

        fourchCmd->SetBaseDirectory(heartFolder);
        fourchCmd->SetPointsFile(FourChamberView::POINTS_FILE_INDEX);
        fourchCmd->SetOriginSpacingFile(FourChamberView::GEOMETRY_FILE);
        fourchCmd->SetLabelsFile(FourChamberView::LABELS_FILE); 

        bool process = true;

        // mode: slicers
        QString seg_start = "seg_s2a.nrrd"; 
        if (!QFile::exists(heartFolder + "/" + seg_start)) {
            Warn("Attention", "Segmentation [" +seg_start.toStdString()+ "] does not exist.\n\n Please run the script for cylinders first.");
            return;
        }

        QString slicersOutput = fourchCmd->DockerCreateSlicers(seg_start);
        if (slicersOutput=="ERROR_IN_PROCESSING") {
            Warn("Error in processing", "Error in create slicers");
            return;
        }

        // mode: crop
        process = true;
        QString seg_crop = "seg_s3p.nrrd"; // output of crop
        if (QFile::exists(heartFolder + "/" + seg_crop)) {
            int reply = Ask("Question", "Segmentation [" +seg_crop.toStdString()+ "] already exists.\n\n Process again?");
            process = (reply == QMessageBox::Yes);
        }

        if (process) { 
            QString cropOutput = fourchCmd->DockerCropSvcIvc();
            if (cropOutput=="ERROR_IN_PROCESSING") {
                Warn("Error in processing", "Error in crop slicers");
                return;
            }
        }

        // mode: myo
        process = true;
        QString seg_myo = "seg_s3p.nrrd";
        if (QFile::exists(heartFolder + "/" + seg_myo)) {
            int reply = Ask("Question", "Segmentation [" +seg_myo.toStdString()+ "] already exists.\n\n Process again?");
            process = (reply == QMessageBox::Yes);
        }

        QString myoOutput = "ERROR_IN_PROCESSING";
        if (process) {
            myoOutput = fourchCmd->DockerCreateMyo();
            if (myoOutput=="ERROR_IN_PROCESSING") {
                Warn("Error in processing", "Error in create myo");
                return;
            }
        } else {
            myoOutput = heartFolder + "/" + seg_myo; // best guess
        }

        MITK_INFO << "Loading segmentation ("+seg_myo.toStdString()+") from file";
        mitk::Image::Pointer myoSeg = mitk::IOUtil::Load<mitk::Image>(myoOutput.toStdString());
        UpdateDataManager(myoSeg, seg_myo.toStdString(), nodes[0]);

        m_Controls.button_select_pts_b->setEnabled(false);
    }
}

void FourChamberView::SelectPointsValvePlains(){


    if (m_Controls.button_select_pts_c->text().contains("Check Valve Plains")) {
        if (!CheckPointsInJsonFile(ManualPoints::VALVE_PLAINS)) {
            Warn("Attention - Missing Points", "All points need to be defined before running the script.");
            SelectPointsCheck();
            return;
        }

        fourchTools->SetValvePoints(json_points);
        UpdatePointsIndexFile(json_points);
        m_Controls.button_select_pts_c->setText("Run Scripts for Valve Plains");
    } else {
        
        QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
        if (nodes.size() != 1) {
            Warn("Attention", "Please load and select only the Segmentation from the Data Manager to convert!");
            return;
        } //_if

        mitk::BaseData::Pointer data = nodes[0]->GetData();
        if (!data) return; 

        mitk::Image::Pointer seg = dynamic_cast<mitk::Image *>(data.GetPointer());
        if (!seg) return;
        
        if (!fourchTools->ValvePointsSet()) {
            fourchTools->SetValvePoints(json_points);
        }
        // Run Valve Plains script
        Inform("Attention", "Running Scripts for Valve Plains");

        // create fourchCmd object, define bits and run container from method
        std::unique_ptr<CemrgFourChamberCmd> fourchCmd = std::unique_ptr<CemrgFourChamberCmd>(new CemrgFourChamberCmd());
        QString heartFolder = Path(SDIR.SEG);

        cp(Path(FourChamberView::POINTS_FILE_INDEX), heartFolder + "/" + FourChamberView::POINTS_FILE_INDEX);
        cp(Path(FourChamberView::GEOMETRY_FILE), heartFolder + "/" + FourChamberView::GEOMETRY_FILE);

        if (!QFile::exists(heartFolder + "/" + FourChamberView::LABELS_FILE)) {
            UserSelectDefineLabels();
        }

        fourchCmd->SetBaseDirectory(heartFolder);
        fourchCmd->SetPointsFile(FourChamberView::POINTS_FILE_INDEX);
        fourchCmd->SetOriginSpacingFile(FourChamberView::GEOMETRY_FILE);
        fourchCmd->SetLabelsFile(FourChamberView::LABELS_FILE);

        QString seg_myo = "seg_s3p.nrrd";
        if (!QFile::exists(heartFolder + "/" + seg_myo)) {
            Warn("Attention", "Segmentation [" +seg_myo.toStdString()+ "] does not exist.\n\n Please run the script for slicers first.");
            return;
        }

        bool process = true;
        QString seg_valves = "seg_s4a.nrrd";
        if (QFile::exists(heartFolder + "/" + seg_valves)) {
            int reply = Ask("Question", "Segmentation [" +seg_valves.toStdString()+ "] already exists.\n\n Process again?");
            process = (reply == QMessageBox::Yes);
        }

        QString valvePlanesOutput = "ERROR_IN_PROCESSING";
        if (process) {
            valvePlanesOutput = fourchCmd->DockerCreateValvePlanes();
            if (valvePlanesOutput=="ERROR_IN_PROCESSING") {
                Warn("Error in processing", "Error in create valve planes");
                return;
            }
        } else {
            valvePlanesOutput = heartFolder + "/" + seg_valves; // best guess
        }

        process = true;
        QString seg_clean = "seg_s5.nrrd";
        if (QFile::exists(heartFolder + "/" + seg_clean)) {
            int reply = Ask("Question", "Segmentation [" +seg_clean.toStdString()+ "] already exists.\n\n Process again?");
            process = (reply == QMessageBox::Yes);
        }

        QString cleanSegOutput = "ERROR_IN_PROCESSING";
        if (process) {
            cleanSegOutput = fourchCmd->DockerCleanSegmentation();
            if (cleanSegOutput=="ERROR_IN_PROCESSING") {
                Warn("Error in processing", "Error in clean segmentation");
                return;
            }
        } else {
            cleanSegOutput = heartFolder + "/" + seg_clean; // best guess
        }

        MITK_INFO << "Loading segmentation (" + seg_clean.toStdString() + ") from file";
        mitk::Image::Pointer cleanSeg = mitk::IOUtil::Load<mitk::Image>(cleanSegOutput.toStdString());
        UpdateDataManager(cleanSeg, seg_clean.toStdString(), nodes[0]);
        
        m_Controls.button_select_pts_c->setEnabled(false);
    }
}

bool FourChamberView::CheckPointsInJsonFile(ManualPoints mpt){
    MITK_INFO << "Checking points in json file";
    // reload json file
    ReloadJsonPoints();

    QStringList pointsToCheck = SegPointIds.GetPointLabelOptions(mpt);
    bool allPointsExist = true;
    double testArray[3] = {0,0,0};
    foreach (QString Key, pointsToCheck) {
        double arr[3] = {0,0,0};
        ParseJsonArray(json_points, Key, arr);
        if (ArrayEqual(arr, testArray, 3)) {
            return false;
        }
    }
    return allPointsExist;
}

void FourChamberView::SetJsonPointsFromIndex(QJsonObject json) {
    std::unique_ptr<CemrgMultilabelSegmentationUtils> segUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
    foreach (QString Key, json.keys()) {
        unsigned int arr[3] = {0,0,0};
        ParseJsonArray<unsigned int>(json, Key, arr);

        std::vector<double> world, originVector(3), spacingVector(3);
        std::vector<unsigned int> index(3);

        for (unsigned int jx = 0; jx < 3; jx++) {
            index.at(jx) = arr[jx];
            originVector.at(jx) = origin[jx];
            spacingVector.at(jx) = spacing[jx];
        }

        segUtils->IndexToWorldOriginSpacing(index, world, originVector, spacingVector);

        json_points[Key] = CemrgCommonUtils::CreateJSONArrayDouble(world);
    }
}

void FourChamberView::UpdatePointsIndexFile(QJsonObject json) {
    bool indexFileExists = CheckForExistingFile(Path(SDIR.SEG), FourChamberView::POINTS_FILE_INDEX);
    QJsonObject points_index;
    if (!indexFileExists) {
        QStringList pt_keys = SegPointIds.GetPointLabelOptions(ManualPoints::CYLINDERS);
        pt_keys.append(SegPointIds.GetPointLabelOptions(ManualPoints::SLICERS));
        pt_keys.append(SegPointIds.GetPointLabelOptions(ManualPoints::VALVE_PLAINS));

        QStringList values = QStringList(), types = QStringList();
        InitialiseQStringListsFromSize(pt_keys.size(), values, types);
        points_index = CemrgCommonUtils::CreateJSONObject(pt_keys, values, types);
    } else {
        points_index = CemrgCommonUtils::ReadJSONFile(Path(SDIR.SEG), FourChamberView::POINTS_FILE_INDEX);
    }

    std::unique_ptr<CemrgMultilabelSegmentationUtils> segUtils = std::unique_ptr<CemrgMultilabelSegmentationUtils>(new CemrgMultilabelSegmentationUtils());
    foreach (QString key, json.keys()) {
        double arr[3];
        ParseJsonArray(json, key, arr);
        
        std::vector<unsigned int> index;
        std::vector<double> world(3), originVector(3), spacingVector(3);

        for (unsigned int jx = 0; jx < 3; jx++) {
            world.at(jx) = arr[jx];
            originVector.at(jx) = origin[jx];
            spacingVector.at(jx) = spacing[jx];
        }

        segUtils->WorldToIndexOriginSpacing(world, index, originVector, spacingVector);
        points_index[key] = CemrgCommonUtils::CreateJSONArrayUInt(index);
    }

    CemrgCommonUtils::WriteJSONFile(points_index, Path(SDIR.SEG), FourChamberView::POINTS_FILE_INDEX);
}

void FourChamberView::ReloadJsonPoints(){
    QJsonObject json = CemrgCommonUtils::ReadJSONFile(directory, FourChamberView::POINTS_FILE);
    QStringList allKeys = json_points.keys();

    int countDifferent = 0;
    foreach (QString Key, allKeys) {
        double arrCurrent[3] = {0,0,0};
        double arrNew[3] = {0,0,0};
        ParseJsonArray(json, Key, arrNew);
        ParseJsonArray(json_points, Key, arrCurrent);
        if (!ArrayEqual(arrCurrent, arrNew, 3)) {
            countDifferent++;
        }
    }

    if (countDifferent > 0) {
        // This assumes that the file is always newer than what the points are already saved.
        json_points = json;
    }
}

void FourChamberView::SelectPointsReset(){
    MITK_INFO << "Resetting points";
    int reply = Ask("Question", "Are you sure you want to reset the points?\nThis will overwrite the file too.");
    if(reply==QMessageBox::Yes){
        InitialiseJsonObjects();
        CemrgCommonUtils::WriteJSONFile(json_points, directory, FourChamberView::POINTS_FILE);

        // remove pointset from storage
        this->GetDataStorage()->Remove(this->GetDataStorage()->GetNamedNode("four_chamber_point_set"));

        // reset buttons
        m_Controls.button_select_pts_a->setText("Check Cylinders Points");
        m_Controls.button_select_pts_a->setEnabled(true);
        m_Controls.button_select_pts_b->setText("Check Slicers Points");
        m_Controls.button_select_pts_b->setEnabled(true);
        m_Controls.button_select_pts_c->setText("Check Valve Plains Points");
        m_Controls.button_select_pts_c->setEnabled(true);
    }

}

void FourChamberView::SelectPointsCheck() {
    
    QJsonObject points = CemrgCommonUtils::ReadJSONFile(directory, FourChamberView::POINTS_FILE);
    QStringList cylinders = SegPointIds.CYLINDERS();
    QStringList slicers = SegPointIds.SLICERS();
    QStringList valve_plains = SegPointIds.VALVE_PLAINS();

    std::string output = "Selected Points\n\n";

    output += PrintPoints(points, cylinders, "Cylinders") + '\n';
    output += PrintPoints(points, slicers, "Slicers") + '\n';
    output += PrintPoints(points, valve_plains, "Valve Plains") + '\n';

    QMessageBox::information(nullptr, "Selected Points", output.c_str());
}

std::string FourChamberView::PrintPoints(QJsonObject json, QStringList keysList, QString title) {
    std::string output = title.toStdString() + "\n";
    for (int ix = 0; ix < keysList.size(); ix++) {
        double arr[3] = {0, 0, 0};
        QString key = keysList.at(ix);
        if (json[key].isUndefined()) {
            output += key.toStdString() + ": Undefined\n";
            continue;
        }
        output += key.toStdString() + ": ";
        for (int jx = 0; jx < 3; jx++) {
            arr[jx] = json[key].toArray().at(jx).toDouble();
        }

        if (arr[0] == 0 && arr[1] == 0 && arr[2] == 0) {
            output += "NOT SET\n";
            continue;
        }

        for (int jx = 0; jx < 3; jx++) {
            output += QString::number(arr[jx]).toStdString();
            output += (jx < 2) ? ", " : ")\n";
        }
    }

    return output;
}

void FourChamberView::UpdateDataManager(mitk::Image::Pointer segmentation, std::string name, mitk::DataNode::Pointer& node, bool confirmLabels) {
    try {

        MITK_INFO << "[UpdateDataManager]";
        mitk::LabelSetImage::Pointer mlseg = mitk::LabelSetImage::New();
        mlseg->InitializeByLabeledImage(segmentation);
        mlseg->SetGeometry(segmentation->GetGeometry());
        MITK_INFO << "Adding to storage";
        CemrgCommonUtils::AddToStorage(mlseg, name, this->GetDataStorage());
        mlseg->Modified();

        if (confirmLabels) {
            for (LabelsType ltt : LabelsTypeRange(LabelsType::LV_BP, LabelsType::LAA)) {
                std::cout << "Label: " << userLabels.Get(ltt) << " Name: " << userLabels.LabelName(ltt) << '\n';
                mlseg->GetLabel(userLabels.Get(ltt))->SetName(userLabels.LabelName(ltt));
            }
        } 

    } catch (mitk::Exception &e) {
        MITK_ERROR << "Exception caught: " << e.GetDescription();
        CemrgCommonUtils::AddToStorage(segmentation, name, this->GetDataStorage());
    }

    if (node) {
        this->GetDataStorage()->Remove(node);
    }

}

bool FourChamberView::ImageNeedsFlipping(mitk::Image::Pointer image) {
    MITK_INFO << "Checking if image needs flipping";
    bool flip = false;
    for (int ix = 0; ix < 3; ix++) {
        std::cout << "Dimension[" << ix << "]: " << dimensions[ix] << " Image[" << ix << "]: " << image->GetDimension(ix) << '\n';
        if (dimensions[ix] != image->GetDimension(ix)) {
            flip = true;
            break;
        }
    }
    return flip;
}

mitk::Image::Pointer FourChamberView::SwapAxesIfNecessary(mitk::Image::Pointer image) {
    PrintImageInfo(image);
    if (ImageNeedsFlipping(image)) {
        MITK_INFO << "Swapping axes";
        image = CemrgCommonUtils::SwapAxes(image, {2, 1, 0}); //swap_axes(image, 0, 2)
    }
    MITK_INFO(ImageNeedsFlipping(image)) << "Image did not flip correctly";
    PrintImageInfo(image);

    return image;
}

void FourChamberView::PrintImageInfo(mitk::Image::Pointer image) {
    std::string msg = "Image Info: ";
    for (int ix = 0; ix < 3; ix++) {
        msg += "Dimension[" + std::to_string(ix) + "]: " + std::to_string(image->GetDimension(ix)) + " ";
    }
    MITK_INFO << msg;
}

void FourChamberView::DisableCarpButtons() {
    MITK_INFO(carpless) << "Disabling CARP buttons";
    m_Controls.button_calcuvcs->setEnabled(!carpless);
    m_Controls.button_ventfibres->setEnabled(!carpless);
    m_Controls.button_surf2vol->setEnabled(!carpless);
    m_Controls.button_definetags->setEnabled(!carpless);
}

// helper`
bool FourChamberView::RequestAnyFolderFromUser(QString & dir, std::string msg, bool project_dir){

    bool successfulAssignment = true;

    // ask the user for a directory 
    if (dir.isEmpty()) {
        
        MITK_INFO << "Directory is empty. Requesting user for dirrectory.";
        dir = QFileDialog::getExistingDirectory( NULL, msg.c_str(),
            mitk::IOUtil::GetProgramPath().c_str(),QFileDialog::ShowDirsOnly|QFileDialog::DontUseNativeDialog);

        MITK_INFO << ("Directory selected:" + dir).toStdString();

        if (dir.isEmpty() || dir.simplified().contains(" ")) {
            MITK_WARN << "Please select a project directory with no spaces in the path!";
            QMessageBox::warning(NULL, "Attention", "Please select a project dir with no spaces in the path!");
            dir = QString();
            successfulAssignment = false;
        }//_if

        if (successfulAssignment && project_dir){
            QString now = QDate::currentDate().toString(Qt::ISODate);
            QString logfilename = dir + "/LOGFILE_" + now + ".log";
            std::string logfname_str = logfilename.toStdString();

            mitk::LoggingBackend::SetLogFile(logfname_str.c_str());
            MITK_INFO << ("Changed logfile location to: " + logfilename).toStdString();
        }

    } else {
        MITK_INFO << ("Folder already set: " + dir).toStdString();
    }//_if

    return successfulAssignment;

}

bool FourChamberView::CheckForExistingFile(QString dir, QString filename) {
    MITK_INFO << ("Checking for existing " + filename + " file").toStdString();
    QFileInfo fi(dir + "/" + filename);
    bool load_file = false;
    if (fi.exists()){
        std::string msg2 = "File " + filename.toStdString() + " already exists in: \n[" + dir.toStdString() + "]\n\nYES=Load it \t NO=Overwrite it";
        int reply_load_json = Ask( "Attention" , msg2.c_str());
        if (reply_load_json == QMessageBox::Yes){
            load_file = true;
        } else {
            MITK_INFO << ("Overwriting " + filename + " file").toStdString();
        }
    }
    return load_file;
}

int FourChamberView::Ask(std::string title, std::string msg){
    MITK_INFO << "[USER_INPUT]" + msg;
    return QMessageBox::question(NULL, title.c_str(), msg.c_str(), QMessageBox::Yes, QMessageBox::No);
}

void FourChamberView::Warn(std::string title, std::string msg){
    MITK_WARN << msg;
    QMessageBox::warning(NULL, title.c_str(), msg.c_str());
}

void FourChamberView::Inform(std::string title, std::string msg){
    MITK_INFO << msg;
    QMessageBox::information(NULL, title.c_str(), msg.c_str());
}

void FourChamberView::InitialiseJsonObjects() {

    QStringList pt_keys = SegPointIds.GetPointLabelOptions(ManualPoints::CYLINDERS);
    pt_keys.append(SegPointIds.GetPointLabelOptions(ManualPoints::SLICERS));
    pt_keys.append(SegPointIds.GetPointLabelOptions(ManualPoints::VALVE_PLAINS));

    QStringList values = QStringList(), types = QStringList();
    InitialiseQStringListsFromSize(pt_keys.size(), values, types);

    json_points = CemrgCommonUtils::CreateJSONObject(pt_keys, values, types);
    
}

void FourChamberView::InitialiseQStringListsFromSize(int num, QStringList& values, QStringList& types) {
    for (int ix = 0; ix < num; ix++) {
        values << "0.0,0.0,0.0";
        types << "array";
    }
}

void FourChamberView::PrintMeshingParameters(QString path_to_par){
    std::ofstream fo(path_to_par.toStdString());
    fo << "#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -" << '\n';
    fo << "#Data file for meshtools3d utility"<< '\n';
    fo << "#-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -"<< '\n';
    fo << "[segmentation]"<< '\n';
    fo << "seg_dir = " << meshing_parameters.seg_dir.toStdString() << '\n';
    fo << "seg_name = " << meshing_parameters.seg_name.toStdString() << '\n';
    fo << "mesh_from_segmentation = " << meshing_parameters.mesh_from_segmentation << '\n';
    fo << "boundary_relabeling = " << meshing_parameters.boundary_relabeling << '\n';
    fo << "[meshing]"<< '\n';
    fo << "facet_angle         = "<< meshing_parameters.facet_angle << '\n';
    fo << "facet_size          = "<< meshing_parameters.facet_size << '\n';
    fo << "facet_distance      = "<< meshing_parameters.facet_distance << '\n';
    fo << "cell_rad_edge_ratio = "<< meshing_parameters.cell_rad_edge_ratio << '\n';
    fo << "cell_size           = "<< meshing_parameters.cell_size << '\n';
    fo << "rescaleFactor       = "<< meshing_parameters.rescaleFactor << '\n';
    fo << "[laplacesolver]" << '\n';
    fo << "abs_toll        = " << meshing_parameters.abs_tol << '\n';
    fo << "rel_toll        = " << meshing_parameters.rel_tol << '\n';
    fo << "itr_max         = " << meshing_parameters.itr_max << '\n';
    fo << "dimKrilovSp     = " << meshing_parameters.dimKrilovSp << '\n';
    fo << "verbose         = " << meshing_parameters.verbose << '\n';
    fo << "[others]"<< '\n';
    fo << "eval_thickness  = " << meshing_parameters.eval_thickness << '\n';
    fo << "[output]"<< '\n';
    fo << "outdir          = " << meshing_parameters.out_dir.toStdString() << '\n';
    fo << "name            = " << meshing_parameters.out_name.toStdString() << '\n';
    fo << "out_medit       = " << meshing_parameters.out_medit << '\n';
    fo << "out_carp        = " << meshing_parameters.out_carp << '\n';
    fo << "out_carp_binary = " << meshing_parameters.out_carp_binary << '\n';
    fo << "out_vtk         = " << meshing_parameters.out_vtk << '\n';
    fo << "out_vtk_binary  = " << meshing_parameters.out_vtk_binary << '\n';
    fo << "out_potential   = " << meshing_parameters.out_potential << '\n';
    fo.close();

    QStringList keys = QStringList();
    QStringList values = QStringList();
    QStringList types = QStringList();

    QFileInfo fi(path_to_par);
    meshing_parameters.KeysAndValues(keys, values, types);
    QJsonObject json = CemrgCommonUtils::CreateJSONObject(keys, values, types);
    MITK_INFO(CemrgCommonUtils::WriteJSONFile(json, fi.absolutePath(), fi.baseName() + ".json"));
}

void FourChamberView::LoadMeshingParametersFromJson(QString dir, QString json_file){
    QJsonObject json = CemrgCommonUtils::ReadJSONFile(dir, json_file);
    meshing_parameters.seg_dir = json["seg_dir"].toString();
    meshing_parameters.seg_name = json["seg_name"].toString();
    meshing_parameters.mesh_from_segmentation = json["mesh_from_segmentation"].toBool();
    meshing_parameters.boundary_relabeling = json["boundary_relabeling"].toBool();
    meshing_parameters.facet_angle = json["facet_angle"].toDouble();
    meshing_parameters.facet_size = json["facet_size"].toDouble();
    meshing_parameters.facet_distance = json["facet_distance"].toDouble();
    meshing_parameters.cell_rad_edge_ratio = json["cell_rad_edge_ratio"].toDouble();
    meshing_parameters.cell_size = json["cell_size"].toDouble();
    meshing_parameters.rescaleFactor = json["rescaleFactor"].toDouble();
    meshing_parameters.abs_tol = json["abs_tol"].toDouble();
    meshing_parameters.rel_tol = json["rel_tol"].toDouble();
    meshing_parameters.itr_max = json["itr_max"].toInt();
    meshing_parameters.dimKrilovSp = json["dimKrilovSp"].toInt();
    meshing_parameters.verbose = json["verbose"].toBool();
    meshing_parameters.eval_thickness = json["eval_thickness"].toBool();
    meshing_parameters.out_dir = json["out_dir"].toString();
    meshing_parameters.out_name = json["out_name"].toString();
    meshing_parameters.out_medit = json["out_medit"].toBool();
    meshing_parameters.out_carp = json["out_carp"].toBool();
    meshing_parameters.out_carp_binary = json["out_carp_binary"].toBool();
    meshing_parameters.out_vtk = json["out_vtk"].toBool();
    meshing_parameters.out_vtk_binary = json["out_vtk_binary"].toBool();
    meshing_parameters.out_potential = json["out_potential"].toBool();
    meshing_parameters.working_mesh = json["working_mesh"].toString();

    if (meshing_parameters.working_mesh.isEmpty()) {
        Inform("Working mesh not set", "Defaulting to myocardium");
        meshing_parameters.working_mesh = "myocardium";
    }

    MITK_INFO << "Loaded meshing parameters from json file";
}

QString FourChamberView::GetPointTypeString(ManualPoints mpt) {
    QString res;
    switch (mpt) {
        case ManualPoints::CYLINDERS :
            res = "Cylinders";
            break;
        case ManualPoints::SLICERS :
            res = "Slicers";
            break;
        case ManualPoints::VALVE_PLAINS :
            res = "ValvePlains";
            break;
        default:
            res = "";
            break;
    }

    return res;
}

// User Select Functions

bool FourChamberView::UserSelectDefineLabels() {
    QDialog *inputs = new QDialog(0, 0);
    bool userInputAccepted = false;
    m_labels.setupUi(inputs);

    connect(m_labels.buttonBox, SIGNAL(accepted()), inputs, SLOT(accept()));
    connect(m_labels.buttonBox, SIGNAL(rejected()), inputs, SLOT(reject()));

    int dialogCode = inputs->exec();
    if (dialogCode == QDialog::Accepted) {
        HeartLabels hl;
        QStringList keys = hl.GetKeys();
        foreach (QString key, keys) {
            QString real_key = "line_" + key.left(key.size() - 6); // removing '_label'
            QLineEdit* lineEdit = inputs->findChild<QLineEdit*>(real_key);
            if (lineEdit) {
                bool ok;
                int foundLabel = lineEdit->text().toInt(&ok);
                if (ok) {
                    std::cout << "Label: " << key.toStdString() << " Value: " << foundLabel << '\n';
                    hl.SetLabel(key, foundLabel);
                }
            } else {
                std::cout << "lineEdit not found: " << real_key << "\n";
            }
        }

        ThicknessInfo ti;
        foreach (QString key, ti.GetKeys()) {
            QString ui_key = "line_" + key;
            if (ui_key.contains("_multiplier")) {
                ui_key = ui_key.left(ui_key.size() - 11); // removing '_multiplier'
            }
            QLineEdit* lineEdit = inputs->findChild<QLineEdit*>(ui_key);
            if (lineEdit) {
                bool ok;
                double foundValue = lineEdit->text().toDouble(&ok);
                if (ok) {
                    std::cout << "Name: " << key.toStdString() << " Value: " << foundValue << '\n';
                    ti.SetKey(key, foundValue);
                }
            } 
        }
        ti.Update();
        CemrgCommonUtils::WriteJSONFile(hl.UniteJson(ti.GetJson()), Path(SDIR.SEG), FourChamberView::LABELS_FILE);
        userInputAccepted = true;
    }
    return userInputAccepted;
}

bool FourChamberView::UserSelectMeshtools3DParameters(QString pre_input_path){
    bool userAccepted = false;
    QDialog* inputs = new QDialog(0, 0);
    QSignalMapper* signalMapper = new QSignalMapper(this);

    m_m3d.setupUi(inputs);
    connect(m_m3d.dialog_button, SIGNAL(accepted()), inputs, SLOT(accept()));
    connect(m_m3d.dialog_button, SIGNAL(rejected()), inputs, SLOT(reject()));

    connect(m_m3d.button_load_image, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(m_m3d.button_load_image, directory);
    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(M3dBrowseFile(const QString&)));

    QFileInfo fi(pre_input_path);
    m_m3d.lineEdit_input_path->setPlaceholderText(pre_input_path);
    m_m3d.lineEdit_out_dir->setText(directory + "/" + SDIR.MESH + "/myocardium_OUT");

    int dialogCode = inputs->exec();

    // Act on dialog return code
    if (dialogCode == QDialog::Accepted) {

        // parse all booleans
        meshing_parameters.eval_thickness = m_m3d.checkBox_eval_thickness->isChecked();

        meshing_parameters.out_medit = m_m3d.checkBox_out_medit->isChecked();
        meshing_parameters.out_carp = m_m3d.checkBox_out_carp->isChecked();
        meshing_parameters.out_vtk = m_m3d.checkBox_out_vtk->isChecked();
    
        if (meshing_parameters.CheckFormats()){
            Inform("Attention", "No output format selected.");
            return false;
        }

        if (m_m3d.checkBox_out_binary->isChecked()) {
            meshing_parameters.SetBinaryFormats();
        }

        // parse all strings
        if (!m_m3d.lineEdit_input_path->text().isEmpty()) {
            fi.setFile(m_m3d.lineEdit_input_path->text());
        }
        meshing_parameters.seg_dir = fi.absolutePath();
        meshing_parameters.seg_name = fi.fileName();

        meshing_parameters.out_dir = m_m3d.lineEdit_out_dir->text();
        if (m_m3d.lineEdit_out_name->text().isEmpty()) {
            meshing_parameters.out_name = "heart_mesh";
        } else {
            meshing_parameters.out_name = m_m3d.lineEdit_out_name->text();
        }

        if (m_m3d.lineEdit_working_mesh->text().isEmpty()) {
            meshing_parameters.working_mesh = "myocardium";
        } else {
            meshing_parameters.working_mesh = m_m3d.lineEdit_working_mesh->text();
        }

        if (meshing_parameters.out_name == meshing_parameters.working_mesh) {
            meshing_parameters.out_name = meshing_parameters.out_name + "_OUT";
        }

        // parse all numbers
        bool ok1, ok2, ok3, ok4, ok5, ok6, ok7, ok8, ok9, ok10;
        meshing_parameters.facet_angle = m_m3d.lineEdit_facet_angle->text().toDouble(&ok1);
        meshing_parameters.facet_size = m_m3d.lineEdit_facet_size->text().toDouble(&ok3);
        meshing_parameters.facet_distance = m_m3d.lineEdit_facet_dist->text().toDouble(&ok2);
        meshing_parameters.cell_rad_edge_ratio = m_m3d.lineEdit_cell_rad_edge_ratio->text().toDouble(&ok4);

        meshing_parameters.cell_size = m_m3d.lineEdit_cell_size->text().toDouble(&ok5);
        meshing_parameters.rescaleFactor = m_m3d.lineEdit_rescaleFactor->text().toDouble(&ok6);

        meshing_parameters.itr_max = m_m3d.lineEdit_itr_max->text().toInt(&ok9);
        meshing_parameters.abs_tol = m_m3d.lineEdit_abs_tol->text().toDouble(&ok7);
        meshing_parameters.rel_tol = m_m3d.lineEdit_rel_tol->text().toDouble(&ok8);
        meshing_parameters.dimKrilovSp = m_m3d.lineEdit_dimKrilovSp->text().toInt(&ok10);

        if (!ok1) meshing_parameters.facet_angle = 30.0;
        if (!ok3) meshing_parameters.facet_size = 0.8;
        if (!ok2) meshing_parameters.facet_distance = 4;
        if (!ok4) meshing_parameters.cell_rad_edge_ratio = 2.0;
        if (!ok5) meshing_parameters.cell_size = 0.8;
        if (!ok6) meshing_parameters.rescaleFactor = 1000;
        if (!ok9) meshing_parameters.itr_max = 700;
        if (!ok7) meshing_parameters.abs_tol = 1e-6;
        if (!ok8) meshing_parameters.rel_tol = 1e-6;
        if (!ok10) meshing_parameters.dimKrilovSp = 500;

        userAccepted = true;
    }

    return userAccepted;
}
bool FourChamberView::UserSelectIdentifyLabels(int index, unsigned int label, QColor qc) {
    QDialog *inputs = new QDialog(0, 0);
    bool userInputAccepted = false;
    m_IdLabels.setupUi(inputs);

    m_IdLabels.label_colour->setStyleSheet("background-color: " + qc.name() + "; color: rgb(0, 0, 0);");
    m_IdLabels.combo_id_label->addItems(FourChamberView::SEGMENTATION_LIST);
    m_IdLabels.lineEdit_current_label->setText(QString::number(label));

    connect(m_IdLabels.buttonBox, SIGNAL(accepted()), inputs, SLOT(accept()));
    connect(m_IdLabels.buttonBox, SIGNAL(rejected()), inputs, SLOT(reject()));
    pickedLabelName = "";
    
    int dialogCode = inputs->exec();
    if (dialogCode == QDialog::Accepted) {
        pickedLabelName = m_IdLabels.combo_id_label->currentText();

        SegmentationLabels dslHandler;
        defaultLabel = dslHandler.GetLabelFromString(pickedLabelName.toStdString());

        bool ok;
        imageLabel = label;
        userLabel = m_IdLabels.lineEdit_new_label->text().toInt(&ok);
        if (!ok) {
            userLabel = defaultLabel;
        }
        std::cout << "[UserSelectIdentifyLabels] Label for " << pickedLabelName.toStdString() << " is " << userLabel << '\n';

        splitCurrentLabel = (pickedLabelName == "MIXED_LABEL");
        deleteCurrentLabel = (pickedLabelName == "DELETE");
        if (splitCurrentLabel || deleteCurrentLabel) {
            std::string title = splitCurrentLabel ? "Split Label" : "Delete Label";
            std::string msg = splitCurrentLabel ? "Label marked for splitting" : "Label marked for deletion";
            Inform(title, msg);
            defaultLabel = -1;
            userLabel = -1;
            pickedLabelName = "";
        }

        userInputAccepted = true;
    } else {
        inputs->deleteLater();
    }
    return userInputAccepted;
}

bool FourChamberView::UserSelectVFibresParameters(QString meshPath) {
    QDialog *inputs = new QDialog(0, 0);
    QSignalMapper *signalMapper = new QSignalMapper(this);

    // meshPath = Path(SDIR.UVC + "/BiV");
    vfibres_parameters.SetDirectory(meshPath);
    QDir dir(meshPath);

    bool userInputAccepted = false;
    m_vfibres.setupUi(inputs);
    m_vfibres.lineEdit_out_dir->setPlaceholderText(meshPath);
    m_vfibres.lineEdit_out_dir->setEnabled(false);

    m_vfibres.lineEdit_mesh_name->setText(dir.relativeFilePath(vfibres_parameters.meshname()));
    
    m_vfibres.lineEdit_apex_to_base->setPlaceholderText(dir.relativeFilePath(vfibres_parameters.apex_to_base()));
    m_vfibres.lineEdit_epi->setPlaceholderText(dir.relativeFilePath(vfibres_parameters.epi()));
    m_vfibres.lineEdit_lv->setPlaceholderText(dir.relativeFilePath(vfibres_parameters.lv()));
    m_vfibres.lineEdit_rv->setPlaceholderText(dir.relativeFilePath(vfibres_parameters.rv()));

    connect(m_vfibres.dialog_button, SIGNAL(accepted()), inputs, SLOT(accept()));
    connect(m_vfibres.dialog_button, SIGNAL(rejected()), inputs, SLOT(reject()));

    connect(m_vfibres.button_load1, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(m_vfibres.button_load2, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(m_vfibres.button_load3, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(m_vfibres.button_load4, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(m_vfibres.button_load5, SIGNAL(clicked()), signalMapper, SLOT(map()));

    signalMapper->setMapping(m_vfibres.button_load1, "1;" + meshPath);
    signalMapper->setMapping(m_vfibres.button_load2, "2;" + meshPath);
    signalMapper->setMapping(m_vfibres.button_load3, "3;" + meshPath);
    signalMapper->setMapping(m_vfibres.button_load4, "4;" + meshPath);
    signalMapper->setMapping(m_vfibres.button_load5, "5;" + meshPath);

    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(VFibresBrowseFile(const QString &)));
    int dialogCode = inputs->exec();
    if (dialogCode == QDialog::Accepted) {
        bool ok1, ok2, ok3, ok4;
        double aendo = m_vfibres.lineEdit_alpha_endo->text().toDouble(&ok1);
        double aepi = m_vfibres.lineEdit_alpha_epi->text().toDouble(&ok2);
        double bendo = m_vfibres.lineEdit_beta_endo->text().toDouble(&ok3);
        double bepi = m_vfibres.lineEdit_beta_epi->text().toDouble(&ok4);

        if (ok1) vfibres_parameters.SetAlphaEndo(aendo);
        if (ok2) vfibres_parameters.SetAlphaEpi(aepi);
        if (ok3) vfibres_parameters.SetBetaEndo(bendo);
        if (ok4) vfibres_parameters.SetBetaEpi(bepi);

        vfibres_parameters.SetType(m_vfibres.combo_type->currentText()); 

        if (!m_vfibres.lineEdit_apex_to_base->text().isEmpty()) {
            vfibres_parameters.SetApexToBase(m_vfibres.lineEdit_apex_to_base->text());
        }

        if (!m_vfibres.lineEdit_epi->text().isEmpty()) {
            vfibres_parameters.SetEpi(m_vfibres.lineEdit_epi->text());
        }

        if (!m_vfibres.lineEdit_lv->text().isEmpty()) {
            vfibres_parameters.SetLV(m_vfibres.lineEdit_lv->text());
        }

        if (!m_vfibres.lineEdit_rv->text().isEmpty()) {
            vfibres_parameters.SetRV(m_vfibres.lineEdit_rv->text());
        }

        if (!m_vfibres.lineEdit_bad_elem->text().isEmpty()) {
            vfibres_parameters.SetBadElem(m_vfibres.lineEdit_bad_elem->text());
        }

        if (!m_vfibres.lineEdit_mesh_name->text().isEmpty()) {
            vfibres_parameters.SetMeshname(m_vfibres.lineEdit_mesh_name->text());   
        }

        if (!m_vfibres.lineEdit_out_name->text().isEmpty()) {
            vfibres_parameters.SetOutput(m_vfibres.lineEdit_out_name->text());
        } else {
            vfibres_parameters.SetDefaultOutput();
        }

        userInputAccepted = true;
    }
    
    return userInputAccepted;
}

void FourChamberView::M3dBrowseFile(const QString &dir) {
    QString titlelabel, input = "";
    std::string msg;

    msg = "Select segmentation image for meshing";

    Inform("Attention", msg.c_str());
    input = QFileDialog::getOpenFileName(NULL, msg.c_str(), dir, QmitkIOUtil::GetFileOpenFilterString());

    m_m3d.lineEdit_input_path->setText(input);
}

void FourChamberView::VFibresBrowseFile(const QString &idNdir) {
    QString titlelabel, input = "";
    std::string msg = "Select ";

    QStringList idNdirSplit = idNdir.split(";");
    int id = idNdirSplit.at(0).toInt();
    QString dir = idNdirSplit.at(1);
    QDir d(dir);

    switch (id) {
        case 1: 
            msg += "apex_to_base Laplace Solution";
            break;
        case 2:
            msg += "Epi Laplace Solution";
            break;
        case 3:
            msg += "LV Laplace Solution";
            break;
        case 4: 
            msg += "RV Laplace Solution";
            break; 
        default: // case 5
            msg += "Bad Elements Indices File";
            break;
    }

    Inform("Attention", msg.c_str());
    input = QFileDialog::getOpenFileName(NULL, msg.c_str(), idNdir.mid(1), QmitkIOUtil::GetFileOpenFilterString());

    QFileInfo fi(input);
    switch (id) {
        case 1:
            m_vfibres.lineEdit_apex_to_base->setText(d.relativeFilePath(input));
            break;
        case 2:
            m_vfibres.lineEdit_epi->setText(d.relativeFilePath(input));
            break;
        case 3:
            m_vfibres.lineEdit_lv->setText(d.relativeFilePath(input));
            break;
        case 4:
            m_vfibres.lineEdit_rv->setText(d.relativeFilePath(input));
            break;
        case 5:
            m_vfibres.lineEdit_bad_elem->setText(d.relativeFilePath(input));
            break;
        default:
            break;
    }    
}

void FourChamberView::SetButtonsEnable(bool enable) {
    // Sets all buttons to enable or disable.
    // Used at the beginning of the pipeline
    m_Controls.button_prepseg->setEnabled(enable) ;
    m_Controls.button_corrections->setVisible(enable);
    m_Controls.button_smooth_segmentation->setVisible(enable);
    m_Controls.button_clean_segmentation->setVisible(enable);
    m_Controls.button_meshing->setEnabled(enable);
    m_Controls.button_extract_myo->setEnabled(enable);
    m_Controls.button_uvcs->setEnabled(enable) ; 
    m_Controls.button_ventfibres->setEnabled(enable) ; 
    m_Controls.button_atrfibres->setEnabled(enable) ;
    m_Controls.button_surf2vol->setEnabled(enable) ;
    m_Controls.button_definetags->setEnabled(enable) ; 
    m_Controls.button_simset->setEnabled(enable) ;
}

template <typename T>
QString FourChamberView::ArrayToString(T *arr, int size, const QString &title) {
    QString msg =  title + ": (";
    for (int ix = 0; ix < size; ix++) {
        QString endstr = (ix < size - 1) ? "," : ")";
        msg += QString::number(arr[ix]); 
        msg += endstr;
    }
    msg += '\n';
    return msg;
}

QString FourChamberView::MitkColorToHex(const mitk::Color& colour) {
    QColor qcolour = MitkColorToQColor(colour);
        
    return qcolour.name();
}

QColor FourChamberView::MitkColorToQColor(const mitk::Color& colour) {
    int red = static_cast<int>(colour[0] * 255);
    int green = static_cast<int>(colour[1] * 255);
    int blue = static_cast<int>(colour[2] * 255);

    QColor res(red, green, blue);
    return res;
}

bool FourChamberView::ArrayEqual(double *arr1, double *arr2, int size, double tol) {
    for (int ix = 0; ix < size; ix++) {
        if (fabs(arr1[ix] - arr2[ix]) > tol) {
            return false;
        }
    }
    return true;
}

template <typename T>
void FourChamberView::ParseJsonArray(QJsonObject json, QString key, T *arr, int size) {
    if (json[key].isUndefined()){
        MITK_WARN << ("Key [" + key + "] is Undefined for JSON object").toStdString();
    }
    for (int ix = 0; ix < size; ix++) {
        arr[ix] = (T) json[key].toArray().at(ix).toDouble();
    }
}
