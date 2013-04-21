#include "JuceHeader.h"
#include "MidiCodePlugin.h"
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
	const char* str = "C:\\Documents and Settings\\George\\My Documents\\LumaGen\\Source\\input.js";
    v8::Handle<v8::String> source = ReadFile(str);
    if (source.IsEmpty()) {
		printf("Error reading '%s'\n", str);
    }
	else {
		v8::Handle<v8::String> filename = v8::String::New("None");
		if (!ExecuteString(source, filename, false, true, track_)) {
			std::cerr << "Failed to parse script" << std::endl;
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
	track_->Update(seconds, numSamples / sampleRate_, events, offsets);

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
			MidiMessage msg = MidiMessage::noteOn(1, noteOnEvent->pitch, noteOnEvent->velocity / 127.0f);
			midiMessages.addEvent (msg, static_cast<int>(offsets[j]));
			std::cout << "add note off event";
		}
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
