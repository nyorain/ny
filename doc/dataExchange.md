DataExchange
============

Window systems usually provide several mechanisms for data exchange between applications.
The two most common data exchange options are the clipboard and drag-and-drop.
Since the data exchange proccess for both cases is usually pretty similiar, ny abstracts them
into a common api (as it is done on some backends).

If an application has no interest/possibilities for exchanging data with other applications
it can fully ignore any functionality, but implementing clipboard and dnd support often
provides a better user experience (every application displaying text should be able to
copy text to the clipboard, otherwise it can make for a poor user experience).

There are mainly two interfaces for exchanging data with other applications without having
to know the other application or what data it provides.

DataSource
----------

ny::DataSource, usually implemented by the application itself and used to provide data
to other applications. A typical implementation that only provides a known string in text format
could look like this:

```cpp
///DataSource implementation for textual data.
class TextDataSource : public ny::DataSource
{
public:
	TextDataSource(const std::string& text) : text_(text) {}
	~TextDataSource() = default;

	//This function must provide the data in the requested format wrapped into an
	//std::any object. The different DataFormats specify which type the std::any object must hold.
	//In our case, if dataFormat::text is requested, we return the std::string wrapped into
	//the any object, otherwise return an emtpy any object to signal that we don't support
	//the requested format.
	std::any data(const DataFormat& format) override
	{
		if(format == ny::dataFormat::text) return {text_};
		return {};
	}

	//This function should return a list of all supported dataTypes.
	//Since this DataSource implementation can only be used for textual data, we
	//only return dataFormat::text.
	std::vector<DataFormats> dataFormats() const override
	{
		return {ny::dataFormat::text};
	}

protected:
	std::string text_;
};
```

An DataSource implementation object has to be passed when the application tries to initiate
data exchange (i.e. changes the clipboard or starts a dnd operation).


DataOffer
---------

ny::DataOffer, usually implemented by backends and used by the application to retrieve data
from another application. Typical usage of a ny::DataOffer looks like this:

```cpp
//Retrieve the dataOffer object e.g. from the clipboard or a dnd event.
//Note that dataOffers are usually supplied as owned smart pointer since they
//must hold the implementation.
auto dataOffer = ...; //decltype(dataOffer): std::unique_ptr<ny::DataOffer>

//first request the supported data formats
//since this request might be done asynchronous we simply block for the reply here.
//Usually applications should use a timeout when waiting for the response.
//See AsyncRequest for more information.
auto fmtRequest = dataOffer->formats();
fmtRequest.wait();

//We can receive the formats and check if a format we can handle is supported.
//This example only deals with textual data.
auto formats = fmtRequest.get();
if(std::find(formats.begin(), formats.end(), ny::DataFormat::text) == formats.end()) return;

//If we didn't return above, the text data format is supported
//therefore request to retrieve the data as text
//Since this request is usually async, we have again to block until a response is received
//Again, applications should use a timeout here
auto dataRequest = dataOffer->data(ny::DataFormat::text);
dataRequest.wait();

//Receive the any object holding the data and check if it is valid
auto data = dataRequest.get(); //decltype(data): std::any
auto text = std::any_cast<std::string>(&data);
if(!text) return;

//Do something with *text.
ny::log("DataOffer text content: ", *text);
```


Clipboard
=========

The clipboard has only two fundamental functions: reading or writing it.
When it is written (i.e. changed or ownership claimed by the application) the application
has to provide a ny::DataSource implementation object that will further be used to
provide the data to requesting applications.

```cpp
if(!appContext.clipboard(std::make_unique<MyDataSourceImpl>()))
	ny::warning("Setting the clipboard failed");
```

Note that setting the clipboard might fail, i.e. if the backend has no clipboard support.

Drag and drop
=============

WindowContexts have to be explicitly enabled for drag and drop with types it accepts.
Then it will receive DataOffer events.
An application can also initiate a drag and drop interaction.

Data Formats
============

There are several predefined data formats that have different standard representations on the
different backends. Other than that, ny supports custom data formats in form
of general mime type support.
Every application can add their own data formats than can then be accepted by other applications
or can check for custom data formats when receiving data.

The definition of the DataFormat class with an example custom object.

```cpp
///Description of a data format by mime and non-mime strings.
///Every DataFormat object should have at least one mime or nonMime name entry, otherwise
///it is considered empty/invalid.
///Mime-types should be preferred and don't have to be standardized. Standardized mime-types
///should be preffered (i.e. add a "image/png" mime entry instead of "image/x-png").
///More significant names/better descriptions should be first in the vectors and the worse
///last.
///For equality comparison between 2 DataFormat objects, their first mime entries (or if
///not existent for neither of them, their first nonMime entries) will be compared using strcmp.
class DataFormat
{
public:
	std::vector<const char*> mime;
	std::vector<const char*> nonMime;

public:
	static const DataFormat none {}; //empty object, used for invalid formats
	static const DataFormat raw;
	static const DataFormat text;
	static const DataFormat uriList;
	static const DataFormat image;
};

bool operator==(const DataFormat& a, const DataFormat& b)
{
	if(a.mime.empty() && b.mime.empty())
		return !std::strcmp(a.nonMime.front(), b.nonMime.front());

	if(!a.mime.empty() && !b.mime.empty())
		return !std::strcmp(a.mime.front(), b.mime.front());

	return false;
}

bool operator!=(const DataFormat& a, const DataFormat& b) { return !(a == b); }

///Simply create a custom DataFormat.
ny::DataFormat mp3Format({"audio/mp3", "audio/mpeg"}, {"mp3", "mp3-audio", "mpeg3", "mpeg"});
```

For custom data formats, DataOffers/DataSources must always wrap a
std::vector<uint8_t> (i.e. a raw buffer) into the returned std::any object.

Standard Data Formats
--------------------

For the common standardized formats below, DataOffers/DataSources must use the specified
types. Note how the associated type may differ for DataOffer/DataSource since DataOffer
will retrieve data and therefore give the data ownership, while the data from
a DataSource only has to be read and therefore don't has to be owned.
In the table below, Range means nytl::Range.

<center>

| Format 	| Source type 			| Offer type 			| example mime-type 			|
|-----------|-----------------------|-----------------------|-------------------------------|
| raw		| Range\<uint8_t>		| vector\<uint8_t>		| "application/octet-stream"	|
| text		| Range\<char>			| string				| "text/plain"					|
| uriList	| Range\<const char*> 	| vector\<string> 		| "text/uri-list"				|
| image		| ImageData				| OwnedImageData		| "image/x-ny-data"				|

</center>




```cpp
///Allows to deal with single-threaded asynchronous requests.
///Usually wait() will not just wait but instead keep the internal event loop running.
///It also allows to register a callback to be called on request completion.
///It can also easily be used for synchronous requests, since it could be ready from the
///beginning. Since several operations are implemented sync or async by different backends,
///this abstraction is needed.
///The registered callback function will always be called from the gui thread during wait
///or some other event dispatching functions.
///The member functions of AsyncRequest objects should always only be used from the main gui
///thread that it was retrieved from. It cannot be directly used from other threads like
///e.g. std::future), but the flexible callback design can be easily used to achieve something
///similiar.
template <typename R>
class AsyncRequest
{
public:
	virtual ~AsyncRequest() = default;

	///Waits until the request is finished.
	///When this call returns, the registered callback function was triggered or (if there
	///is none) this request will be ready and the return object can be retrieved with get.
	///While waiting, the internal gui thread event loop will be run.
	virtual void wait(LoopControl* lc = nullptr) = 0;

	///Returns whether the AsyncRequest is valid.
	///If this is false, calling other member functions results in undefined behaviour.
	///Requests which return objects where retrieved (by get or callback) are invalid.
	virtual bool valid() const = 0;

	///Returns whether the AsyncRequest is ready, i.e. if an object of type R can
	///be retrieved with get. Calling get if this returns false results in an
	///exception.
	virtual bool ready() const = 0;

	///Returns the retrieved object if it is available.
	///Otherwise (i.e. if this function was called while ready() returns false) this
	///will throw a std::logic_error.
	virtual R get() = 0;

	///Sets the callback that retrieves the data to the given function.
	///Note that if a wait function is called (or the requests finished in some othe way)
	///this function is called immedietly and therefore the requests gets non-ready and invalid
	///instantly. Note that there is already a callback function for this request, it is
	///cleared and set to the given one.
	///If this is called by the request is ready, the callback will be instanly triggered.
	///The callback is only once called since after it is called the AsyncRequest is set to
	///an invalid state.
	virtual void callback(CallbackFunc) = 0;
};

class DataSource
{
public:
	virtual ~DataSource() = default;

	virtual std::vector<DataFormats> formats() const = 0;
	virtual std::any data(const DataFormat& format) const = 0;

	///Returns an image preview of the data being dragged.
	virtual ImageData preview() const { return {}; };
};

class DataOffer
{
public:
	using FormatsRequest = std::unique_ptr<AsyncRequest<std::vector<Formats>>>;
	using DataRequest = std::unique_ptr<AsyncReqeuest<std::any>>;

public:
	DataOffer() = default;
	virtual ~DataOffer() = default;

	virtual FormatsRequest formats() const = 0;
	virtual DataRequest data(const DataFormat& fmt) = 0;
```
