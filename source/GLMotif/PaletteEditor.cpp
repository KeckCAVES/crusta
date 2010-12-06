#include <GLMotif/PaletteEditor.h>


#include <cstdio>
#include <sstream>

#include <GLMotif/StyleSheet.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/TextField.h>
#include <GLMotif/Slider.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/WidgetManager.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/File.h>
#include <Vrui/Vrui.h>


namespace GLMotif {


PaletteEditor::CallbackData::
CallbackData(PaletteEditor* paletteEditor_) :
    paletteEditor(paletteEditor_)
{
}


PaletteEditor::
PaletteEditor() :
    PopupWindow("PaletteEditorPopup", Vrui::getWidgetManager(),
                "Palette Editor"),
    colorMapEditor(NULL), rangeEditor(NULL), controlPointValue(NULL),
    inColorMapEditorCallback(false), inColorPickerCallback(false),
    inRangeEditorCallback(false)
{
    const StyleSheet& ss = *Vrui::getWidgetManager()->getStyleSheet();

    //create the palette editor GUI
    RowColumn* colorMapDialog = new RowColumn("ColorMapDialog", this, false);

    colorMapEditor = new ColorMapEditor("ColorMapEditor", colorMapDialog);
    colorMapEditor->setBorderWidth(ss.size*0.5f);
    colorMapEditor->setBorderType(Widget::LOWERED);
    colorMapEditor->setForegroundColor(Color(0.0f, 1.0f, 0.0f));
    colorMapEditor->setMarginWidth(ss.size);
    colorMapEditor->setPreferredSize(Vector(ss.fontHeight*20.0,
                                            ss.fontHeight*10.0,
                                            0.0f));
    colorMapEditor->setControlPointSize(ss.size);
    colorMapEditor->setSelectedControlPointColor(Color(1.0f, 0.0f, 0.0f));
    colorMapEditor->getSelectedControlPointChangedCallbacks().add(
        this, &PaletteEditor::selectedControlPointChangedCallback);
    colorMapEditor->getColorMapChangedCallbacks().add(
        this, &PaletteEditor::colorMapChangedCallback);

    //create the range editor GUI
    rangeEditor = new RangeEditor("RangeEditor", colorMapDialog);
    rangeEditor->getRangeChangedCallbacks().add(
        this, &PaletteEditor::rangeChangedCallback);


    RowColumn* lastRow = new RowColumn("LastRow", colorMapDialog, false);
    lastRow->setOrientation(RowColumn::HORIZONTAL);
    lastRow->setAlignment(Alignment::HCENTER);

    //create the info and quick maps
    RowColumn* infoQuick = new RowColumn("InfoQuick", lastRow, false);

    RowColumn* info = new RowColumn("Info", infoQuick, false);
    info->setOrientation(RowColumn::VERTICAL);
    info->setNumMinorWidgets(2);

    new Label("ControlPointValueLabel", info, "Control Point Value");

    controlPointValue = new TextField("ControlPointValue", info, 12);
    controlPointValue->setPrecision(6);
    controlPointValue->setLabel("");

    info->manageChild();


    RowColumn* quickRoot = new RowColumn("QuickRoot", infoQuick, false);
    quickRoot->setOrientation(RowColumn::VERTICAL);
    quickRoot->setAlignment(Alignment::HCENTER);
    quickRoot->setPacking(RowColumn::PACK_TIGHT);
    quickRoot->setNumMinorWidgets(1);

    new Label("ColorInOutLabel", quickRoot, "Quick Color Maps");

    RowColumn* colorInOuts = new RowColumn("ColorInOuts", quickRoot,
                                           false);
    colorInOuts->setOrientation(RowColumn::VERTICAL);
    colorInOuts->setPacking(RowColumn::PACK_GRID);
    colorInOuts->setNumMinorWidgets(2);
    for (int i=0; i<8; ++i)
    {
        std::ostringstream index;
        index << i;
        ColorMapInOut* inout = new ColorMapInOut(
            (std::string("InOut")+index.str()).c_str(), colorInOuts,
            index.str().c_str());
        inout->getInCallbacks().add(this, &PaletteEditor::colorMapInCallback);
        inout->getOutCallbacks().add(this, &PaletteEditor::colorMapOutCallback);
    }

    colorInOuts->manageChild();
    quickRoot->manageChild();

    infoQuick->manageChild();

    //create the RGB color editor
    RowColumn* pickerBox = new RowColumn("ColorPickerBox", lastRow, false);

    colorPicker = new ColorPicker("ColorPicker", pickerBox, true);
    colorPicker->getColorChangedCallbacks().add(this,
        &PaletteEditor::colorPickerValueChangedCallback);

    pickerBox->setOrientation(RowColumn::HORIZONTAL);

    pickerBox->manageChild();

    lastRow->manageChild();

    //create the button box
    RowColumn* buttonBox = new RowColumn("ButtonBox", colorMapDialog, false);
    buttonBox->setOrientation(RowColumn::HORIZONTAL);
    buttonBox->setPacking(RowColumn::PACK_GRID);
    buttonBox->setAlignment(Alignment::RIGHT);

    Button* removeControlPointButton = new Button(
        "RemoveControlPointButton", buttonBox, "Remove Control Point");
    removeControlPointButton->getSelectCallbacks().add(
        this, &PaletteEditor::removeControlPointCallback);

    Button* loadPaletteButton = new Button(
        "LoadPaletteButton", buttonBox, "Load Palette");
    loadPaletteButton->getSelectCallbacks().add(
        this, &PaletteEditor::loadPaletteCallback);

    Button* savePaletteButton = new Button(
        "SavePaletteButton", buttonBox, "Save Palette");
    savePaletteButton->getSelectCallbacks().add(
        this, &PaletteEditor::savePaletteCallback);

    buttonBox->manageChild();

    //let the color map widget eat any size increases
    colorMapDialog->setRowWeight(0,1.0f);

    colorMapDialog->manageChild();
}


const ColorMapEditor* PaletteEditor::
getColorMapEditor() const
{
    return colorMapEditor;
}
ColorMapEditor* PaletteEditor::
getColorMapEditor()
{
    return colorMapEditor;
}

const RangeEditor* PaletteEditor::
getRangeEditor() const
{
    return rangeEditor;
}
RangeEditor* PaletteEditor::
getRangeEditor()
{
    return rangeEditor;
}


void PaletteEditor::
selectedControlPointChangedCallback(
    ColorMapEditor::SelectedControlPointChangedCallbackData* cbData)
{
    if (inColorMapEditorCallback)
        return;

    inColorMapEditorCallback = true;
    if (cbData->reason ==
        ColorMapEditor::SelectedControlPointChangedCallbackData::DESELECT)
    {
        Misc::ColorMap::Color color(0.5f,0.5f,0.5f, 1.0f);
        controlPointValue->setLabel("");
        if (!inColorPickerCallback)
            colorPicker->setCurrentColor(color);
    }
    else
    {
        //propagate the selected control point's properties
        const Misc::ColorMap::Point& p =
            colorMapEditor->getSelectedControlPoint();
        controlPointValue->setValue(p.value);
        if (!inColorPickerCallback)
            colorPicker->setCurrentColor(p.color);
    }
    inColorMapEditorCallback = false;
}

void PaletteEditor::
colorMapChangedCallback(Misc::CallbackData* cbData)
{
    if (inColorMapEditorCallback)
        return;

    inColorMapEditorCallback = true;
    if (colorMapEditor->hasSelectedControlPoint())
    {
        const Misc::ColorMap::Point& p =
            colorMapEditor->getSelectedControlPoint();
        controlPointValue->setValue(p.value);
        if (!inColorPickerCallback)
            colorPicker->setCurrentColor(p.color);
    }
    if (!inRangeEditorCallback)
    {
        const Misc::ColorMap::ValueRange vr =
            colorMapEditor->getColorMap().getValueRange();
        rangeEditor->setRange(vr.min, vr.max);
    }
    inColorMapEditorCallback = false;
}

void PaletteEditor::
rangeChangedCallback(RangeEditor::RangeChangedCallbackData* cbData)
{
    if (inRangeEditorCallback)
        return;

    inRangeEditorCallback = true;
    if (!inColorMapEditorCallback)
    {
        colorMapEditor->getColorMap().setValueRange(
            Misc::ColorMap::ValueRange(cbData->min, cbData->max));
    }
    inRangeEditorCallback = false;
}

void PaletteEditor::
colorMapInCallback(ColorMapInOut::InCallbackData* cbData)
{
    cbData->inout->setColorMap(colorMapEditor->getColorMap());
}

void PaletteEditor::
colorMapOutCallback(ColorMapInOut::OutCallbackData* cbData)
{
    colorMapEditor->getColorMap() = cbData->inout->getColorMap();
    colorMapEditor->touch();
}

void PaletteEditor::
colorPickerValueChangedCallback( ColorPicker::ColorChangedCallbackData* cbData)
{
    if (inColorPickerCallback)
        return;

    inColorPickerCallback = true;
    if (!inColorMapEditorCallback && colorMapEditor->hasSelectedControlPoint())
    {
        Misc::ColorMap::Point& p = colorMapEditor->getSelectedControlPoint();
        p.color = cbData->newColor;
        colorMapEditor->touch();
    }
    inColorPickerCallback = false;
}

void PaletteEditor::
removeControlPointCallback(Misc::CallbackData*)
{
    colorMapEditor->deleteSelectedControlPoint();
}

void PaletteEditor::
loadPaletteCallback(Misc::CallbackData*)
{
    FileSelectionDialog* fileDialog = new FileSelectionDialog(
        Vrui::getWidgetManager(), "Load Palette", 0, ".pal");
    fileDialog->getOKCallbacks().add(this, &PaletteEditor::loadFileOKCallback);
    fileDialog->getCancelCallbacks().add(this,
        &PaletteEditor::loadFileCancelCallback);
    Vrui::getWidgetManager()->popupPrimaryWidget(fileDialog,
        Vrui::getWidgetManager()->calcWidgetTransformation(this));
}

void PaletteEditor::
savePaletteCallback(Misc::CallbackData* cbData)
{
    try
    {
        char numberedFileName[40];
        Misc::createNumberedFileName("SavedPalette.pal", 4, numberedFileName);
        colorMapEditor->getColorMap().save(numberedFileName);
    }
    catch (std::runtime_error)
    {
        /* Ignore errors and carry on: */
    }
}

void PaletteEditor::loadFileOKCallback(
    FileSelectionDialog::OKCallbackData* cbData)
{
    //load the selected palette
    colorMapEditor->getColorMap().load(cbData->selectedFileName);
    colorMapEditor->touch();
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}

void PaletteEditor::loadFileCancelCallback(
    FileSelectionDialog::CancelCallbackData* cbData)
{
    //destroy the file selection dialog
    Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
}


} //end namespace GLMotif
