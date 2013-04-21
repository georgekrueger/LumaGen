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
	sampleRate_ = sampleRate;
	sampleCount_ = 0;
}

void MidiCodePlugin::releaseResources()
{
}

void MidiCodePlugin::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	int numSamples = buffer.getNumSamples();
	int sampleCountPre = sampleCount_;
	sampleCount_ += numSamples;
	double seconds = sampleCount_ / sampleRate_;
	if (sampleCount_ >= 44100) {
		MidiMessage msg = MidiMessage::noteOn(0, 60, 1.0f);
		int sampleInFrame = 44100 - sampleCountPre;
		midiMessages.addEvent (msg, sampleInFrame);
		sampleCount_ -= 44100;
	}
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
