#ifndef MOBILEPLUS_NATIVEINTENT_H
#define MOBILEPLUS_NATIVEINTENT_H

#include <mobileplus/Global.h>
#include <binder/Parcel.h>

namespace android {


class Uri;
class Part;
class StringUri;
class OpaqueUri;
class HierarchicalUri;
class ComponentName;


class Uri {

public:
	Uri(){
		type = 0;
		uri = NULL;
	}
	virtual ~Uri()
	{
		if (!uri)
			delete uri;
	}

	virtual void readFromParcel(const Parcel& parcel);
	virtual inline Uri& operator=(const Uri& other);
	static const int NULL_TYPE_ID = 0;
	static const int STRING_URI = 1;
	static const int OPAQUE_URI = 2;
	static const int HIERARCHICAL_URI = 3;
	int32_t type;
	Uri* uri;
};


class Part {

public:
	Part(){
		type = 0;
	}
	~Part() {}

	void readFrom(const Parcel& parcel);
	inline Part& operator=(const Part& other);

	static const int BOTH = 0;
	static const int ENCODED = 1;
	static const int DECODED = 2;
	int32_t type;
	String16 encoded;
	String16 decoded;
};

class StringUri : public Uri {
public:
	StringUri() {
		uriString = String16();
	}
	~StringUri() {}
	void readFromParcel(const Parcel& parcel);
	String16 getAuthority();
	inline StringUri& operator=(const StringUri& other);

	String16 uriString;
};

class OpaqueUri : public Uri {

public:
	OpaqueUri() {}
	~OpaqueUri() {}
	void readFromParcel(const Parcel& parcel);
	inline OpaqueUri& operator=(const OpaqueUri& other);

	String16 scheme;
	Part ssp;
	Part fragment;
};

class HierarchicalUri : public Uri{
public:
	HierarchicalUri() {}
	~HierarchicalUri() {}
	void readFromParcel(const Parcel& parcel);
	inline HierarchicalUri& operator=(const HierarchicalUri& other);

	String16 scheme;
	Part authority;
	Part path;
	Part query;
	Part fragment;
};

class ComponentName {

public:
	ComponentName() {}

	ComponentName(const char* _mPackage, const char* _mClass = nullptr){
		mPackage = String16(_mPackage);
		mClass = (_mClass) ? String16(_mClass) : String16("null");
	}

	ComponentName(const Parcel &parcel){
		readFromParcel(parcel);
	}

	~ComponentName() {}

	void readFromParcel(const Parcel& parcel);
	inline ComponentName& operator=(const ComponentName& other);

	String16 mPackage;
	String16 mClass;
};




// base/core/java/android/graphics/Rect.java
class NativeRect{
public:
	NativeRect(){
		left = 0;
		right = 0;
		top = 0;
		bottom = 0;
	}
	~NativeRect() {}
	void readFromParcel(const Parcel& parcel);
	inline NativeRect& operator=(const NativeRect& other);

	int32_t left;
	int32_t right;
	int32_t top;
	int32_t bottom;
};


class NativeIntent {
public:
	NativeIntent(){	
		mFlags = 0;
		mCategoriesSize = 0;
	}

	NativeIntent(const Parcel& parcel){
		readFromParcel(parcel);
	}

	~NativeIntent(){
		if (!mCategories)
				delete[] mCategories;
		if (!mUri)
				delete[] mUri;
	}

	void readFromParcel(const Parcel& parcel);
	bool hasUri();
	static void writeUri(Parcel& parcel, const String16& filePath);
//	static void writeHierarchicalUri(Parcel& parcel, const String16& scheme, const String16& authority, const String16& path, const String16& query, const String16& fragment);
	inline NativeIntent& operator=(const NativeIntent& other);

	
	String16 mAction;
	Uri mData;
	String16 mType;
	String16 mIdentifier; 
	String16 mPackage;
	ComponentName mComponent;
	String16* mCategories;
	int32_t	mFlags;
	//mContentUserHint;

	NativeRect mSourceBounds;
	int32_t mCategoriesSize;
	size_t uriPos;
	size_t typePos;

	int mBundleLengthPos;
	int mUriNum;
	Uri* mUri;
	int* mStartPos;
	int* mEndPos;

};

};
#endif

