#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include "parser.h"

using namespace std;

string to_string(unsigned int);

//constructor
fldformat::fldformat(void)
{
	fill_default();
}

fldformat::fldformat(const string &filename)
{
	fill_default();
	if(!load_format(filename))
	{
		printf("Error: Unable to load format\n");
		exit(1);
	}
}

//copy constructor
fldformat::fldformat(const fldformat &from)
{
	fill_default();
	*this=from;
}

//destructor
fldformat::~fldformat(void)
{
	clear();
	if(parent)
	{
		if(parent->altformat==this)
		{
			parent->altformat=NULL;
			return;
		}
	}
}

//default format
void fldformat::fill_default(void) //TODO: When API stabilizes, enhance constructors to use initializers instead of this function
{
	description.clear();
	lengthFormat=fll_unknown;
	lengthLength=0;
	lengthInclusive=false;
	maxLength=1024;
	addLength=0;
	dataFormat=fld_ascii;
	tagFormat=flt_ascii;
	tagLength=0;
	data.clear();
	subfields.clear();
	altformat=NULL;
	parent=NULL;
	hasBitmap=-1;
}

//empty to default
void fldformat::clear(void)
{
	fldformat *tmpfrm=parent;

	if(altformat!=NULL)
		delete altformat;

	fill_default();
	parent=tmpfrm; //make parent immune to clear
}

bool fldformat::empty(void) const
{
	return description.empty()
	    && lengthFormat==fll_unknown
	    && lengthLength==0
	    && lengthInclusive==false
	    && maxLength==1024
	    && addLength==0
	    && dataFormat==fld_ascii
	    && tagFormat==flt_ascii
	    && tagLength==0
	    && data==""
	    && subfields.empty()
	    && altformat==NULL
	    && hasBitmap==-1;
}

//forks the format. All data and subformat are also copied so that all pointers will have new values to newly copied data but non-pointers will have same values
fldformat& fldformat::operator= (const fldformat &from)
{
	if(this==&from)
		return *this;

	clear();

	description=from.description;
	lengthFormat=from.lengthFormat;
	lengthLength=from.lengthLength;
	lengthInclusive=from.lengthInclusive;
	maxLength=from.maxLength;
	addLength=from.addLength;
	dataFormat=from.dataFormat;
	tagFormat=from.tagFormat;
	tagLength=from.tagLength;
	data=from.data;

	subfields=from.subfields;
	for(map<int,fldformat>::iterator i=subfields.begin(); i!=subfields.end(); ++i)
		i->second.parent=this;

	if(from.altformat)
		altformat=new fldformat(*from.altformat);

	hasBitmap=from.hasBitmap;

	return *this;
}


//relink data from another format. The old format will become empty
void fldformat::moveFrom(fldformat &from)
{
	if(this==&from)
		return;

	clear();

	description=from.description;
	lengthFormat=from.lengthFormat;
	lengthLength=from.lengthLength;
	lengthInclusive=from.lengthInclusive;
	maxLength=from.maxLength;
	addLength=from.addLength;
	dataFormat=from.dataFormat;
	tagFormat=from.tagFormat;
	tagLength=from.tagLength;
	data=from.data;
	subfields=from.subfields;
	for(map<int,fldformat>::iterator i=subfields.begin(); i!=subfields.end(); ++i)
		i->second.parent=this;

	altformat=from.altformat;
	hasBitmap=from.hasBitmap;

	from.altformat=NULL;

	from.clear();
}

void fldformat::print_format(string numprefix)
{
	if(!numprefix.empty())
		printf("%s\t", numprefix.c_str());
	else
		printf("message\t");

	if(dataFormat==fld_isobitmap)
		printf("ISOBITMAP");
	else
	{
		if(lengthFormat==fll_fixed)
			printf("F");
		else if(lengthFormat==fll_unknown)
			printf("U");
		else if(lengthFormat==fll_ber)
			printf("M");
		else
			for(size_t i=0; i<lengthLength; i++)
				switch(lengthFormat)
			        {
					case fll_ascii:
						printf("L");
						break;
					case fll_bin:
						printf("B");
						break;
					case fll_bcd:
						printf("C");
						break;
					case fll_ebcdic:
						printf("E");
						break;
					case fll_fixed:
					case fll_unknown:
					case fll_ber:
						break; // logic error
				}

		if(lengthInclusive)
			printf("I");

		printf("%lu", maxLength);

		if(addLength)
			printf("%+ld", addLength);

		switch(dataFormat)
	        {
			case fld_subfields:
				printf("SF");
				break;
			case fld_tlv:
				switch(tagFormat)
				{
					case flt_ebcdic:
						printf("TLV%luEBCDIC", tagLength);
						break;
					case flt_bcd:
						printf("TLV%luBCD", tagLength);
						break;
					case flt_bin:
						printf("TLV%luBIN", tagLength);
						break;
					case flt_ascii:
						printf("TLV%luASCII", tagLength);
						break;
					case flt_ber:
						printf("TLVBER");
						break;
				}
				break;
			case fld_bcdsf:
				printf("BCDSF");
				break;
			case fld_bitmap:
				printf("BITMAP");
				break;
			case fld_bitstr:
				printf("BITSTR");
				break;
			case fld_ebcdic:
				printf("EBCDIC");
				break;
			case fld_bcd:
				printf("BCD");
				break;
			case fld_hex:
				printf("HEX");
				break;
			case fld_ascii:
				printf("ASCII");
				break;
			case fld_isobitmap:
				break; //logic error
		}

		if(!data.empty())
			printf("=%s", data.c_str());
	}

	printf("\t%s\n", description.c_str());

	switch(dataFormat)
	{
		case fld_subfields:
		case fld_bcdsf:
		case fld_tlv:
			for(map<int,fldformat>::iterator i=subfields.begin(); i!=subfields.end(); ++i)
				i->second.print_format((numprefix.empty() ? "" : numprefix + ".") + (i->first==-1? "*" : to_string(i->first)));
			break;
		default:
			break;
	}

	if(altformat)
		altformat->print_format(numprefix);
}

// load format from file
// returns false on failure, true on success
// if already loaded, adds new as altformat
bool fldformat::load_format(const string &filename)	//TODO: auto set maxLength for some unknown length type formats
{
	string line;
	char number[256];
	char format[256];
	char descr[256];
	size_t j=0;
	int k=0;
	fldformat *frmtmp, *frmnew;
	map<string,fldformat> orphans;
	std::ifstream file(filename.c_str());

	if(!file)
	{
		if(debug)
			perror("Error: Unable to open file\n");
		return false;
	}

	orphans["message"];

	while (!getline(file, line).eof())
	{
		if(line.empty())
			continue;

		j=sscanf(line.c_str(), "%s %s %[][a-zA-Z0-9 .,/;:'\"\\|?!@#$%^&*(){}<>_+=-]", number, format, descr);
		if(number[0]=='#')
			continue;
		if(j==2)
			strcpy(descr, "No description");
		else if (j!=3)
		{
			if(debug)
				printf("Warning: Wrong format in line, skipping:\n%s\n", line.c_str());
			continue;
		}

		//printf("Number[%s] format[%s] descr[%s]\n", number, format, descr);

		frmtmp=new fldformat;

		if (!frmtmp->parseFormat(format, orphans))
		{
			if(debug)
				printf("Error: Unable to parse format, skipping:\n%s\n", line.c_str());
			delete frmtmp;
			continue;
		}

		frmtmp->description.assign(descr);

		frmnew=orphans["message"].get_by_number(number, orphans, frmtmp->dataFormat==fld_bitmap || frmtmp->dataFormat==fld_isobitmap);
		if(frmnew)
		{
			if(!frmnew->empty())
			{
				frmnew->altformat=frmtmp;
				frmnew->altformat->parent=frmnew;
			}
			else
			{
				frmnew->moveFrom(*frmtmp);
				delete frmtmp;
			}
		}
		else
		{
			if(orphans.count(number) && !orphans[number].empty())
			{
				frmnew=orphans[number].get_lastaltformat();
				frmnew->altformat=frmtmp;
				frmnew->altformat->parent=frmnew;
			}
			else
			{
				orphans[number].moveFrom(*frmtmp);
				delete frmtmp;
			}
		}

		k++;
	}

	if(k>1)
	{
		if(orphans["message"].description.empty())
		{
			printf("Warning: No 'message' format, implying default\n");
			orphans["message"].description.assign("No description");
		}

		if(!empty())
		{
			frmtmp=get_lastaltformat();
			frmtmp->altformat=new fldformat;
			frmtmp->altformat->parent=frmtmp;
			frmtmp->altformat->moveFrom(orphans["message"]);
		}
		else
			moveFrom(orphans["message"]);
	}
	else
	{
		if(debug)
			printf("Error: No fields loaded\n");
		return false;
	}

	if(debug)
		printf("Info: Loaded %d fields\n", k);

	return true;
}

// parses a string and returns a pointer to format by its number.
// If not found but its parent exists, a new entry would be created.
// If the parent does not yet exists, NULL is returned.
// During parent search, always choses latest altformat wherever possible
fldformat* fldformat::get_by_number(const char *number, map<string,fldformat> &orphans, bool isBitmap)
{
	size_t i, l;
	int n;
	string key;

	if(altformat)
		return get_lastaltformat()->get_by_number(number, orphans, isBitmap);

	if(!number)
	{
		if(debug)
			printf("Error: No number provided\n");
		return NULL;
	}

	l=strlen(number);

	for(i=0; i<l; i++) //find non-numeric character
		if((number[i]<'0' || number[i]>'9') && number[i]!='*')
			break;

	if(i==l) //if all are numeric, this is the last iteration
	{
		if(number[i-1]=='*')
			n=-1;
		else
		{
			n=atoi(number);
			if(n<0)
			{
				if(debug)
					printf("Negative field number %s\n", number);
				return NULL;
			}
		}

		if(isBitmap)
		{
			if(get_lastaltformat()->hasBitmap!=-1)
			{
				printf("Error: Only one bitmap per subfield is allowed (%s)\n", get_lastaltformat()->description.c_str());
				exit(1);
			}
			get_lastaltformat()->hasBitmap=n;
		}

		return get_lastaltformat()->subfields[n].get_lastaltformat();
	}

	if(i==0 || number[i]!='.') //if has non-numeric characters, then search the orphans
	{
		for(; i<l; i++)
			if(number[i]=='.')
				break;

		if(i==l || i+1==l) //last iteration, no parent
			return NULL;

		key=string(number,i);

		if(!orphans.count(key)) //no parent, not last iteration.
			return NULL;

		return orphans[key].get_lastaltformat()->get_by_number(number+i+1, orphans, isBitmap);
	}

	//now we know it's not the last iteration
	if(number[i-1]=='*')
		n=-1;
	else
	{
		n=atoi(number);
		if(n<0)
		{
			if(debug)
				printf("Negative field number %s\n", number);
			return NULL;
		}
	}

	if(!subfields.count(n))
	{
		if(debug)
			printf("Warning: Parent format not loaded yet [%s][%d] %s\n", number, n, description.c_str());
		return NULL;
	}

	return subfields[n].get_lastaltformat()->get_by_number(number+i+1, orphans, isBitmap);
}

//parses format string
bool fldformat::parseFormat(const char *format, map<string,fldformat> &orphans)
{
	size_t i, j=0;
	const char *p;
	fldformat *tmpfrm;

	if(!format)
	{
		if(debug)
			printf("Error: Null pointer to second arg\n");
		return false;
	}

	if(!strcmp(format, "ISOBITMAP"))
	{
		dataFormat=fld_isobitmap;
		maxLength=192;
		return true;
	}

	switch (format[0])
	{
		case 'F':
			lengthFormat=fll_fixed;

			lengthLength=0;

			j=1;

			break;

		case 'L':
			lengthFormat=fll_ascii;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='L')
					break;

			lengthLength=j;

			break;

		case 'B':
			lengthFormat=fll_bin;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='B')
					break;

			lengthLength=j;

			break;

		case 'C':
			lengthFormat=fll_bcd;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='C')
					break;

			lengthLength=j;

			break;

		case 'E':
			lengthFormat=fll_ebcdic;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='E')
					break;

			lengthLength=j;

			break;

		case 'U':
			lengthFormat=fll_unknown;
			lengthLength=0;
			j=1;
			break;

		case 'M':
			lengthFormat=fll_ber;
			lengthLength=1;
			j=1;
			break;

		case 'R':
			tmpfrm=orphans["message"].get_by_number(format+1, orphans);
			if(!tmpfrm)
			{
				if(orphans.count(format+1))
				{
					*this=*orphans[format+1].get_lastaltformat();
				}
				else
				{
					if(debug)
						printf("Error: Unable to find referenced format (%s) (no parent loaded)\n", format+1);
					return false;
				}
			}
			else if(tmpfrm->empty())
			{
				if(debug)
					printf("Error: Unable to find referenced format (%s) (parent loaded)\n", format+1);
				tmpfrm->erase();
				return false;
			}
			else
				*this=*tmpfrm;

			description.clear();

			return true;

			break;

		default:
			if(debug)
				printf("Error: Unrecognized length format (%s)\n", format);
	}

	if(format[j]=='I')
	{
		lengthInclusive=true;
		j++;
	}
	else
		lengthInclusive=false;

	for(i=j; i<strlen(format); i++)
		if (format[i]<'0' || format[i]>'9')
			break;
	if(i==j)
	{
		if(debug)
			printf("Error: Unrecognized max length (%s)\n", format);
		return false;
	}

	maxLength=atoi(format+j);

	if(format[i]=='+' || format[i]=='-')
	{
		for(j=i+1; j<strlen(format); j++)
			if (format[j]<'0' || format[j]>'9')
				break;

		if(j==i+1)
		{
			if(debug)
				printf("Error: Unrecognized additional length (%s)\n", format);
			return false;
		}

		addLength=(format[i]=='+')? atoi(format+i+1) : -atoi(format+i+1);
		i=j;
	}
	else
		addLength=0;

	if(!maxLength)
		return true; //allow zero length fields

	p=strstr(format+i, "=");
	if(p)
	{
		if(p[1])
			data.assign(p+1);
		else
			data.assign(maxLength, ' '); //replacing empty string with a space character. It is a feature, not bug.
	}

	if(!strncmp(format+i, "SF", 2))
	{
		dataFormat=fld_subfields;
		i+=2;
	}
	else if(!strncmp(format+i, "TLV", 3))
	{
		i+=3;
		dataFormat=fld_tlv;
		if(!strncmp(format+i, "BER", 3))
		{
			tagFormat=flt_ber;
			i+=3;
		}
		else
		{
			tagLength=atoi(format+i);
			for(; format[i]>='0' && format[i]<='9'; i++);

			if(!strncmp(format+i, "EBCDIC", 6))
			{
				tagFormat=flt_ebcdic;
				i+=6;
			}
			else if(!strncmp(format+i, "EBC", 3))
			{
				tagFormat=flt_ebcdic;
				i+=3;
			}
			else if(!strncmp(format+i, "BCD", 3))
			{
				tagFormat=flt_bcd;
				i+=3;
			}
			else if(!strncmp(format+i, "BIN", 3))
			{
				tagFormat=flt_bin;
				i+=3;
			}
			else if(!strncmp(format+i, "ASCII", 5))
			{
				tagFormat=flt_ascii;
				i+=5;
			}
			else if(!strncmp(format+i, "ASC", 3))
			{
				tagFormat=flt_ascii;
				i+=3;
			}
			else if(format[i]=='\0')
				tagFormat=flt_ascii;
			else
			{
				if(debug)
					printf("Error: Unrecognized TLV tag format (%s)\n", format+i);
				return false;
			}
		}
	}
	else if(!strncmp(format+i, "BCDSF", 5))
	{
		dataFormat=fld_bcdsf;
		i+=5;
	}
	else if(!strncmp(format+i, "BITMAP", 6))
	{
		dataFormat=fld_bitmap;
		i+=6;
	}
	else if(!strncmp(format+i, "BITSTR", 6))
	{
		dataFormat=fld_bitstr;
		i+=6;
	}
	else if(!strncmp(format+i, "EBCDIC", 6))
	{
		dataFormat=fld_ebcdic;
		i+=6;
	}
	else if(!strncmp(format+i, "EBC", 3))
	{
		dataFormat=fld_ebcdic;
		i+=3;
	}
	else if(!strncmp(format+i, "BCD", 3))
	{
		dataFormat=fld_bcd;
		i+=3;
	}
	else if(!strncmp(format+i, "HEX", 3))
	{
		dataFormat=fld_hex;
		i+=3;
	}
	else if(!strncmp(format+i, "ASCII", 5))
	{
		dataFormat=fld_ascii;
		i+=3;
	}
	else if(!strncmp(format+i, "ASC", 3))
	{
		dataFormat=fld_ascii;
		i+=3;
	}
	else
	{
		if(debug)
			printf("Error: Unrecognized data format (%s)\n", format+i);
		return false;
	}

	if(p && p!=format+i)
	{
		if(debug)
			printf("Error: Unrecognized trailing characters in format string (%s, %s)\n", format, format+i);
		return false;
	}

	if(p && dataFormat!=fld_bitstr && dataFormat!=fld_ebcdic && dataFormat!=fld_bcd && dataFormat!=fld_hex && dataFormat!=fld_ascii)
	{
		if(debug)
			printf("Error: Mandatory data specified for subfield format\n");
		return false;
	}

	//printf("Field: %s, Length type: %d, LengthLength: %d, Max Length: %d, Data format: %d, Mandatory data: %s\n", description.c_str(), lengthFormat, lengthLength, maxLength, dataFormat, data.c_str());

	return true;
}

void fldformat::erase(void)
{
	if(!parent)
	{
		printf("Error: Cannot remove field without parent\n");
		exit(1);
	}

	for(map<int,fldformat>::iterator i=parent->subfields.begin(); i!=parent->subfields.end(); ++i)
		if(&i->second==this)
		{
			parent->subfields.erase(i);
			break;
		}
}

const fldformat& fldformat::sf(int n) const
{
	std::map<int,fldformat>::const_iterator i=subfields.find(n);

	if(i==subfields.end())
	{
		if(subfields.count(-1))
			i=subfields.find(-1);
		else
			exit(0);
	}

	return i->second;
}

fldformat::iterator fldformat::begin(void)
{
	return iterator(subfields.count(-1)?&subfields[-1]:NULL, subfields.begin(), subfields.begin(), subfields.end(), 0);
}

fldformat::iterator fldformat::end(void)
{
	if(subfields.count(-1))
		return iterator(&subfields[-1], subfields.begin(), subfields.begin(), subfields.begin(), -1);
	return iterator(0, subfields.end(), subfields.begin(), subfields.end(), 0);
}

fldformat::iterator fldformat::find(int n)
{
	if(subfields.count(n))
		return iterator(subfields.count(-1)?&subfields[-1]:NULL, subfields.find(n), subfields.begin(), subfields.end(), n);
	else
		if(subfields.count(-1) && n>=0)
		{
			for(map<int,fldformat>::iterator i=subfields.begin(); i!=subfields.end(); ++i)
				if(i->first>n)
					return iterator(&subfields[-1], --i, subfields.begin(), subfields.end(), n);
			return iterator(&subfields[-1], --subfields.end(), subfields.begin(), subfields.end(), n);
		}
		else
			return end();
}

fldformat::const_iterator fldformat::begin(void) const
{
	return const_iterator(subfields.count(-1)?&(subfields.find(-1)->second):NULL, subfields.begin(), subfields.begin(), subfields.end(), 0);
}

fldformat::const_iterator fldformat::end(void) const
{
	if(subfields.count(-1))
		return const_iterator(&(subfields.find(-1)->second), subfields.begin(), subfields.begin(), subfields.begin(), -1);
	return const_iterator(0, subfields.end(), subfields.begin(), subfields.end(), 0);
}

fldformat::const_iterator fldformat::find(int n) const
{
	if(subfields.count(n))
		return const_iterator(subfields.count(-1)?&(subfields.find(-1)->second):NULL, subfields.find(n), subfields.begin(), subfields.end(), n);
	else
		if(subfields.count(-1) && n>=0)
		{
			for(map<int,fldformat>::const_iterator i=subfields.begin(); i!=subfields.end(); ++i)
				if(i->first>n)
					return const_iterator(&(subfields.find(-1)->second), --i, subfields.begin(), subfields.end(), n);
			return const_iterator(&(subfields.find(-1)->second), --subfields.end(), subfields.begin(), subfields.end(), n);
		}
		else
			return end();
}

