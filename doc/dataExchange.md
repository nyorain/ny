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

ny::DatOffer, usually implemented by backends and used by the application to retrieve data
from another application. Typical usage of a ny::DataOffer looks like this:

```cpp
//Retrieve the dataOffer object e.g. from the clipboard or a dnd event.
//Note that dataOffers are usually supplied as owned smart pointer since they
//must hold the implementation.
auto dataOffer = ...;

//First check which dataTypes are known to be supported.
const auto& fmts = dataOffer->dataFormats();

//Since we are (in this example) only able to handle plain textual data, we check
//if the the text format is supported
bool contains = (std::find(fmts.begin(), fmts.end(), ny::dataFormats::text) == fmts.end());

if(contains)
{
	//If text format is supported, we ask the other application to send us
	//the data in text format.
	//This request might be of asynchronous nature, therefore we request the
	//text data but register a callback that will be called with the text data if
	//the text was retrieved or with an empty std::any object if retrieving the data failed.
	//If the DataOffer imlpementation communicates synchronously with other applications
	//this function will call the given callback instantly.
	offer.data(format, [&](const std::any& object) {
		auto text = std::any_cast<std::string>(&object);
		if(!text)
		{
			ny::log("Unable to retrieve dataOffer data in text format");
			return;
		}

		//do something with the string stored in text...
	});
}
else
{
	//If the format is not supported, it might be due to the still outstanding support
	//announcement. Keep it mind that communication with other applications might be asynchronous.
	//Therefore we register a callback to check when additional formats are supported.
	//Note that most dataOffer implementations know their supported formats already
	//on creation time and therefore never trigger this callback.
	dataOffer->onFormat += [&](ny::DataOffer& offer, ny::DataFormat format) {
		if(format = ny::dataFormats::text)
		{
			//If text format is supported, we ask the other application to send us
			//the data in text format.
			offer.data(format, [&](const std::any& object) {
				auto text = std::any_cast<std::string>(&object);
				if(!text)
				{
					ny::log("Unable to retrieve dataOffer data in text format");
					return;
				}

				//do something with the string stored in text...
			});
		}
	};
}

//Note that we have to make sure that the dataOffer actually stays alive until we retrieved
//the data or e.g. some timeout time is reached.
//It should therefore be moved into storage that outlives the current scope if this is about to
//end since when we dataOffer gets destroyed our callbacks will not be triggered.
//If a registered callback was already triggered (since some backends communicate synchronous),
//the application can destroy the dataOffer (or let it simply go out scope).
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
class DataSource
{
public:
	virtual ~DataSource() = default;

	virtual std::vector<DataFormats> formats() const = 0;
	virtual std::any data(const DataFormat& format) const = 0;

	///Returns an image preview of the data being dragged.
	virtual ImageData preview() const { return {}; };
};
```
