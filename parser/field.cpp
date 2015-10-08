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
			if(fld[i]!=NULL)
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
	if(from.fld)
	{
		fld=(field**)calloc(from.frm->maxFields,sizeof(field*));
		for(unsigned int i=0; i < from.fields; i++)
			if(from.fld[i])
			{
				fld[i]=new field;
				fld[i]->copyFrom(*from.fld[i]);
			}
	}
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
				if(fld[i])
					fld[i]->print_message();
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
		if(fld[i] && fld[i]->frm && fld[i]->frm->dataFormat!=FRM_ISOBITMAP && fld[i]->frm->dataFormat!=FRM_BITMAP && !fld[i]->is_empty())
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
		if(fld[i]!=NULL)
			if(frmnew->fields<=i || !frmnew->fld[i] || !fld[i]->change_format(frmnew->fld[i]))
				break;

	if(i!=fields)
	{
		if(debug)
			printf("Error: Unable to change field format (%d). Reverting.\n", i);

		for(frm=frmold; i!=0; i--)
			if(fld[i]!=NULL)
				if(frmold->fields<=i || !frmold->fld[i] || !fld[i]->change_format(frmold->fld[i]))
					if(debug)
						printf("Error: Unable to revert\n");

		if(fld[0]!=NULL)
			if(frmold->fields==0 || !frmold->fld[0] || !fld[0]->change_format(frmold->fld[0]))
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

		if(!curfld->fld)
		{
			if(curfld->frm->maxFields)
				curfld->fld=(field**)calloc(curfld->frm->maxFields, sizeof(field*));
			else
				return def;
		}

		if(n[i] >= curfld->frm->maxFields)
			return def;

		if(!curfld->fld[n[i]])
		{
			curfld->fld[n[i]]=(field*)calloc(1, sizeof(field));
			switch(curfld->frm->dataFormat)
			{
				case FRM_TLV1:
				case FRM_TLV2:
				case FRM_TLV3:
				case FRM_TLV4:
				case FRM_TLVEMV:
					curfld->fld[n[i]]->frm=curfld->frm->fld[0];
				default:
					curfld->fld[n[i]]->frm=curfld->frm->fld[n[i]];
			}
		}

		if(curfld->fields <= n[i])
			curfld->fields=n[i]+1;

		curfld=curfld->fld[n[i]];
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

int field::field_format(int altformat, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
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

		if(!curfld->fld)
		{
			if(curfld->frm->maxFields)
				curfld->fld=(field**)calloc(curfld->frm->maxFields, sizeof(field*));
			else
				return 2;
		}

		if(n[i] >= curfld->frm->maxFields)
			return 2;

		if(!curfld->fld[n[i]])
		{
			curfld->fld[n[i]]=(field*)calloc(1, sizeof(field));
			switch(curfld->frm->dataFormat)
			{
				case FRM_TLV1:
				case FRM_TLV2:
				case FRM_TLV3:
				case FRM_TLV4:
				case FRM_TLVEMV:
					curfld->fld[n[i]]->frm=curfld->frm->fld[0];
				default:
					curfld->fld[n[i]]->frm=curfld->frm->fld[n[i]];
			}
		}

		if(curfld->fields <= n[i])
			curfld->fields=n[i]+1;

		tmpfrm=curfld->frm;

		curfld=curfld->fld[n[i]];
	}

	if(!curfld || !curfld->frm)
		return 2;

	if(i==0)
	{
		if(curfld->altformat>=altformat || altformat==0) //no need or unable to go back in the list
		{
			if(altformat==0 && curfld->altformat==1)
				curfld->altformat=0;

			return 3;
		}

		if(altformat==1 && curfld->altformat==0)
		{
			curfld->altformat=1;
			return 0;
		}

		tmpfrm=curfld->frm;

		for(i=curfld->altformat==0?1:curfld->altformat; i<altformat; i++)
			if(!tmpfrm->altformat)
				return 4;
			else
				tmpfrm=tmpfrm->altformat;

		if(curfld->change_format(tmpfrm))
			curfld->altformat=altformat;
		else
			return 1;

		return 0;
	}

	switch(tmpfrm->dataFormat)
	{
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
			tmpfrm=tmpfrm->fld[0];
		default:
			tmpfrm=tmpfrm->fld[n[i-1]];
	}

	for(i=1; i<altformat; i++)
		if(!tmpfrm->altformat)
			return 4;
		else
			tmpfrm=tmpfrm->altformat;

	if(curfld->change_format(tmpfrm))
		curfld->altformat=altformat;
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

		if(!curfld->fld)
		{
			if(curfld->frm->maxFields)
				curfld->fld=(field**)calloc(curfld->frm->maxFields, sizeof(field*));
			else
				return def;
		}

		if(n[i] >= curfld->frm->maxFields)
			return def;

		if(!curfld->fld[n[i]])
		{
			curfld->fld[n[i]]=(field*)calloc(1, sizeof(field));
			switch(curfld->frm->dataFormat)
			{
				case FRM_TLV1:
				case FRM_TLV2:
				case FRM_TLV3:
				case FRM_TLV4:
				case FRM_TLVEMV:
					curfld->fld[n[i]]->frm=curfld->frm->fld[0];
				default:
					curfld->fld[n[i]]->frm=curfld->frm->fld[n[i]];
			}
		}

		if(curfld->fields <= n[i])
			curfld->fields=n[i]+1;

		curfld=curfld->fld[n[i]];
	}

	if(!curfld || !curfld->frm)
		return def;

	if(!curfld->fld)
	{
		if(curfld->frm->maxFields)
			curfld->fld=(field**)calloc(curfld->frm->maxFields, sizeof(field*));
		else
			return def;
	}

	if(curfld->fields >= curfld->frm->maxFields)
		return def;

	for(i=0; i<curfld->frm->maxFields; i++)
		if(curfld->fld[i]==NULL)
			break;

	if(i==curfld->frm->maxFields)
		return def;

	curfld->fld[i]=(field*)calloc(1, sizeof(field));
	switch(curfld->frm->dataFormat)
	{
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
			curfld->fld[i]->frm=curfld->frm->fld[0];
			break;
		default:
			curfld->fld[i]->frm=curfld->frm->fld[i];
	}
	
	if(curfld->fields <= i)
		curfld->fields=i+1;

	switch(curfld->fld[i]->frm->dataFormat)
	{
		case FRM_ISOBITMAP:
		case FRM_BITMAP:
		case FRM_BITSTR:
		case FRM_BIN:
		case FRM_ASCII:
		case FRM_BCD:
		case FRM_EBCDIC:
			curfld->fld[i]->data=(char*)calloc(curfld->fld[i]->frm->maxLength+1, 1);
			break;
		case FRM_HEX:
			curfld->fld[i]->data=(char*)calloc(curfld->fld[i]->frm->maxLength*2 + 1, 1);
			break;
		default:
			return def;
	}

	curfld->fld[i]->tag=(char*)malloc(strlen(tag)+1);

	strcpy(curfld->fld[i]->tag, tag);

	return curfld->fld[i]->data;
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

		if(!curfld || !curfld->fld)
			return def;

		curfld=curfld->fld[n[i]];
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

		if(!curfld || !curfld->fld)
			return def;

		curfld=curfld->fld[n[i]];
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

		if(!curfld || !curfld->fld)
			return;

		curfld=curfld->fld[n[i]];
	}

	if(!curfld || !curfld->fld || !curfld->fld[n[i]])
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

		if(!curfld || !curfld->fld)
			return 0;

		curfld=curfld->fld[n[i]];
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
				if(curfld->fld)
					for(i=0; i<curfld->frm->maxFields; i++)
						if(!curfld->fld[i])
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

