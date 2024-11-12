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

#ifndef FourChamberLandmarksView_h
#define FourChamberLandmarksView_h

#include <berryISelectionListener.h>
#include <mitkUnstructuredGrid.h>
#include <QmitkAbstractView.h>
#include <QMessageBox>
#include <vtkIdList.h>
#include <vtkActor.h>
#include <CemrgAtriaClipper.h>
#include <CemrgScarAdvanced.h>
#include <FourChamberCommon.h>

#include "ui_FourChamberLandmarksViewControls.h"
#include "ui_FourChamberLandmarksViewPointLabel.h"
// #include "ui_FourChamberLandmarksViewRefined.h"

/**
  \brief FourChamberLandmarksView

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class FourChamberLandmarksView : public QmitkAbstractView {

    // this is needed for all Qt objects that should have a Qt meta-object
    // (everything that derives from QObject and wants to have signal/slots)
    Q_OBJECT

public:

    static const std::string VIEW_ID;
    static void SetDirectoryFile(const QString directory, const QString laSubdir, const QString laName, const QString raSubdir, const QString raName);
    ~FourChamberLandmarksView();

    // helper functions

protected slots:

    /// \brief Called when the user clicks the GUI button
    void Help();
    void SaveSelectedPoints();
    void ComboSelectWorkingMesh(int index);

protected :
        
    virtual void CreateQtPartControl(QWidget *parent) override;
    virtual void SetFocus() override;
    /// \brief called by QmitkFunctionality when DataManager's selection has changed
    virtual void OnSelectionChanged(
            berry::IWorkbenchPart::Pointer source, const QList<mitk::DataNode::Pointer>& nodes) override;

    Ui::FourChamberLandmarksViewControls m_Controls;
    Ui::FourChamberLandmarksViewPointLabel m_PickedPoints;

private:

    void iniPreSurf();
    void Visualiser(double opacity=1.0);

    void SphereSourceVisualiser(vtkSmartPointer<vtkPolyData> pointSources, QString colour="1.0,0.0,0.0", double scaleFactor=0.1);
    void PickCallBack();
    static void KeyCallBackFunc(vtkObject*, long unsigned int, void* ClientData, void*);

    void UserSelectLabel();
    void RemoveGlyphActor();

    static QString directory;
    static QString laSubdir, laName;
    static QString raSubdir, raName;

    QStringList outputFilesSelected;
    QStringList names;
    std::vector<int> availableLabels;

    int countChanges; // count the number of changes in the selected combo box

    mitk::Surface::Pointer surface;
    mitk::UnstructuredGrid::Pointer ugrid;
    vtkSmartPointer<vtkActor> surfActor;

    PickedPointType pickedPoint;

    std::vector<int> roughSeedLabels;
    vtkSmartPointer<vtkIdList> roughSeedIds;
    vtkSmartPointer<vtkPolyData> roughLineSeeds;

    std::vector<int> refinedSeedLabels;
    vtkSmartPointer<vtkIdList> refinedSeedIds;
    vtkSmartPointer<vtkPolyData> refinedLineSeeds;

    QDialog* inputsPickedPoints;
    double maxScalar, minScalar;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkCallbackCommand> callBack;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkActor> glyphActor;

};

#endif // FourChamberLandmarksView_h
