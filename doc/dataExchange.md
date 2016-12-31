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
///Create a custom DataFormat for mp3 data.
ny::DataFormat mp3Format;

///The name of the format should be a mime-type describing it the best.
///This will be used for comparing DataFormats and will be name mainly given to it
///when data with this format is presented to other applications.
mp3Format.name = "audio/mp3";

///One can also store additional names that could help other applications figure it out.
///Note that here should only be listed names that are only used desribed to describe the exact
///same type, otherwise other applications may expect some other data.
mp3Format.additionalNames = {"audio/mpeg", "mp3", "mp3-audio", "mpeg3", "mpeg"};

///We could now e.g. return this DataFormat from our own DataSource implementation when
///we can provide mp3 data.
```

For custom data formats, DataOffers/DataSources must always wrap a
std::vector<uint8_t> (i.e. a raw buffer) into the returned std::any object.

Standard Data Formats
--------------------

For the common standardized formats below, DataOffers/DataSources must use the specified
types. Note how the associated type may differ for DataOffer/DataSource since DataOffer
will retrieve data and therefore give the data ownership, while the data from
a DataSource only has to be read and therefore don't has to be owned.
In the table below, Range means nytl::Span.

<center>

| Format 	| std::any wrapped type 	| mime-type name				|
|-----------|---------------------------|-------------------------------|
| raw		| std::vector<uint8_t>		| "application/octet-stream"	|
| text		| std::string				| "text/plain"					|
| uriList	| std::vector<string> 		| "text/uri-list"				|
| image		| ny::OwnedImageData		| "image/x-ny-data"				|
| <custom>  | std::vector<uint8_t>		| custom						|

</center>
