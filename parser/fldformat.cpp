#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

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

		for(unsigned int i=0; i<parent->fields; i++)
			if(parent->fld[i]==this)
			{
				parent->fld[i]=NULL;
				return;
			}
	}
}

//default format
void fldformat::fill_default(void)
{
	lengthFormat=FRM_UNKNOWN;
	lengthLength=0;
	lengthInclusive=0;
	maxLength=1024;
	addLength=0;
	dataFormat=FRM_SUBFIELDS;
	tagFormat=0;
	description=NULL;
	data=NULL;
	maxFields=196;
	fields=0;
	fld=NULL;
	altformat=NULL;
	parent=NULL;
}

//empty to default
void fldformat::clear(void)
{
	unsigned int i;
	fldformat *tmpfrm=parent;

	if(fields!=0)
	{      
		for(i=0; i<fields; i++)
			if(fld[i]!=NULL)
				delete fld[i];
		free(fld);
	}

	if(description!=NULL)
		free(description);

	if(data!=NULL)
		free(data);

	if(altformat!=NULL)
		delete altformat;

	fill_default();
	parent=tmpfrm; //make parent immune to clear
}

int fldformat::is_empty(void)
{
	return lengthFormat==FRM_UNKNOWN
	    && lengthLength==0
	    && lengthInclusive==0
	    && maxLength==1024
	    && addLength==0
	    && dataFormat==FRM_SUBFIELDS
	    && tagFormat==0
	    && description==NULL
	    && data==NULL
	    && maxFields==196
	    && fields==0
	    && fld==NULL
	    && altformat==NULL;
}

//forks the format. All data and subformat are also copied so that all pointers will have new values to newly copied data but non-pointers will have same values
void fldformat::copyFrom(const fldformat &from)
{
	if(this==&from)
		return;

	clear();

	lengthFormat=from.lengthFormat;
	lengthLength=from.lengthLength;
	lengthInclusive=from.lengthInclusive;
	maxLength=from.maxLength;
	addLength=from.addLength;
	dataFormat=from.dataFormat;
	tagFormat=from.tagFormat;
	if(from.description)
	{
		description=(char *)malloc((strlen(from.description)+1)*sizeof(char));
		strcpy(description, from.description);
	}
	if(from.data)
	{
		data=(char *)malloc((strlen(from.data)+1)*sizeof(char));
		strcpy(data, from.data);
	}
	maxFields=from.maxFields;
	fields=from.fields;
	if(from.fld)
	{
		fld=(fldformat**)calloc(from.maxFields,sizeof(fldformat*));
		for(unsigned int i=0; i < from.fields; i++)
			if(from.fld[i])
			{
				fld[i]=new fldformat;
				fld[i]->copyFrom(*from.fld[i]);
			}
	}
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

	lengthFormat=from.lengthFormat;
	lengthLength=from.lengthLength;
	lengthInclusive=from.lengthInclusive;
	maxLength=from.maxLength;
	addLength=from.addLength;
	dataFormat=from.dataFormat;
	tagFormat=from.tagFormat;
	description=from.description;
	data=from.data;
	maxFields=from.maxFields;
	fields=from.fields;
	fld=from.fld;
	altformat=from.altformat;

	from.description=NULL;
	from.data=NULL;
	from.fld=NULL;
	from.altformat=NULL;
	from.fields=0;

	from.clear();
}

// load format from file
// returns 0 on failure, 1 on success
// if already loaded, adds new as altformat
int fldformat::load_format(const char *filename)
{
	char line[256];
	char number[sizeof(line)];
	char format[sizeof(line)];
	char descr[sizeof(line)];
	char chr;
	unsigned int i=0;
	unsigned int j=0;
	int k=0;
	fldformat *frmtmp, *frmnew;
	FILE *file;
	map<string,fldformat> orphans;

	orphans["message"];

	if((file=fopen(filename, "r"))==NULL)
	{
		if(debug)
			perror("Error: Unable to open file\n");
		return 0;
	}

	while ((chr=fgetc(file))!=EOF)
	{
		if(chr!='\n' && chr!='\r' && i<sizeof(line))
		{
			line[i++]=chr;
			continue;
		}
		
		line[i]='\0';

		i=0;

		if(line[0]=='\0')
			continue;

		j=sscanf(line, "%s %s %[][a-zA-Z0-9 .,/;:'\"\\|?!@#$%^&*(){}<>_+=-]", number, format, descr);
		if(number[0]=='#')
			continue;
		if(j==2)
			strcpy(descr, "No description");
		else if (j!=3)
		{
			if(debug)
				printf("Warning: Wrong format in line, skipping:\n%s\n", line);
			continue;
		}

		//printf("Number[%s] format[%s] descr[%s]\n", number, format, descr);

		frmtmp=new fldformat;

		if (!frmtmp->parseFormat(format, orphans))
		{
			if(debug)
				printf("Error: Unable to parse format, skipping:\n%s\n", line);
			delete frmtmp;
			continue;
		}

		frmnew=orphans["message"].get_by_number(number, orphans);
		if(frmnew)
		{
			frmtmp->description=(char*)malloc(strlen(number)+2+strlen(descr)+1);
			strcpy(frmtmp->description, number);
			strcpy(frmtmp->description+strlen(frmtmp->description), ". ");
			strcpy(frmtmp->description+strlen(frmtmp->description), descr);

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
			frmtmp->description=(char*)malloc(strlen(descr)+1);
			strcpy(frmtmp->description, descr);

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
	fclose(file);

	if(k>1)
	{
		if(!orphans["message"].description)
		{
			printf("Warning: No 'message' format, implying default\n");
			orphans["message"].description=(char*)malloc(strlen("No description")+1);
			strcpy(orphans["message"].description, "No description");
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
		return 0;
	}

	if(debug)
		printf("Info: Loaded %d fields\n", k);

	return 1;
}

inline fldformat* fldformat::get_altformat(void)
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
	unsigned int i, l, n;
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
			n=0;
		else	
			n=atoi(number);

		frmtmp=get_lastaltformat();

		if(frmtmp->fields==0)
			frmtmp->fld=(fldformat**)calloc(frmtmp->maxFields, sizeof(fldformat*));
	
		if(frmtmp->fields < n+1)
			frmtmp->fields=n+1;

		if(frmtmp->fld[n]==NULL)
		{
			frmtmp->fld[n]=new fldformat;
			frmtmp->fld[n]->parent=frmtmp;
			return frmtmp->fld[n];
		}
		else
			return frmtmp->fld[n]->get_lastaltformat();
	}

	if(i==0 || number[i]!='.') //if has non-numeric characters, then search the orphans
	{
		for(; i<l; i++)
			if(number[i]=='.')
				break;

		if(i==l || i+1==l) //no parent, last element
			return NULL;

		key=string(number,i);

		if(!orphans.count(key)) //no parent, not last element.
			return NULL;

		return orphans[key].get_lastaltformat()->get_by_number(number+i+1, orphans);
	}

	//now we know it's not the last iteration
	if(number[i-1]=='*')
		n=0;
	else
	{
		n=atoi(number);
	}

	if(fields < n + 1)
	{
		if(debug)
			printf("Warning: Parent format not loaded yet [%s][%d][%d] %s\n", number, n, fields, description);
		return NULL;
	}

	return fld[n]->get_lastaltformat()->get_by_number(number+i+1, orphans);
}

//parses format string
int fldformat::parseFormat(char *format, map<string,fldformat> &orphans)
{
	unsigned int i, j=0;
	char *p;
	fldformat *tmpfrm;

	if(!format)
	{
		if(debug)
			printf("Error: Null pointer to second arg\n");
		return 0;
	}

	if(!strcmp(format, "ISOBITMAP"))
	{
		dataFormat=FRM_ISOBITMAP;
		maxLength=192;
		return 1;
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
					printf("Error: Unable to find referenced format (%s) (no parent loaded)\n", format+1);
					return 0;
				}
			}
			else if(tmpfrm->is_empty())
			{
				printf("Error: Unable to find referenced format (%s) (no parent loaded)\n", format+1);
				delete tmpfrm;
				return 0;
			}
			else
				copyFrom(*tmpfrm);

			if(description)
			{
				free(description);
				description=NULL;
			}

			return 1;

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
		return 0;
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
			return 0;
		}

		addLength=(format[i]=='+')? atoi(format+i+1) : -atoi(format+i+1);
		i=j;
	}
	else
		addLength=0;

	if(!maxLength)
		return 1; //allow zero length fields

	p=strstr(format+i, "=");
	if(p)
	{
		if(p[1])
		{
			data=(char *)malloc((strlen(p+1)+1)*sizeof(char));
			strcpy(data, p+1);
		}
		else
		{
			data=(char *)malloc(2*sizeof(char));
			strcpy(data, " "); //replacing empty string with a space character. It is a feature, not bug.
		}
		*p='\0';
	}

	if(!strcmp(format+i, "SF"))
	{
		dataFormat=FRM_SUBFIELDS;
		maxFields=196;
	}
	else if(!strncmp(format+i, "TLV1", 4))
	{
		dataFormat=FRM_TLV1;
		maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV2",4))
	{
		dataFormat=FRM_TLV2;
		maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV3",4))
	{
		dataFormat=FRM_TLV3;
		maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV4",4))
	{
		dataFormat=FRM_TLV4;
		maxFields=16;
		i+=4;
	}
	else if(!strcmp(format+i, "TLVEMV"))
	{
		dataFormat=FRM_TLVEMV;
		tagFormat=FRM_HEX;
		maxFields=16;
	}
	else if(!strncmp(format+i, "TLVDS", 5))
	{
		dataFormat=FRM_TLVDS;
		maxFields=100;
		i+=5;
	}
	else if(!strcmp(format+i, "BCDSF"))
	{
		dataFormat=FRM_BCDSF;
		maxFields=16;
	}
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
		return 0;
	}

	if(dataFormat==FRM_TLV1 || dataFormat==FRM_TLV2 || dataFormat==FRM_TLV3 || dataFormat==FRM_TLV4 || dataFormat==FRM_TLVDS)
	{
		if(!strcmp(format+i, "EBCDIC") || !strcmp(format+i, "EBC"))
			tagFormat=FRM_EBCDIC;
		else if(!strcmp(format+i, "BCD"))
			tagFormat=FRM_BCD;
		else if(!strcmp(format+i, "HEX"))
			tagFormat=FRM_HEX;
		else if(!strcmp(format+i, "ASCII") || !strcmp(format+i, "ASC") || format[i]=='\0')
			tagFormat=FRM_ASCII;
		else
		{
			if(debug)
				printf("Error: Unrecognized TLV tag format (%s)\n", format+i+4);
			if(p)
				*p='=';
			return 0;
		}
	}

	if(p)
		*p='=';

	if(p && dataFormat!=FRM_BITSTR && dataFormat!=FRM_EBCDIC && dataFormat!=FRM_BCD && dataFormat!=FRM_BIN && dataFormat!=FRM_HEX && dataFormat!=FRM_ASCII)
	{
		if(debug)
			printf("Error: Mandatory data specified for subfield format\n");
		return 0;
	}

	//printf("Field: %s, Length type: %d, LengthLength: %d, Max Length: %d, Data format: %d, Mandatory data: %s\n", description, lengthFormat, lengthLength, maxLength, dataFormat, data);

	return 1;
}

const char *fldformat::get_description(void)
{
	static const char dummy[]="";
	
	if(!description)
		return dummy;
	else
		return description;
}

