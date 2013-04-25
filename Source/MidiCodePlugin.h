#ifndef MIDICODEPLUGIN_H
#define MIDICODEPLUGIN_H

namespace Music { class Track; }

class MidiCodePlugin : public AudioPluginInstance
{
public:
	MidiCodePlugin();
	~MidiCodePlugin();

    virtual void fillInPluginDescription (PluginDescription& d) const;
	virtual const String getName() const { return "MidiCodePlugin"; }
	virtual void prepareToPlay (double sampleRate, int estimatedSamplesPerBlock);
	virtual void releaseResources();
	virtual void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
	virtual const String getInputChannelName (int channelIndex) const { return String(); }
	virtual const String getOutputChannelName (int channelIndex) const { return String(); }
	virtual bool isInputChannelStereoPair (int index) const {return false; }
	virtual bool isOutputChannelStereoPair (int index) const { return false; }
	virtual bool acceptsMidi() const { return false; }
	virtual bool producesMidi() const { return true; }
	virtual AudioProcessorEditor* createEditor();
	virtual bool hasEditor() const { return true; }
	virtual int getNumParameters() { return 0; }
	virtual const String getParameterName (int parameterIndex) { return String(); }
	virtual float getParameter (int parameterIndex) { return 0.0; }
	virtual const String getParameterText (int parameterIndex) { return String(); }
	virtual void setParameter (int parameterIndex, float newValue) {}
	virtual int getNumPrograms() { return 0; }
	virtual int getCurrentProgram() { return 0; }
	virtual void setCurrentProgram (int index) {}
	virtual const String getProgramName (int index) { return String(); }
	virtual void changeProgramName (int index, const String& newName) {}
	virtual void getStateInformation (juce::MemoryBlock& destData);
	virtual void setStateInformation (const void* data, int sizeInBytes);

	void setCode(const String& s) { code_ = s; std::cout << s << std::endl; }

private:
	double sampleRate_;
	int sampleCount_;
	Music::Track* track_;
	String code_;
};

class GFormatPluginFormat   : public AudioPluginFormat
{
public:
    //==============================================================================
    GFormatPluginFormat();
    ~GFormatPluginFormat() {}

    void getAllTypes (OwnedArray <PluginDescription>& results);

    //==============================================================================
    String getName() const                                      { return "GFormat"; }
    bool fileMightContainThisPluginType (const String&)         { return false; }
    FileSearchPath getDefaultLocationsToSearch()                { return FileSearchPath(); }
    void findAllTypesForFile (OwnedArray <PluginDescription>&, const String&)     {}
    bool doesPluginStillExist (const PluginDescription&)        { return true; }
    String getNameOfPluginFromIdentifier (const String& fileOrIdentifier)   { return fileOrIdentifier; }
    StringArray searchPathsForPlugins (const FileSearchPath&, bool)         { return StringArray(); }
    AudioPluginInstance* createInstanceFromDescription (const PluginDescription& desc);
};

#endif