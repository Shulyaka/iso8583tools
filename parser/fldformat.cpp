#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

//constructor
fldformat::fldformat(void)
{
	this->fill_default();
}

//copy constructor
fldformat::fldformat(const fldformat &from)
{
	this->fill_default();
	this->copyFrom(&from);
}

//destructor
fldformat::~fldformat(void)
{
	this->clear();
	if(this->parent)
	{
		if(this->parent->altformat==this)
		{
			this->parent->altformat=NULL;
			return;
		}

		for(unsigned int i=0; i<this->parent->fields; i++)
			if(this->parent->fld[i]==this)
			{
				this->parent->fld[i]=NULL;
				return;
			}
	}
}

//default format
void fldformat::fill_default(void)
{
	this->lengthFormat=FRM_UNKNOWN;
	this->lengthLength=0;
	this->lengthInclusive=0;
	this->maxLength=1024;
	this->addLength=0;
	this->dataFormat=FRM_SUBFIELDS;
	this->tagFormat=0;
	this->description=NULL;
	this->data=NULL;
	this->maxFields=196;
	this->fields=0;
	this->fld=NULL;
	this->altformat=NULL;
	this->parent=NULL;
}

//empty to default
void fldformat::clear(void)
{
	unsigned int i;
	fldformat *tmpfrm=this->parent;

	if(this->fields!=0)
	{      
		for(i=0; i<this->fields; i++)
			if(this->fld[i]!=NULL)
				delete this->fld[i];
		free(this->fld);
	}

	if(this->description!=NULL)
		free(this->description);

	if(this->data!=NULL)
		free(this->data);

	if(this->altformat!=NULL)
		delete this->altformat;

	this->fill_default();
	this->parent=tmpfrm; //make parent immune to clear
}

int fldformat::is_empty(void)
{
	return this->lengthFormat==FRM_UNKNOWN
	    && this->lengthLength==0
	    && this->lengthInclusive==0
	    && this->maxLength==1024
	    && this->addLength==0
	    && this->dataFormat==FRM_SUBFIELDS
	    && this->tagFormat==0
	    && this->description==NULL
	    && this->data==NULL
	    && this->maxFields==196
	    && this->fields==0
	    && this->fld==NULL
	    && this->altformat==NULL;
}

//forks the format. All data and subformat are also copied so that all pointers will have new values to newly copied data but non-pointers will have same values
void fldformat::copyFrom(const fldformat *from)
{
	unsigned int i;

	if(!from)
	{
		printf("Error: Field not provided\n");
		return;
	}

	if(this==from)
		return;

	this->clear();

	this->lengthFormat=from->lengthFormat;
	this->lengthLength=from->lengthLength;
	this->lengthInclusive=from->lengthInclusive;
	this->maxLength=from->maxLength;
	this->addLength=from->addLength;
	this->dataFormat=from->dataFormat;
	this->tagFormat=from->tagFormat;
	if(from->description)
	{
		this->description=(char *)malloc((strlen(from->description)+1)*sizeof(char));
		strcpy(this->description, from->description);
	}
	if(from->data)
	{
		this->data=(char *)malloc((strlen(from->data)+1)*sizeof(char));
		strcpy(this->data, from->data);
	}
	this->maxFields=from->maxFields;
	this->fields=from->fields;
	if(from->fld)
	{
		this->fld=(fldformat**)calloc(from->maxFields,sizeof(fldformat*));
		for(i=0; i < from->fields; i++)
			if(from->fld[i])
			{
				this->fld[i]=new fldformat;
				this->fld[i]->copyFrom(from->fld[i]);
			}
	}
	if(from->altformat)
	{
		this->altformat=new fldformat;
		this->altformat->copyFrom(from->altformat);
	}
}

//relink data from another format. The old format will become empty
void fldformat::moveFrom(fldformat *from)
{
	if(!from)
	{
		printf("Error: Field not provided\n");
		return;
	}

	if(this==from)
		return;

	this->clear();

	this->lengthFormat=from->lengthFormat;
	this->lengthLength=from->lengthLength;
	this->lengthInclusive=from->lengthInclusive;
	this->maxLength=from->maxLength;
	this->addLength=from->addLength;
	this->dataFormat=from->dataFormat;
	this->tagFormat=from->tagFormat;
	this->description=from->description;
	this->data=from->data;
	this->maxFields=from->maxFields;
	this->fields=from->fields;
	this->fld=from->fld;
	this->altformat=from->altformat;

	from->description=NULL;
	from->data=NULL;
	from->fld=NULL;
	from->altformat=NULL;
	from->fields=0;

	from->clear();
}

// load format from file
// returns 0 on failure, 1 on success
// if already loaded, adds new as altformat
int fldformat::load_format(char *filename)
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
				frmnew->moveFrom(frmtmp);
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
				orphans[number].moveFrom(frmtmp);
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

		if(!this->is_empty())
		{
			frmtmp=this->get_lastaltformat();
			frmtmp->altformat=new fldformat;
			frmtmp->altformat->parent=frmtmp;
			frmtmp->altformat->moveFrom(&orphans["message"]);
		}
		else
			this->moveFrom(&orphans["message"]);
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
	return this->altformat;
}

inline fldformat* fldformat::get_lastaltformat(void)
{
	fldformat *altformat;
	for(altformat=this; altformat->altformat!=NULL; )
		altformat=altformat->altformat;
	return altformat;
}

// parses a string and returns a pointer to format by its number.
// If not found but its parent exists, a new entry would be created.
// If the parent does not yet exists, NULL is returned.
// During parent search, always choses latest altformat wherever possible
fldformat* fldformat::get_by_number(const char *number, map<string,fldformat> &orphans)
{
	unsigned int i, l, n;
	string key;
	fldformat *altformat;

	if(this->altformat)
		return this->get_lastaltformat()->get_by_number(number, orphans);

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

		altformat=this->get_lastaltformat();

		if(altformat->fields==0)
			altformat->fld=(fldformat**)calloc(altformat->maxFields, sizeof(fldformat*));
	
		if(altformat->fields < n+1)
			altformat->fields=n+1;

		if(altformat->fld[n]==NULL)
		{
			altformat->fld[n]=new fldformat;
			altformat->fld[n]->parent=altformat;
			return altformat->fld[n];
		}
		else
			return altformat->fld[n]->get_lastaltformat();
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

	if(this->fields < n + 1)
	{
		if(debug)
			printf("Warning: Parent format not loaded yet [%s][%d][%d] %s\n", number, n, this->fields, this->description);
		return NULL;
	}

	return this->fld[n]->get_lastaltformat()->get_by_number(number+i+1, orphans);
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
		this->dataFormat=FRM_ISOBITMAP;
		this->maxLength=192;
		return 1;
	}

	switch (format[0])
	{
		case 'F':
			this->lengthFormat=FRM_FIXED;

			this->lengthLength=0;

			j=1;

			break;

		case 'L':
			this->lengthFormat=FRM_ASCII;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='L')
					break;

			this->lengthLength=j;

			break;

		case 'B':
			this->lengthFormat=FRM_BIN;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='B')
					break;

			this->lengthLength=j;

			break;

		case 'C':
			this->lengthFormat=FRM_BCD;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='C')
					break;

			this->lengthLength=j;

			break;

		case 'E':
			this->lengthFormat=FRM_EBCDIC;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='E')
					break;

			this->lengthLength=j;

			break;

		case 'U':
			this->lengthFormat=FRM_UNKNOWN;
			this->lengthLength=0;
			j=1;
			break;

		case 'M':
			this->lengthFormat=FRM_EMVL;
			this->lengthLength=1;
			j=1;
			break;

		case 'R':
			tmpfrm=orphans["message"].get_by_number(format+1, orphans);
			if(!tmpfrm)
			{
				if(orphans.count(format+1))
				{
					this->copyFrom(orphans[format+1].get_lastaltformat());
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
				this->copyFrom(tmpfrm);

			if(this->description)
			{
				free(this->description);
				this->description=NULL;
			}

			return 1;

			break;

		default:
			if(debug)
				printf("Error: Unrecognized length format (%s)\n", format);
	}

	if(format[j]=='I')
	{
		this->lengthInclusive=1;
		j++;
	}
	else
		this->lengthInclusive=0;

	for(i=j; i<strlen(format); i++)
		if (format[i]<'0' || format[i]>'9')
			break;
	if(i==j)
	{
		if(debug)
			printf("Error: Unrecognized max length (%s)\n", format);
		return 0;
	}

	this->maxLength=atoi(format+j);

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

		this->addLength=(format[i]=='+')? atoi(format+i+1) : -atoi(format+i+1);
		i=j;
	}
	else
		this->addLength=0;

	if(!this->maxLength)
		return 1; //allow zero length fields

	p=strstr(format+i, "=");
	if(p)
	{
		if(p[1])
		{
			this->data=(char *)malloc((strlen(p+1)+1)*sizeof(char));
			strcpy(this->data, p+1);
		}
		else
		{
			this->data=(char *)malloc(2*sizeof(char));
			strcpy(this->data, " "); //replacing empty string with a space character. It is a feature, not bug.
		}
		*p='\0';
	}

	if(!strcmp(format+i, "SF"))
	{
		this->dataFormat=FRM_SUBFIELDS;
		this->maxFields=196;
	}
	else if(!strncmp(format+i, "TLV1", 4))
	{
		this->dataFormat=FRM_TLV1;
		this->maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV2",4))
	{
		this->dataFormat=FRM_TLV2;
		this->maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV3",4))
	{
		this->dataFormat=FRM_TLV3;
		this->maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV4",4))
	{
		this->dataFormat=FRM_TLV4;
		this->maxFields=16;
		i+=4;
	}
	else if(!strcmp(format+i, "TLVEMV"))
	{
		this->dataFormat=FRM_TLVEMV;
		this->tagFormat=FRM_HEX;
		this->maxFields=16;
	}
	else if(!strncmp(format+i, "TLVDS", 5))
	{
		this->dataFormat=FRM_TLVDS;
		this->maxFields=100;
		i+=5;
	}
	else if(!strcmp(format+i, "BCDSF"))
	{
		this->dataFormat=FRM_BCDSF;
		this->maxFields=16;
	}
	else if(!strcmp(format+i, "BITMAP"))
		this->dataFormat=FRM_BITMAP;
	else if(!strcmp(format+i, "BITSTR"))
		this->dataFormat=FRM_BITSTR;
	else if(!strcmp(format+i, "EBCDIC") || !strcmp(format+i, "EBC"))
		this->dataFormat=FRM_EBCDIC;
	else if(!strcmp(format+i, "BCD"))
		this->dataFormat=FRM_BCD;
	else if(!strcmp(format+i, "BIN"))
		this->dataFormat=FRM_BIN;
	else if(!strcmp(format+i, "HEX"))
		this->dataFormat=FRM_HEX;
	else if(!strcmp(format+i, "ASCII") || !strcmp(format+i, "ASC"))
		this->dataFormat=FRM_ASCII;
	else
	{
		if(debug)
			printf("Error: Unrecognized data format (%s)\n", format+i);
		if(p)
			*p='=';
		return 0;
	}

	if(this->dataFormat==FRM_TLV1 || this->dataFormat==FRM_TLV2 || this->dataFormat==FRM_TLV3 || this->dataFormat==FRM_TLV4 || this->dataFormat==FRM_TLVDS)
	{
		if(!strcmp(format+i, "EBCDIC") || !strcmp(format+i, "EBC"))
			this->tagFormat=FRM_EBCDIC;
		else if(!strcmp(format+i, "BCD"))
			this->tagFormat=FRM_BCD;
		else if(!strcmp(format+i, "HEX"))
			this->tagFormat=FRM_HEX;
		else if(!strcmp(format+i, "ASCII") || !strcmp(format+i, "ASC") || format[i]=='\0')
			this->tagFormat=FRM_ASCII;
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

	if(p && this->dataFormat!=FRM_BITSTR && this->dataFormat!=FRM_EBCDIC && this->dataFormat!=FRM_BCD && this->dataFormat!=FRM_BIN && this->dataFormat!=FRM_HEX && this->dataFormat!=FRM_ASCII)
	{
		if(debug)
			printf("Error: Mandatory data specified for subfield format\n");
		return 0;
	}

	//printf("Field: %s, Length type: %d, LengthLength: %d, Max Length: %d, Data format: %d, Mandatory data: %s\n", this->description, this->lengthFormat, this->lengthLength, this->maxLength, this->dataFormat, this->data);

	return 1;
}

const char *fldformat::get_description(void)
{
	static const char dummy[]="";
	
	if(!this->description)
		return dummy;
	else
		return this->description;
}
