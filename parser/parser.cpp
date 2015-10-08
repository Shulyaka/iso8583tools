#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

void parse_ebcdic(const char*, char*, unsigned int);
int parse_hex(const char*, char*, unsigned int);
int parse_bcdr(const char*, char*, unsigned int);
int parse_bcdl(const char*, char*, unsigned int);

// return value:
// >0: successfully parsed, the value is the length
// <0: Parse failed but you could try a greater maxlength of at least the negated returned value
// =0: Parse failed and don't try again, there is no point, it would fail anyway with any greater length
int field::parse_message(char *msgbuf, unsigned int length)
{
	field message;
	int parsedlength;

	if(!msgbuf)
	{
		if(debug)
			printf("Error: No buffer\n");
		return 0;
	}

	if(!frm)
	{
		if(debug)
			printf("Error: No frm\n");
		return 0;
	}

	message.frm=frm;

	parsedlength=message.parse_field(msgbuf, length);

	if(parsedlength>0)
		moveFrom(message);
	else
		if(debug)
			printf("Error: Can't parse\n");

	return parsedlength;
}

int field::parse_field_length(const char *buf, unsigned int maxlength)
{
	unsigned int lenlen=0;
	unsigned int newlength=0;
	char lengthbuf[7];
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

	if(maxlength==0)
	{
		if(debug)
			printf("Error: Zero length\n");
		return -1;
	}

	lenlen=frm->lengthLength;

	if(lenlen > maxlength)
	{
		if(debug)
			printf("Error: Buffer less than length size\n");
		return -lenlen;
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
				((char *)(&newlength))[i]=buf[(lenlen>4?4:lenlen)-i-1];
			break;

		case FRM_EMVL:
			if((unsigned char)buf[0]>127)
			{
				lenlen=2;
				if(lenlen > maxlength)
				{
					if(debug)
						printf("Error: Buffer less than length size\n");
					return -lenlen;
				}
			}
			else
				lenlen=1;
				
			((char *)(&newlength))[0]=buf[lenlen-1];	
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

			newlength=atoi(lengthbuf);
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

			memcpy(lengthbuf, buf, lenlen);
			lengthbuf[lenlen]='\0';
			newlength=atoi(lengthbuf);
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

			newlength=atoi(lengthbuf);
			break;

		case FRM_UNKNOWN:
			newlength=maxlength<frm->maxLength?maxlength:frm->maxLength;
			break;

		case FRM_FIXED:
			newlength=frm->maxLength;
			break;

		default:
			if(frm->dataFormat!=FRM_ISOBITMAP)
			{
				if(debug)
					printf("Error: Unknown length format\n");
				return 0;
			}
	}

	newlength-=frm->addLength;

	if(frm->dataFormat==FRM_BCDSF && frm->lengthFormat!=FRM_FIXED)
		newlength*=2;

	if(frm->lengthInclusive && newlength<=lenlen)
	{
		if(debug)
			printf("Error: Wrong length (%s)\n", frm->description);
		return 0;
	}

	length=newlength;

	return newlength;
}

// return value:
// >0: successfully parsed, the value is the length
// <0: Parse failed but you could try a greater maxlength of at least the negated returned value
// =0: Parse failed and don't try again, there is no point, it would fail anyway with any greater length
int field::parse_field(const char *buf, unsigned int maxlength)
{
	fldformat *tmpfrm;
	int newlength;
	int minlength=0;
	unsigned int i;

	if(!buf)
	{
		if(debug)
			printf("Error: No buf\n");
		return 0;
	}

	if(blength && frm && frm->altformat)
	{
		tmpfrm=frm->altformat;
		clear();
		if(debug)
			printf("Field was already parsed. Retrying with alternate format \"%s\"\n", tmpfrm->description);
	}
	else
		tmpfrm=frm;

	for(i=0; tmpfrm!=NULL; tmpfrm=frm->altformat)
	{
		frm=tmpfrm;
		altformat=i++;

		newlength=parse_field_alt(buf, maxlength);
		if(newlength>0)
			return newlength;

		if(newlength!=0 && (minlength==0 || newlength>minlength))
			minlength=newlength;

		if(debug && tmpfrm->altformat)
			printf("Info: parse_field: Retrying with alternate format \"%s\"\n", tmpfrm->altformat->description);

		clear();

		frm=tmpfrm; //save last attempted format
		altformat=i-1;
	}

	if(debug && i==0)
		printf("Error: No frm\n");

	return minlength;
}

int field::parse_field_alt(const char *buf, unsigned int maxlength)
{
	unsigned int lenlen=0;
	unsigned int newblength=0;
	char lengthbuf[7];
	unsigned int i, j;
	int bitmap_start=-1;
	int bitmap_end=0;
	int parse_failed;
	int cursf;
	unsigned int pos;
	int sflen;
	int minlength=0;
	unsigned int taglength;
	fldformat tmpfrm;

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

	if(frm->lengthFormat==FRM_EMVL)
	{
		if((unsigned char)buf[0]>127)
			lenlen=2;
		else
			lenlen=1;
	}

	if(frm->dataFormat!=FRM_ISOBITMAP)
	{
		sflen=parse_field_length(buf, maxlength);

		if(sflen<=0)
			return sflen;

		if(length > frm->maxLength)
		{
			if(debug)
				printf("Warning: field length exceeds max\n");
			return 0;
//			length=frm->maxLength;
		}

		if(frm->lengthInclusive)
			length-=lenlen;

		switch(frm->dataFormat)
		{
			case FRM_BITMAP:
			case FRM_BITSTR:
				newblength=(length+7)/8;
				break;
			case FRM_HEX:
//			case FRM_BCDSF:
				length=length*2;
			case FRM_BCDSF:
			case FRM_BCD:
				newblength=(length+1)/2;
				break;
			default:
				newblength=length;
		}

		if(lenlen + newblength > maxlength)
		{
			if(debug)
				printf("Error: Field '%s'(%d) is bigger than buffer %d+%d>%d\n", frm->description, length, lenlen, newblength, maxlength);
			return -(lenlen+newblength);
		}
//		else if(lenlen + newblength < maxlength)
//		{
//			if(debug)
//				printf("Error: Field '%s'(%d) is smaller than buffer %d+%d<%d\n", frm->description, length, lenlen, newblength, maxlength);
//			return 0;
//		}
	}

	//Now we know the length except for ISOBITMAP
//	printf("Length is %d for %s\n", length, frm->description);
	
	switch(frm->dataFormat)
	{
		case FRM_SUBFIELDS:
			fld=(field**)calloc(frm->maxFields, sizeof(field*));
			parse_failed=1;
			cursf=0;
			pos=lenlen;
			minlength=0;
			while(parse_failed)
			{
				parse_failed=0;
				for(; cursf < frm->fields; cursf++)
				{
					sflen=0;
					if(pos==length+lenlen) // Some subfields are missing or canceled by bitmap
						break;

					if(bitmap_start!=-1 && fld[bitmap_start]->frm->dataFormat==FRM_ISOBITMAP && bitmap_end < cursf)
						break;

					if(!frm->fld[cursf] && bitmap_start!=-1 && bitmap_end >= cursf && fld[bitmap_start]->data[cursf-bitmap_start-1]=='1')
					{
						if(debug)
							printf("Error: No format for subfield %d which is present in bitmap\n", cursf);
						parse_failed=1;
						break;
					}

					if(frm->fld[cursf] && (bitmap_start==-1 || (bitmap_end > cursf-1 && fld[bitmap_start]->data[cursf-bitmap_start-1]=='1')))
					{
						if(fld[cursf])
						{
							sflen=fld[cursf]->blength;
							fld[cursf]->clear();
							fld[cursf]->blength=sflen;
						}
						else
							fld[cursf]=(field*)calloc(1, sizeof(field));

						fld[cursf]->frm=frm->fld[cursf];

						if(fld[cursf]->frm->lengthFormat==FRM_UNKNOWN && fld[cursf]->frm->dataFormat!=FRM_ISOBITMAP) //for unknown length, search for the smallest
						{
							sflen=-1;
							for(i=fld[cursf]->blength+1; i<length+lenlen-pos+1; i=-sflen)
							{
								if(debug)
									printf("trying pos %d length %d/%d for %s\n", pos, i, length+lenlen-pos, fld[cursf]->frm->description);
								fld[cursf]->blength=0;
								sflen=fld[cursf]->parse_field(buf+pos, i);

								if(sflen>=0)
									break;

								fld[cursf]->frm=frm->fld[cursf];

								if(-sflen<=i)
									sflen=-(i+1);
							}
						}
						else
							sflen=fld[cursf]->parse_field(buf+pos, length+lenlen-pos);

						if(sflen==0 && fld[cursf]->frm->maxLength==0)
						{
							if(debug)
								printf("Optional subfield %d (%s) skipped\n", cursf, fld[cursf]->frm->description);
							fld[cursf]=NULL;
							continue;
						}

						if(sflen<=0)
						{
							if(debug)
								printf("Error: unable to parse subfield (%d)\n", sflen);
							fld[cursf]=NULL;
							parse_failed=1;
							break;
						}

						if(fld[cursf]->frm->dataFormat==FRM_BITMAP || fld[cursf]->frm->dataFormat==FRM_ISOBITMAP)
						{
							if(debug && bitmap_start!=-1)
								printf("Warning: Only one bitmap per subfield allowed\n");
							bitmap_start=cursf;
							bitmap_end=strlen(fld[cursf]->data)+cursf;
						}

						fld[cursf]->start=pos;

						pos+=sflen;
					}
				}

				fields=cursf;

				if(!parse_failed && pos!=length+lenlen)
				{
					if(bitmap_start!=-1)
						for(i=frm->fields; i<=bitmap_end; i++)
							if(fld[bitmap_start]->data[i-bitmap_start-1]=='1')
								if(debug)
									printf("Error: No format for field %i which is present in bitmap\n", i);

					if(debug)
						printf("Error: Not enough subfield formats (%d, %d) for %s\n", pos, length, frm->description);
					parse_failed=1;
				}

				if(parse_failed)
				{
					if(sflen<0 && (minlength==0 || sflen-pos>minlength))
						minlength=sflen-pos;

					for(cursf--; cursf>=0; cursf--)
					{
						if(cursf==bitmap_start)
							bitmap_start=-1;

						if(fld[cursf] && ((fld[cursf]->frm->lengthFormat==FRM_UNKNOWN && fld[cursf]->frm->dataFormat!=FRM_ISOBITMAP && fld[cursf]->blength < length+lenlen-fld[cursf]->start) || fld[cursf]->frm->altformat))
						{
							if(debug)
								printf("Come back to sf %d of %s (%s)\n", cursf, frm->description, fld[cursf]->frm->description);
							pos=fld[cursf]->start;
							break;
						}
						else if(fld[cursf])
						{
							fld[cursf]->clear();
							fld[cursf]=0;
						}
					}

					if(cursf<0)
					{
						if(debug)
							printf("Not comming back (%s)\n", frm->description);
						break;
					}
				}
			}

			if(parse_failed)
			{
				return minlength;
			}
			
			break;

		case FRM_BCDSF:
			data=(char*)malloc(length+1);
			if(!parse_bcdl(buf+lenlen, data, length))
			{
				if(debug)
					printf("Error: Not BCD field\n");
				return 0;
			}

			tmpfrm.copyFrom(*frm);
			tmpfrm.lengthFormat=FRM_FIXED;
			tmpfrm.lengthLength=0;
			tmpfrm.maxLength=length;
			tmpfrm.addLength=0;
			tmpfrm.dataFormat=FRM_SUBFIELDS;

			change_format(&tmpfrm);

			sflen=parse_field(data, length);
			
			free(data);
			data=0;
			change_format(frm);

			if(sflen<=0)
			{
				if(debug)
					printf("Error: Unable to parse BCD subfields\n");
				return sflen;
			}

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

			fld=(field**)calloc(frm->maxFields, sizeof(field*));
			pos=lenlen;
			taglength=frm->dataFormat-FRM_TLV1+1;

			for(i=0; i<frm->maxFields && pos!=maxlength; i++)
			{
				if(pos==length+lenlen)
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
					return -(pos+taglength);
				}

				fields=i+1;
				fld[i]=(field*)calloc(1, sizeof(field));

				switch(frm->tagFormat)
				{
					case FRM_EBCDIC:
						fld[i]->tag=(char*)malloc(taglength+1);
						parse_ebcdic(buf+pos, fld[i]->tag, taglength);
						break;
					case FRM_BCD:
					case FRM_HEX:
						fld[i]->tag=(char*)malloc(taglength*2+1);
						parse_hex(buf+pos, fld[i]->tag, taglength*2);
						break;
					case FRM_ASCII:
					default:
						fld[i]->tag=(char*)malloc(taglength+1);
						memcpy(fld[i]->tag, buf+pos, taglength);
						fld[i]->tag[taglength]='\0';
				}
				//printf("Tag=(%d)'%s'\n", taglength, fld[i]->tag);

				pos+=taglength;

				fld[i]->frm=frm->fld[0];

				sflen=fld[i]->parse_field(buf+pos, length+lenlen-pos);

				if(sflen<=0)
				{
					if(debug)
						printf("Error: unable to parse TLV subfield\n");
					return sflen;
				}

				fld[i]->start=pos-taglength;
				fld[i]->blength=fld[i]->blength+taglength;

				pos+=sflen;
			}

			if(pos!=length+lenlen)
			{
				if(debug)
					printf("Error: Too much TLV subfields (%d, %d)\n", pos, length);
				return 0;
			}

			break;

		case FRM_TLVDS:

			fld=(field**)calloc(frm->maxFields, sizeof(field*));
			fields=0;
			pos=lenlen;

			if(frm->tagFormat==FRM_BCD || frm->tagFormat==FRM_HEX)
				taglength=1;
			else
				taglength=2;

			for(i=0; i<frm->maxFields && pos!=maxlength; i++)
			{
				if(pos==length+lenlen)
					break;

				if(pos+taglength>maxlength)
				{
					if(debug)
						printf("Error: Not enough length for TLV tag\n");
					return -(pos+taglength);
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

				if(j+1>fields)
					fields=j+1;

				fld[j]=(field*)calloc(1, sizeof(field));

				pos+=taglength;

				fld[j]->frm=frm->fld[j];

				sflen=fld[j]->parse_field(buf+pos, length+lenlen-pos);

				if(sflen<=0)
				{
					if(debug)
						printf("Error: unable to parse TLV subfield %d\n", j);
					return sflen;
				}

				fld[j]->start=pos-taglength;
				fld[j]->blength=fld[j]->blength+taglength;

				pos+=sflen;
			}

			if(pos!=length+lenlen)
			{
				if(debug)
					printf("Error: Too much TLV subfields (%d, %d)\n", pos, length);
				return 0;
			}

			break;

		case FRM_ISOBITMAP:
			for(i=0; i==0 || buf[i*8-8]>>7; i++)
			{
				data=(char*)realloc(data, (i+1)*64);
				length=i*64+63;
				newblength=i*8+8;
				if((i+1)*8>maxlength)
				{
					if(debug)
						printf("Error: Field is too long\n");
					return -(i+1)*8;
				}

				data[(i+1)*64-1]='\0';

				if(i!=0)
					data[i*64 - 1]='0';

				for(j=1; j<64; j++)
					data[i*64+j-1]=buf[i*8+j/8] & (1<<(7-j%8)) ? '1':'0';
			}

			break;

		case FRM_BITMAP:
		case FRM_BITSTR:
			data=(char*)calloc(frm->maxLength+1, 1);
			for(i=0; i<length;i++)
				data[i]=buf[lenlen + i/8] & (1<<(7-i%8)) ? '1':'0';

			break;

		case FRM_BIN:
		case FRM_ASCII:
			data=(char*)malloc(frm->maxLength+1);
			memcpy(data, buf+lenlen, length);
			data[length]='\0';
			break;

		case FRM_HEX:
			data=(char*)malloc(frm->maxLength*2+1);
			parse_hex(buf+lenlen, data, length);
			break;

		case FRM_BCD:
			data=(char*)malloc(frm->maxLength+1);
			if(!parse_bcdr(buf+lenlen, data, length))
				return 0;
			break;

		case FRM_EBCDIC:
			data=(char*)malloc(frm->maxLength+1);
			parse_ebcdic(buf+lenlen, data, newblength);
			break;

		default:
			if(debug)
				printf("Error: Unknown data format\n");
			return 0;
	}

	if(frm->data && data && strcmp(frm->data, data))
	{
		if(debug)
			printf("Error: Format mandatory data (%s) does not match field data (%s) for %s\n", frm->data, data, frm->description);
		free(data);
		data=NULL;
		return 0;
	}

	blength=lenlen+newblength;

	if(debug && data && frm->dataFormat!=FRM_SUBFIELDS)
		printf("%s \t[%d(%d)] [%s]\n", frm->description, length, blength, data);

	return lenlen+newblength;
}

void parse_ebcdic(const char *from, char *to, unsigned int len)
{
//	const unsigned char ebcdic2ascii[256]="\0\x001\x002\x003 \t \x07F   \x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013 \n\x008 \x018\x019  \x01C\x01D\x01E\x01F\x080 \x01C  \x00A\x017\x01B     \x005\x006\x007  \x016    \x004    \x014\x015 \x01A  âäàáãåçñ¢.<(+|&éêëèíîïìß!$*);¬-/ÂÄÀÁÃÅÇÑ\x0A6,%_>?øÉÊËÈÍÎÏÌ`:#@'=\"Øabcdefghi   ý  \x010jklmnopqr  Æ æ  ~stuvwxyz   Ý  ^£¥       []    {ABCDEFGHI ôöòóõ}JKLMNOPQR ûüùúÿ\\ STUVWXYZ ÔÖÒÓÕ0123456789 ÛÜÙÚŸ";
	const unsigned char ebcdic2ascii[257]=" \x001\x002\x003 \t \x07F   \x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013 \n\x008 \x018\x019  \x01C\x01D\x01E\x01F\x080 \x01C  \x00A\x017\x01B     \x005\x006\x007  \x016    \x004    \x014\x015 \x01A           .<(+|&         !$*); -/        \x0A6,%_>?         `:#@'=\" abcdefghi      \x010jklmnopqr       ~stuvwxyz      ^         []    {ABCDEFGHI      }JKLMNOPQR      \\ STUVWXYZ      0123456789      ";

	for(unsigned int i=0; i<len; i++)
		to[i]=ebcdic2ascii[(unsigned char)from[i]];
	
	to[len]='\0';
}

int parse_bcdr(const char *from, char *to, unsigned int len)
{
	unsigned char t;
	unsigned int u=len/2*2==len?0:1;
	unsigned int separator_found=0;

	for(unsigned int i=0; i<(len+1)/2; i++)
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

int parse_bcdl(const char *from, char *to, unsigned int len)
{
	unsigned char t;
	unsigned int u=len/2*2==len?0:1;
	unsigned int separator_found=0;

	for(unsigned int i=0; i<(len+1)/2; i++)
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

int parse_hex(const char *from, char *to, unsigned int len)
{
	unsigned char t;
	unsigned int u=len/2*2==len?0:1;

	for(unsigned int i=0; i<(len+1)/2; i++)
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

