/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
/*=========================================================================
 *
 * Morphological Quantification
 *
 * Cardiac Electromechanics Research Group
 * http://www.cemrgapp.com
 * orod.razeghi@kcl.ac.uk
 *
 * This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
 *
=========================================================================*/

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <berryIWorkbenchPage.h>

// Qmitk
#include <mitkImage.h>
#include <mitkIOUtil.h>
#include <mitkProgressBar.h>
#include <mitkNodePredicateProperty.h>
#include <mitkManualSegmentationToSurfaceFilter.h>

#include "FourChamberCommon.h"
#include "FourChamberView.h"
#include "FourChamberGuidePointsView.h"

// VTK
#include <vtkGlyph3D.h>
#include <vtkSphere.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkDataSetMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkProperty.h>
#include <vtkCellPicker.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCell.h>
#include <vtkMath.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkFloatArray.h>
#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>
#include <vtkCellDataToPointData.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkClipPolyData.h>

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
#include <QDesktopWidget>

// CemrgAppModule
#include <CemrgAtriaClipper.h>
#include <CemrgCommandLine.h>
#include <CemrgCommonUtils.h>
#include <FourChamberCommon.h>

QString FourChamberGuidePointsView::fileName;
QString FourChamberGuidePointsView::directory;
QString FourChamberGuidePointsView::whichAtrium;

const std::string FourChamberGuidePointsView::VIEW_ID = "org.mitk.views.fourchamberguidepointsview";

void FourChamberGuidePointsView::CreateQtPartControl(QWidget *parent) {

    // create GUI widgets from the Qt Designer's .ui file
    m_Controls.setupUi(parent);
    connect(m_Controls.button_guide1, SIGNAL(clicked()), this, SLOT(Help()));
    connect(m_Controls.radio_load_la, SIGNAL(toggled(bool)), this, SLOT(LeftAtriumReactToToggle()));
    connect(m_Controls.radio_load_ra, SIGNAL(toggled(bool)), this, SLOT(RightAtriumReactToToggle()));
    connect(m_Controls.check_show_all, SIGNAL(stateChanged(int)), this, SLOT(CheckBoxShowAll(int)));
    connect(m_Controls.button_save, SIGNAL(clicked()), this, SLOT(Save()));
    connect(m_Controls.alpha_slider, SIGNAL(valueChanged(int)), this, SLOT(ChangeAlpha(int)));

    inputsSelector = new QDialog(0, Qt::Window);
    m_Selector.setupUi(inputsSelector);
    connect(m_Selector.buttonBox, SIGNAL(accepted()), inputsSelector, SLOT(accept()));
    connect(m_Selector.buttonBox, SIGNAL(rejected()), inputsSelector, SLOT(reject()));

    //Setup renderer
    surfActor = vtkSmartPointer<vtkActor>::New();
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.5,0.5,0.5);
    renderer->AutomaticLightCreationOn();
    renderer->LightFollowCameraOn();
    // renderer->TwoSidedLightingOn();
    // renderer->UpdateLightsGeometryToFollowCamera();
    vtkSmartPointer<vtkTextActor> txtActor = vtkSmartPointer<vtkTextActor>::New();
    std::string shortcuts = GetShortcuts();
    txtActor->SetInput(shortcuts.c_str());
    txtActor->GetTextProperty()->SetFontSize(14);
    txtActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    renderer->AddActor2D(txtActor);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow =
            vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_Controls.widget_1->SetRenderWindow(renderWindow);
    m_Controls.widget_1->renderWindow()->AddRenderer(renderer);

    //Setup keyboard interactor
    callBack = vtkSmartPointer<vtkCallbackCommand>::New();
    callBack->SetCallback(KeyCallBackFunc);
    callBack->SetClientData(this);
    interactor = m_Controls.widget_1->renderWindow()->GetInteractor();
    interactor->SetInteractorStyle(vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New());
    interactor->GetInteractorStyle()->KeyPressActivationOff();
    interactor->GetInteractorStyle()->AddObserver(vtkCommand::KeyPressEvent, callBack);

    //Initialisation
    m_Controls.radio_load_la->setChecked(true);
    SetSubdirs();
    iniPreSurf();
    if (surface.IsNotNull()) {
        InitialisePickerObjects();
        Visualiser();
    }

    m_Controls.button_save->setEnabled(false);
    Help(true);
    pluginLoaded = true;
}

void FourChamberGuidePointsView::SetFocus() {
    m_Controls.button_guide1->setFocus();
}

void FourChamberGuidePointsView::OnSelectionChanged(
        berry::IWorkbenchPart::Pointer /*src*/, const QList<mitk::DataNode::Pointer>& /*nodes*/) {
}

FourChamberGuidePointsView::~FourChamberGuidePointsView() {
    inputsSelector->deleteLater();
}

// slots
void FourChamberGuidePointsView::Help(bool firstTime){
    std::string msg = "";
    if(firstTime){
        msg = "HELP\n";
    } else if(!m_Controls.radio_load_la->isChecked()) {
        msg = "LEFT ATRIUM POINTS\n";
    } else {
        msg = "RIGHT ATRIUM POINTS\n";
    }
    QMessageBox::information(NULL, "Help", msg.c_str());
}

void FourChamberGuidePointsView::SetDirectoryFile(const QString directory) {
    FourChamberGuidePointsView::directory = directory;
}

void FourChamberGuidePointsView::SetSubdirs(){
    QString dir = FourChamberGuidePointsView::directory;
    parfiles = dir + "/parfiles";
    apex_septum_files = parfiles + "/apex_septum_templates";
    surfaces_uvc_la = dir + "/surfaces_uvc_LA";
    surfaces_uvc_ra = dir + "/surfaces_uvc_RA";

    path_to_la = surfaces_uvc_la + "/la/la.vtk";
    path_to_ra = surfaces_uvc_ra + "/ra/ra.vtk";}
void FourChamberGuidePointsView::iniPreSurf() {
    //Find the selected node
    QString path = m_Controls.radio_load_la->isChecked() ? path_to_la : path_to_ra;
    mitk::UnstructuredGrid::Pointer shell = mitk::IOUtil::Load<mitk::UnstructuredGrid>(path.toStdString());
    if (shell.IsNull()) {
        MITK_WARN << "Shell is null!";
        return;
    }
    surface = shell;
}

void FourChamberGuidePointsView::Visualiser(double opacity){
    MITK_INFO << "[Visualiser]";
    double max_scalar = 4; // RA
    double min_scalar = 3; // LA

    SphereSourceVisualiser(pickedPointsHandler->GetLineSeeds(), "0.4,0.1,0.0", 0.1);

    //Create a mapper and actor for surface
    vtkSmartPointer<vtkDataSetMapper> surfMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    surfMapper->SetInputData(surface->GetVtkUnstructuredGrid());
    surfMapper->SetScalarRange(min_scalar, max_scalar);
    surfMapper->SetScalarModeToUseCellData();
    surfMapper->ScalarVisibilityOn();

    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(min_scalar, max_scalar);
    lut->SetHueRange(0.5, 0.0);  // this is the way_neighbourhood_size you tell which colors you want to be displayed.
    lut->Build();     // this is important

    surfMapper->SetLookupTable(lut);

    vtkSmartPointer<vtkActor> surfActor = vtkSmartPointer<vtkActor>::New();
    surfActor->SetMapper(surfMapper);
    surfActor->GetProperty()->SetOpacity(opacity);
    renderer->AddActor(surfActor);
}

void FourChamberGuidePointsView::SphereSourceVisualiser(vtkSmartPointer<vtkPolyData> pointSources, QString colour, double scaleFactor){
    // e.g colour = "0.4,0.1,0.0" - values for r,g, and b separated by commas.
    double r, g, b;
    bool okr, okg, okb;
    r = colour.section(',',0,0).toDouble(&okr);
    g = colour.section(',',1,1).toDouble(&okg);
    b = colour.section(',',2,2).toDouble(&okb);

    if(!okr) r=1.0;
    if(!okg) g=0.0;
    if(!okb) b=0.0;

    vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
    vtkSmartPointer<vtkSphereSource> glyphSource = vtkSmartPointer<vtkSphereSource>::New();
    glyph3D->SetInputData(pointSources);
    glyph3D->SetSourceConnection(glyphSource->GetOutputPort());
    glyph3D->SetScaleModeToDataScalingOff();
    glyph3D->SetScaleFactor(surface->GetVtkUnstructuredGrid()->GetLength()*scaleFactor);
    glyph3D->Update();

    //Create a mapper and actor for glyph
    vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(glyph3D->GetOutputPort());
    vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
    glyphActor->SetMapper(glyphMapper);
    glyphActor->GetProperty()->SetColor(r,g,b);
    glyphActor->PickableOff();
    renderer->AddActor(glyphActor);
}

void FourChamberGuidePointsView::PickCallBack() {

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    std::cout << "Tolerance: " << 1E-4 * surface->GetVtkUnstructuredGrid()->GetLength() << std::endl;
    picker->SetTolerance(1E-4 * surface->GetVtkUnstructuredGrid()->GetLength());
    int* eventPosition = interactor->GetEventPosition();
    int result = picker->Pick(float(eventPosition[0]), float(eventPosition[1]), 0.0, renderer);
    if (result == 0) return;
    double* pickPosition = picker->GetPickPosition();
    std::cout << "Pick position: " << pickPosition[0] << " " << pickPosition[1] << " " << pickPosition[2] << std::endl;
    vtkIdList* pickedCellPointIds = surface->GetVtkUnstructuredGrid()->GetCell(picker->GetCellId())->GetPointIds();

    double distance;
    int pickedSeedId = -1;
    double minDistance = 1E10;
    for (int i=0; i<pickedCellPointIds->GetNumberOfIds(); i++) {
        distance = vtkMath::Distance2BetweenPoints(
                    pickPosition, surface->GetVtkUnstructuredGrid()->GetPoint(pickedCellPointIds->GetId(i)));
        std::cout << "Point ID: " << pickedCellPointIds->GetId(i) << std::endl;
        std::cout << "Distance: " << distance << std::endl; 
        if (distance < minDistance) {
            minDistance = distance;
            pickedSeedId = pickedCellPointIds->GetId(i);
        }//_if
    }//_for
    if (pickedSeedId == -1){
        std::cout << "No point was picked!" << std::endl;
        pickedSeedId = pickedCellPointIds->GetId(0);
    }
    
    // pushedLabel gets updated in UserSelectPvLabel function
    pickedPointsHandler->AddPointFromUnstructuredGrid(surface, pickedSeedId, pushedLabel);
    MITK_INFO << pickedPointsHandler->ToString();

    m_Controls.widget_1->renderWindow()->Render();
}

void FourChamberGuidePointsView::KeyCallBackFunc(vtkObject*, long unsigned int, void* ClientData, void*) {

    FourChamberGuidePointsView* self;
    self = reinterpret_cast<FourChamberGuidePointsView*>(ClientData);
    std::string key = self->interactor->GetKeySym();

    if (key == "space") {
        //Ask the labels
        self->UserSelectPvLabel();
        self->PickCallBack();

    } else if (key == "Delete") {

        int lastLabel = self->pickedPointsHandler->CleanupLastPoint();

        if (lastLabel != -1) {
            if (lastLabel == AtrialLandmarksType::LA_APEX)
                self->m_Selector.radioBtn_LA_APEX->setEnabled(true);
            else if (lastLabel == AtrialLandmarksType::LA_SEPTUM)
                self->m_Selector.radioBtn_LA_SEPTUM->setEnabled(true);
            else if (lastLabel == AtrialLandmarksType::RA_APEX)
                self->m_Selector.radioBtn_RA_APEX->setEnabled(true);
            else if (lastLabel == AtrialLandmarksType::RA_SEPTUM)
                self->m_Selector.radioBtn_RA_SEPTUM->setEnabled(true);
            else if (lastLabel == AtrialLandmarksType::RAA_APEX)
                self->m_Selector.radioBtn_RAA_APEX->setEnabled(true);
            
        }//_if

        self->m_Controls.widget_1->renderWindow()->Render();
    } else if (key == "H" || key == "h"){
        self->Help();
    }
}

// helper functions
void FourChamberGuidePointsView::InitialisePickerObjects(){
    pickedPointsHandler = std::unique_ptr<CemrgMeshPointSelector>(new CemrgMeshPointSelector());
    QStringList names = QStringList();
    names << "la.lvapex.vtx" << "la.rvsept_pt.vtx" << "ra.lvapex.vtx" << "ra.rvsept_pt.vtx" << "raa_apex.txt";
    std::vector<int> labels = {11, 13, 15, 17, 19};

    pickedPointsHandler->SetAvailableLabels(names, labels);

}

std::string FourChamberGuidePointsView::GetShortcuts(){
    std::string res = "";
    
    res += " POINT SELECTION:\n\tSpace: select location\n\tDelete: remove location";
    res += "\nHELP:\n\tH/h: Guides";

    return res;
}

void FourChamberGuidePointsView::LeftAtriumReactToToggle() {
    if (pluginLoaded && m_Controls.radio_load_la->isChecked()) {
        QMessageBox::information(NULL, "Info", "Left Atrium selected");
        iniPreSurf();
        Visualiser();
    }
}

void FourChamberGuidePointsView::RightAtriumReactToToggle() {
    if (pluginLoaded && m_Controls.radio_load_ra->isChecked()) {
        QMessageBox::information(NULL, "Info", "Right Atrium selected");
        iniPreSurf();
        Visualiser();
    }
}

void FourChamberGuidePointsView::CheckBoxShowAll(int checkedState) {
    std::cout << "Show all atria: " << checkedState << std::endl;
    QMessageBox::information(NULL, "Info", "Showing both atria");
}

void FourChamberGuidePointsView::ChangeAlpha(int alpha) {
    double opacity = alpha / 100.0;
    Visualiser(opacity);
}

void FourChamberGuidePointsView::Save() {
    QMessageBox::information(NULL, "Info", "Saving points to files");
}

void FourChamberGuidePointsView::UserSelectPvLabel(){
    int dialogCode = inputsSelector->exec();
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - inputsSelector->width()) / 2;
    int y = (screenGeometry.height() - inputsSelector->height()) / 2;
    inputsSelector->move(x,y);

    //Act on dialog return code
    if (dialogCode == QDialog::Accepted) {

        if (m_Selector.radioBtn_LA_APEX->isChecked()) {
            pushedLabel = AtrialLandmarksType::LA_APEX; // LA_APEX
            m_Selector.radioBtn_LA_APEX->setEnabled(false);
        } else if (m_Selector.radioBtn_LA_SEPTUM->isChecked()) {
            pushedLabel = AtrialLandmarksType::LA_SEPTUM; // LA_SEPTUM
            m_Selector.radioBtn_LA_SEPTUM->setEnabled(false);
        } else if (m_Selector.radioBtn_RA_APEX->isChecked()) {
            pushedLabel = AtrialLandmarksType::RA_APEX; // RA_APEX
            m_Selector.radioBtn_RA_APEX->setEnabled(false);
        } else if (m_Selector.radioBtn_RA_SEPTUM->isChecked()) {
            pushedLabel = AtrialLandmarksType::RA_SEPTUM; // RA_SEPTUM
            m_Selector.radioBtn_RA_SEPTUM->setEnabled(false);
        } else if (m_Selector.radioBtn_RAA_APEX->isChecked()) {
            pushedLabel = AtrialLandmarksType::RAA_APEX; // RAA_APEX
            m_Selector.radioBtn_RAA_APEX->setEnabled(false);
        } 
        MITK_INFO << "Label selected: " << pushedLabel;
        
    } else if (dialogCode == QDialog::Rejected) {
        inputsSelector->close();
    }//_if
}


/*
========================
 CemrgApp radiobtn codes
========================
=== Landmarks ===
SVC_POST - radioBtn_RA_SVC_POST - 29
IVC_POST - radioBtn_RA_IVC_POST - 31
RAA_VALVE_P - radioBtn_RAA_TCV -  33
CS_VALVE_P - radioBtn_RA_CS_TCV - 35
SVC_ANT  - radioBtn_RA_SVC_ANT -  37
IVC_ANT  - radioBtn_RA_IVC_ANT -  39

=== Region ===
IVC_ANT - radioBtn_RA_IVC_ANT -      29
CS_TOP - radioBtn_RA_CS -            31
IVC_SVC_ANT - radioBtn_RA_IvcSvc -   33
SVC_ANT - radioBtn_RA_SVC_ANT -      35
RAA_ANT - radioBtn_RAA_ANT -         37
RAA_CS_ANT - radioBtn_RAA_CS_ANT -   39
*/
