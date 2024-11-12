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

#ifndef CemrgFourChamberTools_h
#define CemrgFourChamberTools_h

#include <QString>
#include <QJsonObject>

#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkCommon.h>

// VTK
#include <vtkSmartPointer.h>
#include <vtkConnectivityFilter.h>
#include <vtkIdList.h>
#include <vtkRegularPolygonSource.h>

// ITK
#include <itkImage.h>
#include <itkImageRegionIterator.h>

#include "CemrgMultilabelSegmentationUtils.h"
#include "FourChamberCommon.h"
#include "CemrgCommandLine.h"


class MITKCEMRGAPPMODULE_EXPORT CemrgFourChamberTools {
    typedef itk::Image<uint8_t, 3> ImageType;
    typedef itk::ImageRegionIterator<ImageType> IteratorType;
    
    public:
        CemrgFourChamberTools();
        ~CemrgFourChamberTools();

        inline void SetDebug(bool value) { debug = value; };
        inline void SetDebugOn() { SetDebug(true); };
        inline void SetDebugOff() { SetDebug(false); };

        void SetSegDir(std::string segDirectory);

        inline std::string GetSegDir() { return segDir; };
        inline std::string GetDebugDir() { return directory; };
        inline QString QGetSegDir() { return QString::fromStdString(segDir); };
        inline QString QGetDebugDir() { return QString::fromStdString(directory); };

        inline void SetLabel(LabelsType stt, int label) { chosenLabels.Set(stt, label); };
        inline void UpdateChosenLabels(SegmentationLabels newLabels) { chosenLabels = newLabels; };

        // Manipulation utilities
        inline void SetCylinders(QJsonObject json){ _cylinders.SetPointsFromJson(json); };
        inline void SetSlicers(QJsonObject json) { _slicers.SetPointsFromJson(json); };
        inline void SetValvePoints(QJsonObject json) { _valvePoints.SetPointsFromJson(json); };

        inline bool CylindersSet() { return _cylinders.IsPointSet(); };
        inline bool SlicersSet() { return _slicers.IsPointSet(); };
        inline bool ValvePointsSet() { return _valvePoints.IsPointSet(); };

    private : 
        CylinderPointsType _cylinders;
        SlicersPointsType _slicers;
        ValvePlainsPointsType _valvePoints;

        bool debug;
        std::string segDir, directory;

        mitk::Image::Pointer currentImage;
        SegmentationLabels chosenLabels;
};
#endif // CemrgFourChamberTools_h
