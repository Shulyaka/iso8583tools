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
	static char def[1023];
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

	for(i=0; i<10; i++)
	{
		if(!fld || !fld->frm)
			return def;

		if(n[i]==-1)
		{
			if(fld->data)
				return fld->data;
			else
			{
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
		}

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
			fld->fld[n[i]]->frm=fld->frm->fld[n[i]];
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

//a segfault-safe accessor function. Return a pointer to the fields contents. If the field does not exist, returns a valid pointer to an empty string. The field structure is not modified.
const char* get_field(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	static const char def[]="";
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

	for(i=0; i<10; i++)
	{
		if(!fld)
			return def;

		if(n[i]==-1)
		{
			if(fld->data)
				return fld->data;
			else
				return def;
		}

		if(!fld->fld || !fld->fld[n[i]])
			return def;

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->data)
		return def;

	return fld->data;
}

void remove_field(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

	for(i=0; i<9; i++)
	{
		if(!fld || !fld->fld || !fld->fld[n[i]])
			return;

		if(n[i+1]==-1)
		{
			freeField(fld->fld[n[i]]);
			fld->fld[n[i]]=NULL;
			return;
		}

		fld=fld->fld[n[i]];
	}

	if(!fld || !fld->fld || !fld->fld[n[9]])
		return;

	freeField(fld->fld[n[9]]);
	fld->fld[n[9]]=NULL;
	return;
}

int has_field(field *fld, int n0, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, int n9)
{
	int n[]={n0, n1, n2, n3, n4, n5, n6, n7, n8, n9};
	int i;

	for(i=0; i<10; i++)
	{
		if(!fld)
			return 0;

		if(n[i]==-1)
				return 1;

		if(!fld->fld || !fld->fld[n[i]])
			return 0;

		fld=fld->fld[n[i]];
	}

	if(!fld)
		return 0;

	return 1;
}

/*
void remove_field(field *fld, unsigned int n)
{
	if(!fld || !fld->fld || !fld->fld[n])
		return;

	freeField(fld->fld[n]);

	fld->fld[n]=NULL;
}
*/
