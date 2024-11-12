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
#include "FourChamberGuidePointsView.h"
#include "AtrialFibresView.h"

// VTK
#include <vtkGlyph3D.h>
#include <vtkSphere.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
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
    m_Controls.widget_1->GetRenderWindow()->AddRenderer(renderer);

    //Setup keyboard interactor
    callBack = vtkSmartPointer<vtkCallbackCommand>::New();
    callBack->SetCallback(KeyCallBackFunc);
    callBack->SetClientData(this);
    interactor = m_Controls.widget_1->GetRenderWindow()->GetInteractor();
    interactor->SetInteractorStyle(vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New());
    interactor->GetInteractorStyle()->KeyPressActivationOff();
    interactor->GetInteractorStyle()->AddObserver(vtkCommand::KeyPressEvent, callBack);

    //Initialisation
    m_Controls.radio_load_la->setChecked(true);
    iniPreSurf();
    if (surface.IsNotNull()) {
        InitialisePickerObjects();
        Visualiser();
    }

    m_Controls.button_save2_refined->setEnabled(false);
    Help(true);
}

void FourChamberGuidePointsView::SetFocus() {
    m_Controls.button_guide1->setFocus();
}

void FourChamberGuidePointsView::OnSelectionChanged(
        berry::IWorkbenchPart::Pointer /*src*/, const QList<mitk::DataNode::Pointer>& /*nodes*/) {
}

FourChamberGuidePointsView::~FourChamberGuidePointsView() {
    inputsRough->deleteLater();
    inputsRefined->deleteLater();
}

// slots
void FourChamberGuidePointsView::Help(bool firstTime){
    std::string msg = "";
    if(firstTime){
        msg = "HELP\n";
    } else if(!m_Controls.radio_load_la->isChecked()) {
        msg = "lEFT ATRIUM POINTS\n";
    } else {
        msg = "RIGHT ATRIUM POINTS\n";
    }
    QMessageBox::information(NULL, "Help", msg.c_str());
}

void FourChamberGuidePointsView::SetDirectoryFile(const QString directory) {
    FourChamberGuidePointsView::directory = directory;
    parfiles = directory + "/parfiles";
    apex_septum_files = parfiles + "/apex_septum_templates";
    surfaces_uvc_la = directory + "/surfaces_uvc_LA";
    surfaces_uvc_ra = directory + "/surfaces_uvc_RA";

    path_to_la = surfaces_uvc_la + "/la/la.vtk";
    path_to_ra = surfaces_uvc_ra + "/ra/ra.vtk";
}

void FourChamberGuidePointsView::iniPreSurf() {
    //Find the selected node
    QString path = m_Controls.radio_load_la->isChecked() ? path_to_la : path_to_ra;
    mitk::Surface::Pointer shell = CemrgCommonUtils::LoadVTKMesh(path.toStdString());
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

    SphereSourceVisualiser(pickedPointsHandler->GetLineSeeds());

    //Create a mapper and actor for surface
    vtkSmartPointer<vtkPolyDataMapper> surfMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    surfMapper->SetInputData(surface->GetVtkPolyData());
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
    glyph3D->SetScaleFactor(surface->GetVtkPolyData()->GetLength()*scaleFactor);
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

void FourChamberGuidePointsView::PickCallBack(bool refinedLandmarks) {

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(1E-4 * surface->GetVtkPolyData()->GetLength());
    int* eventPosition = interactor->GetEventPosition();
    int result = picker->Pick(float(eventPosition[0]), float(eventPosition[1]), 0.0, renderer);
    if (result == 0) return;
    double* pickPosition = picker->GetPickPosition();
    vtkIdList* pickedCellPointIds = surface->GetVtkPolyData()->GetCell(picker->GetCellId())->GetPointIds();

    double distance;
    int pickedSeedId = -1;
    double minDistance = 1E10;
    for (int i=0; i<pickedCellPointIds->GetNumberOfIds(); i++) {
        distance = vtkMath::Distance2BetweenPoints(
                    pickPosition, surface->GetVtkPolyData()->GetPoint(pickedCellPointIds->GetId(i)));
        if (distance < minDistance) {
            minDistance = distance;
            pickedSeedId = pickedCellPointIds->GetId(i);
        }//_if
    }//_for
    if (pickedSeedId == -1){
        pickedSeedId = pickedCellPointIds->GetId(0);
    }

    pickedPointsHandler->AddPointFromSurface(surface, pickedSeedId);

    m_Controls.widget_1->GetRenderWindow()->Render();
}

void FourChamberGuidePointsView::KeyCallBackFunc(vtkObject*, long unsigned int, void* ClientData, void*) {

    FourChamberGuidePointsView* self;
    self = reinterpret_cast<FourChamberGuidePointsView*>(ClientData);
    std::string key = self->interactor->GetKeySym();

    if (key == "space") {
        //Ask the labels
        self->PickCallBack();
        self->UserSelectPvLabel();

    } else if (key == "Delete") {

        int lastLabel = self->pickedPointsHandler->CleanupLastPoint();

        if (lastLabel != -1) {
            int radioButtonNumber = lastLabel - 10;
            if (radioButtonNumber == 1)
            self->m_Selector.radioBtn_LA_APEX->setEnabled(true);
            else if (radioButtonNumber == 3)
            self->m_Selector.radioBtn_LA_SEPTUM->setEnabled(true);
            else if (radioButtonNumber == 5)
            self->m_Selector.radioBtn_RA_APEX->setEnabled(true);
            else if (radioButtonNumber == 7)
            self->m_Selector.radioBtn_RA_SEPTUM->setEnabled(true);
            else if (radioButtonNumber == 9)
            self->m_Selector.radioBtn_RAA_APEX->setEnabled(true);
            
        }//_if

        self->m_Controls.widget_1->GetRenderWindow()->Render();
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


    roughSeedIds = vtkSmartPointer<vtkIdList>::New();
    roughSeedIds->Initialize();
    roughLineSeeds = vtkSmartPointer<vtkPolyData>::New();
    roughLineSeeds->Initialize();
    roughLineSeeds->SetPoints(vtkSmartPointer<vtkPoints>::New());


    refinedSeedIds = vtkSmartPointer<vtkIdList>::New();
    refinedSeedIds->Initialize();
    refinedLineSeeds = vtkSmartPointer<vtkPolyData>::New();
    refinedLineSeeds->Initialize();
    refinedLineSeeds->SetPoints(vtkSmartPointer<vtkPoints>::New());
}

std::string FourChamberGuidePointsView::GetShortcuts(){
    std::string res = "";
    
    res += " POINT SELECTION:\n\tSpace: select location\n\tDelete: remove location";
    res += "\nHELP:\n\tH/h: Guides";

    return res;
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
            pickedPointsHandler->PushBackLabel(11); // LA_APEX
            m_Selector.radioBtn_LA_APEX->setEnabled(false);
        } else if (m_Selector.radioBtn_LA_SEPTUM->isChecked()) {
            pickedPointsHandler->PushBackLabel(13); // LA_SEPTUM
            m_Selector.radioBtn_LA_SEPTUM->setEnabled(false);
        } else if (m_Selector.radioBtn_RA_APEX->isChecked()) {
            pickedPointsHandler->PushBackLabel(15); // RA_APEX
            m_Selector.radioBtn_RA_APEX->setEnabled(false);
        } else if (m_Selector.radioBtn_RA_SEPTUM->isChecked()) {
            pickedPointsHandler->PushBackLabel(17); // RA_SEPTUM
            m_Selector.radioBtn_RA_SEPTUM->setEnabled(false);
        } else if (m_Selector.radioBtn_RAA_APEX->isChecked()) {
            pickedPointsHandler->PushBackLabel(19); // RAA_APEX
            m_Selector.radioBtn_RAA_APEX->setEnabled(false);
        } 
        
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
