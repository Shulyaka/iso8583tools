#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

void print_message(field *fld)
{
	unsigned int i,j;

	fldformat *frm;

	if(!fld)
	{
		printf("Error: First arg is null\n");
		return;
	}

	frm=fld->frm;

	if(!frm)
	{
		printf("Error: Second arg is null\n");
		return;
	}

	printf("%s", frm->description);
	if(fld->tag)
		printf(" [%s]", fld->tag);
	if(fld->data)
		printf(" (%d): [%s]\n", fld->length, fld->data);
	else
		printf(":\n");

	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
		case FRM_BCDSF:
		case FRM_TLVDS:
			for(i=0; i<fld->fields; i++)
				if(fld->fld[i])
					print_message(fld->fld[i]);
			break;
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
			for(i=0; i<fld->fields; i++)
				if(fld->fld[i])
					print_message(fld->fld[i]);
			break;
	}
}

void freeFormat(fldformat *frm)
{
	unsigned int i; 

	if(!frm)
	{      
		printf("Warning: already freed\n");
		return;       
	}      

	if(frm->fields!=0)
	{      
		for(i=0; i<frm->fields; i++)
			if(frm->fld[i]!=NULL)
				freeFormat(frm->fld[i]);    
		free(frm->fld);
	}      

	if(frm->description!=NULL)
		free(frm->description);

	free(frm);
}

void freeField(field *fld)
{
	unsigned int i;

	if(!fld)
	{
		printf("Warning: already freed\n");
		return;
	}

	if(fld->fields!=0)
	{
		for(i=0; i<fld->fields; i++)
			if(fld->fld[i]!=NULL)
				freeField(fld->fld[i]);
		free(fld->fld);
	}

	if(fld->data!=NULL)
		free(fld->data);

	if(fld->tag!=NULL)
		free(fld->tag);

	free(fld);
}

//a segfault-save accessor function. Returns a pointer to the field contents. If the field does not exist, it would be created. If it cannot be created, a valid pointer to a dummy array is returned.
char* add_field(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static char def[255]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

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

char* add_tag(const char *tag, field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static char def[255]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

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
const char* get_field(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const char def[255]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

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

const char* get_tag(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const char def[10]={0};
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

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

void remove_field(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

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

	freeField(fld->fld[n[i]]);
	fld->fld[n[i]]=NULL;
	return;
}

//returns zero if fields does not exists or has no subfields or empty.
//otherwise, returns field length or number of subfields.
int has_field(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

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

