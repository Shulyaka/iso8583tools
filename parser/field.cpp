#include <cstdio>
#include <cstdlib> //for malloc(). TODO: Remove malloc()
#include <sstream>
#include <ctime>
#include <vector>

#include "parser.h"

using namespace std;

field::field(const string &str)
{
	fill_default();
	tag=0;
	frm=new fldformat();
	deletefrm=true;
	firstfrm=frm;
	data=str;
}

field::field(const fldformat *format, const string &str)
{
	fill_default();
	tag=0;
	frm=format;
	firstfrm=frm;
	data=str;
}

field::field(const std::string &filename, const string &str)
{
	fill_default();
	tag=0;
	frm=new fldformat(filename);
	deletefrm=true;
	firstfrm=frm;
	data=str;
}

//copy constructor
field::field(const field &from)
{
	data=from.data;
	start=from.start;
	blength=from.blength;
	flength=from.flength;
	deletefrm=from.deletefrm;
	frm=from.frm;
	if(deletefrm)
	{
		firstfrm=new fldformat(*from.firstfrm);
		frm=firstfrm;
		for(const fldformat *tmpfrm=from.firstfrm; tmpfrm!=NULL; tmpfrm=tmpfrm->get_altformat(), frm=frm->get_altformat())
			if(tmpfrm==from.frm)
				break;
	}
	else
		firstfrm=from.firstfrm;
	subfields=from.subfields;
	altformat=from.altformat;
	tagmap=from.tagmap;
	tag=from.tag;
}

field::~field(void)
{
	clear();
	if(deletefrm)
		delete frm;
}

void field::fill_default(void)
{
	data.clear();
	start=0;
	blength=0;
	flength=0;
	frm=NULL;
	subfields.clear();
	altformat=0;
	deletefrm=false;
	firstfrm=frm;
	tagmap.clear();
}

void field::clear(void)
{
	const fldformat *tmpfrm=frm, *tmpfirstfrm=firstfrm;
	unsigned int tmpaltformat=altformat;
	bool delfrm=deletefrm;
	fill_default();
	deletefrm=delfrm;
	firstfrm=tmpfirstfrm; //make formats immune to clear()
	frm=tmpfrm;
	altformat=tmpaltformat;
}

//forks the field. All data and subfields are also copied so that all pointers except frm will have new values to newly copied data but non-pointers will have same values
//If field is not empty, the format is retained
field& field::operator= (const field &from)
{
	const fldformat *tmpfrm=frm, *tmpfirstfrm=firstfrm;
	bool keepfrm=false;

	if(this==&from)
		return *this;

	if(!empty())
		keepfrm=true;

	clear();

	data=from.data;
	start=from.start;
	blength=from.blength;
	flength=from.flength;
	deletefrm=from.deletefrm;
	frm=from.frm;
	if(deletefrm)
	{
		firstfrm=new fldformat(*from.firstfrm);
		frm=firstfrm;
		for(const fldformat *tmpfrm=from.firstfrm; tmpfrm!=NULL; tmpfrm=tmpfrm->get_altformat(), frm=frm->get_altformat())
			if(tmpfrm==from.frm)
				break;
	}
	else
		firstfrm=from.firstfrm;
	subfields=from.subfields;
	altformat=from.altformat;

	if(keepfrm)
		set_frm(tmpfirstfrm, tmpfrm);

	tagmap=from.tagmap;

	return *this;
}

//relink data from another field. The old field will become empty
void field::moveFrom(field &from)
{
	if(this==&from)
		return;

	*this=from;
	from.clear();
}

void field::swap(field &from)
{
	if(this==&from)
		return;

	field tmpfld;
	tmpfld.moveFrom(from);
	from.moveFrom(*this);
	moveFrom(tmpfld);
}

int field::compare (const field& other) const
{
	if(empty() && other.empty())
		return 0;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_bcdsf:
		case fldformat::fld_tlv:
			break;
		default:
			return data.compare(other.data);
	}

	for(const_iterator i=begin(), j=other.begin(); i!=end() && j!=other.end(); ++i, ++j)
	{
		while((!i->second.frm || i->second.frm->dataFormat==fldformat::fld_isobitmap || i->second.frm->dataFormat==fldformat::fld_bitmap) && i!=end()) //skip subfields without format and bitmaps
			++i;
		while((!j->second.frm || j->second.frm->dataFormat==fldformat::fld_isobitmap || j->second.frm->dataFormat==fldformat::fld_bitmap) && j!=other.end())
			++j;
		if(i==end() && j==other.end())
			break;
		if(i==end())
			return 1;
		if(j==other.end())
			return -1;
		if(i->first<j->first)
			return 1;
		if(j->first<i->first)
			return -1;
		int r=i->second.compare(j->second);
		if(r)
			return r;
	}
	return 0;
}

void field::print_message(string numprefix) const
{
	if(!numprefix.empty())
		printf("%s ", numprefix.c_str());

	printf("%s", frm->get_description().c_str());

	if(!data.empty())
		printf(" (%lu): \"%s\"\n", flength, data.c_str());
	else
		printf(":\n");

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_bcdsf:
		case fldformat::fld_tlv:
			for(const_iterator i=begin(); i!=end(); ++i)
				if(!i->second.empty())
					i->second.print_message(numprefix + to_string(i->first) + ".");
			break;
		default:
			break;
	}
}

bool field::empty(void) const
{
	if(frm->empty())
		return true;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_bcdsf:
		case fldformat::fld_tlv:
			break;
		default:
			return data.empty();
	}

	if(subfields.empty())
		return true;

	for(const_iterator i=begin(); i!=end(); ++i)
		if(i->second.frm && i->second.frm->dataFormat!=fldformat::fld_isobitmap && i->second.frm->dataFormat!=fldformat::fld_bitmap && !i->second.empty())
			return false;

	return true;
}

// Switches to the next applicable altformat.
void field::switch_altformat(void)
{
	const fldformat *frmtmp=frm;

	for(unsigned int i=altformat+1; (frmtmp=frmtmp->get_altformat())!=NULL; i++)
	{
		try
		{
			change_format(frmtmp);
			altformat=i;
			return;
		}
		catch (const exception& e)
		{
			continue;
		}
	}

	throw invalid_argument("No applicable altformats found after the current");
}

// Rewinds to the first applicable altformat.
void field::reset_altformat(void)
{
	const fldformat *frmtmp=firstfrm;

	for(unsigned int i=0; frmtmp!=NULL; frmtmp=frmtmp->get_altformat(), i++)
	{
		try
		{
			change_format(frmtmp);
			altformat=i;
			return;
		}
		catch (const exception& e)
		{
			continue;
		}
	}

	throw invalid_argument("None formats are applicable");
}

// Assigns a new format to the field. Not to be used to switch to an altformat because it assumes the new format to be the root of altformat, so the information about the first altformat is lost and reset_altformat() would not reset to original altformat, use switch_altformat() instead.
// If frmaltnew is not null, it must be an altformat of frmnew
void field::set_frm(const fldformat *frmnew, const fldformat *frmaltnew)
{
	const fldformat *frmtmp=frmnew;

	for(unsigned int i=0; frmtmp!=NULL; frmtmp=frmtmp->get_altformat(), i++)
	{
		try
		{
			change_format(frmtmp);
			altformat=i;
			break;
		}
		catch (const exception& e)
		{
			continue;
		}
	}

	if(!frmtmp)
		throw invalid_argument("The new format is not applicable");

	if(deletefrm)
	{
		delete firstfrm;
		deletefrm=false;
	}

	firstfrm=frmnew;

	if(frmaltnew)
	{
		frmtmp=frm;
		for(unsigned int i=altformat; frmtmp!=NULL; frmtmp=frmtmp->get_altformat(), i++)
			if(frmtmp==frmaltnew)
			{
				change_format(frmaltnew);
				altformat=i;
				return;
			}
	}
}

// Internal function to change current format, not to be called directly
// If the new format does not suit the already present field tree, the function will restore the original format. The function guarantees consistency of the resulting field format, however it is not guaranteed that all subfields will remain on the same altformats in case of failure.
void field::change_format(const fldformat *frmnew)
{
	iterator i;
	vector<const fldformat*> frmold(1, frm);
	bool sameformat(frm==frmnew);

	if(!frmnew)
		throw invalid_argument("No new format provided");

	if(debug)
		printf("trying to change sf %d format from %s to %s\n", tag, frm->get_description().c_str(), frmnew->get_description().c_str());

	if(!data.empty() && !frmnew->data.empty() && data!=frmnew->data)
		throw invalid_argument("data does not match mandatory data of the new format");

	frm=frmnew;

	try
	{
		for(i=begin(); i!=end(); ++i)
		{
			frmold.push_back(i->second.frm);
			i->second.set_frm(&frmnew->sf(i->first));
		}
	}
	catch (const exception& e)
	{
		if(debug)
			printf("Error: Unable to change field format (%d). Reverting.\n", i->first);

		try
		{
			frmold.pop_back();

			if(i!=begin())
			{
				for(--i; i!=begin(); --i)
				{
					i->second.change_format(frmold.back());
					frmold.pop_back();
				}

				i->second.change_format(frmold.back());
				frmold.pop_back();
			}

			frm=frmold.back();
			frmold.pop_back();

			if(!frmold.empty())
				throw runtime_error("field count mismatch");
		}
		catch (const exception& e1)
		{
			throw runtime_error("Unable to revert to the original format. The field is in undefined state.");
		}

		if(!sameformat)
			throw invalid_argument("Fields don't fit in new format");
	}

	if(frm->hasBitmap!=-1 && !subfields.empty() && (--i)->first > frm->hasBitmap)
		sf(frm->hasBitmap); //make sure to not skip bitmap field on serialize
}

//returns reference to subfield. If it does not exists, it will be added.
field& field::sf(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	if(n0 < 0)
		throw out_of_range("Bad subfield number");

	if(!sfexist(n0))
	{
		if(!frm->sfexist(n0))
			throw out_of_range("No format for the subfield");

		if(!subfields.count(n0))
		{
			if(frm->dataFormat!=fldformat::fld_tlv)
				tagmap[n0]=n0;
			else if(subfields.empty())
				tagmap[0]=n0;
			else
				tagmap[tagmap.rbegin()->first+1]=n0;
		}

		if(subfields[n0].empty())
		{
			subfields[n0].set_frm(&frm->sf(n0));
			subfields[n0].tag=n0;
		}

		if(frm->hasBitmap!=-1 && frm->dataFormat != fldformat::fld_tlv && n0 > frm->hasBitmap)
			sf(frm->hasBitmap); //make sure to not skip bitmap field on serialize
	}

	if(n1<0)
		return subfields[n0];
	else
		return subfields[n0].sf(n1, n2, n3, n4, n5, n6, n7, n8, n9);
}

bool field::sfexist(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9) const
{
	if(n0 < 0)
		return false;

	map<int,field>::const_iterator it = subfields.find(n0);

	if(it == subfields.end())
		return false;

	if(n1<0)
		return true;
	else
		return it->second.sfexist(n1, n2, n3, n4, n5, n6, n7, n8, n9);
}

int field::vsnprintf(size_t pos, size_t size, const char *format, va_list ap)
{
	static char tmpstr[256];
	char *strptr = size+1>sizeof(tmpstr)/sizeof(char) ? (char*)malloc((size+1)*sizeof(char)) : tmpstr; //TODO: replace malloc with new

	int count=std::vsnprintf(strptr, size+1, format, ap);

	if(count<0)
	{
		if(strptr!=tmpstr)
			free(strptr);
		return count;
	}

	data.replace(pos, string::npos, strptr, size<(size_t)count ? size : count);

	if(strptr!=tmpstr)
		free(strptr);

	return count;
}

size_t field::strftime(size_t max, const char *format, const struct tm *tm)
{
	static char tmpstr[256];
	char *strptr = max+1>sizeof(tmpstr)/sizeof(char) ? (char*)malloc((max+1)*sizeof(char)) : tmpstr;

	size_t count=std::strftime(strptr, max+1, format, tm);

	if(count==0)
	{
		if(strptr!=tmpstr)
			free(strptr);
		return count;
	}

	data.assign(strptr, count);

	if(strptr!=tmpstr)
		free(strptr);

	return count;
}

string field::to_string(unsigned int n)
{
	ostringstream ss;
	ss << n;
	return ss.str();
}

field::iterator field::begin(void)
{
	return iterator(tagmap.begin(), subfields);
}

field::iterator field::end(void)
{
	return iterator(tagmap.end(), subfields);
}

field::iterator field::find(int n)
{
	map<int,field>::const_iterator i=subfields.find(n);
	if(i==subfields.end())
		return end();
	return iterator(tagmap.find(i->second.tag), subfields);
}

field::const_iterator field::begin(void) const
{
	return const_iterator(tagmap.begin(), subfields);
}

field::const_iterator field::end(void) const
{
	return const_iterator(tagmap.end(), subfields);
}

field::const_iterator field::find(int n) const
{
	map<int,field>::const_iterator i=subfields.find(n);
	if(i==subfields.end())
		return end();
	return const_iterator(tagmap.find(i->second.tag), subfields);
}

field::reverse_iterator field::rbegin(void)
{
	return reverse_iterator(end());
}

field::const_reverse_iterator field::rbegin(void) const
{
	return const_reverse_iterator(end());
}

field::reverse_iterator field::rend(void)
{
	return reverse_iterator(begin());
}

field::const_reverse_iterator field::rend(void) const
{
	return const_reverse_iterator(begin());
}

