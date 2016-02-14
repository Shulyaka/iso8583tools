#include <cstdio>
#include <cstdlib> //for exit(). TODO: Remove exit() and introduce exceptions
#include <sstream>
#include <ctime>

#include "parser.h"

using namespace std;

string to_string(unsigned int);

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
// Returns true on success, false if no applicable altformats left after the current
bool field::switch_altformat(void)
{
	const fldformat *frmtmp=frm;

	for(unsigned int i=altformat+1; (frmtmp=frmtmp->get_altformat())!=NULL; i++)
		if(change_format(frmtmp))
		{
			altformat=i;
			return true;
		}

	return false;
}

// Rewinds to the first applicable altformat.
// Returns true on success, false if none formats are applicable (which is an error)
bool field::reset_altformat(void)
{
	const fldformat *frmtmp=firstfrm;

	for(unsigned int i=0; frmtmp!=NULL; frmtmp=frmtmp->get_altformat(), i++)
		if(change_format(frmtmp))
		{
			altformat=i;
			return true;
		}

	return false;
}

// Assigns a new format to the field. Not to be used to switch to an altformat because it assumes the new format to be the root of altformat, so the information about the first altformat is lost and reset_altformat() would not reset to original altformat, use switch_altformat() instead.
// If frmaltnew is not null, it must be an altformat of frmnew
bool field::set_frm(const fldformat *frmnew, const fldformat *frmaltnew)
{
	const fldformat *frmtmp=frmnew;

	for(unsigned int i=0; frmtmp!=NULL; frmtmp=frmtmp->get_altformat(), i++)
		if(change_format(frmtmp))
		{
			altformat=i;
			break;
		}

	if(!frmtmp)
		return false;

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
				altformat=i;
				return change_format(frmaltnew);
			}
	}

	return true;
}

// Internal function to change current format, not to be called directly
// If the new format does not suit the already present field tree, the function will restore the original format. The function guarantees consistency of the resulting field format, however it is not guaranteed that all subfields will remain on the same altformats in case of failure.
bool field::change_format(const fldformat *frmnew)
{
	iterator i;
	const fldformat *frmold=frm;

	if(!frmnew)
		return false;

	if(frm == frmnew)
		return true;

	frm=frmnew;

	for(i=begin(); i!=end(); ++i)
		if(!frmnew->sfexist(i->first) || !i->second.change_format(&frmnew->sf(i->first)))
			break;

	if(i!=end())
	{
		if(debug)
			printf("Error: Unable to change field format (%d). Reverting.\n", i->first);

		for(frm=frmold; i!=begin(); --i)
			if(!frmold->sfexist(i->first) || !i->second.change_format(&frmold->sf(i->first)))
				if(debug)
					printf("Error: Unable to revert\n");

		if(!frmold->sfexist(i->first) || !i->second.change_format(&frmold->sf(i->first)))
			if(debug)
				printf("Error: Unable to revert\n");

		return false;
	}

	if(frm->hasBitmap!=-1 && !subfields.empty() && (--i)->first > frm->hasBitmap)
		sf(frm->hasBitmap); //make sure to not skip bitmap field on serialize

	return true;
}

//returns reference to subfield. If it does not exists, it will be added.
field& field::sf(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int tnum;

	if(n0 < 0 && frm->dataFormat != fldformat::fld_tlv)
	{
		printf("Error: Wrong subfield number: %d\n", n0);
		exit(1);
	}

	if(!sfexist(n0))
	{
		if(!frm->sfexist(n0))
		{
			printf("Error: Wrong format for subfield number: %d\n", n0);
			exit(1);
		}

		if(frm->dataFormat == fldformat::fld_tlv)
		{
			map<int,int>::const_iterator t = tagmap.find(n0);

			if(t == tagmap.end())
			{
				if(subfields.empty())
					tnum=0;
				else
					tnum=subfields.rbegin()->first+1;

				tagmap[n0]=tnum;
			}
			else
				tnum = t->second;
		}
		else
			tnum=n0;

		if(subfields[tnum].empty())
		{
			subfields[tnum].set_frm(&frm->sf(n0));
			subfields[tnum].tag=n0;
		}

		if(frm->hasBitmap!=-1 && frm->dataFormat != fldformat::fld_tlv && tnum > frm->hasBitmap)
			sf(frm->hasBitmap); //make sure to not skip bitmap field on serialize
	}
	else
	{
		if(frm->dataFormat == fldformat::fld_tlv)
			tnum = tagmap[n0];
		else
			tnum=n0;
	}

	if(n1<0)
		return subfields[tnum];
	else
		return subfields[tnum].sf(n1, n2, n3, n4, n5, n6, n7, n8, n9);
}

bool field::sfexist(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9) const
{
	if(n0 < 0)
		return false;

	if(frm->dataFormat == fldformat::fld_tlv)
	{
		map<int,int>::const_iterator t = tagmap.find(n0);

		if(t == tagmap.end())
			return false;

		n0 = t->second;
	}

	const_iterator it = subfields.find(n0);

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
	char *strptr = size+1>sizeof(tmpstr)/sizeof(char) ? (char*)malloc((size+1)*sizeof(char)) : tmpstr;

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

string to_string(unsigned int n)
{
	ostringstream ss;
	ss << n;
	return ss.str();
}

field::iterator field::begin(void)
{
	return iterator(subfields.begin());
}

field::iterator field::end(void)
{
	return iterator(subfields.end());
}

field::iterator field::find(int n)
{
	return iterator(subfields.find(n));
}

field::const_iterator field::begin(void) const
{
	return const_iterator(subfields.begin());
}

field::const_iterator field::end(void) const
{
	return const_iterator(subfields.end());
}

field::const_iterator field::find(int n) const
{
	return const_iterator(subfields.find(n));
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

