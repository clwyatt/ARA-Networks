#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include <string>
using std::string;

#include <cstdlib>

// ITK IO includes
#include <itkOrientedImage.h>
#include <itkImageFileReader.h>
#include <itkThresholdImageFilter.h>
#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkImageFileWriter.h>
#include <itkResampleImageFilter.h>
#include <itkAffineTransform.h>
#include <itkNearestNeighborInterpolateImageFunction.h>

#include "vul_arg.h"

// define ITK types
const unsigned int Dimension = 3;
typedef unsigned short LabelPixelType;
typedef float FloatPixelType;
typedef itk::OrientedImage< LabelPixelType, Dimension > LabelImageType;
typedef itk::OrientedImage< FloatPixelType, Dimension > FloatImageType;

const LabelPixelType LEFT_WM_LABEL = 41;
const LabelPixelType RIGHT_WM_LABEL = 2;
const LabelPixelType LEFT_VENT4_LABEL = 43;
const LabelPixelType RIGHT_VENT4_LABEL = 4;

const FloatPixelType THRESH_PERCENTAGE = 0.5;

void fill_roi(LabelImageType::Pointer &input, LabelPixelType label,
              LabelImageType::Pointer &output)
{
  // extract map of left WM label
  typedef itk::ThresholdImageFilter< LabelImageType > FilterType;

  FilterType::Pointer thresh1 = FilterType::New();
  thresh1->SetInput( input );
  thresh1->ThresholdOutside(label, label);
  thresh1->SetOutsideValue(1);

  FilterType::Pointer thresh2 = FilterType::New();
  thresh2->SetInput( thresh1->GetOutput() );
  thresh2->ThresholdAbove(label-1);
  thresh2->SetOutsideValue(0);

  // compute the distance transform
  typedef itk::DanielssonDistanceMapImageFilter<LabelImageType,
                                                FloatImageType> DistanceFilterType;
  DistanceFilterType::Pointer dfilter = DistanceFilterType::New();
  dfilter->SetInput( thresh2->GetOutput() );
  try
    {
      dfilter->Update();
    }
  catch( itk::ExceptionObject& err )
    {
      cerr << "Distance Transform failed (fatal)." << endl;
      cerr << err << endl;
      exit( EXIT_FAILURE );
    }

  // locate the largest distance value
  typedef itk::ImageRegionConstIteratorWithIndex<FloatImageType> IteratorType;
  IteratorType imageit(dfilter->GetOutput(), dfilter->GetOutput()->GetRequestedRegion());
  imageit.GoToBegin();
  FloatPixelType themax = imageit.Get();
  FloatImageType::IndexType theindex = imageit.GetIndex();
  while( !imageit.IsAtEnd() )
    {
      if(imageit.Get() > themax)
        {
          themax = imageit.Get();
          theindex = imageit.GetIndex();
        }

      ++imageit;
    }

  // fill roi based the distance map at a percentage of the max distance
  typedef itk::ImageRegionIterator<LabelImageType> OutputIteratorType;
  OutputIteratorType outputit(output, output->GetLargestPossibleRegion() );
  outputit.GoToBegin();
  imageit.GoToBegin();
  while( !imageit.IsAtEnd() )
    {
      if(imageit.Get() > themax*THRESH_PERCENTAGE)
        {
          outputit.Set(1);
        }

      ++imageit;
      ++outputit;
    }

}

void extract_wm_roi(LabelImageType::Pointer &input)
{
  // allocate label map for result
  LabelImageType::Pointer result = LabelImageType::New();
  result->SetRegions(input->GetLargestPossibleRegion() );
  result->SetSpacing(input->GetSpacing() );
  result->SetOrigin(input->GetOrigin() );
  result->SetDirection(input->GetDirection() );
  result->Allocate();

  fill_roi(input, LEFT_WM_LABEL, result);
  fill_roi(input, RIGHT_WM_LABEL, result);

  // write the ouput file
  typedef itk::ImageFileWriter< LabelImageType >  WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( "wm-roi.nii.gz" );
  writer->SetInput( result );

  try
    {
      writer->Update();
    }
  catch( itk::ExceptionObject& err )
    {
      cerr << "Writing ouput image failed (fatal)." << endl;
      cerr << err << endl;
      exit( EXIT_FAILURE );
    }
}

void extract_csf_roi(LabelImageType::Pointer &input)
{
  // allocate label map for result
  LabelImageType::Pointer result = LabelImageType::New();
  result->SetRegions(input->GetLargestPossibleRegion() );
  result->SetSpacing(input->GetSpacing() );
  result->SetOrigin(input->GetOrigin() );
  result->SetDirection(input->GetDirection() );
  result->Allocate();

  fill_roi(input, LEFT_VENT4_LABEL, result);
  fill_roi(input, RIGHT_VENT4_LABEL, result);

  // write the ouput file
  typedef itk::ImageFileWriter< LabelImageType >  WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( "csf-roi.nii.gz" );
  writer->SetInput( result );

  try
    {
      writer->Update();
    }
  catch( itk::ExceptionObject& err )
    {
      cerr << "Writing ouput image failed (fatal)." << endl;
      cerr << err << endl;
      exit( EXIT_FAILURE );
    }
}

void extract_global_roi(LabelImageType::Pointer &input)
{
  // allocate label map for result
  LabelImageType::Pointer result = LabelImageType::New();
  result->SetRegions(input->GetLargestPossibleRegion() );
  result->SetSpacing(input->GetSpacing() );
  result->SetOrigin(input->GetOrigin() );
  result->SetDirection(input->GetDirection() );
  result->Allocate();

  // extract all labels
  typedef itk::ImageRegionIterator<LabelImageType> IteratorType;
  IteratorType inputit(input, input->GetLargestPossibleRegion() );
  IteratorType resultit(result, result->GetLargestPossibleRegion() );
  inputit.GoToBegin();
  resultit.GoToBegin();
  while( !inputit.IsAtEnd() )
    {
      if(inputit.Get() > 0)
        {
          resultit.Set(1);
        }

      ++inputit;
      ++resultit;
    }

  // write the ouput file
  typedef itk::ImageFileWriter< LabelImageType >  WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( "global-roi.nii.gz" );
  writer->SetInput( result );

  try
    {
      writer->Update();
    }
  catch( itk::ExceptionObject& err )
    {
      cerr << "Writing ouput image failed (fatal)." << endl;
      cerr << err << endl;
      exit( EXIT_FAILURE );
    }
}

int main(int argc, char ** argv)
{
  // parse command line
  vul_arg<string> labelFile(0, "label image file name");
  vul_arg<string> epiFile(0, "average epi file name");
  vul_arg_parse(argc, argv);

  // setup input and label file reader
  typedef itk::ImageFileReader< LabelImageType >  LabelReaderType;
  LabelReaderType::Pointer labelReader = LabelReaderType::New();
  labelReader->SetFileName( labelFile().c_str() );

  typedef itk::ImageFileReader< FloatImageType >  EPIReaderType;
  EPIReaderType::Pointer epiReader = EPIReaderType::New();
  epiReader->SetFileName( epiFile().c_str() );

  // read input label file
  LabelImageType::Pointer input;
  FloatImageType::Pointer epi;
  try
        {
          labelReader->Update();
          input = labelReader->GetOutput();

          epiReader->Update();
          epi = epiReader->GetOutput();
        }
  catch( itk::ExceptionObject& err )
        {
          cerr << "Reading input images failed (fatal)." << endl;
          cerr << err << endl;
          exit( EXIT_FAILURE );
        }

  // resample labels onto EPI space
  typedef itk::ResampleImageFilter<LabelImageType, LabelImageType> ResampleFilterType;
  ResampleFilterType::Pointer resampler = ResampleFilterType::New();

  typedef itk::AffineTransform< double, Dimension >  TransformType;
  TransformType::Pointer transform = TransformType::New();

  typedef itk::NearestNeighborInterpolateImageFunction<
                       LabelImageType, double >  InterpolatorType;
  InterpolatorType::Pointer interpolator = InterpolatorType::New();
  resampler->SetInterpolator(interpolator);
  resampler->SetDefaultPixelValue(0);
  resampler->SetOutputSpacing( epi->GetSpacing() );
  resampler->SetOutputDirection( epi->GetDirection() );
  resampler->SetOutputOrigin( epi->GetOrigin() );
  resampler->SetSize( epi->GetRequestedRegion().GetSize() );
  resampler->SetInput( input );

  transform->SetIdentity();
  resampler->SetTransform( transform );

  LabelImageType::Pointer resampled_input;
  try
    {
      resampler->Update();
      resampled_input = resampler->GetOutput();
    }
  catch( itk::ExceptionObject& err )
    {
      cerr << "Resampling input image failed (fatal)." << endl;
      cerr << err << endl;
      exit( EXIT_FAILURE );
    }

  extract_wm_roi(resampled_input);
  extract_csf_roi(resampled_input);
  extract_global_roi(resampled_input);

  return EXIT_SUCCESS;
}
