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
 * Surface Mesh Points Selector
 *
 * Cardiac Electromechanics Research Group
 * http://www.cemrgapp.com
 * orod.razeghi@kcl.ac.uk
 *
 * This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
 *
 * =========================================================================*/

// Qmitk
#include <mitkProgressBar.h>
#include <mitkIOUtil.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkMorphologicalOperations.h>

#include "CemrgAtrialTools.h"

// VTK
#include <vtkPointLocator.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkClipPolyData.h>
#include <vtkImplicitBoolean.h>
#include <vtkCenterOfMass.h>
#include <vtkPlane.h>
#include <vtkSphere.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkFloatArray.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolygon.h>
#include <vtkCellArray.h>
#include <vtkConnectivityFilter.h>
#include <vtkImageResize.h>
#include <vtkImageChangeInformation.h>
#include <vtkGeometryFilter.h>
#include <vtkMassProperties.h>
#include <vtkCellLocator.h>

// ITK
#include <itkSubtractImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkImageDuplicator.h>
#include <itkResampleImageFilter.h>
#include <itkNearestNeighborInterpolateImageFunction.h>

// Qt
#include <QtDebug>
#include <QString>
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>
#include <numeric>

CemrgMeshPointSelector::CemrgMeshPointSelector() {
    seedIds = vtkSmartPointer<vtkIdList>::New();
    seedIds->Initialize();

    lineSeeds = vtkSmartPointer<vtkPolyData>::New();
    lineSeeds->Initialize();
    lineSeeds->SetPoints(vtkSmartPointer<vtkPoints>::New());

    seedLabels = std::vector<int>();
    pointNames = QStringList();
}

void CemrgMeshPointSelector::SetAvailableLabels(QStringList names, std::vector<int> labels) {
    pointNames = names;
    availableLabels = labels;
    for (int ix = 0; ix < availableLabels.size(); ix++)     {
        labelSet.push_back(false);
    }
}

void CemrgMeshPointSelector::AddPointFromSurface(mitk::Surface::Pointer surface, int pickedSeedId) {
    double *point = surface->GetVtkPolyData()->GetPoint(pickedSeedId);
    AddPoint(point, pickedSeedId);
}

void CemrgMeshPointSelector::AddPoint(double *point, int pickedSeedId) {
    seedIds->InsertNextId(pickedSeedId);
    lineSeeds->GetPoints()->InsertNextPoint(point);
    lineSeeds->Modified();
}
void CemrgMeshPointSelector::PushBackLabel(int label) {
    seedLabels.push_back(label);
}

void CemrgMeshPointSelector::PushBackLabelFromAvailable(int index) {
    seedLabels.push_back(availableLabels.at(index));
    labelSet.at(index) = true;
}

bool CemrgMeshPointSelector::AllLabelsSet() {
    bool result = true;
    if (IsEmpty())     {
        return false;
    }

    for (int ix = 0; ix < labelSet.size(); ix++)     {
        if (!labelSet.at(ix))         {
            result = false;
            break;
        }
    }
    return result;
}

int CemrgMeshPointSelector::CleanupLastPoint() {
    // Clean up last dropped seed point
    vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkPoints> points = lineSeeds->GetPoints();
    for (int i = 0; i < points->GetNumberOfPoints() - 1; i++)     {
        newPoints->InsertNextPoint(points->GetPoint(i));
    }
    lineSeeds->SetPoints(newPoints);
    vtkSmartPointer<vtkIdList> newSeedIds = vtkSmartPointer<vtkIdList>::New();
    newSeedIds->Initialize();
    vtkSmartPointer<vtkIdList> roughSeedIds = seedIds;
    for (int i = 0; i < roughSeedIds->GetNumberOfIds() - 1; i++)     {
        newSeedIds->InsertNextId(roughSeedIds->GetId(i));
    }
    seedIds = newSeedIds;

    int res = -1;

    if (seedLabels.empty() == false)     {
        res = seedLabels.back();
        seedLabels.pop_back();
    }

    return res;
}

void CemrgMeshPointSelector::Clear() {
    seedIds->Reset();
    lineSeeds->Reset();
    seedLabels.clear();
    pointNames.clear();
    availableLabels.clear();
}

std::string CemrgMeshPointSelector::ToString() {
    std::string res = "";
    for (int i = 0; i < seedLabels.size(); i++)     {
        res += "Label: " + std::to_string(seedLabels.at(i)) + " Point ID: " + std::to_string(seedIds->GetId(i)) + '\n';
    }

    for (int j = 0; j < lineSeeds->GetPoints()->GetNumberOfPoints(); j++)     {
        double *point = lineSeeds->GetPoints()->GetPoint(j);
        res += "Point: (" + std::to_string(point[0]) + ", " + std::to_string(point[1]) + ", " + std::to_string(point[2]) + ")\n";
    }

    for (int k = 0; k < seedLabels.size(); k++)     {
        res += "Label: " + std::to_string(seedLabels.at(k));
    }

    return res;
}

int CemrgMeshPointSelector::FindLabel(QString name) {
    int res = -1;
    for (int i = 0; i < pointNames.size(); i++)     {
        if (pointNames.at(i) == name)         {
            res = availableLabels.at(i);
            break;
        }
    }
    return res;
}

int CemrgMeshPointSelector::FindIndex(int label) {
    int res = -1;
    for (int i = 0; i < availableLabels.size(); i++)     {
        if (availableLabels.at(i) == label)         {
            res = i;
            break;
        }
    }
    return res;
}

std::vector<double> CemrgMeshPointSelector::FindPoint(int index) {
    if (index < 0 || index >= lineSeeds->GetPoints()->GetNumberOfPoints())     {
        return std::vector<double>();
    }

    double *point = lineSeeds->GetPoints()->GetPoint(index);
    std::vector<double> res = {point[0], point[1], point[2]};

    return res;
}

std::string CemrgMeshPointSelector::PrintVtxFile(QString name) {
    std::string res = "";

    // look for label with name in available labels
    int thisLabel = FindLabel(name);

    // look for ID with label in seedLabels
    int thisId = FindIndex(thisLabel);

    res = "1\nextra\n" + std::to_string(thisId) + '\n';
    return res;
}

std::string CemrgMeshPointSelector::PrintCoordTxtFile(QString name) {
    std::string res = "";
    int thisLabel = FindLabel(name);
    int thisId = FindIndex(thisLabel);
    std::vector<double> point = FindPoint(thisId);

    res = std::to_string(point[0]) + " " + std::to_string(point[1]) + " " + std::to_string(point[2]) + '\n';
    return res;
}

void CemrgMeshPointSelector::SaveToFile(QString path, Qstring name, QString type = "vtx") {
    std::string toPrint;
    if (type == "vtx")     {
        toPrint = PrintVtxFile(name);
    }
    else if (type == "coord")     {
        toPrint = PrintCoordTxtFile(name);
    }

    std::ofstream file;
    file.open(path.toStdString());
    file << toPrint;
    file.close();
}
