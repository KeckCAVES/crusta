#include <crusta/GLMotif/PaletteEditor.h>


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
#include <IO/OpenFile.h>
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
///\todo allow changing the preferred size through API
const Vector preferredSize(10.0f, 5.0f, 0.0f);

    const StyleSheet& ss = *Vrui::getWidgetManager()->getStyleSheet();

    //create the palette editor GUI
    RowColumn* colorMapDialog = new RowColumn("ColorMapDialog", this, false);

    colorMapEditor = new ColorMapEditor("ColorMapEditor", colorMapDialog);
    colorMapEditor->setBorderWidth(ss.size*0.5f);
    colorMapEditor->setBorderType(Widget::LOWERED);
    colorMapEditor->setForegroundColor(Color(0.0f, 1.0f, 0.0f));
    colorMapEditor->setMarginWidth(ss.size);
    colorMapEditor->setPreferredSize(Vector(ss.fontHeight*preferredSize[0],
                                            ss.fontHeight*preferredSize[1],
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
    lastRow->setNumMinorWidgets(1);

    //create the info and quick maps
    RowColumn* infoQuick = new RowColumn("InfoQuick", lastRow, false);
    infoQuick->setOrientation(RowColumn::VERTICAL);
    infoQuick->setNumMinorWidgets(1);

    RowColumn* info = new RowColumn("Info", infoQuick, false);
    info->setOrientation(RowColumn::HORIZONTAL);
    info->setNumMinorWidgets(1);

    new Label("ControlPointValueLabel", info, "CP Value");

    controlPointValue = new TextField("ControlPointValue", info, 12);
    controlPointValue->setPrecision(6);
    controlPointValue->setString("");

    info->manageChild();


    RowColumn* quickRoot = new RowColumn("QuickRoot", infoQuick, false);
    quickRoot->setOrientation(RowColumn::VERTICAL);
    quickRoot->setNumMinorWidgets(1);

    Label* quickLabel = new Label("ColorInOutLabel", quickRoot,
                                  "Quick Color Maps");
    quickLabel->setHAlignment(GLFont::Center);

    RowColumn* colorInOuts = new RowColumn("ColorInOuts", quickRoot,
                                           false);
    colorInOuts->setOrientation(RowColumn::VERTICAL);
    colorInOuts->setPacking(RowColumn::PACK_GRID);
    colorInOuts->setNumMinorWidgets(2);
    for (int i=0; i<4; ++i)
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
    colorPicker = new ColorPicker("ColorPicker", lastRow);
    colorPicker->getColorChangedCallbacks().add(this,
        &PaletteEditor::colorPickerValueChangedCallback);

    lastRow->manageChild();

    //create the button box
    RowColumn* buttonBox = new RowColumn("ButtonBox", colorMapDialog, false);
    buttonBox->setOrientation(RowColumn::HORIZONTAL);
    buttonBox->setPacking(RowColumn::PACK_GRID);
    buttonBox->setAlignment(Alignment::HCENTER);

    Button* removeControlPointButton = new Button(
        "RemoveControlPointButton", buttonBox, "Remove CP");
    removeControlPointButton->getSelectCallbacks().add(
        this, &PaletteEditor::removeControlPointCallback);

    Button* loadPaletteButton = new Button(
        "LoadPaletteButton", buttonBox, "Load");
    loadPaletteButton->getSelectCallbacks().add(
        this, &PaletteEditor::loadPaletteCallback);

    Button* savePaletteButton = new Button(
        "SavePaletteButton", buttonBox, "Save");
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
        controlPointValue->setString("");
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
        Vrui::getWidgetManager(), "Load Palette", IO::openDirectory("."), ".pal");
    fileDialog->getOKCallbacks().add(this, &PaletteEditor::loadFileOKCallback);
    fileDialog->getCancelCallbacks().add(this,
        &PaletteEditor::loadFileCancelCallback);
    Vrui::popupPrimaryWidget(fileDialog);
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
