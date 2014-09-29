#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

unsigned int parse_field(char*, unsigned int, field*);
unsigned int parse_field_alt(char*, unsigned int, field*);
void parse_ebcdic(char*, char*, unsigned int);
int parse_hex(char*, char*, unsigned int);
int parse_bcdr(char*, char*, unsigned int);
int parse_bcdl(char*, char*, unsigned int);

field *parse_message(char *msgbuf, unsigned int length, fldformat *frm)
{
	field *message;

	if(!msgbuf)
	{
		if(debug)
			printf("Error: No buffer\n");
		return NULL;
	}

	if(!frm)
	{
		if(debug)
			printf("Error: No frm\n");
		return NULL;
	}
	
	message=(field *)calloc(1, sizeof(field));

	message->frm=frm;

	if(!parse_field(msgbuf, length, message))
	{
		if(debug)
			printf("Error: Can't parse\n");
		freeField(message);
		return NULL;
	}
	
	return message;
}

unsigned int parse_field_length(char *buf, unsigned int maxlength, fldformat *frm)
{
	unsigned int lenlen=0;
	unsigned int length=0;
	char lengthbuf[7];
	unsigned char tmpc;
	unsigned int i;

	if(!buf)
	{
		if(debug)
			printf("Error: No buf\n");
		return 0;
	}

	if(!frm)
	{
		if(debug)
			printf("Error: No frm\n");
		return 0;
	}

	lenlen=frm->lengthLength;

	if(lenlen > maxlength)
	{
		if(debug)
			printf("Error: Buffer less than length size\n");
		return 0;
	}

	switch(frm->lengthFormat)
	{
		case FRM_BIN:
			if(lenlen>4)
				for(i=0; i < lenlen-4; i++)
					if(buf[i]!='\0')
					{
						if(debug)
							printf("Error: Length is too big\n");
						return 0;
					}
			for(i=0; i<(lenlen>4?4:lenlen); i++)
				((char *)(&length))[i]=buf[(lenlen>4?4:lenlen)-i-1];
			break;

		case FRM_EMVL:
			if((unsigned char)buf[0]>127)
			{
				lenlen=2;
				if(lenlen > maxlength)
				{
					if(debug)
						printf("Error: Buffer less than length size\n");
					return 0;
				}
			}
			else
				lenlen=1;
				
			((char *)(&length))[0]=buf[lenlen-1];	
			break;
		
		case FRM_BCD:
			if(lenlen>3)
			{
				for(i=0; i < lenlen-3; i++)
					if(buf[i]!='\0')
					{
						if(debug)
							printf("Error: Length is too big\n");
						return 0;
					}

				parse_hex(buf + lenlen - 3, lengthbuf, 6);
			}
			else
				parse_hex(buf, lengthbuf, lenlen*2);

			length=atoi(lengthbuf);
			break;

		case FRM_ASCII:
			if(lenlen>6)
				for(i=0; i < lenlen-6; i++)
					if(buf[i]!='0')
					{
						if(debug)
							printf("Error: Length is too big\n");
						return 0;
					}

			tmpc=buf[lenlen];
			buf[lenlen]='\0';
			length=atoi(buf);
			buf[lenlen]=tmpc;
			break;
		
		case FRM_EBCDIC:
			if(lenlen>6)
				for(i=0; i < lenlen-6; i++)
					if(buf[i]!=(char)0xF0)
					{
						if(debug)
							printf("Error: Length is too big\n");
						return 0;
					}
				
			if(lenlen>6)
				parse_ebcdic(buf + lenlen - 6, lengthbuf, 6);
			else
				parse_ebcdic(buf, lengthbuf, lenlen);

			length=atoi(lengthbuf);
			break;

		case FRM_UNKNOWN:
			length=maxlength;
			break;

		case FRM_FIXED:
			length=frm->maxLength;
			break;

		default:
			if(frm->dataFormat!=FRM_ISOBITMAP)
			{
				if(debug)
					printf("Error: Unknown length format\n");
				return 0;
			}
	}

	if(frm->dataFormat==FRM_BCDSF && frm->lengthFormat!=FRM_FIXED)
		length*=2;

	return length;
}

unsigned int parse_field(char *buf, unsigned int maxlength, field *fld)
{
	fldformat *frm;
	unsigned int length;
	unsigned int i;

	if(!buf)
	{
		if(debug)
			printf("Error: No buf\n");
		return 0;
	}

	if(!fld)
	{
		if(debug)
			printf("Error: No fld\n");
		return 0;
	}

	frm=fld->frm;

	if(!frm)
	{
		if(debug)
			printf("Error: No frm\n");
		return 0;
	}

	fld->altformat=0;

	length=parse_field_alt(buf, maxlength, fld);
	if(length)
		return length;

	if(frm->altformat)
		if(debug)
			printf("Info: parse_field: Retrying with alternate format %s\n", frm->altformat->description);

	emptyField(fld);

	for(i=0; frm->altformat!=NULL; i++)
	{
		frm=frm->altformat;
		fld->frm=frm;
		fld->altformat=i;

		length=parse_field_alt(buf, maxlength, fld);
		if(length)
			return length;

		emptyField(fld);
	}

	return 0;
}

unsigned int parse_field_alt(char *buf, unsigned int maxlength, field *fld)
{
	unsigned int lenlen=0;
	unsigned int blength=0;
	char lengthbuf[7];
	unsigned int i, j;
	int bitmap_found=-1;
	unsigned int pos;
	unsigned int sflen;
	unsigned int taglength;
	fldformat *frm;
	fldformat tmpfrm;

	if(!buf)
	{
		if(debug)
			printf("Error: No buf\n");
		return 0;
	}

	if(!fld)
	{
		if(debug)
			printf("Error: No fld\n");
		return 0;
	}

	frm=fld->frm;

	if(!frm)
	{
		if(debug)
			printf("Error: No frm\n");
		return 0;
	}

	lenlen=frm->lengthLength;

	if(frm->lengthFormat==FRM_EMVL)
	{
		if((unsigned char)buf[0]>127)
			lenlen=2;
		else
			lenlen=1;
	}

	if(frm->dataFormat!=FRM_ISOBITMAP)
	{
		fld->length=parse_field_length(buf, maxlength, frm);

		if(frm->maxLength < fld->length)
		{
			if(frm->lengthFormat!=FRM_UNKNOWN)
				if(debug)
					printf("Warning: field length exceeds max, reducing\n");
			fld->length=frm->maxLength;
		}

		if((!frm->lengthInclusive && fld->length==0) || (frm->lengthInclusive && fld->length<=lenlen))
		{
			if(debug)
				printf("Error: Wrong length (%s)\n", frm->description);
			return 0;
		}

		if(frm->lengthInclusive)
			fld->length-=lenlen;

		switch(frm->dataFormat)
		{
			case FRM_BITMAP:
			case FRM_BITSTR:
				blength=(fld->length+7)/8;
				break;
			case FRM_HEX:
//			case FRM_BCDSF:
				fld->length=fld->length*2;
			case FRM_BCDSF:
			case FRM_BCD:
				blength=(fld->length+1)/2;
				break;
			default:
				blength=fld->length;
		}

		if(lenlen + blength > maxlength)
		{
			if(debug)
				printf("Error: Field '%s'(%d) is too long %d+%d>%d\n", frm->description, fld->length, lenlen, blength, maxlength);
			return 0;
		}
	}

	//Now we know the length except for ISOBITMAP
//	printf("Length is %d for %s\n", fld->length, frm->description);
	
	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
			fld->fld=(field**)calloc(frm->maxFields, sizeof(field*));
			pos=lenlen;
			for(i=0; i < frm->fields; i++)
			{
				if(pos==fld->length+lenlen)
				{
					//printf("Warning: Some subfields are missing or canceled by bitmap for '%s'\n", frm->description);
					break;
				}

				if(bitmap_found!=-1 && frm->fld[bitmap_found]->dataFormat==FRM_ISOBITMAP && fld->fld[bitmap_found]->length < i-bitmap_found)
					break;

				if(!frm->fld[i] && bitmap_found!=-1 && fld->fld[bitmap_found]->length >= i-bitmap_found && fld->fld[bitmap_found]->data[i-bitmap_found-1]=='1')
				{
					if(debug)
						printf("Error: No format for subfield %d which is present in bitmap\n", i);
					return 0;
				}	

				if(frm->fld[i] && (bitmap_found==-1 || (fld->fld[bitmap_found]->length > i-bitmap_found-1 && fld->fld[bitmap_found]->data[i-bitmap_found-1]=='1')))
				{
					if(frm->fld[i]->dataFormat==FRM_BITMAP || frm->fld[i]->dataFormat==FRM_ISOBITMAP)
						bitmap_found=i;
					
					fld->fld[i]=(field*)calloc(1, sizeof(field));
					fld->fld[i]->frm=frm->fld[i];

					if(frm->fld[i]->lengthFormat==FRM_UNKNOWN && i+1 < frm->fields && frm->fld[i+1] && frm->fld[i+1]->data && (bitmap_found==-1 || (fld->fld[bitmap_found]->length >= i-bitmap_found && fld->fld[bitmap_found]->data[i-bitmap_found]=='1')) && (j=strstr(buf+pos+1, frm->fld[i+1]->data)-buf-pos) && j < fld->length+lenlen-pos) //only probe for the next field to be fixed-data and ignoring its altformat. This could be replaced with a more proper algorithm which would however have a greateky increased computational complexity and I have not met any cases where it would be really necessary. Please let me know if you find one. And only look for the first occurence of the next field fixed data.
					{
						sflen=parse_field(buf+pos, j, fld->fld[i]);
						if(!sflen)
						{
							if(debug)
								printf("Error: unable to parse unknown-length field with following delimiter. Retrying without delimiter. Please report this error\n");
							sflen=parse_field(buf+pos, fld->length+lenlen-pos, fld->fld[i]);
						}
					}
					else
						sflen=parse_field(buf+pos, fld->length+lenlen-pos, fld->fld[i]);
					if(!sflen)
					{
						if(debug)
							printf("Error: unable to parse subfield\n");
						return 0;
					}

					pos+=sflen;
				}
			}

			fld->fields=i;

			if(pos!=fld->length+lenlen)
			{
				if(bitmap_found!=-1)
					for(i=frm->fields; i<=bitmap_found + fld->fld[bitmap_found]->length; i++)
						if(fld->fld[bitmap_found]->data[i-bitmap_found-1]=='1')
							if(debug)
								printf("Error: No format for field %i which is present in bitmap\n", i);

				if(debug)
					printf("Error: Not enough subfield formats (%d, %d)\n", pos, fld->length);
				return 0;
			}
			
			break;

		case FRM_BCDSF:
			fld->data=(char*)malloc(fld->length+1);
			if(!parse_bcdl(buf+lenlen, fld->data, fld->length))
			{
				if(debug)
					printf("Error: Not BCD field\n");
				return 0;
			}
			
			tmpfrm.lengthFormat=FRM_FIXED;
			tmpfrm.lengthLength=0;
			tmpfrm.maxLength=fld->length;
			tmpfrm.dataFormat=FRM_SUBFIELDS;
			tmpfrm.tagFormat=0;
			tmpfrm.description=frm->description;
			tmpfrm.data=frm->data;
			tmpfrm.maxFields=frm->maxFields;
			tmpfrm.fields=frm->fields;
			tmpfrm.fld=frm->fld;
			fld->frm=&tmpfrm;

			parse_field(fld->data, fld->length, fld);
			
			free(fld->data);
			fld->data=0;
			fld->frm=frm;			

			break;

		case FRM_TLV1:
		case FRM_TLV2:
		case FRM_TLV3:
		case FRM_TLV4:
		case FRM_TLVEMV:

			if(!frm->fld[0])
			{
				if(debug)
					printf("Error: No tlv subfield format\n");
				return 0;
			}

			fld->fld=(field**)calloc(frm->maxFields, sizeof(field*));
			pos=lenlen;
			taglength=frm->dataFormat-FRM_TLV1+1;

			for(i=0; i<frm->maxFields && pos!=maxlength; i++)
			{
				if(pos==fld->length+lenlen)
					break;

				if(frm->dataFormat==FRM_TLVEMV)
				{
					if((buf[pos]&0x1F)==0x1F)
						taglength=2;
					else
						taglength=1;
				}

				if(pos+taglength>maxlength)
				{
					if(debug)
						printf("Error: Not enough length for TLV tag\n");
					return 0;
				}

				fld->fields=i+1;
				fld->fld[i]=(field*)calloc(1, sizeof(field));

				switch(frm->tagFormat)
				{
					case FRM_EBCDIC:
						fld->fld[i]->tag=(char*)malloc(taglength+1);
						parse_ebcdic(buf+pos, fld->fld[i]->tag, taglength);
						break;
					case FRM_BCD:
					case FRM_HEX:
						fld->fld[i]->tag=(char*)malloc(taglength*2+1);
						parse_hex(buf+pos, fld->fld[i]->tag, taglength*2);
						break;
					case FRM_ASCII:
					default:
						fld->fld[i]->tag=(char*)malloc(taglength+1);
						memcpy(fld->fld[i]->tag, buf+pos, taglength);
						fld->fld[i]->tag[taglength]='\0';
				}
				//printf("Tag=(%d)'%s'\n", taglength, fld->fld[i]->tag);

				pos+=taglength;

				fld->fld[i]->frm=frm->fld[0];

				sflen=parse_field(buf+pos, fld->length+lenlen-pos, fld->fld[i]);

				if(!sflen)
				{
					if(debug)
						printf("Error: unable to parse TLV subfield\n");
					return 0;
				}
				pos+=sflen;
			}

			if(pos!=fld->length+lenlen)
			{
				if(debug)
					printf("Error: Too much TLV subfields (%d, %d)\n", pos, fld->length);
				return 0;
			}

			break;

		case FRM_TLVDS:

			fld->fld=(field**)calloc(frm->maxFields, sizeof(field*));
			fld->fields=0;
			pos=lenlen;

			if(frm->tagFormat==FRM_BCD || frm->tagFormat==FRM_HEX)
				taglength=1;
			else
				taglength=2;

			for(i=0; i<frm->maxFields && pos!=maxlength; i++)
			{
				if(pos==fld->length+lenlen)
					break;

				if(pos+taglength>maxlength)
				{
					if(debug)
						printf("Error: Not enough length for TLV tag\n");
					return 0;
				}

				switch(frm->tagFormat)
				{
					case FRM_EBCDIC:
						parse_ebcdic(buf+pos, lengthbuf, taglength);
						j=atoi(lengthbuf);
						break;
					case FRM_BCD:
						parse_bcdl(buf+pos, lengthbuf, taglength*2);
						j=atoi(lengthbuf);
					case FRM_HEX:
						parse_hex(buf+pos, lengthbuf, taglength*2);
						sscanf(lengthbuf, "%X", &j);
						break;
					case FRM_ASCII:
					default:
						memcpy(lengthbuf, buf+pos, taglength);
						lengthbuf[taglength]='\0';
						j=atoi(lengthbuf);
				}

				if(!frm->fld[j])
				{
					if(debug)
						printf("Error: No format for TLV tag %d.\n", j);
					return 0;
				}

				if(j+1>fld->fields)
					fld->fields=j+1;

				fld->fld[j]=(field*)calloc(1, sizeof(field));

				pos+=taglength;

				fld->fld[j]->frm=frm->fld[j];

				sflen=parse_field(buf+pos, fld->length+lenlen-pos, fld->fld[j]);

				if(!sflen)
				{
					if(debug)
						printf("Error: unable to parse TLV subfield %d\n", j);
					return 0;
				}
				pos+=sflen;
			}

			if(pos!=fld->length+lenlen)
			{
				if(debug)
					printf("Error: Too much TLV subfields (%d, %d)\n", pos, fld->length);
				return 0;
			}

			break;

		case FRM_ISOBITMAP:
			for(i=0; i==0 || buf[i*8-8]>>7; i++)
			{
				fld->data=(char*)realloc(fld->data, (i+1)*64);
				fld->length=i*64+63;
				blength=i*8+8;
				if((i+1)*8>maxlength)
				{
					if(debug)
						printf("Error: Field is too long\n");
					return 0;
				}

				fld->data[(i+1)*64-1]='\0';

				if(i!=0)
					fld->data[i*64 - 1]='0';

				for(j=1; j<64; j++)
					fld->data[i*64+j-1]=buf[i*8+j/8] & (1<<(7-j%8)) ? '1':'0';
			}

			break;

		case FRM_BITMAP:
		case FRM_BITSTR:
			fld->data=(char*)calloc(frm->maxLength+1, 1);
			for(i=0; i<fld->length;i++)
				fld->data[i]=buf[lenlen + i/8] & (1<<(7-i%8)) ? '1':'0';

			break;

		case FRM_BIN:
		case FRM_ASCII:
			fld->data=(char*)malloc(frm->maxLength+1);
			memcpy(fld->data, buf+lenlen, fld->length);
			fld->data[fld->length]='\0';
			break;

		case FRM_HEX:
			fld->data=(char*)malloc(frm->maxLength*2+1);
			parse_hex(buf+lenlen, fld->data, fld->length);
			break;

		case FRM_BCD:
			fld->data=(char*)malloc(frm->maxLength+1);
			if(!parse_bcdr(buf+lenlen, fld->data, fld->length))
				return 0;
			break;

		case FRM_EBCDIC:
			fld->data=(char*)malloc(frm->maxLength+1);
			parse_ebcdic(buf+lenlen, fld->data, blength);
			break;

		default:
			if(debug)
				printf("Error: Unknown data format\n");
			return 0;
	}

	if(frm->data && fld->data && strcmp(frm->data, fld->data))
	{
		if(debug)
			printf("Error: Format mandatory data (%s) does not match field data (%s) for %s\n", frm->data, fld->data, frm->description);
		free(fld->data);
		fld->data=NULL;
		return 0;
	}

//	if(fld->data && frm->dataFormat!=FRM_SUBFIELDS)
//		printf("%s \t[%d] [%s]\n", frm->description, fld->length, fld->data);

	return lenlen+blength;
}

void parse_ebcdic(char *from, char *to, unsigned int len)
{
	unsigned int i;
//	const unsigned char ebcdic2ascii[256]="\0\x001\x002\x003 \t \x07F   \x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013 \n\x008 \x018\x019  \x01C\x01D\x01E\x01F\x080 \x01C  \x00A\x017\x01B     \x005\x006\x007  \x016    \x004    \x014\x015 \x01A  âäàáãåçñ¢.<(+|&éêëèíîïìß!$*);¬-/ÂÄÀÁÃÅÇÑ\x0A6,%_>?øÉÊËÈÍÎÏÌ`:#@'=\"Øabcdefghi   ý  \x010jklmnopqr  Æ æ  ~stuvwxyz   Ý  ^£¥       []    {ABCDEFGHI ôöòóõ}JKLMNOPQR ûüùúÿ\\ STUVWXYZ ÔÖÒÓÕ0123456789 ÛÜÙÚŸ";
	const unsigned char ebcdic2ascii[257]=" \x001\x002\x003 \t \x07F   \x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013 \n\x008 \x018\x019  \x01C\x01D\x01E\x01F\x080 \x01C  \x00A\x017\x01B     \x005\x006\x007  \x016    \x004    \x014\x015 \x01A           .<(+|&         !$*); -/        \x0A6,%_>?         `:#@'=\" abcdefghi      \x010jklmnopqr       ~stuvwxyz      ^         []    {ABCDEFGHI      }JKLMNOPQR      \\ STUVWXYZ      0123456789      ";

	for(i=0; i<len; i++)
		to[i]=ebcdic2ascii[(unsigned char)from[i]];
	
	to[len]='\0';
}

int parse_bcdr(char *from, char *to, unsigned int len)
{
	unsigned int i;
	unsigned char t;
	unsigned int u=len/2*2==len?0:1;
	unsigned int separator_found=0;

	for(i=0; i<(len+1)/2; i++)
	{
		if(i!=0 || u==0)
		{
			t=((unsigned char)from[i]) >> 4;
			if(17<len && len<38 && !separator_found && t==0xD)     //making one exception for track2
			{
				separator_found=1;
				to[i*2-u]='^';
			}
			else if (t > 9)
			{
				if(debug)
					printf("Error: parse_bcdr: The string is not BCD (%02x, %02x, %d, %d)\n", from[i], t, separator_found, len);
				return 0;
			}
			else
				to[i*2-u]='0' + t;
		}
		else if(((unsigned char)from[i])>>4!=0)
		{
			if(debug)
				printf("Error: parse_bcdr: First 4 bits not zero\n");
			return 0;
		}

		t=((unsigned char)from[i]) & 0x0F;
		if(17<len && len<38 && !separator_found && t==0xD)     //making one exception for track2
		{
			separator_found=1;
			to[i*2-u]='^';
		}
		else if (t > 9)
		{
			if(debug)
				printf("Error: parse_bcdr: The string is not BCD\n");
			return 0;
		}
		else
			to[i*2+1-u]='0' + t;
	}
	to[len]='\0';
	return 1;
}

int parse_bcdl(char *from, char *to, unsigned int len)
{
	unsigned int i;
	unsigned char t;
	unsigned int u=len/2*2==len?0:1;
	unsigned int separator_found=0;

	for(i=0; i<(len+1)/2; i++)
	{
		t=((unsigned char)from[i]) >> 4;
		if(17<len && len<38 && !separator_found && t==0xD)     //making one exception for track2
		{
			separator_found=1;
			to[i*2]='^';
		}
		else if (t > 9)
		{
			if(debug)
				printf("Error: parse_bcdl: The string is not BCD (%02x, %02x, %d, %d)\n", from[i], t, separator_found, len);
			return 0;
		}
		else
			to[i*2]='0' + t;

		if(u==0 || i!=(len+1)/2-1)
		{
			t=((unsigned char)from[i]) & 0x0F;
			if(17<len && len<38 && !separator_found && t==0xD)     //making one exception for track2
			{
				separator_found=1;
				to[i*2]='^';
			}
			else if (t > 9)
			{
				if(debug)
					printf("Error: parse_bcdl: The string is not BCD\n");
				return 0;
			}
			else
				to[i*2+1]='0' + t;
		}
		else if((((unsigned char)from[i]) & 0x0F)!=0)
		{
			if(debug)
				printf("Error: parse_bcdl: Last 4 bits not zero\n");
			return 0;
		}
	}
	to[len]='\0';

	return 1;
}

int parse_hex(char *from, char *to, unsigned int len)
{
	unsigned int i;
	unsigned char t;
	unsigned int u=len/2*2==len?0:1;

	for(i=0; i<(len+1)/2; i++)
	{
		if(i!=0 || u==0)
		{
			t=((unsigned char)from[i]) >> 4;
			if (t > 9)
				to[i*2-u]='A' + t - 10;
			else
				to[i*2-u]='0' + t;
		}
		else if(((unsigned char)from[i])>>4!=0)
		{
			if(debug)
				printf("Warning: parse_hex: First 4 bits not zero\n");
			return 0;
		}

		t=((unsigned char)from[i]) & 0x0F;
		if (t > 9)
			to[i*2+1-u]='A' + t - 10;
		else
			to[i*2+1-u]='0' + t;
	}
	to[len]='\0';
	return 1;
}

