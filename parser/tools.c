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

field* add_field(field *fld, unsigned int n)
{
	fldformat *frm;

	if(!fld)
	{
		printf("Error: no parent field\n");
		return NULL;
	}

	frm=fld->frm;

	if(!frm || !frm->fld || !frm->fld[n])
	{
		printf("Error: add_field: No format %d\n", n);
		return NULL;
	}

	if(!fld->fld)
		fld->fld=(field**)calloc(frm->maxFields, sizeof(field*));

	if(!fld->fld[n])
	{
		fld->fld[n]=(field*)calloc(1, sizeof(field));
		fld->fld[n]->frm=frm->fld[n];
	}

	if(fld->fields <= n)
		fld->fields=n+1;

	if(!fld->fld[n]->data)
	{
		switch(frm->fld[n]->dataFormat)
		{
			case FRM_ISOBITMAP:
			case FRM_BITMAP:
			case FRM_BITSTR:
			case FRM_BIN:
			case FRM_ASCII:
			case FRM_BCD:
			case FRM_EBCDIC:
				fld->fld[n]->data=(char*)calloc(frm->fld[n]->maxLength+1, 1);
				break;

			case FRM_HEX:
				fld->fld[n]->data=(char*)calloc(frm->fld[n]->maxLength*2 + 1, 1);
				break;

		}
	}

	return fld->fld[n];
}

void remove_field(field *fld, unsigned int n)
{
	if(!fld || !fld->fld || !fld->fld[n])
		return;

	freeField(fld->fld[n]);

	fld->fld[n]=NULL;
}

