#include "JuceHeader.h"
#include "MidiCodePlugin.h"
#include "MidiCodePluginEditor.h"
#include "Music.h"
#include "JSFuncs.h"

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
	track_ = new Music::Track;
	v8::HandleScope handle_scope;
	// execute script
	//const char* str = "C:\\Documents and Settings\\George\\My Documents\\LumaGen\\Source\\input.js";
	if (code_.length() > 0)
	{
		const char* str = code_.getCharPointer().getAddress();
		v8::Handle<v8::String> source = v8::String::New(str, strlen(str));
		v8::Handle<v8::String> filename = v8::String::New("None");
		if (!ExecuteString(source, filename, false, true, track_)) {
			std::cerr << "Failed to parse script" << std::endl;
			//DialogWindow w("Parse Error", Colour(0, 0, 0), true);
			//w.setVisible(true);		
		}
	}
}

void MidiCodePlugin::releaseResources()
{
	delete track_;
	track_ = NULL;
}

void MidiCodePlugin::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	int numSamples = buffer.getNumSamples();
	int sampleCountPre = sampleCount_;
	sampleCount_ += numSamples;
	double seconds = sampleCount_ / sampleRate_;

	std::vector<Music::Track::Event> events;
	std::vector<float> offsets;
	float elapsed = (numSamples / sampleRate_) * 1000;
	track_->Update(seconds, elapsed, events, offsets);

	// convert offsets to samples
	for (int j=0; j<events.size(); j++) {
		// first convert offset to samples
		int offsetInSamples = static_cast<int>(offsets[j] / 1000 * sampleRate_);
		offsets[j] = static_cast<float>(offsetInSamples);
	}

	// send events to plugin
	for (int j=0; j<events.size(); j++) {
		if (Music::Track::NoteOffEvent* noteOffEvent = boost::get<Music::Track::NoteOffEvent>(&events[j])) {
			MidiMessage msg = MidiMessage::noteOff(1, noteOffEvent->pitch, 0);
			midiMessages.addEvent (msg, static_cast<int>(offsets[j]));
			std::cout << "add note on event";
		}
		else if (Music::Track::NoteOnEvent* noteOnEvent = boost::get<Music::Track::NoteOnEvent>(&events[j])) {
			MidiMessage msg = MidiMessage::noteOn(1, noteOnEvent->pitch, noteOnEvent->velocity);
			midiMessages.addEvent (msg, static_cast<int>(offsets[j]));
			std::cout << "add note off event";
		}
	}
}

AudioProcessorEditor* MidiCodePlugin::createEditor()
{
	MidiCodePluginEditor* e = new MidiCodePluginEditor(this);
	e->setCode(code_);
	return e;
}

void MidiCodePlugin::getStateInformation (juce::MemoryBlock& destData)
{
	const String::CharPointerType& charPtr = code_.getCharPointer();
	destData.setSize(charPtr.sizeInBytes());
	destData.copyFrom (charPtr.getAddress(), 0, charPtr.sizeInBytes());
}

void MidiCodePlugin::setStateInformation (const void* data, int sizeInBytes)
{
	String::CharPointerType c(static_cast<const String::CharPointerType::CharType*>(data));
	code_ = c;
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
