#include "JuceHeader.h"
#include "MidiCodePlugin.h"

MidiCodePlugin::MidiCodePlugin()
{
}

MidiCodePlugin::~MidiCodePlugin()
{
}

void MidiCodePlugin::fillInPluginDescription (PluginDescription& d) const
{
	d.name = getName();
    d.uid = d.name.hashCode();
    d.category = "Midi Generator";
    d.pluginFormatName = "GFormat";
    d.manufacturerName = "George";
    d.version = "1.0";
    d.isInstrument = false;
    d.numInputChannels = 0;
    d.numOutputChannels = 0;
}

void MidiCodePlugin::prepareToPlay (double sampleRate, int estimatedSamplesPerBlock)
{
}

void MidiCodePlugin::releaseResources()
{
}

void MidiCodePlugin::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
}


//==============================================================================
GFormatPluginFormat::GFormatPluginFormat()
{
    
}

AudioPluginInstance* GFormatPluginFormat::createInstanceFromDescription (const PluginDescription& desc)
{
    return new MidiCodePlugin;
}

void GFormatPluginFormat::getAllTypes (OwnedArray <PluginDescription>& results)
{
	MidiCodePlugin p;
	PluginDescription midiCodePluginDesc;
    p.fillInPluginDescription (midiCodePluginDesc);
    results.add (new PluginDescription (midiCodePluginDesc));
}
