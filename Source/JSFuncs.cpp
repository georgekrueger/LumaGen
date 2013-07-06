
#include "JSFuncs.h"
#include "Music.h"
#include <assert.h>
#include <iostream>
#include <sstream>
#include <list>
#include <boost/variant.hpp>

using namespace v8;
using namespace std;

Music::Track* gTrack = NULL;

// Note
static Persistent<ObjectTemplate> gNoteTemplate;
static Persistent<ObjectTemplate> gRestTemplate;
static Persistent<ObjectTemplate> gPatternTemplate;
static Persistent<ObjectTemplate> gWeightedGenTemplate;
static Persistent<ObjectTemplate> gSeqGenTemplate;
static Persistent<ObjectTemplate> gTransposeGenTemplate;
static Persistent<ObjectTemplate> gTrackTemplate;
v8::Handle<v8::Value> MakeNote(const v8::Arguments& args);
Handle<ObjectTemplate> MakeNoteTemplate();
v8::Handle<v8::Value> MakeRest(const v8::Arguments& args);
Handle<ObjectTemplate> MakeRestTemplate();
v8::Handle<v8::Value> MakePattern(const v8::Arguments& args);
Handle<ObjectTemplate> MakePatternTemplate();
v8::Handle<v8::Value> MakeWeightedGen(const v8::Arguments& args);
Handle<ObjectTemplate> MakeWeightedGenTemplate();
v8::Handle<v8::Value> MakeTransposeGen(const v8::Arguments& args);
Handle<ObjectTemplate> MakeTransposeGenTemplate();
v8::Handle<v8::Value> MakeScaleGen(const v8::Arguments& args);
//Handle<Value> GetPitch(Local<String> name, const AccessorInfo& info);
v8::Handle<v8::Value> MakeSeqGen(const v8::Arguments& args);
Handle<ObjectTemplate> MakeSeqGenTemplate();

typedef boost::variant<boost::shared_ptr<Music::Generator>, boost::shared_ptr<SongTrack> > MusicObject;

Persistent<Context> CreateV8Context()
{
	v8::HandleScope handle_scope;

	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

	// Bind the global 'print' function to the C++ Print callback.
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
	global->Set(v8::String::New("note"), v8::FunctionTemplate::New(MakeNote));
	global->Set(v8::String::New("rest"), v8::FunctionTemplate::New(MakeRest));
	global->Set(v8::String::New("pat"), v8::FunctionTemplate::New(MakePattern));
	global->Set(v8::String::New("pick"), v8::FunctionTemplate::New(MakeWeightedGen));
	global->Set(v8::String::New("trans"), v8::FunctionTemplate::New(MakeTransposeGen));
	global->Set(v8::String::New("scale"), v8::FunctionTemplate::New(MakeScaleGen));
	global->Set(v8::String::New("walk"), v8::FunctionTemplate::New(MakeSeqGen));
	
	v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);

	return context;
}

/**
 * Utility function that extracts the C++ object from a JS wrapper object.
 */
template <class T>
T* ExtractObjectFromJSWrapper(Handle<Object> obj) {
	Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
	void* ptr = field->Value();
	return static_cast<T*>(ptr);
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope;
    if (first) {
      first = false;
    } else {
      printf(" ");
    }
    v8::String::Utf8Value str(args[i]);
    const char* cstr = ToCString(str);
    printf("%s", cstr);
  }
  printf("\n");
  fflush(stdout);
  return v8::Undefined();
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Handle<v8::String> source,
	v8::Handle<v8::Value> name,
	bool print_result,
	bool report_exceptions,
	Music::Track* track)
{
	gTrack = track;

	v8::HandleScope handle_scope;
	v8::TryCatch try_catch;
	v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
	if (script.IsEmpty()) {
		// Print errors that happened during compilation.
		if (report_exceptions)
			ReportException(&try_catch);
		return false;
	} else {
		v8::Handle<v8::Value> result = script->Run();
		if (result.IsEmpty()) {
			assert(try_catch.HasCaught());
			// Print errors that happened during execution.
			if (report_exceptions)
				ReportException(&try_catch);
			return false;
		} else {
			assert(!try_catch.HasCaught());
			if (print_result && !result->IsUndefined()) {
				// If all went well and the result wasn't undefined then print
				// the returned value.
				v8::String::Utf8Value str(result);
				const char* cstr = ToCString(str);
				printf("%s\n", cstr);
			}
			return true;
		}
	}

	gTrack = NULL;
}

// Reads a file into a v8 string.
v8::Handle<v8::String> ReadFile(const char* name) {
  FILE* file = fopen(name, "rb");
  if (file == NULL) return v8::Handle<v8::String>();

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = fread(&chars[i], 1, size - i, file);
    i += read;
  }
  fclose(file);
  v8::Handle<v8::String> result = v8::String::New(chars, size);
  delete[] chars;
  return result;
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

void ReportException(v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope;
  v8::String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  v8::Handle<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    printf("%s\n", exception_string);
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber();
    printf("%s:%i: %s\n", filename_string, linenum, exception_string);
    // Print line of source code.
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = ToCString(sourceline);
    printf("%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn();
    for (int i = 0; i < start; i++) {
      printf(" ");
    }
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++) {
      printf("^");
    }
    printf("\n");
    v8::String::Utf8Value stack_trace(try_catch->StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      printf("%s\n", stack_trace_string);
    }
  }
}

Music::GeneratorSharedPtr GetGeneratorFromJSValue(Handle<Value> value)
{
	Music::GeneratorSharedPtr gen;
	if (value->IsString()) {
		v8::String::Utf8Value str(value);
		string pitchStr = string(ToCString(str));
		gen.reset(new Music::SingleValueGenerator<string>(pitchStr));
	}
	else if (value->IsUint32() ) {
		unsigned int val = value->Uint32Value();
		gen.reset(new Music::SingleValueGenerator<double>(val));
	}
	else if ( value->IsInt32() ) {
		int val = value->Int32Value();
		gen.reset(new Music::SingleValueGenerator<double>(val));
	}
	else if (value->IsNumber()) {
		double val = value->NumberValue();
		gen.reset(new Music::SingleValueGenerator<double>(val));
	}
	else if (value->IsObject()) {
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(value->ToObject());
		gen = boost::get< Music::GeneratorSharedPtr >(*obj);
	}
	return gen;
}

unsigned long WEIGHT_SCALE = 1000;

Handle<Value> MakeNote(const Arguments& args) {
	HandleScope handle_scope;

	if (args.Length() != 3) {
		cout << "Error creating a note: " << endl;
	}
	
	Music::GeneratorSharedPtr pitchGen;
	Music::GeneratorSharedPtr velGen;
	Music::GeneratorSharedPtr lenGen;

	Local<Value> arg;

	arg = args[0];
	if (arg->IsNumber()) {
		double value = arg->NumberValue();
		pitchGen.reset(new Music::SingleValueGenerator<double>(value));
	}
	else if (arg->IsObject()) {
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
		pitchGen = boost::get< Music::GeneratorSharedPtr >(*obj);
	}
	else {
		cout << "Error: Do not know how to handle first arg of note" << endl;
	}

	arg = args[1];
	if (arg->IsNumber()) {
		double value = arg->NumberValue();
		velGen.reset(new Music::SingleValueGenerator<double>(value));
	}
	else if (arg->IsObject()) {
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
		velGen = boost::get< Music::GeneratorSharedPtr >(*obj);
	}
	else {
		cout << "Error: Do not know how to handle second arg of note" << endl;
	}

	arg = args[2];
	if (arg->IsNumber()) {
		double value = arg->NumberValue();
		lenGen.reset(new Music::SingleValueGenerator<double>(value));
	}
	else if (arg->IsObject()) {
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
		lenGen = boost::get< Music::GeneratorSharedPtr >(*obj);
	}
	else {
		cout << "Error: Do not know how to handle second arg of note" << endl;
	}

	boost::shared_ptr<Music::NoteGenerator> noteGen(new Music::NoteGenerator(pitchGen, velGen, lenGen));

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gNoteTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakeNoteTemplate();
		gNoteTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gNoteTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = templ->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced from within JavaScript.
	MusicObject* obj = new MusicObject(noteGen);
	Handle<External> objPtr = External::New(obj);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, objPtr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}

Handle<ObjectTemplate> MakeNoteTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors for each of the fields of the request.
	//result->SetAccessor(String::NewSymbol("pitch"), GetPitch);

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

/*Handle<Value> GetPitch(Local<String> name, const AccessorInfo& info) {
	// Extract the C++ object from the JavaScript wrapper.
	MusicObject* event = ExtractObjectFromJSWrapper<MusicObject>(info.Holder());
	WeightedEvent* noteEvent = *boost::get<WeightedEvent*>(event);

	// Fetch the path.
	short pitch = noteEvent->pitch.at(0);

	// Wrap the result in a JavaScript string and return it.
	return Integer::New(pitch);
}*/

Handle<ObjectTemplate> MakeRestTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors for each of the fields of the request.

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakeRest(const Arguments& args)
{
	HandleScope handle_scope;

	Music::GeneratorSharedPtr lenGen;

	Local<Value> arg = args[0];
	if (arg->IsNumber()) {
		double value = arg->NumberValue();
		lenGen.reset(new Music::SingleValueGenerator<double>(value));
	}
	else if (arg->IsObject()) {
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
		lenGen = boost::get< Music::GeneratorSharedPtr >(*obj);
	}
	else {
		cout << "Error: Do not know how to handle first arg of rest" << endl;
	}

	Music::RestGenSharedPtr restGen(new Music::RestGenerator(lenGen));

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gRestTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakeRestTemplate();
		gRestTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gRestTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = templ->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	MusicObject* obj = new MusicObject(restGen);
	Handle<External> ptr = External::New(obj);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result); 
}

v8::Handle<v8::Value> makeStaticPattern(const v8::Arguments& args) 
{
	HandleScope scope;

	MusicObject* holder = ExtractObjectFromJSWrapper<MusicObject>(args.Holder());
	Music::GeneratorSharedPtr* gen = boost::get<Music::GeneratorSharedPtr>(holder);
	if (!gen) {
		cerr << "this object of MakeStatic is not a generator!" << endl;
		return v8::Undefined();
	}
	Music::PatternGenerator* patGen = dynamic_cast<Music::PatternGenerator*>(gen->get());
	if (!patGen) {
		cerr << "this object of MakeStatic is not a pattern generator!" << endl;
		return v8::Undefined();
	}

	Music::PatternGenSharedPtr newPatGen = patGen->MakeStatic();

	// TODO: Move this boilerplate code below into a function
	
	if (gPatternTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakePatternTemplate();
		gPatternTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gPatternTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = gPatternTemplate->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	MusicObject* obj = new MusicObject(newPatGen);
	Handle<External> ptr = External::New(obj);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return scope.Close(result);
}

v8::Handle<v8::Value> playPattern(const v8::Arguments& args) 
{
	HandleScope scope;

	MusicObject* holder = ExtractObjectFromJSWrapper<MusicObject>(args.Holder());
	Music::GeneratorSharedPtr* gen = boost::get<Music::GeneratorSharedPtr>(holder);
	if (!gen) {
		cerr << "playPattern: this object is not a generator!" << endl;
		return v8::Undefined();
	}
	/*Music::PatternGenSharedPtr patGen = dynamic_cast<Music::PatternGenSharedPtr>(gen);
	if (!patGen) {
		cerr << "playPattern: this object is not a pattern generator!" << endl;
		return v8::Undefined();
	}*/
	
	gTrack->Add(*gen, Music::BAR);

	return v8::Undefined();
}

Handle<ObjectTemplate> MakePatternTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors for each of the fields
	result->Set(v8::String::New("static"), v8::FunctionTemplate::New(makeStaticPattern));
	result->Set(v8::String::New("play"), v8::FunctionTemplate::New(playPattern));

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakePattern(const Arguments& args) {
	HandleScope handle_scope;

	vector<Music::GeneratorSharedPtr> gens;
	Music::GeneratorSharedPtr repeatGen;

	if (args.Length() < 2) {
		return v8::Undefined();
	}

	for (int i=0; i<args.Length()-1; i++)
	{
		Local<Value> arg = args[i];

		if (arg->IsObject())
		{
			MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
			Music::GeneratorSharedPtr gen = boost::get<Music::GeneratorSharedPtr>(*obj);
			gens.push_back(gen);
		}
	}

	// last argument is repeat count
	Local<Value> arg = args[args.Length()-1];
	if (arg->IsObject())
	{
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
		Music::GeneratorSharedPtr* gen = boost::get<Music::GeneratorSharedPtr>(obj);
		if (!gen) {
			cerr << "The repeat argument to pattern is an object but not a generator!" << endl;
			return v8::Undefined();
		}
		repeatGen = *gen;
	}
	else
	{
		double value = arg->NumberValue();
		repeatGen.reset(new Music::SingleValueGenerator<double>(value));
	}

	Music::PatternGenSharedPtr patternGen( new Music::PatternGenerator(gens, repeatGen) );

	if (gPatternTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakePatternTemplate();
		gPatternTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gPatternTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = gPatternTemplate->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	MusicObject* obj = new MusicObject(patternGen);
	Handle<External> ptr = External::New(obj);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}

v8::Handle<v8::Value> removePatternFromTrack(const v8::Arguments& args) 
{
	HandleScope scope;

	MusicObject* holder = ExtractObjectFromJSWrapper<MusicObject>(args.Holder());
	boost::shared_ptr<SongTrack> track = boost::get< boost::shared_ptr<SongTrack> >(*holder);

	holder = ExtractObjectFromJSWrapper<MusicObject>(args[0]->ToObject());
	Music::GeneratorSharedPtr patternGen = boost::get<Music::GeneratorSharedPtr>(*holder);
	
	track->track->Remove(patternGen);

	return v8::Undefined();
}

v8::Handle<v8::Value> clearTrack(const v8::Arguments& args) 
{
	HandleScope scope;

	MusicObject* holder = ExtractObjectFromJSWrapper<MusicObject>(args.Holder());
	boost::shared_ptr<SongTrack> track = boost::get< boost::shared_ptr<SongTrack> >(*holder);
	
	track->track->Clear();

	return v8::Undefined();
}

Handle<ObjectTemplate> MakeWeightedGenTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakeWeightedGen(const Arguments& args) {
	HandleScope handle_scope;

	vector<Music::WeightedGenerator::WeightedValue> gens;

	for (int i=0; i<args.Length(); i++)
	{
		Local<Value> arg = args[i];
		if (arg->IsArray())
		{
			Array* arr = Array::Cast(*arg);
			if (arr->Length() == 2) {
				Local<Value> genValue = arr->Get(0);
				Music::GeneratorSharedPtr genPtr = GetGeneratorFromJSValue(genValue);
				Local<Value> weightValue = arr->Get(1);
				if (weightValue->IsNumber()) {
					double weight = weightValue->NumberValue();
					int scaledWeight = weight * WEIGHT_SCALE;
					gens.push_back( make_pair(genPtr, scaledWeight) );
				}
			}
		}
		else {
			Music::GeneratorSharedPtr genPtr = GetGeneratorFromJSValue(arg);
			gens.push_back( make_pair(genPtr, 1.0 * WEIGHT_SCALE) );
		}
	}

	boost::shared_ptr<Music::WeightedGenerator> weightedGen( new Music::WeightedGenerator(gens) );

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gWeightedGenTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakeWeightedGenTemplate();
		gWeightedGenTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gWeightedGenTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = gWeightedGenTemplate->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	MusicObject* obj = new MusicObject(weightedGen);
	Handle<External> ptr = External::New(obj);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}

Handle<ObjectTemplate> MakeSeqGenTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakeSeqGen(const Arguments& args) {
	HandleScope handle_scope;

	Local<Value> arg = args[0];

	double startValue = 0;
	if (!arg->IsNumber()) {
		cerr << "Arg 1 to seq gen must be a number" << endl;
	}
	startValue = arg->NumberValue();

	arg = args[1];
	Music::GeneratorSharedPtr stepGen;
	if (arg->IsNumber()) {
		double value = arg->NumberValue();
		stepGen.reset(new Music::SingleValueGenerator<double>(value));
	}
	else if (arg->IsObject()) {
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
		stepGen = boost::get< Music::GeneratorSharedPtr >(*obj);
	}
	else {
		cout << "Error: Do not know how to handle second arg of seq" << endl;
	}

	double numIter = 0;
	if (args.Length() > 2) {
		arg = args[2];
		if (!arg->IsNumber()) {
			cerr << "Arg 2 to seq gen must be a number (num iterations)" << endl;
		}
		numIter = arg->NumberValue();
	}

	boost::shared_ptr<Music::SequenceGenerator> seqGen( new Music::SequenceGenerator(startValue, stepGen, numIter) );

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gSeqGenTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakeSeqGenTemplate();
		gSeqGenTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gSeqGenTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = gSeqGenTemplate->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	MusicObject* obj = new MusicObject(seqGen);
	Handle<External> ptr = External::New(obj);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}

Handle<ObjectTemplate> MakeTransposeGenTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakeTransposeGen(const Arguments& args) {
	HandleScope handle_scope;

	if (args.Length() != 2) {
		// error
		cerr << "Incorrect number of arguments to TransposeGen (2 required)" << endl;
		return Handle<Value>();
	}

	Local<Value> arg = args[0];
	if (!arg->IsObject())
	{
		cerr << "First argument to TransposeGen must be a generator!" << endl;
		return Handle<Value>();
	}
	MusicObject* musicObj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
	Music::GeneratorSharedPtr gen = boost::get<Music::GeneratorSharedPtr>(*musicObj);

	arg = args[1];

	Music::GeneratorSharedPtr transGen;
	if (arg->IsNumber()) {
		double value = arg->NumberValue();
		transGen.reset(new Music::SingleValueGenerator<double>(value));
	}
	else if (arg->IsObject()) {
		MusicObject* obj = ExtractObjectFromJSWrapper<MusicObject>(arg->ToObject());
		transGen = boost::get< Music::GeneratorSharedPtr >(*obj);
	}
	else {
		cout << "Error: Do not know how to handle second arg of transpose" << endl;
	}

	boost::shared_ptr<Music::TransposeGenerator> transposeGen( new Music::TransposeGenerator(gen, transGen) );

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gTransposeGenTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakeTransposeGenTemplate();
		gTransposeGenTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}

	// Create an empty object wrapper.
	Handle<Object> result = gTransposeGenTemplate->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	MusicObject* obj = new MusicObject(transposeGen);
	Handle<External> ptr = External::New(obj);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakeScaleGen(const Arguments& args) {
	HandleScope handle_scope;

	if (args.Length() != 1) {
		cerr << "Incorrect number of arguments to ScaleGen" << endl;
		return Handle<Value>();
	}

	Local<Value> arg = args[0];
	if (!arg->IsString()) {
		cerr << "Argument to ScaleGen must be a scale string (i.e. 'C_MAJ')!" << endl;
		return Handle<Value>();
	}

	v8::String::Utf8Value str(arg);
	string scaleStr = string(ToCString(str));
	// TODO: check if scale is valid
	Music::setGlobalScale(scaleStr);

	return v8::Undefined();
}

