#ifndef _GLMotif_PaletteEditor_H_
#define _GLMotif_PaletteEditor_H_

#include <crustavrui/GLMotif/ColorMapEditor.h>
#include <crustavrui/GLMotif/ColorMapInOut.h>
#include <crustavrui/GLMotif/ColorPicker.h>
#include <crustavrui/GLMotif/RangeEditor.h>

#include <crustavrui/vrui.h>


namespace Misc {
class CallbackData;
}


namespace GLMotif {

class TextField;
class Slider;


class PaletteEditor : public PopupWindow
{
public:
    /** base class for callback data sent by palette editors */
    class CallbackData : public Misc::CallbackData
    {
    public:
        CallbackData(PaletteEditor* paletteEditor_);
        /** pointer to the palette editor that sent the callback */
        PaletteEditor* paletteEditor;
    };

    PaletteEditor();

    /**\{ eturns pointer to the underlying color map widget */
    const ColorMapEditor* getColorMapEditor() const;
    ColorMapEditor* getColorMapEditor();
    /**\}*/
    /**\{ returns pointer to the underlying color map widget */
    const RangeEditor* getRangeEditor() const;
    RangeEditor* getRangeEditor();
    /**\}*/

protected:
    void selectedControlPointChangedCallback(
        ColorMapEditor::SelectedControlPointChangedCallbackData* cbData);
    void colorMapChangedCallback(Misc::CallbackData* cbData);
    void rangeChangedCallback(RangeEditor::RangeChangedCallbackData* cbData);
    void colorMapInCallback(ColorMapInOut::InCallbackData* cbData);
    void colorMapOutCallback(ColorMapInOut::OutCallbackData* cbData);
    void colorPickerValueChangedCallback(
        ColorPicker::ColorChangedCallbackData* cbData);
    void removeControlPointCallback(Misc::CallbackData* cbData);

    void loadPaletteCallback(Misc::CallbackData* cbData);
    void savePaletteCallback(Misc::CallbackData* cbData);

    void loadFileOKCallback(
        FileSelectionDialog::OKCallbackData* cbData);
    void loadFileCancelCallback(
        FileSelectionDialog::CancelCallbackData* cbData);

    /** the color map editor */
    ColorMapEditor* colorMapEditor;
    /** the range editor */
    RangeEditor* rangeEditor;
    /** the data value of the currently selected control point */
    TextField* controlPointValue;
    /** the color picker widget */
    ColorPicker* colorPicker;

    /**\{ flags to prevent infinite recursion in callback processing */
    bool inColorMapEditorCallback;
    bool inColorPickerCallback;
    bool inRangeEditorCallback;
    /**\}*/
};


} //end namespace GLMotif


#endif //_GLMotif_PaletteEditor_H_
