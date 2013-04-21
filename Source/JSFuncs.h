#ifndef __JSFUNC_H__
#define __JSFUNC_H__

#include <v8.h>
#include <list>
#include <boost/shared_ptr.hpp>

namespace Music { class Track; }
class Plugin;

struct SongTrack
{
	Music::Track* track;
	Plugin* plugin;
	float volume;
};

std::list<boost::shared_ptr<SongTrack> >& GetTracks();

v8::Persistent<v8::Context> CreateV8Context();
bool ExecuteString(v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions);
v8::Handle<v8::Value> Print(const v8::Arguments& args);
v8::Handle<v8::String> ReadFile(const char* name);
void ReportException(v8::TryCatch* handler);
const char* ToCString(const v8::String::Utf8Value& value);

#endif