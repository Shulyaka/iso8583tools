#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

field::field(void)
{
	fill_default();
}

//copy constructor
field::field(const field &from)
{
	fill_default();
	copyFrom(from);
}

field::~field(void)
{
	clear();
}

void field::fill_default(void)
{
	data=NULL;
	tag=NULL;
	start=0;
	blength=0;
	length=0;
	fields=0;
	frm=NULL;
	fld=NULL;
	altformat=0;
}

void field::clear(void)
{
	fldformat *tmpfrm=frm;

	if(fields!=0)
	{
		for(unsigned int i=0; i<fields; i++)
			if(sfexist(i))
				delete fld[i];
		free(fld);
	}

	if(data!=NULL)
		free(data);

	if(tag!=NULL)
		free(tag);

	fill_default();
	frm=tmpfrm; //make frm immune to clear()
}

//forks the field. All data and subfields are also copied so that all pointers except frm will have new values to newly copied data but non-pointers will have same values
void field::copyFrom(const field &from)
{
	if(this==&from)
		return;

	clear();

	if(from.data)
	{
		data=(char *)malloc((strlen(from.data)+1)*sizeof(char));
		strcpy(data, from.data);
	}
	if(from.tag)
	{
		tag=(char *)malloc((strlen(from.tag)+1)*sizeof(char));
		strcpy(tag, from.tag);
	}
	start=from.start;
	blength=from.blength;
	length=from.length;
	fields=from.fields;
	frm=from.frm;
	for(unsigned int i=0; i < from.fields; i++)
		if(from.sfexist(i))
			sf(i).copyFrom(*from.fld[i]);
	altformat=from.altformat;
}

//relink data from another format. The old format will become empty
void field::moveFrom(field &from)
{
	if(this==&from)
		return;

	clear();

	data=from.data;
	tag=from.tag;
	start=from.start;
	blength=from.blength;
	length=from.length;
	fields=from.fields;
	frm=from.frm;
	fld=from.fld;
	altformat=from.altformat;

	from.data=NULL;
	from.tag=NULL;
	from.fld=NULL;
	from.fields=0;

	from.clear();
}

void field::print_message(void)
{
	if(!frm)
	{
		if(debug)
			printf("Error: No format assigned\n");
		return;
	}

	printf("%s", frm->description);
	if(tag)
		printf(" [%s]", tag);
	if(data)
		printf(" (%d): [%s]\n", length, data);
	else
		printf(":\n");

	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_BCDSF:
		case FRM_TLVDS:
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
			for(unsigned int i=0; i<fields; i++)
				if(sfexist(i))
					sf(i).print_message();
			break;
	}
}

int field::is_empty(void)
{
	if(!frm)
		return 1;

	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_BCDSF:
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
		case FRM_TLVDS:
			break;
		case FRM_ISOBITMAP:
		case FRM_BITMAP:
			return 0;
		default:
			if(!data || !data[0])
				return 1;
			else
				return 0;
	}

	if(!fields)
		return 1;

	for(unsigned int i=0; i<fields; i++)
		if(sfexist(i) && sf(i).frm && sf(i).frm->dataFormat!=FRM_ISOBITMAP && sf(i).frm->dataFormat!=FRM_BITMAP && !sf(i).is_empty())
			return 0;

	return 1;
}

int field::change_format(fldformat *frmnew)
{
	unsigned int i;
	fldformat *frmold;

	if(!frmnew)
		return 0;

	if(frm == frmnew)
		return 1;

	frmold=frm;

	frm=frmnew;

	for(i=0; i<fields; i++)
		if(sfexist(i))
			if(frmnew->fields<=i || !frmnew->sfexist(i) || !sf(i).change_format(&frmnew->sf(i)))
				break;

	if(i!=fields)
	{
		if(debug)
			printf("Error: Unable to change field format (%d). Reverting.\n", i);

		for(frm=frmold; i!=0; i--)
			if(sfexist(i))
				if(frmold->fields<=i || !frmold->sfexist(i) || !sf(i).change_format(&frmold->sf(i)))
					if(debug)
						printf("Error: Unable to revert\n");

		if(sfexist(0))
			if(frmold->fields==0 || !frmold->sfexist(0) || !sf(0).change_format(&frmold->sf(0)))
				if(debug)
					printf("Error: Unable to revert\n");

		return 0;
	}

	return 1;
}

//a segfault-safe accessor function. Returns a pointer to the field contents. If the field does not exist, it would be created. If it cannot be created, a valid pointer to a dummy array is returned.
char* field::add_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static char def[255]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!curfld || !curfld->frm)
			return def;

		if(n[i] >= curfld->frm->maxFields)
			return def;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld || !curfld->frm)
		return def;

	if(curfld->data)
		return curfld->data;

	switch(curfld->frm->dataFormat)
	{
		case FRM_ISOBITMAP:
		case FRM_BITMAP:
		case FRM_BITSTR:
		case FRM_BIN:
		case FRM_ASCII:
		case FRM_BCD:
		case FRM_EBCDIC:
			curfld->data=(char*)calloc(curfld->frm->maxLength+1, 1);
			break;
		case FRM_HEX:
			curfld->data=(char*)calloc(curfld->frm->maxLength*2 + 1, 1);
			break;
		default:
			return def;
	}

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

		if(n[i] >= curfld->frm->maxFields)
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

char* field::add_tag(const char *tag, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static char def[255]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!curfld || !curfld->frm)
			return def;

		if(n[i] >= curfld->frm->maxFields)
			return def;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld || !curfld->frm)
		return def;

	if(curfld->fields >= curfld->frm->maxFields)
		return def;

	if(i==curfld->frm->maxFields)
		return def;

	curfld=&curfld->sf(i);

	switch(curfld->frm->dataFormat)
	{
		case FRM_ISOBITMAP:
		case FRM_BITMAP:
		case FRM_BITSTR:
		case FRM_BIN:
		case FRM_ASCII:
		case FRM_BCD:
		case FRM_EBCDIC:
			curfld->data=(char*)calloc(curfld->frm->maxLength+1, 1);
			break;
		case FRM_HEX:
			curfld->data=(char*)calloc(curfld->frm->maxLength*2 + 1, 1);
			break;
		default:
			return def;
	}

	curfld->tag=(char*)malloc(strlen(tag)+1);

	strcpy(curfld->tag, tag);

	return curfld->data;
}

//a segfault-safe accessor function. Return a pointer to the fields contents. If the field does not exist, returns a valid pointer to an empty string. The field structure is not modified.
const char* field::get_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const char def[255]={0};
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

	if(!curfld || !curfld->data)
		return def;

	return curfld->data;
}

const char* field::get_tag(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const char def[10]={0};
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

	if(!curfld || !curfld->tag)
		return def;

	return curfld->tag;
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

	delete curfld->fld[n[i]];
	curfld->fld[n[i]]=NULL;
	return;
}

//returns zero if fields does not exists or has no subfields or empty.
//otherwise, returns field length or number of subfields.
int field::has_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *curfld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!curfld || !curfld->sfexist(n[i]))
			return 0;

		curfld=&curfld->sf(n[i]);
	}

	if(!curfld)
		return 0;

	switch(curfld->frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
		case FRM_BCDSF:
		case FRM_TLVDS:
			if(curfld->fields)
				return curfld->fields;
			else
				for(i=0; i<curfld->frm->maxFields; i++)
					if(!curfld->sfexist(i))
						return i+1;
				return 0;
		default:
			if(curfld->length || !curfld->data)
				return curfld->length;
			else
				return strlen(curfld->data);
	}
}

const char *field::get_description(void)
{
	static const char dummy[]="";
	
	if(!frm)
		return dummy;
	else
		return frm->get_description();
}

//returns reference to subformat. If it does not exists, it will be added.
field& field::sf(int n)
{
	if(n < 0 || !frm || n > frm->maxFields)
	{
		printf("Error: Wrong subfield number: %d/%d\n", n, frm->maxFields);
		exit(1);
	}

	if(fields==0)
		fld=(field**)calloc(frm->maxFields, sizeof(field*));
	
	if(fields < n+1)
		fields=n+1;

	if(fld[n]==NULL)
	{
		if(n > frm->fields || !frm->sfexist(n))
		{
			printf("Error: Wrong format for subfield number: %d/%d\n", n, frm->maxFields);
			exit(1);
		}

		fld[n]=new field;
		fld[n]->frm=&frm->sf(n);
	}

	return *fld[n];
}

bool field::sfexist(int n) const
{
	if(n < 0 || !frm || n > frm->maxFields)
		return false;

	if(fields==0)
		return false;

	if(fld[n]==NULL)
		return false;

	return true;
}
