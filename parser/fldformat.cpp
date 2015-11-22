#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include "parser.h"

using namespace std;

//constructor
fldformat::fldformat(void)
{
	fill_default();
}

//copy constructor
fldformat::fldformat(const fldformat &from)
{
	fill_default();
	copyFrom(from);
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
void fldformat::fill_default(void)
{
	description.clear();
	lengthFormat=FRM_UNKNOWN;
	lengthLength=0;
	lengthInclusive=0;
	maxLength=1024;
	addLength=0;
	dataFormat=FRM_SUBFIELDS;
	tagFormat=0;
	data.clear();
	subfields.clear();
	altformat=NULL;
	parent=NULL;
}

//empty to default
void fldformat::clear(void)
{
	unsigned int i;
	fldformat *tmpfrm=parent;

	if(altformat!=NULL)
		delete altformat;

	fill_default();
	parent=tmpfrm; //make parent immune to clear
}

bool fldformat::is_empty(void) const
{
	return description.empty()
	    && lengthFormat==FRM_UNKNOWN
	    && lengthLength==0
	    && lengthInclusive==0
	    && maxLength==1024
	    && addLength==0
	    && dataFormat==FRM_SUBFIELDS
	    && tagFormat==0
	    && data==""
	    && subfields.empty()
	    && altformat==NULL;
}

//forks the format. All data and subformat are also copied so that all pointers will have new values to newly copied data but non-pointers will have same values
void fldformat::copyFrom(const fldformat &from)
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
	data=from.data;

	subfields=from.subfields;
	for(map<int,fldformat>::iterator i=subfields.begin(); i!=subfields.end(); ++i)
		i->second.parent=this;

	if(from.altformat)
	{
		altformat=new fldformat;
		altformat->copyFrom(*from.altformat);
	}
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
	data=from.data;
	subfields=from.subfields;
	for(map<int,fldformat>::iterator i=subfields.begin(); i!=subfields.end(); ++i)
		i->second.parent=this;

	altformat=from.altformat;

	from.altformat=NULL;

	from.clear();
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
	char chr;
	unsigned int j=0;
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

		frmnew=orphans["message"].get_by_number(number, orphans);
		if(frmnew)
		{
			if(!frmnew->is_empty())
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
			if(orphans.count(number) && !orphans[number].is_empty())
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

		if(!is_empty())
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

inline fldformat* fldformat::get_altformat(void) const
{
	return altformat;
}

inline fldformat* fldformat::get_lastaltformat(void)
{
	fldformat *last;
	for(last=this; last->altformat!=NULL; )
		last=last->altformat;
	return last;
}

// parses a string and returns a pointer to format by its number.
// If not found but its parent exists, a new entry would be created.
// If the parent does not yet exists, NULL is returned.
// During parent search, always choses latest altformat wherever possible
fldformat* fldformat::get_by_number(const char *number, map<string,fldformat> &orphans)
{
	unsigned int i, l;
	int n;
	string key;
	fldformat *frmtmp;

	if(altformat)
		return get_lastaltformat()->get_by_number(number, orphans);

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

		return orphans[key].get_lastaltformat()->get_by_number(number+i+1, orphans);
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

	return subfields[n].get_lastaltformat()->get_by_number(number+i+1, orphans);
}

//parses format string
bool fldformat::parseFormat(char *format, map<string,fldformat> &orphans)
{
	unsigned int i, j=0;
	char *p;
	fldformat *tmpfrm;

	if(!format)
	{
		if(debug)
			printf("Error: Null pointer to second arg\n");
		return false;
	}

	if(!strcmp(format, "ISOBITMAP"))
	{
		dataFormat=FRM_ISOBITMAP;
		maxLength=192;
		return true;
	}

	switch (format[0])
	{
		case 'F':
			lengthFormat=FRM_FIXED;

			lengthLength=0;

			j=1;

			break;

		case 'L':
			lengthFormat=FRM_ASCII;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='L')
					break;

			lengthLength=j;

			break;

		case 'B':
			lengthFormat=FRM_BIN;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='B')
					break;

			lengthLength=j;

			break;

		case 'C':
			lengthFormat=FRM_BCD;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='C')
					break;

			lengthLength=j;

			break;

		case 'E':
			lengthFormat=FRM_EBCDIC;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='E')
					break;

			lengthLength=j;

			break;

		case 'U':
			lengthFormat=FRM_UNKNOWN;
			lengthLength=0;
			j=1;
			break;

		case 'M':
			lengthFormat=FRM_EMVL;
			lengthLength=1;
			j=1;
			break;

		case 'R':
			tmpfrm=orphans["message"].get_by_number(format+1, orphans);
			if(!tmpfrm)
			{
				if(orphans.count(format+1))
				{
					copyFrom(*orphans[format+1].get_lastaltformat());
				}
				else
				{
					if(debug)
						printf("Error: Unable to find referenced format (%s) (no parent loaded)\n", format+1);
					return false;
				}
			}
			else if(tmpfrm->is_empty())
			{
				if(debug)
					printf("Error: Unable to find referenced format (%s) (parent loaded)\n", format+1);
				tmpfrm->erase();
				return false;
			}
			else
				copyFrom(*tmpfrm);

			description.clear();

			return true;

			break;

		default:
			if(debug)
				printf("Error: Unrecognized length format (%s)\n", format);
	}

	if(format[j]=='I')
	{
		lengthInclusive=1;
		j++;
	}
	else
		lengthInclusive=0;

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
		*p='\0';
	}

	if(!strcmp(format+i, "SF"))
		dataFormat=FRM_SUBFIELDS;
	else if(!strncmp(format+i, "TLV1", 4))
	{
		dataFormat=FRM_TLV1;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV2",4))
	{
		dataFormat=FRM_TLV2;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV3",4))
	{
		dataFormat=FRM_TLV3;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV4",4))
	{
		dataFormat=FRM_TLV4;
		i+=4;
	}
	else if(!strcmp(format+i, "TLVEMV"))
	{
		dataFormat=FRM_TLVEMV;
		tagFormat=FRM_HEX;
	}
	else if(!strcmp(format+i, "BCDSF"))
		dataFormat=FRM_BCDSF;
	else if(!strcmp(format+i, "BITMAP"))
		dataFormat=FRM_BITMAP;
	else if(!strcmp(format+i, "BITSTR"))
		dataFormat=FRM_BITSTR;
	else if(!strcmp(format+i, "EBCDIC") || !strcmp(format+i, "EBC"))
		dataFormat=FRM_EBCDIC;
	else if(!strcmp(format+i, "BCD"))
		dataFormat=FRM_BCD;
	else if(!strcmp(format+i, "BIN"))
		dataFormat=FRM_BIN;
	else if(!strcmp(format+i, "HEX"))
		dataFormat=FRM_HEX;
	else if(!strcmp(format+i, "ASCII") || !strcmp(format+i, "ASC"))
		dataFormat=FRM_ASCII;
	else
	{
		if(debug)
			printf("Error: Unrecognized data format (%s)\n", format+i);
		if(p)
			*p='=';
		return false;
	}

	if(dataFormat==FRM_TLV1 || dataFormat==FRM_TLV2 || dataFormat==FRM_TLV3 || dataFormat==FRM_TLV4)
	{
		if(!strcmp(format+i, "EBCDIC") || !strcmp(format+i, "EBC"))
			tagFormat=FRM_EBCDIC;
		else if(!strcmp(format+i, "BCD"))
			tagFormat=FRM_BCD;
/*		else if(!strcmp(format+i, "HEX"))
			tagFormat=FRM_HEX;*/
		else if(!strcmp(format+i, "BIN"))
			tagFormat=FRM_BIN;
		else if(!strcmp(format+i, "ASCII") || !strcmp(format+i, "ASC") || format[i]=='\0')
			tagFormat=FRM_ASCII;
		else
		{
			if(debug)
				printf("Error: Unrecognized TLV tag format (%s)\n", format+i+4);
			if(p)
				*p='=';
			return false;
		}
	}

	if(p)
		*p='=';

	if(p && dataFormat!=FRM_BITSTR && dataFormat!=FRM_EBCDIC && dataFormat!=FRM_BCD && dataFormat!=FRM_BIN && dataFormat!=FRM_HEX && dataFormat!=FRM_ASCII)
	{
		if(debug)
			printf("Error: Mandatory data specified for subfield format\n");
		return false;
	}

	//printf("Field: %s, Length type: %d, LengthLength: %d, Max Length: %d, Data format: %d, Mandatory data: %s\n", description.c_str(), lengthFormat, lengthLength, maxLength, dataFormat, data.c_str());

	return true;
}

const string& fldformat::get_description(void) const
{
	return description;
}

//returns reference to subformat. If it does not exists but wildcard subformat exists, it will be returned instead. If none of them exist, a new subformat will be added.
fldformat& fldformat::sf(int n)
{
	if(!subfields.count(n) && subfields.count(-1))
		return subfields[-1];
	return subfields[n];
}

bool fldformat::sfexist(int n) const
{
	return subfields.count(n) || subfields.count(-1);
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
