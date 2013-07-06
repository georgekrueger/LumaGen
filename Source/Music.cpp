#include "Music.h"
#include <sstream>
#include <ctime>

using namespace std;
using namespace boost;

int round(double d)
{
  return floor(d + 0.5);
}

namespace Music
{

double BPM = 120;
double BEAT_LENGTH = 1 / BPM * 60000;
string globalScale = "C_MAJ";

void setGlobalScale(string scaleStr)
{
	globalScale = scaleStr;
}

static bool randSeeded = false;

const string ScaleStrings[NumScales] = 
{
	"MAJ",
	"MIN",
	"PENTAMIN"
};

const string NamedPitches[12] = 
{
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

struct ScaleInfo
{
	short intervals[12];
	short numIntervals;
};

const ScaleInfo scaleInfo[NumScales] = 
{
	{ { 0, 2, 4, 5, 7, 9, 11 }, 7 },
	{ { 0, 2, 3, 5, 7, 9, 10 }, 7 },
	{ { 0, 3, 5, 7, 10 }, 5 },
};

const char* GetScaleName(ScaleType scale)
{
	return ScaleStrings[scale].c_str();
}

unsigned short GetPitchNumberFromName(string PitchName)
{
	for (int i=0; i<12; i++) {
		if (PitchName == NamedPitches[i]) {
			return i;
		}
	}
	return 0;
}

unsigned short GetMidiPitchFromOffset(Scale scale, int offset)
{
	// degree cannot be less than one
	if (offset < 1) offset = 1;

	const ScaleInfo* info = &scaleInfo[scale.type];
	int octave = (offset-1) / info->numIntervals;
	int degree = ((offset-1) % info->numIntervals) + 1;
	short midiPitch = 12 * octave + scale.root;
	if (degree <= info->numIntervals) {
		midiPitch += info->intervals[degree-1];
	}
	return midiPitch;
}

unsigned short GetOffsetFromMidiPitch(Scale scale, short midiPitch)
{
	const ScaleInfo* info = &scaleInfo[scale.type];
	int octave = midiPitch / 12;
	int offsetInOctave = midiPitch % 12;
	int degree = 1;
	for (unsigned int i=0; i<info->numIntervals; i++)
	{
		if (info->intervals[i] == offsetInOctave) {
			degree = i + 1;
			break;
		}
	}
	return (octave * info->numIntervals + degree);
}

unsigned short GetMidiPitch(Scale scale, int octave, int degree)
{
	const ScaleInfo* info = &scaleInfo[scale.type];
	while (degree < 1) {
		if (octave == 0) break;
		octave -= 1;
		degree += info->numIntervals;
	}
	while (degree > info->numIntervals) {
		octave += 1;
		degree -= info->numIntervals;
	}

	short midiPitch = 12 * octave + scale.root;
	if (degree >= 1 && degree <= info->numIntervals) {
		midiPitch += info->intervals[degree-1];
	}
	return midiPitch;
}

bool GetScalePitchFromMidiPitch(short pitch, Scale scale, short& octave, short& degree)
{
	const ScaleInfo* info = &scaleInfo[scale.type];
	octave = (pitch - scale.root) / 12;
	degree = 0;
	bool found = false;
	for (int i=octave; i <= 12; i++)
	{
		for (int j=0; j<=info->numIntervals; j++)
		{
			short p = i * 12 + scale.root + info->intervals[j];
			if (p == pitch) {
				degree = j + 1;
				found = true;
				break;
			}
		}
		if (found) break;
	}
	return found;
}

// Take a midi pitch note number and transpose by octave and degree, 
// according to the given scale and return a midi pitch
short TransposePitch( short pitch, Scale scale, int transposeAmount )
{
	const ScaleInfo* info = &scaleInfo[scale.type];
	int transOctave = transposeAmount / info->numIntervals;
	int transDegree = transposeAmount % info->numIntervals;
	short octave, degree;
	bool foundPitch = GetScalePitchFromMidiPitch(pitch, scale, octave, degree);
	if (foundPitch) {
		octave += transOctave;
		degree += transDegree;
		return GetMidiPitch(scale, octave, degree);
	}
	return pitch;
}

double BeatsToMilliseconds(double beats)
{
	return (BEAT_LENGTH * beats);
}

void ParseScaleString(const std::string& str, Scale& scale)
{
	scale.type = NO_SCALE;
	scale.root = 0;

	size_t firstSplit = str.find('_', 0);
	if (firstSplit == string::npos)
		return;
	string rootStr = str.substr(0, firstSplit);
	string scaleStr = str.substr(firstSplit+1);
	scale.root = GetPitchNumberFromName(rootStr);

	for (int i=0; i<NumScales; i++) {
		if (scaleStr.compare(ScaleStrings[i]) == 0) {
			scale.type = (ScaleType)i;
			break;
		}
	}
	if (scale.type == NO_SCALE) {
		cout << "Invalid scale: " << scaleStr << endl;
		scale.type = MAJ;
	}
}

ValueListSharedPtr NoteGenerator::Generate()
{
	ValueListSharedPtr pitchResult = pitchGen_->Generate();
	ValueListSharedPtr velocityResult = velocityGen_->Generate();
	ValueListSharedPtr lengthResult = lengthGen_->Generate();

	Scale scale;
	ParseScaleString(globalScale, scale);

	short midiPitch = 60;
	double* pitchRep = NULL;
	if (pitchRep = boost::get<double>(pitchResult->at(0).get())) {
		midiPitch = GetMidiPitchFromOffset(scale, round(*pitchRep));
	}

	double* velocityPtr = boost::get<double>(velocityResult->at(0).get());
	double velocity = 1.0;
	if (velocityPtr != NULL) {
		velocity = *velocityPtr;
	}

	double* lengthPtr = boost::get<double>(lengthResult->at(0).get());
	double length = 1.0;
	if (lengthPtr != NULL) {
		length = *lengthPtr;
	}

	// Create and return a note value
	NoteSharedPtr note = NoteSharedPtr(new Note);
	note->pitch = midiPitch;
	note->velocity = velocity;
	note->length = length;

	std::cout << "Note " << round(*pitchRep) << ", " << midiPitch << std::endl;

	boost::shared_ptr<ValueList> result(new ValueList);
	result->push_back(ValueSharedPtr(new Value(note)));
	return result;
}


ValueListSharedPtr RestGenerator::Generate()
{
	ValueListSharedPtr lengthResult = lengthGen_->Generate();

	double* lengthPtr = boost::get<double>(lengthResult->at(0).get());
	double length = 1.0;
	if (lengthPtr != NULL) {
		length = *lengthPtr;
	}

	// Create and return a rest value
	RestSharedPtr rest = RestSharedPtr(new Rest);
	rest->length = length;

	boost::shared_ptr<ValueList> result(new ValueList);
	result->push_back(ValueSharedPtr(new Value(rest)));
	return result;
}

ValueListSharedPtr PatternGenerator::Generate()
{
	boost::shared_ptr<ValueList> outResult(new ValueList);
	ValueListSharedPtr repeatRes = repeatGen_->Generate();
	double* repeatResult = boost::get<double>(repeatRes->at(0).get());
	if (!repeatResult) {
		return outResult;
	}
	int repeat = round(*repeatResult);
	for (unsigned long j=0; j<repeat; j++)
	{
		for (unsigned long i=0; i<items_.size(); i++)
		{
			ValueListSharedPtr res = items_[i]->Generate();
			outResult->insert(outResult->end(), res->begin(), res->end());
		}
	}
	return outResult;
}

PatternGenSharedPtr PatternGenerator::MakeStatic()
{
	std::vector<GeneratorSharedPtr> gens;
	ValueListSharedPtr repeatRes = repeatGen_->Generate();
	double* repeatResult = boost::get<double>(repeatRes->at(0).get());
	int repeat = round(*repeatResult);

	Scale scale;
	ParseScaleString(globalScale, scale);

	for (unsigned long j=0; j<repeat; j++)
	{
		for (unsigned long i=0; i<items_.size(); i++)
		{
			ValueListSharedPtr valueList = items_[i]->Generate();
			for (unsigned long k=0; k < valueList->size(); k++)
			{
				ValueSharedPtr valuePtr = valueList->at(k);
				if (NoteSharedPtr* note = boost::get<NoteSharedPtr>(valuePtr.get())) 
				{
					GeneratorSharedPtr pitchGen( new SingleValueGenerator<double>(GetOffsetFromMidiPitch(scale, (*note)->pitch)) );
					GeneratorSharedPtr velGen( new SingleValueGenerator<double>((*note)->velocity) );
					GeneratorSharedPtr lenGen( new SingleValueGenerator<double>((*note)->length) );
					NoteGenSharedPtr noteGen( new NoteGenerator(pitchGen, velGen, lenGen) );
					gens.push_back(noteGen);
				}
				else if (RestSharedPtr* rest = boost::get<RestSharedPtr>(valuePtr.get())) {
					GeneratorSharedPtr lenGen( new SingleValueGenerator<double>((*rest)->length) );
					RestGenSharedPtr restGen( new RestGenerator(lenGen) );
					gens.push_back(restGen);
				}
			}
		}
	}

	GeneratorSharedPtr repeatGen(new Music::SingleValueGenerator<double>(1));
	PatternGenSharedPtr newPatternGen(new PatternGenerator(gens , repeatGen));
	return newPatternGen;
}

int random_in_range (unsigned int min, unsigned int max)
{
  int base_random = rand(); /* in [0, RAND_MAX] */
  if (RAND_MAX == base_random) return random_in_range(min, max);
  /* now guaranteed to be in [0, RAND_MAX) */
  int range       = max - min,
      remainder   = RAND_MAX % range,
      bucket      = RAND_MAX / range;
  /* There are range buckets, plus one smaller interval
     within remainder of RAND_MAX */
  if (base_random < RAND_MAX - remainder) {
    return min + base_random/bucket;
  } else {
    return random_in_range (min, max);
  }
}

ValueListSharedPtr WeightedGenerator::Generate()
{
	if (!randSeeded) {
		srand ( static_cast<unsigned int>(time(NULL)) );
		randSeeded = true;
	}

	unsigned long total = 0;
	for (unsigned long i=0; i<values_.size(); i++) {
		total += values_[i].second;
	}
	
	int num = random_in_range(0, total);
	total = 0;
	for (unsigned long i=0; i<values_.size(); i++) {
		total += values_[i].second;
		if (total > num) {
			// found the item we are choosing
			return values_[i].first->Generate();
		}
	}

	return ValueListSharedPtr();
}

ValueListSharedPtr TransposeGenerator::Generate()
{
	// figure out transpose amount based on global scale and input number
	Scale scale;
	short root;
	ParseScaleString(globalScale, scale);

	const ScaleInfo* info = &scaleInfo[scale.type];
	
	ValueListSharedPtr transGenResult = transGen_->Generate();
	double* transposeAmount = boost::get<double>(transGenResult->at(0).get());
	if (!transposeAmount) {
		return ValueListSharedPtr();
	}
	int transpose = round(*transposeAmount);

	boost::shared_ptr<ValueList> events = gen_->Generate();
	for (int i=0; i<events->size(); i++) {
		boost::shared_ptr<Value> value = events->at(i);
		if (Music::NoteSharedPtr* note = boost::get<NoteSharedPtr>(value.get())) {
			(*note)->pitch = TransposePitch( (*note)->pitch, scale, transpose );
		}
	}
	return events;
}

ValueListSharedPtr SequenceGenerator::Generate()
{
	ValueListSharedPtr stepSizeGenResult = stepSizeGen_->Generate();
	double* stepAmount = boost::get<double>(stepSizeGenResult->at(0).get());
	if (!stepAmount) {
		return ValueListSharedPtr();
	}
	if (!value_ || (numIterations_ != 0 && count_ >= numIterations_)) {
		value_ = new double(startValue_);
		count_ = 0;
	}
	else {
		*value_ = *value_ + *stepAmount;
	}

	count_++;

	boost::shared_ptr<ValueList> result(new ValueList);
	result->push_back(ValueSharedPtr(new Value(*value_)));
	return result;
}

Track::Track() : clearRequested_(false), addPartRequested_(false)
{
}

void Track::Add(GeneratorSharedPtr gen, Quantization quantize)
{
	Part part = {false, 0, 0, quantize, gen, gen->Generate()};
	parts_.push_back(part);
	//addPartRequested_ = true;
	//addPart_ = part;
}

void Track::Remove(GeneratorSharedPtr gen)
{
	removeRequest_ = gen;
}

void Track::Clear()
{
	clearRequested_ = true;
}

void Track::Update(double songTime, double elapsedTime, vector<Event>& events, vector<double>& offsets)
{
	// update active notes
	map<short, ActiveNote>::iterator it;
	for (it = activeNotes_.begin(); it != activeNotes_.end(); ) {
		ActiveNote& activeNote = it->second;
		if (elapsedTime >= activeNote.timeLeft) {
			// if at the end of the buffer, wait until next update to generate note off
			if (elapsedTime == activeNote.timeLeft) {
				activeNote.timeLeft = 0;
				it++;
				continue;
			}
			// generate note off event
			NoteOffEvent off;
			off.pitch = activeNote.pitch;
			events.push_back(off);
			offsets.push_back(activeNote.timeLeft);

			// remove active note
			map<short, ActiveNote>::iterator removeIt = it;
			it++;
			activeNotes_.erase(removeIt);
		}
		else {
			it++;
		}
	}

	if (clearRequested_) {
		parts_.clear();
		clearRequested_ = false;
	}

	// check for remove request
	if (removeRequest_) {
		for (list<Part>::iterator i = parts_.begin(); i != parts_.end(); ) {
			Part& part = *i;
			if (part.gen == removeRequest_) {
				parts_.erase(i);
				break;
			}
		}
		removeRequest_.reset();
	}

	// check for add request
	//if (addPartRequested_) {
	//	parts_.push_back(addPart_);
	//	addPartRequested_ = false;
	//}

	for (list<Part>::iterator i = parts_.begin(); i != parts_.end(); )
	{
		Part& part = *i;

		double timeUsed = 0;
		while (timeUsed < elapsedTime)
		{
			double timeUsedThisIteration = 0;

			// if there is left over time from a previously encountered rest, then consume it.
			if (part.waitTime > 0) 
			{
				if (timeUsed + part.waitTime > elapsedTime) {
					// wait time is bigger than time left in window. use up as much wait time as we can.
					double timeLeftInFrame = elapsedTime - timeUsed;
					part.waitTime -= timeLeftInFrame;
					timeUsed = elapsedTime;
					timeUsedThisIteration = timeLeftInFrame;
				}
				else {
					// wait time is smaller than window. use up remaining wait time.
					timeUsed += part.waitTime;
					timeUsedThisIteration = part.waitTime;
					part.waitTime = 0;
				}
			}
			else
			{
				// get the next event in the list
				if (part.currentEvent >= part.events->size()) {
					// TODO: remove the part
					break;
				}
				ValueSharedPtr value = part.events->at(part.currentEvent);
				part.currentEvent++;
				if (!value) {
					break;
				}
				else if (Music::NoteSharedPtr* note = boost::get<NoteSharedPtr>(value.get())) 
				{
					NoteSharedPtr n = *note;
					ActiveNote newActiveNote;
					newActiveNote.pitch = n->pitch;
					// timeUsed is added to active note length because we subtract entire 
					// window size when udpating active notes
					newActiveNote.timeLeft = BeatsToMilliseconds(n->length) + timeUsed;

					map<short, ActiveNote>::iterator activeNoteIter = activeNotes_.find(n->pitch);
					if (activeNoteIter != activeNotes_.end()) {
						// note is already on, turn it off
						NoteOffEvent noteOffEvent;
						noteOffEvent.pitch = n->pitch;
						events.push_back(noteOffEvent);
						offsets.push_back(timeUsed);

						// replace currently active note at this pitch
						ActiveNote& activeNote = activeNoteIter->second;
						activeNote = newActiveNote;
					}
					else {
						// new active note
						activeNotes_[n->pitch] = newActiveNote;
					}
					NoteOnEvent noteOnEvent;
					noteOnEvent.pitch = n->pitch;
					noteOnEvent.velocity = n->velocity;
					events.push_back(noteOnEvent);
					offsets.push_back(timeUsed);
				}
				else if (Music::RestSharedPtr* rest = boost::get<RestSharedPtr>(value.get())) {
					RestSharedPtr r = *rest;
					part.waitTime += BeatsToMilliseconds(r->length);
				}
			}
		}
		i++;
	}

	// update active notes
	for (it = activeNotes_.begin(); it != activeNotes_.end(); ) {
		ActiveNote& activeNote = it->second;
		if (elapsedTime >= activeNote.timeLeft) {
			// if at the end of the buffer, wait until next update to generate note off
			if (elapsedTime == activeNote.timeLeft) {
				activeNote.timeLeft = 0;
				it++;
				continue;
			}
			// generate note off event
			NoteOffEvent off;
			off.pitch = activeNote.pitch;
			events.push_back(off);
			offsets.push_back(activeNote.timeLeft);

			// remove active note
			map<short, ActiveNote>::iterator removeIt = it;
			it++;
			activeNotes_.erase(removeIt);
		}
		else {
			activeNote.timeLeft -= elapsedTime;
			it++;
		}
	}
}

}
