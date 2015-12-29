#include <cstdio>
#include <cstdlib> //for exit(). TODO: Remove exit() and introduce exceptions
#include <sstream>

#include "parser.h"

using namespace std;

string to_string(unsigned int);

field::field(void)
{
	fill_default();
	frm=new fldformat();
	deletefrm=true;
	firstfrm=frm;
}

field::field(fldformat *format)
{
	fill_default();
	frm=format;
	firstfrm=frm;
}

field::field(const std::string &filename)
{
	fill_default();
	frm=new fldformat(filename);
	deletefrm=true;
	firstfrm=frm;
}

//copy constructor
field::field(const field &from)
{
	copyFrom(from);
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
	length=0;
	frm=NULL;
	subfields.clear();
	altformat=0;
	deletefrm=false;
	firstfrm=frm;
}

void field::clear(void)
{
	fldformat *tmpfrm=frm, *tmpfirstfrm=firstfrm;
	unsigned int tmpaltformat=altformat;
	bool delfrm=deletefrm;
	fill_default();
	deletefrm=delfrm;
	firstfrm=tmpfirstfrm; //make formats immune to clear()
	frm=tmpfrm;
	altformat=tmpaltformat;
}

//forks the field. All data and subfields are also copied so that all pointers except frm will have new values to newly copied data but non-pointers will have same values
void field::copyFrom(const field &from)
{
	if(this==&from)
		return;

	clear();

	data=from.data;
	start=from.start;
	blength=from.blength;
	length=from.length;
	deletefrm=from.deletefrm;
	frm=from.frm;
	if(deletefrm)
	{
		firstfrm=new fldformat(*from.firstfrm);
		frm=firstfrm;
		for(fldformat *tmpfrm=from.firstfrm; tmpfrm!=NULL; tmpfrm=tmpfrm->get_altformat(), frm=frm->get_altformat())
			if(tmpfrm==from.frm)
				break;
	}
	else
		firstfrm=from.firstfrm;
	subfields=from.subfields;
	altformat=from.altformat;
}

//relink data from another field. The old field will become empty
void field::moveFrom(field &from)
{
	if(this==&from)
		return;

	copyFrom(from);
	from.clear();
}

void field::print_message(string numprefix) const
{
	if(!numprefix.empty())
		printf("%s ", numprefix.c_str());

	printf("%s", frm->get_description().c_str());

	if(!data.empty())
		printf(" (%d): [%s]\n", length, data.c_str());
	else
		printf(":\n");

	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_BCDSF:
		case FRM_TLV:
		case FRM_TLVEMV:
			for(const_iterator i=begin(); i!=end(); ++i)
				i->second.print_message(numprefix + to_string(i->first) + ".");
			break;
	}
}

bool field::is_empty(void) const
{
	if(frm->is_empty())
		return true;

	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_BCDSF:
		case FRM_TLV:
		case FRM_TLVEMV:
			break;
		case FRM_ISOBITMAP:
		case FRM_BITMAP:
			return false;
		default:
			return data.empty();
	}

	if(subfields.empty())
		return true;

	for(const_iterator i=begin(); i!=end(); ++i)
		if(i->second.frm && i->second.frm->dataFormat!=FRM_ISOBITMAP && i->second.frm->dataFormat!=FRM_BITMAP && !i->second.is_empty())
			return false;

	return true;
}

// Switches to the next applicable altformat.
// Returns true on success, false if no applicable altformats left after the current
bool field::switch_altformat(void)
{
	fldformat *frmtmp=frm;

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
	fldformat *frmtmp=firstfrm;

	for(unsigned int i=0; frmtmp!=NULL; frmtmp=frmtmp->get_altformat(), i++)
		if(change_format(frmtmp))
		{
			altformat=i;
			return true;
		}

	return false;
}

// Assigns a new format to the field. Not to be used to switch to an altformat because it assumes the new format to be the root of altformat, so the information about the first altformat is lost and reset_altformat() would not reset to original altformat, use switch_altformat() instead.
bool field::set_frm(fldformat *frmnew)
{
	fldformat *frmtmp=frmnew;

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

	return true;
}

// Internal function to change current format, not to be called directly
// If the new format does not suit the already present field tree, the function will restore the original format. The function guarantees consistency of the resulting field format, however it is not guaranteed that all subfields will remain on the same altformats in case of failure.
bool field::change_format(fldformat *frmnew)
{
	iterator i;
	fldformat *frmold=frm;

	if(!frmnew)
		return false;

	if(frm == frmnew)
		return true;

	frm=frmnew;

	for(i=begin(); i!=end(); i++)
		if(!frmnew->sfexist(i->first) || !i->second.change_format(&frmnew->sf(i->first)))
			break;

	if(i!=end())
	{
		if(debug)
			printf("Error: Unable to change field format (%d). Reverting.\n", i->first);

		for(frm=frmold; i!=begin(); i--)
			if(!frmold->sfexist(i->first) || !i->second.change_format(&frmold->sf(i->first)))
				if(debug)
					printf("Error: Unable to revert\n");

		if(!frmold->sfexist(i->first) || !i->second.change_format(&frmold->sf(i->first)))
			if(debug)
				printf("Error: Unable to revert\n");

		return false;
	}

	return true;
}

//a segfault-safe accessor function. Returns a pointer to the field contents. If the field does not exist, it would be created. If it cannot be created, a valid pointer to a dummy array is returned.
string& field::add_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static string def="";
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!curfld || !curfld->frm)
			return def;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld || !curfld->frm)
		return def;

	return curfld->data;
}

int field::field_format(int newaltformat, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	fldformat *tmpfrm;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!curfld || !curfld->frm)
			return 2;

		tmpfrm=curfld->frm;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld || !curfld->frm)
		return 2;

	if(i==0)
	{
		if(curfld->altformat>=newaltformat || newaltformat==0) //no need or unable to go back in the list
		{
			if(newaltformat==0 && curfld->altformat==1)
				curfld->altformat=0;

			return 3;
		}

		if(newaltformat==1 && curfld->altformat==0)
		{
			curfld->altformat=1;
			return 0;
		}

		tmpfrm=curfld->frm;

		for(i=curfld->altformat==0?1:curfld->altformat; i<newaltformat; i++)
			if(!tmpfrm->altformat)
				return 4;
			else
				tmpfrm=tmpfrm->altformat;

		if(curfld->change_format(tmpfrm))
			curfld->altformat=newaltformat;
		else
			return 1;

		return 0;
	}

	tmpfrm=&tmpfrm->sf(n[i-1]);

	for(i=1; i<altformat; i++)
		if(!tmpfrm->altformat)
			return 4;
		else
			tmpfrm=tmpfrm->altformat;

	if(curfld->change_format(tmpfrm))
		curfld->altformat=newaltformat;
	else
		return 1;

	return 0;
} 

//a segfault-safe accessor function. Return a pointer to the fields contents. If the field does not exist, returns a valid pointer to an empty string. The field structure is not modified.
const string& field::get_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const string def="";
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!curfld || !curfld->sfexist(n[i]))
			return def;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld)
		return def;

	return curfld->data;
}

void field::remove_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0])-1; i++)
	{
		if(n[i+1]==-1)
			break;

		if(!curfld || !curfld->sfexist(n[i]))
			return;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld || !curfld->sfexist(n[i]))
		return;

	subfields.erase(n[i]);
	return;
}

//returns false if fields does not exists or has no subfields or empty.
//otherwise, returns true
bool field::has_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!curfld || !curfld->sfexist(n[i]))
			return false;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld)
		return false;

	switch(curfld->frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_TLV:
		case FRM_TLVEMV:
		case FRM_BCDSF:
			return subfields.empty();
		default:
			return data.empty();
	}
}

//returns reference to subfield. If it does not exists, it will be added.
field& field::sf(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	if(n0 < 0)
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

		if(subfields[n0].is_empty())
			subfields[n0].set_frm(&frm->sf(n0));
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

	const_iterator it = subfields.find(n0);

	if(it == subfields.end())
		return false;

	if(n1<0)
		return true;
	else
		return it->second.sfexist(n1, n2, n3, n4, n5, n6, n7, n8, n9);
}

string to_string(unsigned int n)
{
	ostringstream ss;
	ss << n;
	return ss.str();
}
