/*=========================================================================BSD 3-Clause License

Copyright (c) 2020, OpenHeartDevelopers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

/*=========================================================================
*
* CemrgApp Plugin - Cemrg Atrial Strain Motion
*
* Cardiac Electromechanics Research Group
* http://www.cemrgapp.com
* info@cemrgapp.com 
*
* This software is distributed WITHOUT ANY WARRANTY or SUPPORT!
* 
=========================================================================*/



// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "kcl_cemrgapp_atrialstrainmotion_Activator.h"
#include "AtrialStrainMotionView.h"

// Qt
#include <QMessageBox>

// mitk image
#include <mitkImage.h>

const std::string AtrialStrainMotionView::VIEW_ID = "org.mitk.views.atrialstrainmotionview";

void AtrialStrainMotionView::SetFocus()
{
  m_Controls.button_step_1->setFocus();
}

void AtrialStrainMotionView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  connect(m_Controls.button_step_1, &QPushButton::clicked, this, &AtrialStrainMotionView::DoImageProcessing);
}

void AtrialStrainMotionView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                                const QList<mitk::DataNode::Pointer> &nodes)
{
  // iterate all selected objects, adjust warning visibility
  foreach (mitk::DataNode::Pointer node, nodes)
  {
    if (node.IsNotNull() && dynamic_cast<mitk::Image *>(node->GetData()))
    {
      m_Controls.labelWarning->setVisible(false);
      m_Controls.button_step_1->setEnabled(true);
      return;
    }
  }

  m_Controls.labelWarning->setVisible(true);
  m_Controls.button_step_1->setEnabled(false);
}

void AtrialStrainMotionView::DoImageProcessing()
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  mitk::DataNode *node = nodes.front();

  if (!node)
  {
    // Nothing selected. Inform the user and return
    QMessageBox::information(nullptr, "Template", "Please load and select an image before starting image processing.");
    return;
  }

  // here we have a valid mitk::DataNode

  // a node itself is not very useful, we need its data item (the image)
  mitk::BaseData *data = node->GetData();
  if (data)
  {
    // test if this data item is an image or not (could also be a surface or something totally different)
    mitk::Image *image = dynamic_cast<mitk::Image *>(data);
    if (image)
    {
      std::stringstream message;
      std::string name;
      message << "Performing image processing for image ";
      if (node->GetName(name))
      {
        // a property called "name" was found for this DataNode
        message << "'" << name << "'";
      }
      message << ".";
      MITK_INFO << message.str();

      // actually do something here...
    }
  }
}
