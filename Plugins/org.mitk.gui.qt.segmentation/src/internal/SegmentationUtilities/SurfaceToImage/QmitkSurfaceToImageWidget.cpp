/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "QmitkSurfaceToImageWidget.h"

#include <mitkException.h>
#include <mitkExceptionMacro.h>
#include <mitkProgressBar.h>
#include <mitkProperties.h>
#include <mitkSurfaceToImageFilter.h>

static const char* const HelpText = "Select a regular image and a surface";

QmitkSurfaceToImageWidget::QmitkSurfaceToImageWidget(mitk::SliceNavigationController* timeNavigationController, QWidget* parent)
  : QmitkSegmentationUtilityWidget(timeNavigationController, parent)
{
  m_Controls.setupUi(this);

  m_Controls.dataSelectionWidget->AddDataStorageComboBox(QmitkDataSelectionWidget::ImagePredicate);
  m_Controls.dataSelectionWidget->AddDataStorageComboBox(QmitkDataSelectionWidget::SurfacePredicate);
  m_Controls.dataSelectionWidget->SetHelpText(HelpText);

  this->EnableButtons(false);

  connect (m_Controls.btnSurface2Image, SIGNAL(pressed()), this, SLOT(OnSurface2ImagePressed()));
  connect(m_Controls.dataSelectionWidget, SIGNAL(SelectionChanged(unsigned int, const mitk::DataNode*)), this, SLOT(OnSelectionChanged(unsigned int, const mitk::DataNode*)));
}

QmitkSurfaceToImageWidget::~QmitkSurfaceToImageWidget()
{
}

void QmitkSurfaceToImageWidget::EnableButtons(bool enable)
{
  m_Controls.btnSurface2Image->setEnabled(enable);
}

void QmitkSurfaceToImageWidget::OnSelectionChanged(unsigned int index, const mitk::DataNode* selection)
{
  QmitkDataSelectionWidget* dataSelectionWidget = m_Controls.dataSelectionWidget;
  mitk::Image::Pointer image = dynamic_cast<mitk::Image*>( dataSelectionWidget->GetSelection(0)->GetData() );
  mitk::Surface::Pointer surface = dynamic_cast<mitk::Surface*>( dataSelectionWidget->GetSelection(1)->GetData() );

  if (image.IsNull() || surface.IsNull() )
  {
    dataSelectionWidget->SetHelpText(HelpText);
    this->EnableButtons(false);
  }
  else
  {
    if( image->GetTimeSteps() != surface->GetTimeSteps() )
    {
      dataSelectionWidget->SetHelpText("Image and surface are of different size");
      this->EnableButtons(false);
    }
    else
    {
      dataSelectionWidget->SetHelpText("");
      this->EnableButtons();
    }
  }
}

void QmitkSurfaceToImageWidget::OnSurface2ImagePressed()
{
  this->EnableButtons(false);

  QmitkDataSelectionWidget* dataSelectionWidget = m_Controls.dataSelectionWidget;
  mitk::Image::Pointer image = dynamic_cast<mitk::Image*>( dataSelectionWidget->GetSelection(0)->GetData() );
  mitk::Surface::Pointer surface = dynamic_cast<mitk::Surface*>( dataSelectionWidget->GetSelection(1)->GetData() );

  if( image.IsNull() || surface.IsNull())
  {
    MITK_ERROR << "Selection does not contain an image and/or a surface";
    this->EnableButtons();
    return;
  }

  mitk::Image::Pointer resultImage(0);
  resultImage = this->ConvertSurfaceToImage( image, surface );

  if( resultImage.IsNull() )
  {
    MITK_ERROR << "Convert Surface to binary image failed";
    m_Controls.btnSurface2Image->setEnabled(true);
    return;
  }

  //create name for result node
  std::string nameOfResultImage = dataSelectionWidget->GetSelection(0)->GetName();
  nameOfResultImage.append("_");
  nameOfResultImage.append(dataSelectionWidget->GetSelection(1)->GetName());

  //create data node and add to data storage
  mitk::DataNode::Pointer resultNode = mitk::DataNode::New();
  resultNode->SetData( resultImage );
  resultNode->SetProperty("name", mitk::StringProperty::New(nameOfResultImage) );
  resultNode->SetProperty("binary", mitk::BoolProperty::New(true) );

  dataSelectionWidget->GetDataStorage()->Add(resultNode, dataSelectionWidget->GetSelection(0));

  this->EnableButtons();
}

mitk::Image::Pointer QmitkSurfaceToImageWidget::ConvertSurfaceToImage( mitk::Image::Pointer image, mitk::Surface::Pointer surface )
{
  mitk::ProgressBar::GetInstance()->AddStepsToDo(2);
  mitk::ProgressBar::GetInstance()->Progress();

  mitk::SurfaceToImageFilter::Pointer surfaceToImageFilter = mitk::SurfaceToImageFilter::New();
  surfaceToImageFilter->MakeOutputBinaryOn();
  surfaceToImageFilter->SetInput(surface);
  surfaceToImageFilter->SetImage(image);
  try
  {
    surfaceToImageFilter->Update();
  }
  catch(itk::ExceptionObject& excpt)
  {
    MITK_ERROR << excpt.GetDescription();
    return 0;
  }

  mitk::ProgressBar::GetInstance()->Progress();

  mitk::Image::Pointer resultImage = surfaceToImageFilter->GetOutput();

  return resultImage;
}

