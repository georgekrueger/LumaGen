/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  22 Apr 2013 10:03:19pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "MidiCodePluginEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MidiCodePluginEditor::MidiCodePluginEditor (MidiCodePlugin* plugin)
    : AudioProcessorEditor(plugin),
      codeEditor (0)
{
    addAndMakeVisible (codeEditor = new TextEditor (L"Code Editor"));
    codeEditor->setMultiLine (true);
    codeEditor->setReturnKeyStartsNewLine (true);
    codeEditor->setReadOnly (false);
    codeEditor->setScrollbarsShown (true);
    codeEditor->setCaretVisible (true);
    codeEditor->setPopupMenuEnabled (false);
    codeEditor->setText (String::empty);


    //[UserPreSize]
	codeEditor->addListener(this);
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

MidiCodePluginEditor::~MidiCodePluginEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
	getProcessor()->editorBeingDeleted (this);
    //[/Destructor_pre]

    deleteAndZero (codeEditor);


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MidiCodePluginEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::white);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MidiCodePluginEditor::resized()
{
    codeEditor->setBounds (8, 8, 576, 344);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void MidiCodePluginEditor::textEditorTextChanged (TextEditor& editor)
{
	String s = editor.getText();
	getProcessor()->setCode(s);
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MidiCodePluginEditor" componentName=""
                 parentClasses="public AudioProcessorEditor, public TextEditorListener"
                 constructorParams="MidiCodePlugin* plugin" variableInitialisers="AudioProcessorEditor(plugin)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ffffffff"/>
  <TEXTEDITOR name="Code Editor" id="642a4c0d924856f7" memberName="codeEditor"
              virtualName="" explicitFocusOrder="0" pos="8 8 576 344" initialText=""
              multiline="1" retKeyStartsLine="1" readonly="0" scrollbars="1"
              caret="1" popupmenu="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
