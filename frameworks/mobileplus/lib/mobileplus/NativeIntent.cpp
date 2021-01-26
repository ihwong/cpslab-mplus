#define LOG_TAG "NativeIntent.cpp"

#include <mobileplus/NativeIntent.h>


namespace android {
inline NativeIntent& NativeIntent::operator=(const NativeIntent& other) 
{
	mAction = other.mAction;
	mData = other.mData;
	mType = other.mType;	
	mFlags = other.mFlags;
	mPackage = other.mPackage;	
	mComponent = other.mComponent;
	mSourceBounds = other.mSourceBounds;
	mCategoriesSize = other.mCategoriesSize;
	if (!mCategoriesSize) {
		mCategories = new String16[mCategoriesSize]();
		for (int i = 0; i < mCategoriesSize; i++) 
			mCategories[i] = other.mCategories[i];
	}
	return *this;
}

void NativeIntent::readFromParcel(const Parcel& parcel){

	
	mBundleLengthPos = parcel.readInt32();
	mUriNum = parcel.readInt32();
	if (mUriNum){
		mUri = new Uri[mUriNum];
		mStartPos = new int[mUriNum];
		mEndPos = new int[mUriNum];

		for (int i=0; i<mUriNum; i++){
			
			mStartPos[i] = parcel.readInt32();
			int origPos = parcel.dataPosition();
	
			parcel.setDataPosition(mStartPos[i]);
			mUri[i].readFromParcel(parcel);
			mEndPos[i] = parcel.dataPosition();

			parcel.setDataPosition(origPos);
			ALOG(LOG_INFO, LOG_TAG, "readFromParcel(), mStartPos[%d]=%d, mEndPos[%d]=%d, type=%d", i, mStartPos[i], i, mEndPos[i], mUri[i].type);	
		}
	}

	mAction = parcel.readString16();
	mData.readFromParcel(parcel);
	mType = parcel.readString16();
	mIdentifier = parcel.readString16();
	mFlags = parcel.readInt32();
	mPackage = parcel.readString16();
	mComponent.readFromParcel(parcel);

	if(parcel.readInt32() != 0){
		mSourceBounds.readFromParcel(parcel);
	}

	mCategoriesSize = parcel.readInt32();
	if(mCategoriesSize > 0){
		mCategories = new String16[mCategoriesSize]();
		for (int i=0; i<mCategoriesSize; i++){	
			mCategories[i] = parcel.readString16();
		}
	}

}

void NativeIntent::writeUri(Parcel& parcel, const String16& filePath)
{
	parcel.writeInt32(3);	// Hierarchical uri
	parcel.writeString16(String16("file"));
	parcel.writeInt32(0);
	parcel.writeString16(String16());
	parcel.writeString16(String16());
	parcel.writeInt32(0);
	parcel.writeString16(filePath);
	parcel.writeString16(filePath);
	parcel.writeInt32(0);
	parcel.writeString16(String16());
	parcel.writeString16(String16());
	parcel.writeInt32(0);
	parcel.writeString16(String16());
	parcel.writeString16(String16());
}

inline Uri& Uri::operator=(const Uri& other){

	if (!uri){
		delete uri;
	}

	type = other.type;
	switch(type){				 
		case (STRING_URI):{
			uri = new StringUri();
			* ((StringUri*) uri) = * ((StringUri*)other.uri);
			break;
		}
		case (OPAQUE_URI):{
			uri = new OpaqueUri();
			*((OpaqueUri*) uri) = *((OpaqueUri*) other.uri);
			break;
		}
		case (HIERARCHICAL_URI):{
			uri = new HierarchicalUri();
			*((HierarchicalUri*)uri) = *((HierarchicalUri*) other.uri);
			break;
		}
	}
	
	return *this;
}

inline Part& Part::operator=(const Part& other)
{
	type = other.type;
	encoded = other.encoded;
	decoded = other.decoded;
	return *this;
}

inline StringUri& StringUri::operator=(const StringUri& other){
	uriString = other.uriString;
	return *this;	
}

inline OpaqueUri& OpaqueUri::operator=(const OpaqueUri& other){
	scheme = other.scheme;
	ssp = other.ssp;
	fragment = other.fragment;
	return *this;
}

inline HierarchicalUri& HierarchicalUri::operator=(const HierarchicalUri& other ){
	scheme = other.scheme;
	authority = other.authority;
	path = other.path;
	query = other.query;
	fragment = other.fragment;
	return *this;
}


void Uri::readFromParcel(const Parcel& parcel){
	if(!uri)
		delete uri;

	type= parcel.readInt32();
	switch(type){
		case (STRING_URI):{
			uri = new StringUri();
			uri->readFromParcel(parcel);		
			break;
		}
		case (OPAQUE_URI):{
			uri = new OpaqueUri();
			uri->readFromParcel(parcel);		
			break;
		}
		case (HIERARCHICAL_URI):{
			uri = new HierarchicalUri();
			uri->readFromParcel(parcel);		
			break;
		}
	}
				
}

void Part::readFrom(const Parcel& parcel){
	type = parcel.readInt32();
	if (type == BOTH){
		encoded = parcel.readString16();
		decoded = parcel.readString16();
	}
	else if (type == ENCODED){
		encoded = parcel.readString16();
	}
	else if (type == DECODED){
		decoded = parcel.readString16();
	}
	
}

void StringUri::readFromParcel(const Parcel& parcel){
	uriString = parcel.readString16();
}

void OpaqueUri::readFromParcel(const Parcel& parcel){
	scheme = parcel.readString16();
	ssp.readFrom(parcel);
	fragment.readFrom(parcel);

}

void HierarchicalUri::readFromParcel(const Parcel& parcel){
	scheme = parcel.readString16();
	authority.readFrom(parcel);
	path.readFrom(parcel);
	query.readFrom(parcel);
	fragment.readFrom(parcel);
}

void ComponentName::readFromParcel(const Parcel& parcel){
	mPackage = parcel.readString16();
	if(mPackage.string() != NULL)
			mClass = parcel.readString16();
	else
			mClass = String16();
}

inline ComponentName& ComponentName::operator=(const ComponentName& other){
	mPackage = other.mPackage;
	mClass = other.mClass;
	return *this;
}

void NativeRect::readFromParcel(const Parcel& parcel){
	left = parcel.readInt32();
	top = parcel.readInt32();
	right = parcel.readInt32();
	bottom = parcel.readInt32();
}

inline NativeRect& NativeRect::operator=(const NativeRect& other){
	left = other.left;
	top = other.top;
	right = other.right;
	bottom = other.bottom;
	return *this;
}

bool NativeIntent::hasUri()
{
	return (mUriNum == 0)? false : true;
}


String16 StringUri::getAuthority()
{
	if (uriString.startsWith(String16("content"))) {
		String8 uriString8 = String8(uriString);
		char* uri = new char[uriString8.length() + 1];
		strcpy(uri, uriString8.string());
		char* start = uri + strlen("content://");
		char* end = start;
		while (*end != '/') 
			end++;
		int authorityLen = end - start;
		char* authority = new char[authorityLen + 1];
		strncpy(authority, start, authorityLen);
		return String16(authority);
	}
	return String16();
}



};
