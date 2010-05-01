/***********************************************************************
PaletteEditor - Class to represent a GLMotif popup window to edit
one-dimensional transfer functions with RGB color and opacity.
Copyright (c) 2005-2007 Oliver Kreylos

This file is part of the 3D Data Visualizer (Visualizer).

The 3D Data Visualizer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The 3D Data Visualizer is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the 3D Data Visualizer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GLMotif/PaletteEditor.h>

#include <cstdio>
#include <GL/GLColorMap.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/TextField.h>
#include <GLMotif/Slider.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/WidgetManager.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/File.h>
#include <Vrui/Vrui.h>

/******************************
Methods of class PaletteEditor:
******************************/

void PaletteEditor::selectedControlPointChangedCallback(Misc::CallbackData* cbData)
    {
    GLMotif::ColorMap::SelectedControlPointChangedCallbackData* cbData2=static_cast<GLMotif::ColorMap::SelectedControlPointChangedCallbackData*>(cbData);

    if(cbData2->newSelectedControlPoint!=0)
        {
        /* Copy the selected control point's data and color value to the color editor: */
        controlPointValue->setValue(colorMap->getSelectedControlPointValue());
        GLMotif::ColorMap::ColorMapValue colorValue=colorMap->getSelectedControlPointColorValue();
        colorPanel->setBackgroundColor(colorValue);
        colorPicker->setCurrentColor(colorValue);
        }
    else
        {
        controlPointValue->setLabel("");
        GLMotif::ColorMap::ColorMapValue color(0.5f,0.5f,0.5f, 1.0f);
        colorPanel->setBackgroundColor(color);
        colorPicker->setCurrentColor(color);
        }
    }

void PaletteEditor::colorMapChangedCallback(Misc::CallbackData* cbData)
    {
    if(colorMap->hasSelectedControlPoint())
        {
        /* Copy the updated value of the selected control point to the color editor: */
        controlPointValue->setValue(colorMap->getSelectedControlPointValue());
        colorPicker->setCurrentColor(colorMap->getSelectedControlPointColorValue());
        }
    }

void PaletteEditor::colorPickerValueChangedCallback(
    GLMotif::ColorPicker::ColorChangedCallbackData* cbData)
{
    //copy the new color value to the color panel and the selected control point
    colorPanel->setBackgroundColor(cbData->newColor);
    colorMap->setSelectedControlPointColorValue(cbData->newColor);
}

void PaletteEditor::removeControlPointCallback(Misc::CallbackData* cbData)
    {
    /* Remove the currently selected control point: */
    colorMap->deleteSelectedControlPoint();
    }

void PaletteEditor::loadPaletteCallback(Misc::CallbackData*)
{
    GLMotif::FileSelectionDialog* fileDialog =
        new GLMotif::FileSelectionDialog(Vrui::getWidgetManager(),
                                         "Load Palette", 0, ".pal");
    fileDialog->getOKCallbacks().add(this, &PaletteEditor::loadFileOKCallback);
    fileDialog->getCancelCallbacks().add(this,
        &PaletteEditor::loadFileCancelCallback);
    Vrui::getWidgetManager()->popupPrimaryWidget(fileDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(this));
}

void PaletteEditor::savePaletteCallback(Misc::CallbackData* cbData)
{
    try
    {
        char numberedFileName[40];
        savePalette(Misc::createNumberedFileName("SavedPalette.pal",4,numberedFileName));
    }
    catch(std::runtime_error)
    {
        /* Ignore errors and carry on: */
    }
}

void PaletteEditor::loadFileOKCallback(
    GLMotif::FileSelectionDialog::OKCallbackData* cbData)
{
    //load the selected palette
    loadPalette(cbData->selectedFileName.c_str());
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void PaletteEditor::loadFileCancelCallback(
    GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}


PaletteEditor::PaletteEditor(void)
    :GLMotif::PopupWindow("PaletteEditorPopup",Vrui::getWidgetManager(),"Palette Editor"),
     colorMap(0),controlPointValue(0),colorPanel(0)
    {
    const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();

    /* Create the palette editor GUI: */
    GLMotif::RowColumn* colorMapDialog=new GLMotif::RowColumn("ColorMapDialog",this,false);

    colorMap=new GLMotif::ColorMap("ColorMap",colorMapDialog);
    colorMap->setBorderWidth(ss.size*0.5f);
    colorMap->setBorderType(GLMotif::Widget::LOWERED);
    colorMap->setForegroundColor(GLMotif::Color(0.0f,1.0f,0.0f));
    colorMap->setMarginWidth(ss.size);
    colorMap->setPreferredSize(GLMotif::Vector(ss.fontHeight*20.0,ss.fontHeight*10.0,0.0f));
    colorMap->setControlPointSize(ss.size);
    colorMap->setSelectedControlPointColor(GLMotif::Color(1.0f,0.0f,0.0f));
    colorMap->getSelectedControlPointChangedCallbacks().add(this,&PaletteEditor::selectedControlPointChangedCallback);
    colorMap->getColorMapChangedCallbacks().add(this,&PaletteEditor::colorMapChangedCallback);

    /* Create the RGB color editor: */
    GLMotif::RowColumn* colorEditor=new GLMotif::RowColumn("ColorEditor",colorMapDialog,false);
    colorEditor->setOrientation(GLMotif::RowColumn::HORIZONTAL);
    colorEditor->setAlignment(GLMotif::Alignment::LEFT);

    GLMotif::RowColumn* controlPointData=new GLMotif::RowColumn("ControlPointData",colorEditor,false);
    controlPointData->setOrientation(GLMotif::RowColumn::VERTICAL);
    controlPointData->setNumMinorWidgets(2);

    new GLMotif::Label("ControlPointValueLabel",controlPointData,"Control Point Value");

    controlPointValue=new GLMotif::TextField("ControlPointValue",controlPointData,12);
    controlPointValue->setPrecision(6);
    controlPointValue->setLabel("");

    new GLMotif::Label("ColorEditorLabel",controlPointData,"Control Point Color");

    colorPanel=new GLMotif::Blind("ColorPanel",controlPointData);
    colorPanel->setBorderWidth(ss.size*0.5f);
    colorPanel->setBorderType(GLMotif::Widget::LOWERED);
    colorPanel->setBackgroundColor(GLMotif::Color(0.5f,0.5f,0.5f));
    colorPanel->setPreferredSize(GLMotif::Vector(ss.fontHeight*2.5f,ss.fontHeight*2.5f,0.0f));

    controlPointData->manageChild();

    GLMotif::RowColumn* pickerBox=new GLMotif::RowColumn("ColorPickerBox",colorEditor,false);

    colorPicker = new GLMotif::ColorPicker("ColorPicker",
        pickerBox, true);
    colorPicker->getColorChangedCallbacks().add(this,
        &PaletteEditor::colorPickerValueChangedCallback);

    pickerBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);

    pickerBox->manageChild();

    colorEditor->manageChild();

    /* Create the button box: */
    GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",colorMapDialog,false);
    buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
    buttonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
    buttonBox->setAlignment(GLMotif::Alignment::RIGHT);

    GLMotif::Button* removeControlPointButton=new GLMotif::Button("RemoveControlPointButton",buttonBox,"Remove Control Point");
    removeControlPointButton->getSelectCallbacks().add(this,&PaletteEditor::removeControlPointCallback);

    GLMotif::Button* loadPaletteButton=new GLMotif::Button("LoadPaletteButton",buttonBox,"Load Palette");
    loadPaletteButton->getSelectCallbacks().add(this,&PaletteEditor::loadPaletteCallback);

    GLMotif::Button* savePaletteButton=new GLMotif::Button("SavePaletteButton",buttonBox,"Save Palette");
    savePaletteButton->getSelectCallbacks().add(this,&PaletteEditor::savePaletteCallback);

    buttonBox->manageChild();

    /* Let the color map widget eat any size increases: */
    colorMapDialog->setRowWeight(0,1.0f);

    colorMapDialog->manageChild();
    }

PaletteEditor::Storage* PaletteEditor::getPalette(void) const
    {
    return colorMap->getColorMap();
    }

void PaletteEditor::setPalette(const PaletteEditor::Storage* newPalette)
    {
    colorMap->setColorMap(newPalette);
    }

void PaletteEditor::createPalette(PaletteEditor::ColorMapCreationType colorMapType,const PaletteEditor::ValueRange& newValueRange)
    {
    colorMap->createColorMap(colorMapType,newValueRange);
    }

void PaletteEditor::createPalette(const std::vector<GLMotif::ColorMap::ControlPoint>& controlPoints)
    {
    colorMap->createColorMap(controlPoints);
    }

void PaletteEditor::loadPalette(const char* paletteFileName)
    {
    colorMap->loadColorMap(paletteFileName, GLMotif::ColorMap::ValueRange(0,1));
    }

void PaletteEditor::savePalette(const char* paletteFileName) const
    {
    colorMap->saveColorMap(paletteFileName);
    }

void PaletteEditor::exportColorMap(GLColorMap& glColorMap) const
    {
    /* Update the color map's colors: */
    colorMap->exportColorMap(glColorMap);

    /* Update the color map's value range: */
    glColorMap.setScalarRange(colorMap->getValueRange().first,colorMap->getValueRange().second);
    }
