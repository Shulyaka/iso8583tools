#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

void freeField(field*);
void print_message(field*, fldformat*);


void print_message(field *fld, fldformat *frm)
{
	unsigned int i,j;

	if(!fld)
	{
		printf("Error: First arg is null\n");
		return;
	}

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
					print_message(fld->fld[i], frm->fld[i]);
			break;
		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:
			for(i=0; i<fld->fields; i++)
				if(fld->fld[i])
					print_message(fld->fld[i], frm->fld[0]);
			break;
	}
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

