#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

field::field(void)
{
	this->fill_default();
}

field::~field(void)
{
	this->clear();
}

void field::fill_default(void)
{
	this->data=NULL;
	this->tag=NULL;
	this->start=0;
	this->blength=0;
	this->length=0;
	this->fields=0;
	this->frm=NULL;
	this->fld=NULL;
	this->altformat=0;
}

void field::clear(void)
{
	unsigned int i;

	if(this->fields!=0)
	{
		for(i=0; i<this->fields; i++)
			if(this->fld[i]!=NULL)
				delete this->fld[i];
		free(this->fld);
	}

	if(this->data!=NULL)
		free(this->data);

	if(this->tag!=NULL)
		free(this->tag);

	this->fill_default();
}

void field::print_message(void)
{
	unsigned int i,j;

	fldformat *frm;

	frm=this->frm;

	if(!frm)
	{
		if(debug)
			printf("Error: No format assigned\n");
		return;
	}

	printf("%s", frm->description);
	if(this->tag)
		printf(" [%s]", this->tag);
	if(this->data)
		printf(" (%d): [%s]\n", this->length, this->data);
	else
		printf(":\n");

	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_BCDSF:
		case FRM_TLVDS:
			for(i=0; i<this->fields; i++)
				if(this->fld[i])
					this->fld[i]->print_message();
			break;
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
			for(i=0; i<this->fields; i++)
				if(this->fld[i])
					this->fld[i]->print_message();
			break;
	}
}

int field::is_empty(void)
{
	int i;

	switch(this->frm->dataFormat)
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
			if(!this->data || !this->data[0])
				return 1;
			else
				return 0;
	}

	if(!this->fld)
		return 1;

	for(i=0; i<this->frm->fields; i++)
		if(this->fld[i] && this->fld[i]->frm->dataFormat!=FRM_ISOBITMAP && this->fld[i]->frm->dataFormat!=FRM_BITMAP && !this->fld[i]->is_empty())
			return 0;

	return 1;
}

int field::change_format(fldformat *frmnew)
{
	unsigned int i;
	fldformat *frmold;

	if(!frmnew)
		return 0;

	if(this->frm == frmnew)
		return 1;

	frmold=this->frm;

	this->frm=frmnew;

	for(i=0; i<this->fields; i++)
		if(this->fld[i]!=NULL)
			if(frmnew->fields<=i || !frmnew->fld[i] || !this->fld[i]->change_format(frmnew->fld[i]))
				break;

	if(i!=this->fields)
	{
		if(debug)
			printf("Error: Unable to change field format (%d). Reverting.\n", i);

		for(this->frm=frmold; i!=0; i--)
			if(this->fld[i]!=NULL)
				if(frmold->fields<=i || !frmold->fld[i] || !this->fld[i]->change_format(frmold->fld[i]))
					if(debug)
						printf("Error: Unable to revert\n");

		if(this->fld[0]!=NULL)
			if(frmold->fields==0 || !frmold->fld[0] || !this->fld[0]->change_format(frmold->fld[0]))
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
	field *fld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!fld || !fld->frm)
			return def;

		if(!fld->fld)
		{
			if(fld->frm->maxFields)
				fld->fld=(field**)calloc(fld->frm->maxFields, sizeof(field*));
			else
				return def;
		}

		if(n[i] >= fld->frm->maxFields)
			return def;

		if(!fld->fld[n[i]])
		{
			fld->fld[n[i]]=(field*)calloc(1, sizeof(field));
			switch(fld->frm->dataFormat)
			{
				case FRM_TLV1:
				case FRM_TLV2:
				case FRM_TLV3:
				case FRM_TLV4:
				case FRM_TLVEMV:
					fld->fld[n[i]]->frm=fld->frm->fld[0];
				default:
					fld->fld[n[i]]->frm=fld->frm->fld[n[i]];
			}
		}

		if(fld->fields <= n[i])
			fld->fields=n[i]+1;

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->frm)
		return def;

	if(fld->data)
		return fld->data;

	switch(fld->frm->dataFormat)
	{
		case FRM_ISOBITMAP:
		case FRM_BITMAP:
		case FRM_BITSTR:
		case FRM_BIN:
		case FRM_ASCII:
		case FRM_BCD:
		case FRM_EBCDIC:
			fld->data=(char*)calloc(fld->frm->maxLength+1, 1);
			break;
		case FRM_HEX:
			fld->data=(char*)calloc(fld->frm->maxLength*2 + 1, 1);
			break;
		default:
			return def;
	}

	return fld->data;
}

int field::field_format(int altformat, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	fldformat *tmpfrm;
	field *fld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!fld || !fld->frm)
			return 2;

		if(!fld->fld)
		{
			if(fld->frm->maxFields)
				fld->fld=(field**)calloc(fld->frm->maxFields, sizeof(field*));
			else
				return 2;
		}

		if(n[i] >= fld->frm->maxFields)
			return 2;

		if(!fld->fld[n[i]])
		{
			fld->fld[n[i]]=(field*)calloc(1, sizeof(field));
			switch(fld->frm->dataFormat)
			{
				case FRM_TLV1:
				case FRM_TLV2:
				case FRM_TLV3:
				case FRM_TLV4:
				case FRM_TLVEMV:
					fld->fld[n[i]]->frm=fld->frm->fld[0];
				default:
					fld->fld[n[i]]->frm=fld->frm->fld[n[i]];
			}
		}

		if(fld->fields <= n[i])
			fld->fields=n[i]+1;

		tmpfrm=fld->frm;

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->frm)
		return 2;

	if(i==0)
	{
		if(fld->altformat>=altformat || altformat==0) //no need or unable to go back in the list
		{
			if(altformat==0 && fld->altformat==1)
				fld->altformat=0;

			return 3;
		}

		if(altformat==1 && fld->altformat==0)
		{
			fld->altformat=1;
			return 0;
		}

		tmpfrm=fld->frm;

		for(i=fld->altformat==0?1:fld->altformat; i<altformat; i++)
			if(!tmpfrm->altformat)
				return 4;
			else
				tmpfrm=tmpfrm->altformat;

		if(fld->change_format(tmpfrm))
			fld->altformat=altformat;
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

	if(fld->change_format(tmpfrm))
		fld->altformat=altformat;
	else
		return 1;

	return 0;
} 

char* field::add_tag(const char *tag, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static char def[255]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *fld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!fld || !fld->frm)
			return def;

		if(!fld->fld)
		{
			if(fld->frm->maxFields)
				fld->fld=(field**)calloc(fld->frm->maxFields, sizeof(field*));
			else
				return def;
		}

		if(n[i] >= fld->frm->maxFields)
			return def;

		if(!fld->fld[n[i]])
		{
			fld->fld[n[i]]=(field*)calloc(1, sizeof(field));
			switch(fld->frm->dataFormat)
			{
				case FRM_TLV1:
				case FRM_TLV2:
				case FRM_TLV3:
				case FRM_TLV4:
				case FRM_TLVEMV:
					fld->fld[n[i]]->frm=fld->frm->fld[0];
				default:
					fld->fld[n[i]]->frm=fld->frm->fld[n[i]];
			}
		}

		if(fld->fields <= n[i])
			fld->fields=n[i]+1;

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->frm)
		return def;

	if(!fld->fld)
	{
		if(fld->frm->maxFields)
			fld->fld=(field**)calloc(fld->frm->maxFields, sizeof(field*));
		else
			return def;
	}

	if(fld->fields >= fld->frm->maxFields)
		return def;

	for(i=0; i<fld->frm->maxFields; i++)
		if(fld->fld[i]==NULL)
			break;

	if(i==fld->frm->maxFields)
		return def;

	fld->fld[i]=(field*)calloc(1, sizeof(field));
	switch(fld->frm->dataFormat)
	{
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
			fld->fld[i]->frm=fld->frm->fld[0];
			break;
		default:
			fld->fld[i]->frm=fld->frm->fld[i];
	}
	
	if(fld->fields <= i)
		fld->fields=i+1;

	switch(fld->fld[i]->frm->dataFormat)
	{
		case FRM_ISOBITMAP:
		case FRM_BITMAP:
		case FRM_BITSTR:
		case FRM_BIN:
		case FRM_ASCII:
		case FRM_BCD:
		case FRM_EBCDIC:
			fld->fld[i]->data=(char*)calloc(fld->fld[i]->frm->maxLength+1, 1);
			break;
		case FRM_HEX:
			fld->fld[i]->data=(char*)calloc(fld->fld[i]->frm->maxLength*2 + 1, 1);
			break;
		default:
			return def;
	}

	fld->fld[i]->tag=(char*)malloc(strlen(tag)+1);

	strcpy(fld->fld[i]->tag, tag);

	return fld->fld[i]->data;
}

//a segfault-safe accessor function. Return a pointer to the fields contents. If the field does not exist, returns a valid pointer to an empty string. The field structure is not modified.
const char* field::get_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const char def[255]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *fld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!fld || !fld->fld)
			return def;

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->data)
		return def;

	return fld->data;
}

const char* field::get_tag(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const char def[10]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *fld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!fld || !fld->fld)
			return def;

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->tag)
		return def;

	return fld->tag;
}

void field::remove_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *fld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0])-1; i++)
	{
		if(n[i+1]==-1)
			break;

		if(!fld || !fld->fld)
			return;

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->fld || !fld->fld[n[i]])
		return;

	delete fld->fld[n[i]];
	fld->fld[n[i]]=NULL;
	return;
}

//returns zero if fields does not exists or has no subfields or empty.
//otherwise, returns field length or number of subfields.
int field::has_field(int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	unsigned int i;
	field *fld=this;

	for(i=0; i<sizeof(n)/sizeof(n[0]); i++)
	{
		if(n[i]==-1)
			break;

		if(!fld || !fld->fld)
			return 0;

		fld=fld->fld[n[i]];
	}

	if(!fld)
		return 0;

	switch(fld->frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
		case FRM_BCDSF:
		case FRM_TLVDS:
			if(fld->fields)
				return fld->fields;
			else
				if(fld->fld)
					for(i=0; i<fld->frm->maxFields; i++)
						if(!fld->fld[i])
							return i+1;
				return 0;
		default:
			if(fld->length || !fld->data)
				return fld->length;
			else
				return strlen(fld->data);
	}
}

const char *field::get_description(void)
{
	static const char dummy[]="";
	
	if(!this->frm)
		return dummy;
	else
		return this->frm->get_description();
}

