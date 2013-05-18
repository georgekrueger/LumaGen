#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
//#include <boost/thread.hpp>

namespace Music
{

void setGlobalScale(std::string scaleStr);

///////////////////////////
// Scales
///////////////////////////
enum Scale
{
	MAJ,
	MIN,
	PENTAMIN,
	NO_SCALE,
};
const int NumScales = NO_SCALE;

enum Quantization
{
	NONE,
	BEAT,
	BAR
};

unsigned short GetMidiPitch(Scale scale, int octave, int degree);
const char* GetScaleName(Scale scale);
double BeatsToMilliseconds(double beats);

struct Note
{
	short pitch;
	double velocity;
	double length;
};

struct Rest
{
	double length;
};

typedef boost::shared_ptr<Note> NoteSharedPtr;
typedef boost::shared_ptr<Rest> RestSharedPtr;

typedef boost::variant<std::string, int, double, NoteSharedPtr, RestSharedPtr> Value;
typedef boost::shared_ptr<Value> ValueSharedPtr;

class Generator;
typedef boost::shared_ptr<Generator> GeneratorSharedPtr;
typedef std::vector<ValueSharedPtr> ValueList;
typedef boost::shared_ptr<ValueList> ValueListSharedPtr;

///////////////////////////
// Generator base class
///////////////////////////
class Generator
{
public:
	Generator() {}
	virtual ValueListSharedPtr Generate() = 0;
};

///////////////////////////
// Single value generator
///////////////////////////
template <typename T>
class SingleValueGenerator : public Generator
{
public:
	SingleValueGenerator(T val) : val_(val) {}

	virtual ValueListSharedPtr Generate()
	{
		boost::shared_ptr<ValueList> result(new ValueList);
		result->push_back(ValueSharedPtr(new Value(val_)));
		return result;
	}

	T val_;
};

///////////////////////////
// Note generator
///////////////////////////
class NoteGenerator : public Generator
{
public:
	NoteGenerator(GeneratorSharedPtr pitchGen, GeneratorSharedPtr velocityGen, GeneratorSharedPtr lengthGen) : Generator(),
					pitchGen_(pitchGen), velocityGen_(velocityGen), lengthGen_(lengthGen) {}
	virtual ValueListSharedPtr Generate();

private:
	GeneratorSharedPtr pitchGen_;
	GeneratorSharedPtr velocityGen_;
	GeneratorSharedPtr lengthGen_;
};
typedef boost::shared_ptr<NoteGenerator> NoteGenSharedPtr;

///////////////////////////
// Rest generator
///////////////////////////
class RestGenerator : public Generator
{
public:
	RestGenerator(GeneratorSharedPtr lengthGen) : Generator(), lengthGen_(lengthGen) {}
	virtual ValueListSharedPtr Generate();

private:
	GeneratorSharedPtr lengthGen_;
};
typedef boost::shared_ptr<RestGenerator> RestGenSharedPtr;

///////////////////////////
// Pattern generator
///////////////////////////
class PatternGenerator : public Generator
{
public:
	PatternGenerator(std::vector<GeneratorSharedPtr> items , unsigned long repeat) : Generator(), items_(items), repeat_(repeat) {}
	virtual ValueListSharedPtr Generate();

	boost::shared_ptr<PatternGenerator> MakeStatic();

private:
	std::vector<GeneratorSharedPtr> items_;
	unsigned long repeat_;
};
typedef boost::shared_ptr<PatternGenerator> PatternGenSharedPtr;

class WeightedGenerator : public Generator
{
public:
	typedef std::pair<GeneratorSharedPtr, unsigned long> WeightedValue;

	WeightedGenerator(const std::vector<WeightedValue>& values) : values_(values) {}
	virtual ValueListSharedPtr Generate();

private:
	std::vector<WeightedValue> values_;
};
typedef boost::shared_ptr<WeightedGenerator> WeightedGenPtr;

class TransposeGenerator : public Generator
{
public:
	typedef std::pair<GeneratorSharedPtr, unsigned long> WeightedValue;

	TransposeGenerator(GeneratorSharedPtr gen, double transposeAmount) : gen_(gen), transposeAmount_(transposeAmount) {}
	virtual ValueListSharedPtr Generate();

private:
	GeneratorSharedPtr gen_;
	double transposeAmount_;
};
typedef boost::shared_ptr<WeightedGenerator> WeightedGenPtr;

class Track
{
public:
	Track();

	struct NoteOnEvent
	{
		short pitch;
		double velocity;
	};
	struct NoteOffEvent
	{
		short pitch;
	};
	typedef boost::variant<NoteOnEvent, NoteOffEvent> Event;

	void Add(GeneratorSharedPtr gen, Quantization quantize);
	void Remove(GeneratorSharedPtr gen);
	void Clear();

	void Update(double songTime, double elapsedTime, std::vector<Event>& events, std::vector<double>& offsets);

private:

	struct Part
	{
		bool started;
		double waitTime;
		unsigned long currentEvent;
		Quantization quantize;
		GeneratorSharedPtr gen;
		boost::shared_ptr<ValueList> events;
	};

	struct ActiveNote
	{
		short pitch;
		double timeLeft;
	};
	std::list<Part> parts_;
	std::map<short, ActiveNote> activeNotes_;

	bool clearRequested_;
	bool addPartRequested_;
	Part addPart_;
	GeneratorSharedPtr removeRequest_;
};

}

#endif
