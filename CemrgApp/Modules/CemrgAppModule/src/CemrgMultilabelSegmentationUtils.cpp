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
 * j.solis-lemus@imperial.ac.uk
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
#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkLabelImageGaussianInterpolateImageFunction.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkLabelMapToLabelImageFilter.h>
#include <itkResampleImageFilter.h>
#include <itkLabelImageGaussianInterpolateImageFunction.h>

#include "CemrgCommonUtils.h"
#include "CemrgAtrialTools.h"
#include "CemrgMultilabelSegmentationUtils.h"

CemrgMultilabelSegmentationUtils::CemrgMultilabelSegmentationUtils() {
}

CemrgMultilabelSegmentationUtils::~CemrgMultilabelSegmentationUtils(){

}


// Segmentation Utilities
void CemrgMultilabelSegmentationUtils::ExploreLabelsToSplit(mitk::Image::Pointer seg, std::vector<int> &labels) {
    std::vector<int> labelsInSeg;
    GetLabels(seg, labelsInSeg);

    for (long unsigned int ix = 0; ix < labelsInSeg.size(); ix++) { 
        int label = labelsInSeg.at(ix);
        std::vector<int> tagsInLabel;
        mitk::Image::Pointer bwlabelnInLabel = BwLabelN(ExtractSingleLabel(seg, label), tagsInLabel, true);
        if (tagsInLabel.size() > 1) {
            labels.push_back(label);
        }
    }
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::SplitLabelsOnRepeat(mitk::Image::Pointer seg, int label, unsigned int radius) {
    
    std::vector<int> labelsInSeg;
    GetLabels(seg, labelsInSeg);
    // remove label from labelsInSeg
    labelsInSeg.erase(std::remove(labelsInSeg.begin(), labelsInSeg.end(), label), labelsInSeg.end());

    mitk::Image::Pointer imageLabel = ExtractSingleLabel(seg, label, true);
    ImageType::Pointer itkLabel = ImageType::New();
    CastToItkImage(imageLabel, itkLabel);

    using StrElType = itk::BinaryBallStructuringElement<ImageType::PixelType, ImageType::ImageDimension>;
    using OpeningType = itk::BinaryMorphologicalOpeningImageFilter<ImageType, ImageType, StrElType>;
    StrElType strel;
    strel.SetRadius(radius);
    strel.CreateStructuringElement();
    OpeningType::Pointer imopen = OpeningType::New();
    imopen->SetInput(itkLabel);
    imopen->SetKernel(strel);
    imopen->Update();

    std::vector<int> tagsInLabel;
    mitk::Image::Pointer ccImLabel = BwLabelN(mitk::ImportItkImage(imopen->GetOutput()) , tagsInLabel, true);

    if (tagsInLabel.size() == 1) {
        MITK_INFO << "No repeated labels found";
        return seg;
    }

    ImageType::Pointer itkImage = ImageType::New();
    CastToItkImage(seg, itkImage);
    ImageType::Pointer itkCcImLabel = ImageType::New();
    CastToItkImage(ccImLabel, itkCcImLabel);

    // Remove label from seg
    IteratorType it(itkImage, itkImage->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it) {
        if (it.Get() == label) {
            it.Set(0);
        }
    }

    for (long unsigned int ix = 0; ix < tagsInLabel.size(); ix++) {
        int tag = tagsInLabel.at(ix);
        int newLabel = label*10 + (tag-1);

        int qx = 1;
        // Use std::find to check if the variable is in the vector
        auto it = std::find(tagsInLabel.begin(), tagsInLabel.end(), newLabel);
        while (it != tagsInLabel.end()) {
            newLabel = label*std::pow(10, qx) + (tag-1);
            qx++;
            it = std::find(tagsInLabel.begin(), tagsInLabel.end(), newLabel);
        }

        IteratorType ccIt(itkCcImLabel, itkCcImLabel->GetLargestPossibleRegion());
        IteratorType segIt(itkImage, itkImage->GetLargestPossibleRegion());

        segIt.GoToBegin();
        for (ccIt.GoToBegin(); !ccIt.IsAtEnd(); ++ccIt) {
            if (ccIt.Get() == tag) {
                segIt.Set(newLabel);
            }
            ++segIt;
        }
    }

    mitk::Image::Pointer newSeg = mitk::ImportItkImage(itkImage)->Clone();
    return newSeg;

}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::ReplaceLabel(mitk::Image::Pointer seg, int oldLabel, int newLabel) {
    ImageType::Pointer itkImage = ImageType::New();
    CastToItkImage(seg, itkImage);

    IteratorType segIt(itkImage, itkImage->GetLargestPossibleRegion());
    for (segIt.GoToBegin(); !segIt.IsAtEnd(); ++segIt) {
        if (segIt.Get() == oldLabel) {
            segIt.Set(newLabel);
        }
    }
    
    mitk::Image::Pointer newSeg = mitk::ImportItkImage(itkImage)->Clone();
    newSeg->SetGeometry(seg->GetGeometry());
    
    return newSeg;
}


mitk::Image::Pointer CemrgMultilabelSegmentationUtils::ResampleSmoothLabel(mitk::Image::Pointer image, std::vector<double> spacing, double sigmaFraction, double alphaFraction) {

    using ImageType = itk::Image<unsigned char, 3>;
    using ResampleImageFilterType = itk::ResampleImageFilter<ImageType, ImageType>;
    using GaussianInterpolatorType = itk::LabelImageGaussianInterpolateImageFunction<ImageType, double>;
    ImageType::Pointer input = ImageType::New();
    mitk::CastToItkImage(image, input);

    ImageType::SizeType input_size = input->GetLargestPossibleRegion().GetSize();
    ImageType::SpacingType input_spacing = input->GetSpacing();
    ImageType::SizeType output_size;
    ImageType::SpacingType output_spacing;
    for (int i = 0; i < 3; ++i) {
        output_size[i] = input_size[i] * (input_spacing[i] / spacing[i]);
        output_spacing[i] = spacing[i];
    } //_for

    GaussianInterpolatorType::Pointer gaussianInterpolator = GaussianInterpolatorType::New();
    GaussianInterpolatorType::ArrayType sigma;
    for (unsigned int dim = 0; dim < 3; ++dim) {
        sigma[dim] = output_spacing[dim] * sigmaFraction;
    }
    gaussianInterpolator->SetSigma(sigma);
    gaussianInterpolator->SetAlpha(alphaFraction);

    ResampleImageFilterType::Pointer resizeFilter = ResampleImageFilterType::New();
    resizeFilter->SetInput(input);
    resizeFilter->SetInterpolator(gaussianInterpolator);
    resizeFilter->SetSize(output_size);
    resizeFilter->SetOutputSpacing(output_spacing);
    resizeFilter->SetOutputOrigin(input->GetOrigin());
    resizeFilter->SetOutputDirection(input->GetDirection());
    resizeFilter->UpdateLargestPossibleRegion();

    mitk::Image::Pointer outputIm = mitk::ImportItkImage(resizeFilter->GetOutput())->Clone();
    // outputIm->SetGeometry(image->GetGeometry());
    return outputIm;
}

bool CemrgMultilabelSegmentationUtils::CheckExisting(mitk::Image::Pointer seg, int queryLabel) {
    std::vector<int> labelsInSeg;
    GetLabels(seg, labelsInSeg);

    MITK_INFO << ("Checking if label " + QString::number(queryLabel) + " exists in segmentation").toStdString();
    auto it = std::find(labelsInSeg.begin(), labelsInSeg.end(), queryLabel);

    return (it != labelsInSeg.end()); 
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::LabelMaskAndOperation(mitk::Image::Pointer seg, mitk::Image::Pointer mask, int oldLabel, int newLabel) {
    ImageType::Pointer itkImage = ImageType::New() ;
    mitk::CastToItkImage(seg, itkImage);

    ImageType::Pointer itkMask = ImageType::New() ;
    mitk::CastToItkImage(mask, itkMask);

    if (itkImage.IsNull() || itkMask.IsNull()) {
        MITK_ERROR << "LabelMaskAndOperation: Invalid input image or mask";
        return nullptr;
    }

    IteratorType segIt(itkImage, itkImage->GetLargestPossibleRegion());
    IteratorType maskIt(itkMask, itkMask->GetLargestPossibleRegion());

    for (segIt.GoToBegin(), maskIt.GoToBegin(); !segIt.IsAtEnd(); ++segIt, ++maskIt) {
        if (maskIt.Get() > 0) {
            int valueToSet = (segIt.Get() == oldLabel) ? newLabel : 0;
            segIt.Set(valueToSet);
        }
    }

    mitk::Image::Pointer newSeg = mitk::ImportItkImage(itkImage)->Clone();
    newSeg->SetGeometry(seg->GetGeometry());
    return newSeg;
}

void CemrgMultilabelSegmentationUtils::GetLabels(mitk::Image::Pointer seg, std::vector<int> &labels, int background) {
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(seg, itkImage);

    IteratorType it(itkImage, itkImage->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it) {
        int label = it.Get();
        if (label != background) {
            labels.push_back(label);
        }
    }

    std::sort(labels.begin(), labels.end());
    labels.erase(std::unique(labels.begin(), labels.end()), labels.end());
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::ExtractSingleLabel(mitk::Image::Pointer seg, int label, bool binarise) {
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(seg, itkImage);

    ImageType::Pointer outImage = ImageType::New();
    outImage->SetRegions(itkImage->GetLargestPossibleRegion());
    outImage->SetSpacing(itkImage->GetSpacing());
    outImage->SetOrigin(itkImage->GetOrigin());
    outImage->Allocate();
    outImage->FillBuffer(0);

    IteratorType it(itkImage, itkImage->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it) {
        int valueAtIter = it.Get();
        int valueInOutput = binarise ? 1 : valueAtIter;
        if (valueAtIter == label) {
            outImage->SetPixel(it.GetIndex(), valueInOutput);
        }
    }

    mitk::Image::Pointer outSeg = mitk::ImportItkImage(outImage)->Clone();

    return outSeg;
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::ExtractSetOfLabels(mitk::Image::Pointer seg, std::vector<int> labels, bool binarise) {
    std::vector<int> labelsInSegmentation;
    GetLabels(seg, labelsInSegmentation);

    std::vector<int> labelsToExtract;
    // add labels that are in both labels and labelsInSegmentation 
    std::set_intersection(labels.begin(), labels.end(), labelsInSegmentation.begin(), labelsInSegmentation.end(), std::back_inserter(labelsToExtract));

    using AddFilterType = itk::AddImageFilter<ImageType, ImageType, ImageType>;

    mitk::Image::Pointer outImage = ZerosLike(seg);
    ImageType::Pointer itkOutImage = ImageType::New();
    mitk::CastToItkImage(outImage, itkOutImage);
    
    for (long unsigned int ix = 0; ix < labelsToExtract.size(); ix++) {
        int label = labelsToExtract.at(ix);
        ImageType::Pointer itkLabel = ImageType::New();
        mitk::CastToItkImage(ExtractSingleLabel(seg, label, binarise), itkLabel);

        AddFilterType::Pointer addFilter = AddFilterType::New();
        addFilter->SetInput1(itkOutImage);
        addFilter->SetInput2(itkLabel);
        addFilter->Update();

        itkOutImage = addFilter->GetOutput();        
    }

    mitk::Image::Pointer outSeg = mitk::ImportItkImage(itkOutImage)->Clone();
    return outSeg;
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::BwLabelN(mitk::Image::Pointer seg, std::vector<int> &labels, bool openImage) {
    // assumes seg is a binary image
    using ConnectedComponentImageFilterType = itk::ConnectedComponentImageFilter<ImageType, ImageType>;
    using RelabelFilterType = itk::RelabelComponentImageFilter<ImageType, ImageType> ;

    if (openImage) {
        mitk::MorphologicalOperations::Opening(seg, 1, mitk::MorphologicalOperations::StructuralElementType::Ball);
    }

    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(seg, itkImage);

    ConnectedComponentImageFilterType::Pointer conn1 = ConnectedComponentImageFilterType::New();
    conn1->SetInput(itkImage);
    conn1->Update();

    RelabelFilterType::Pointer labelled = RelabelFilterType::New();
    labelled->SetInput(conn1->GetOutput());
    labelled->Update();

    mitk::Image::Pointer outImage = mitk::ImportItkImage(labelled->GetOutput())->Clone();
    
    GetLabels(outImage, labels);
    
    return outImage;
}

itk::Image<uint8_t, 3>::Pointer CemrgMultilabelSegmentationUtils::ItkBwLabelN(mitk::Image::Pointer seg, std::vector<int> &labels, bool openImage) {
    mitk::Image::Pointer outImage = BwLabelN(seg, labels, openImage);
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(outImage, itkImage);
    return itkImage;
}

bool CemrgMultilabelSegmentationUtils::GetLabelCentreOfMassIndex(mitk::Image::Pointer seg, int label, std::vector<unsigned int> &cogIndx) {
    using LabelStatisticsFilterType = itk::LabelStatisticsImageFilter<ImageType, ImageType>;

    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(seg, itkImage);

    LabelStatisticsFilterType::Pointer labelStats = LabelStatisticsFilterType::New();
    labelStats->SetInput(itkImage);
    labelStats->SetLabelInput(itkImage); // Use the segmentation image as label
    labelStats->Update();

    LabelStatisticsFilterType::LabelPixelType labelValue = label;
    LabelStatisticsFilterType::BoundingBoxType bb = labelStats->GetBoundingBox(labelValue);

    if (bb.empty()) {
        return false;
    }
    
    for (unsigned int ix = 0; ix < 3; ix++) {
        cogIndx.push_back((bb[2*ix] + bb[2*ix + 1]) / 2);
    }

    return true;
}

bool CemrgMultilabelSegmentationUtils::GetLabelCentreOfMass(mitk::Image::Pointer seg, int label, std::vector<double> &cog) {
    std::vector<unsigned int> cogIndx;
    bool result = GetLabelCentreOfMassIndex(seg, label, cogIndx);    
    if (!result) {
        return false;
    }

    IndexToWorld(seg, cogIndx, cog);

    return true;
}

void CemrgMultilabelSegmentationUtils::WorldToIndex(mitk::Image::Pointer image, std::vector<double> world, std::vector<unsigned int>& index) {
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(image, itkImage);

    ImageType::PointType point;
    ImageType::IndexType indexType;
    for (unsigned int ix = 0; ix < 3; ix++) {
        point[ix] = world.at(ix);
    }

    itkImage->TransformPhysicalPointToIndex(point, indexType);

    for (unsigned int ix = 0; ix < 3; ix++) {
        index.push_back(indexType[ix]);
    }
}

void CemrgMultilabelSegmentationUtils::IndexToWorld(mitk::Image::Pointer image, std::vector<unsigned int> index, std::vector<double> &world) {
    
    for (unsigned int ix = 0; ix < 3; ix++) {
        double spacing = image->GetGeometry()->GetSpacing()[ix];
        double origin = image->GetGeometry()->GetOrigin()[ix];

        world.push_back( (index[ix] * spacing) + origin );   
    }

}

void CemrgMultilabelSegmentationUtils::WorldToIndexOriginSpacing(std::vector<double> world, std::vector<unsigned int>& index, std::vector<double> origin, std::vector<double> spacing) {
    for (unsigned int ix = 0; ix < 3; ix++) {
        // round value
        unsigned int value = (unsigned int) std::round( (world[ix] - origin[ix]) / spacing[ix] );
        index.push_back(value);
    }
}

void CemrgMultilabelSegmentationUtils::IndexToWorldOriginSpacing(std::vector<unsigned int> index, std::vector<double> &world, std::vector<double> origin, std::vector<double> spacing) {
    for (unsigned int ix = 0; ix < 3; ix++) {
        world.push_back( (index[ix] * spacing[ix]) + origin[ix] );   
    }
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::ConnectedComponent(mitk::Image::Pointer seg, std::vector<unsigned int> seedIdx, int layer, bool keep) {
    using ConnectedThresholdType = itk::ConnectedThresholdImageFilter<ImageType, ImageType>;
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(seg, itkImage);
    ImageType::IndexType seedPointIndex;
    seedPointIndex[0] = seedIdx.at(0);
    seedPointIndex[1] = seedIdx.at(1);
    seedPointIndex[2] = seedIdx.at(2);
    
    ConnectedThresholdType::Pointer cc = ConnectedThresholdType::New();
    cc->SetInput(itkImage);
    cc->SetSeed(seedPointIndex);
    cc->SetLower(layer);
    cc->SetUpper(layer);

    mitk::Image::Pointer ccImage = mitk::ImportItkImage(cc->GetOutput())->Clone();
    ccImage->SetGeometry(seg->GetGeometry());

    int replaceValue = 0;
    if (keep) {
        seg = RemoveLabel(seg, layer);
        replaceValue = layer;
    }

    // AddMasksReplace 
    ImageType::Pointer itkCcImage = ImageType::New();
    mitk::CastToItkImage(ccImage, itkCcImage);
    mitk::CastToItkImage(seg, itkImage);

    IteratorType it(itkImage, itkImage->GetLargestPossibleRegion());
    IteratorType itCc(itkCcImage, itkCcImage->GetLargestPossibleRegion());
    it.GoToBegin();
    itCc.GoToBegin();
    while (!it.IsAtEnd()) {
        if (itCc.Get() > 0) {
            it.Set(replaceValue);
        }
        ++it;
        ++itCc;
    }
    
    mitk::Image::Pointer outSeg = mitk::ImportItkImage(itkImage)->Clone();

    return outSeg;

}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::ExtractLargestComponent(mitk::Image::Pointer seg, int numLabels, int outputLabel) {
    using ConnectedComponentImageFilterType = itk::ConnectedComponentImageFilter<ImageType, ImageType>;
    using KeepNObjectsImageFilterType = itk::LabelShapeKeepNObjectsImageFilter<ImageType>;

    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(seg, itkImage);

    ConnectedComponentImageFilterType::Pointer conn1 = ConnectedComponentImageFilterType::New();
    conn1->SetInput(itkImage);
    conn1->Update();

    KeepNObjectsImageFilterType::Pointer keepNObjects = KeepNObjectsImageFilterType::New();
    keepNObjects->SetInput(conn1->GetOutput());
    keepNObjects->SetBackgroundValue(0);
    keepNObjects->SetNumberOfObjects(numLabels);
    keepNObjects->SetAttribute(KeepNObjectsImageFilterType::LabelObjectType::NUMBER_OF_PIXELS);
    keepNObjects->Update();

    ImageType::Pointer outImage = keepNObjects->GetOutput();
    IteratorType it(outImage, outImage->GetLargestPossibleRegion());

    it.GoToBegin();
    while (!it.IsAtEnd()) {
        if (it.Get() > 0) {
            it.Set(outputLabel);
        }
        ++it;
    }

    mitk::Image::Pointer outSeg = mitk::ImportItkImage(outImage)->Clone();
    outSeg->SetGeometry(seg->GetGeometry());

    return outSeg;
}

/**
 * @brief Cleans a multilabel segmentation by removing small connected components.
 *
 * This function takes a multilabel segmentation image and removes small connected components
 * that have the same label as the background label. It iterates over each label in the image,
 * extracts the connected components for that label, and checks if there are multiple components.
 * If there are multiple components, it removes all components except the largest one by setting
 * their label to the background label. The resulting cleaned segmentation image is returned.
 *
 * @param seg The input multilabel segmentation image.
 * @param background The label value representing the background.
 * @return The cleaned multilabel segmentation image.
 */
mitk::Image::Pointer CemrgMultilabelSegmentationUtils::CleanMultilabelSegmentation(mitk::Image::Pointer seg, int background) {
    std::vector<int> labels;
    GetLabels(seg, labels);

    mitk::Image::Pointer outImage = seg->Clone();
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(outImage, itkImage);

    for (unsigned int ix = 0; ix < labels.size(); ix++) {
        int label = labels.at(ix);
        if (label == background) {
            continue;
        }

        std::vector<int> testLabels;
        ImageType::Pointer labelImage = ItkBwLabelN(ExtractSingleLabel(seg, label), testLabels);

        int numLabels = testLabels.size();
        if (numLabels > 1) {
            MITK_INFO << "Cleaning label " << label << " with " << numLabels << " components.";
            for (unsigned int jx = 1; jx < testLabels.size(); jx++) {
                int rmLabel = testLabels.at(jx);

                IteratorType it(itkImage, itkImage->GetLargestPossibleRegion());
                IteratorType itLabel(labelImage, labelImage->GetLargestPossibleRegion());
                for (it.GoToBegin(), itLabel.GoToBegin(); !it.IsAtEnd(); ++it, ++itLabel) {
                    if (itLabel.Get() == rmLabel) {
                        it.Set(background);
                    }
                }
            } //_for jx
        } //_if
    } //_for ix

    outImage = mitk::ImportItkImage(itkImage)->Clone();
    outImage->SetGeometry(seg->GetGeometry());

    return outImage;
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::DistanceMap(mitk::Image::Pointer seg, int label) {
    using DistanceMapType = itk::DanielssonDistanceMapImageFilter<ImageType, ImageType>;
    
    mitk::Image::Pointer labelImage = ExtractSingleLabel(seg, label, true);

    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(labelImage, itkImage);

    DistanceMapType::Pointer distanceMap = DistanceMapType::New();
    distanceMap->InputIsBinaryOn(); // ask/test this (default OFF)
    distanceMap->SquaredDistanceOff();
    distanceMap->UseImageSpacingOn(); // ask/test this (default OFF)
    distanceMap->SetInput(itkImage);
    distanceMap->Update();

    mitk::Image::Pointer outImage = mitk::ImportItkImage(distanceMap->GetOutput())->Clone();
    outImage->SetGeometry(seg->GetGeometry());

    return outImage;
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::Threshold(mitk::Image::Pointer seg, int label, int lower, int upper) {
    using ThresholdType = itk::BinaryThresholdImageFilter<ImageType, ImageType>;
    
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(seg, itkImage);

    ThresholdType::Pointer threshold = ThresholdType::New();
    threshold->SetInput(itkImage);
    threshold->SetLowerThreshold(lower);
    threshold->SetUpperThreshold(upper);
    // threshold->SetInsideValue(1);
    threshold->SetOutsideValue(0);
    threshold->Update();

    mitk::Image::Pointer outImage = mitk::ImportItkImage(threshold->GetOutput())->Clone();
    outImage->SetGeometry(seg->GetGeometry());

    std::cout << "Label: " << label << '\n';
    return outImage;
}

mitk::Image::Pointer CemrgMultilabelSegmentationUtils::ZerosLike(mitk::Image::Pointer image) {
    ImageType::Pointer itkImage = ImageType::New();
    mitk::CastToItkImage(image, itkImage);
    
    itkImage->SetRegions(itkImage->GetLargestPossibleRegion());
    itkImage->Allocate();
    itkImage->FillBuffer(0);
    
    mitk::Image::Pointer outImage = mitk::ImportItkImage(itkImage)->Clone();
    outImage->SetGeometry(image->GetGeometry());

    return outImage;
}
