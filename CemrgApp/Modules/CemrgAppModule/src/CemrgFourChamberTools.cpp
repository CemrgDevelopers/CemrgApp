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
 * Four Chamber Tools (inherits CemrgCommandLine)
 *
 * Cardiac Electromechanics Research Group
 * http://www.cemrgapp.com
 * orod.razeghi@kcl.ac.uk
 *
 * This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
 *
=========================================================================*/

// Qt
#include <QDir>
#include <QDirIterator>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>

// MITK
#include <mitkIOUtil.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkMorphologicalOperations.h>
#include <mitkImagePixelWriteAccessor.h>

// ITK
#include <itkConnectedComponentImageFilter.h>
#include <itkConnectedThresholdImageFilter.h>
#include <itkLabelShapeKeepNObjectsImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkBinaryMorphologicalOpeningImageFilter.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkLabelStatisticsImageFilter.h>

#include "CemrgCommonUtils.h"
#include "CemrgAtrialTools.h"
#include "CemrgFourChamberTools.h"

struct PointList {
    std::vector<double> pt1, pt2, pt3, v1, v2, _normal;
    PointList() 
        : pt1(3, 0.0), 
          pt2(3, 0.0), 
          pt3(3, 0.0), 
          v1(3, 0.0), v2(3, 0.0), _normal(3, 0.0) {};
    
    void FillPoint(int whichPoint, std::vector<double> point) {
        switch (whichPoint) {
            case 1: pt1 = point; break;
            case 2: pt2 = point; break;
            case 3: pt3 = point; break;
            default: break;
        }
    }

    std::vector<double> pt(int whichPoint) {
        switch (whichPoint) {
            case 1: return pt1;
            case 2: return pt2;
            case 3: return pt3;
            default: return std::vector<double>(3, 0.0);
        }
    }

    void normalise(std::vector<double>& v) {
        double norm = std::sqrt(std::pow(v.at(0), 2) + std::pow(v.at(1), 2) + std::pow(v.at(2), 2));
        for (int ix = 0; ix < 3; ix++) {
            v.at(ix) /= norm;
        }
    }

    std::vector<double> v(int a, int b) {
        std::vector<double> v(3, 0.0);
        for (int ix = 0; ix < 3; ix++) {
            v.at(ix) = pt(b).at(ix) - pt(a).at(ix);
        }
        normalise(v);

        return v;
    }

    void cross() {
        _normal.at(0) = v1.at(1)*v2.at(2) - v1.at(2)*v2.at(1);
        _normal.at(1) = v1.at(2)*v2.at(0) - v1.at(0)*v2.at(2);
        _normal.at(2) = v1.at(0)*v2.at(1) - v1.at(1)*v2.at(0);
        normalise(_normal);
    }

    void SetVectors() {
        v1 = v(1, 2);
        v2 = v(1, 3);
    }

    std::vector<double> normal() { return _normal; };

    std::vector<double> getCross(std::vector<double> a, std::vector<double> b) {
        std::vector<double> cross(3, 0.0);
        cross.at(0) = a.at(1)*b.at(2) - a.at(2)*b.at(1);
        cross.at(1) = a.at(2)*b.at(0) - a.at(0)*b.at(2);
        cross.at(2) = a.at(0)*b.at(1) - a.at(1)*b.at(0);
        return cross;
    }

    double getNorm(std::vector<double> a) {
        return std::sqrt(std::pow(a.at(0), 2) + std::pow(a.at(1), 2) + std::pow(a.at(2), 2));
    }
};

// CemrgFourChamberTools
CemrgFourChamberTools::CemrgFourChamberTools(){
    _cylinders = CylinderPointsType();
    _slicers = SlicersPointsType();
    _valvePoints = ValvePlainsPointsType();
    debug = false;
    directory = "";
}

CemrgFourChamberTools::~CemrgFourChamberTools(){
}

void CemrgFourChamberTools::SetSegDir(std::string segDirectory) {
    segDir = segDirectory;
    directory = segDir + "/tmp";
}