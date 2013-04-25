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

#ifndef __JUCER_HEADER_MIDICODEPLUGINEDITOR_MIDICODEPLUGINEDITOR_9C4356F2__
#define __JUCER_HEADER_MIDICODEPLUGINEDITOR_MIDICODEPLUGINEDITOR_9C4356F2__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
#include "MidiCodePlugin.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MidiCodePluginEditor  : public AudioProcessorEditor,
                              public TextEditorListener
{
public:
    //==============================================================================
    MidiCodePluginEditor (MidiCodePlugin* plugin);
    ~MidiCodePluginEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	virtual void textEditorTextChanged (TextEditor& editor);
	void setCode(const String& s) { codeEditor->setText(s); }
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();



    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	MidiCodePlugin* getProcessor() const
    {
        return static_cast <MidiCodePlugin*> (getAudioProcessor());
    }
    //[/UserVariables]

    //==============================================================================
    TextEditor* codeEditor;


    //==============================================================================
    // (prevent copy constructor and operator= being generated..)
    MidiCodePluginEditor (const MidiCodePluginEditor&);
    const MidiCodePluginEditor& operator= (const MidiCodePluginEditor&);
};


#endif   // __JUCER_HEADER_MIDICODEPLUGINEDITOR_MIDICODEPLUGINEDITOR_9C4356F2__
