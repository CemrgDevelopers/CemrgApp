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

#ifndef FourChamberGuidePointsView_h
#define FourChamberGuidePointsView_h

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QMessageBox>
#include <vtkIdList.h>
#include <vtkActor.h>

#include <CemrgMeshPointSelector.h>

#include "ui_FourChamberGuidePointsViewControls.h"
#include "ui_FourChamberGuidePointsViewSelector.h"
/**
  \brief FourChamberGuidePointsView

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class FourChamberGuidePointsView : public QmitkAbstractView {

    // this is needed for all Qt objects that should have a Qt meta-object
    // (everything that derives from QObject and wants to have signal/slots)
    Q_OBJECT

public:

    static const std::string VIEW_ID;

    static QString fileName;
    static QString directory;
    static QString whichAtrium;

    static void SetDirectoryFile(const QString directory);
    
    ~FourChamberGuidePointsView();

    void SetSubdirs();
    bool CreateVisualisationMesh(QString dir, QString visName, QString originalName);
    std::string GetShortcuts();

protected slots:

    /// \brief Called when the user clicks the GUI button
    void Help(bool firstTime=false);
    void LeftAtriumReactToToggle(); //
    void RightAtriumReactToToggle(); //
    void CheckBoxShowAll(int checkedState); //
    void ChangeAlpha(int alpha); //
    void Save();

protected:

    virtual void CreateQtPartControl(QWidget *parent) override;
    virtual void SetFocus() override;
    /// \brief called by QmitkFunctionality when DataManager's selection has changed
    virtual void OnSelectionChanged(
            berry::IWorkbenchPart::Pointer source, const QList<mitk::DataNode::Pointer>& nodes) override;

    Ui::FourChamberGuidePointsViewControls m_Controls;
    Ui::FourChamberGuidePointsViewSelector m_Selector;

private:

    void iniPreSurf();
    void Visualiser(double opacity=1.0);

    void SphereSourceVisualiser(vtkSmartPointer<vtkPolyData> pointSources, QString colour="1.0,0.0,0.0", double scaleFactor=0.01);
    void PickCallBack();
    static void KeyCallBackFunc(vtkObject*, long unsigned int, void* ClientData, void*);

    void InitialisePickerObjects();

    void UserSelectPvLabel();

    QString parfiles;
    QString apex_septum_files;
    QString surfaces_uvc_la;
    QString surfaces_uvc_ra;
    QString path_to_la;
    QString path_to_ra;

    std::unique_ptr<CemrgMeshPointSelector> pickedPointsHandler;
    int pushedLabel;

    QDialog* inputsSelector;

    double alpha;

    vtkSmartPointer<vtkActor> surfActor;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkCallbackCommand> callBack;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    mitk::Surface::Pointer surface;

    bool pluginLoaded = false;
};

#endif // FourChamberGuidePointsView_h
