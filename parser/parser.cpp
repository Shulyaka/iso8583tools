#include <cstdio>
#include <cstdlib>
#include <vector>
#include "parser.h"

using namespace std;

unsigned int parse_ebcdic(const std::string::const_iterator&, std::string&, unsigned int);
unsigned int parse_hex(const std::string::const_iterator&, std::string&, unsigned int);
unsigned int parse_bcdr(const std::string::const_iterator&, std::string&, unsigned int);
unsigned int parse_bcdl(const std::string::const_iterator&, std::string&, unsigned int);

// return value:
// >0: successfully parsed, the value is the length
// <0: Parse failed but you could try a greater maxlength of at least the negated returned value
// =0: Parse failed and don't try again, there is no point, it would fail anyway with any greater length
int field::parse_message(const string &msgbuf)
{
	field message(firstfrm);
	int parsedlength;

	if(msgbuf.empty())
	{
		if(debug)
			printf("Error: Empty input\n");
		return 0;
	}

	parsedlength=message.parse_field(msgbuf.begin(), msgbuf.end());

	if(parsedlength>0)
		moveFrom(message);
	else
		if(debug)
			printf("Error: Can't parse\n");

	return parsedlength;
}

int field::parse_field_length(const std::string::const_iterator &buf, const std::string::const_iterator &bufend)
{
	unsigned int lenlen=0;
	unsigned int newlength=0;
	string lengthbuf;

	lenlen=frm->lengthLength;

	if(lenlen > bufend-buf)
	{
		if(debug)
			printf("Error: Buffer less than length size\n");
		return -lenlen;
	}

	switch(frm->lengthFormat)
	{
		case fldformat::fll_bin:
			if(lenlen>4)
				for(unsigned int i=0; i < lenlen-4; i++)
					if(buf[i]!='\0')
					{
						if(debug)
							printf("Error: Length is too big\n");
						return 0;
					}
			for(unsigned int i=0; i<(lenlen>4?4:lenlen); i++)
				((char *)(&newlength))[i]=buf[(lenlen>4?4:lenlen)-i-1];
			break;

		case fldformat::fll_ber:
			if((unsigned char)buf[0]>127)
			{
				lenlen=2;
				if(lenlen > bufend-buf)
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

		case fldformat::fll_bcd:
			if(lenlen>3)
			{
				for(unsigned int i=0; i < lenlen-3; i++)
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

			newlength=atoi(lengthbuf.c_str());
			break;

		case fldformat::fll_ascii:
			if(lenlen>6)
				for(unsigned int i=0; i < lenlen-6; i++)
					if(buf[i]!='0')
					{
						if(debug)
							printf("Error: Length is too big\n");
						return 0;
					}

			lengthbuf.assign(buf, buf+lenlen);
			newlength=atoi(lengthbuf.c_str());
			break;

		case fldformat::fll_ebcdic:
			if(lenlen>6)
				for(unsigned int i=0; i < lenlen-6; i++)
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

			newlength=atoi(lengthbuf.c_str());
			break;

		case fldformat::fll_unknown:
			newlength=bufend-buf < frm->maxLength ? bufend-buf : frm->maxLength;
			break;

		case fldformat::fll_fixed:
			newlength=frm->maxLength;
			break;

		default:
			if(frm->dataFormat!=fldformat::fld_isobitmap)
			{
				if(debug)
					printf("Error: Unknown length format\n");
				return 0;
			}
	}

	newlength-=frm->addLength;

	if(frm->dataFormat==fldformat::fld_bcdsf && frm->lengthFormat!=fldformat::fll_fixed)
		newlength*=2;

	if(frm->lengthInclusive && newlength<=lenlen)
	{
		if(debug)
			printf("Error: Wrong length (%s)\n", frm->get_description().c_str());
		return 0;
	}

	length=newlength;

	return newlength;
}

// return value:
// >0: successfully parsed, the value is the length
// <0: Parse failed but you could try a greater maxlength of at least the negated returned value
// =0: Parse failed and don't try again, there is no point, it would fail anyway with any greater length
int field::parse_field(const std::string::const_iterator &buf, const std::string::const_iterator &bufend)
{
	int newlength;
	int minlength=0;

	if(bufend-buf<=0)
	{
		if(debug)
			printf("Error: buf too small on %s\n", frm->get_description().c_str());
		return -1;
	}

	if(blength && frm && frm->altformat)
	{
		clear();

		if(!switch_altformat())
		{
			if(debug)
				printf("Error: Field was already parsed but unable to switch to altformat\n");
			return 0;
		}
	}
	else
		reset_altformat();

	do
	{
		if(debug && altformat)
			printf("Info: parse_field: Retrying with alternate format \"%s\"\n", frm->get_description().c_str());

		newlength=parse_field_alt(buf, bufend);
		if(newlength>0)
			return newlength;

		if(newlength!=0 && (minlength==0 || newlength>minlength))
			minlength=newlength;

		clear();
	}
	while(switch_altformat());

	return minlength;
}

int field::parse_field_alt(const std::string::const_iterator &buf, const std::string::const_iterator &bufend)
{
	unsigned int lenlen=0;
	unsigned int newblength=0;
	string lengthbuf;
	unsigned int j;
	int bitmap_start=-1;
	int bitmap_end=0;
	int parse_failed;
	fldformat::iterator cursf;
	fldformat *curfrm;
	unsigned int pos;
	int sflen;
	int minlength=0;
	unsigned int taglength=0;
	fldformat tmpfrm;
	fldformat *frmold, *firstfrmold;
	unsigned int altformatold;
	unsigned int maxlength=bufend-buf;
	int curnum;
	vector<int> fieldstack;

	if(bufend-buf<=0)
	{
		if(debug)
			printf("Error: No buf\n");
		return 0;
	}

	lenlen=frm->lengthLength;

	if(frm->lengthFormat==fldformat::fll_ber)
	{
		if((unsigned char)buf[0]>127)
			lenlen=2;
		else
			lenlen=1;
	}

	if(frm->dataFormat!=fldformat::fld_isobitmap)
	{
		sflen=parse_field_length(buf, bufend);

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
			case fldformat::fld_bitmap:
			case fldformat::fld_bitstr:
				newblength=(length+7)/8;
				break;
			case fldformat::fld_hex:
//			case fldformat::fld_bcdsf:
				length=length*2;
			case fldformat::fld_bcdsf:
			case fldformat::fld_bcd:
				newblength=(length+1)/2;
				break;
			default:
				newblength=length;
		}

		if(lenlen + newblength > maxlength)
		{
			if(debug)
				printf("Error: Field '%s'(%d) is bigger than buffer %d+%d>%d\n", frm->get_description().c_str(), length, lenlen, newblength, maxlength);
			return -(lenlen+newblength);
		}
//		else if(lenlen + newblength < maxlength)
//		{
//			if(debug)
//				printf("Error: Field '%s'(%d) is smaller than buffer %d+%d<%d\n", frm->get_description().c_str(), length, lenlen, newblength, maxlength);
//			return 0;
//		}
	}

	//Now we know the length except for ISOBITMAP
//	printf("Length is %d for %s\n", length, frm->get_description().c_str());
	
	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_tlv:
			parse_failed=1;
			if(frm->dataFormat==fldformat::fld_subfields)
				cursf=frm->begin();
			else
				taglength=frm->tagLength;
			pos=lenlen;
			minlength=0;
			while(parse_failed)
			{
				parse_failed=0;

				if(frm->subfields.empty())
					return 0;

				for(; pos!=maxlength;)
				{
					if(pos==length+lenlen) // Some subfields are missing or canceled by bitmap
						break;

					if(frm->dataFormat==fldformat::fld_subfields) // subfield number is defined by its position
					{
						if(cursf==frm->end())
						{
							if(debug)
								printf("Field length is greater than formats available (%s)\n", frm->get_description().c_str());
							parse_failed=1;
							break;
						}

						curfrm=&cursf->second;
						curnum=cursf->first;
						++cursf;
					}
					else // subfield number is encoded as a tag name
					{
						if(frm->tagFormat==fldformat::flt_ber)
						{
							if((buf[pos]&0x1F)==0x1F)
								taglength=2;
							else
								taglength=1;
						}

						if(pos+taglength>=maxlength)
						{
							if(debug)
								printf("Error: Not enough length for TLV tag\n");
							return -(pos+taglength);
						}

						lengthbuf.clear();
						switch(frm->tagFormat)
						{
							case fldformat::flt_ebcdic:
								parse_ebcdic(buf+pos, lengthbuf, taglength);
								curnum=atoi(lengthbuf.c_str());
								break;
							case fldformat::flt_bcd:
								parse_bcdl(buf+pos, lengthbuf, taglength*2);
								curnum=atoi(lengthbuf.c_str());
								break;
							case fldformat::flt_ber:
							case fldformat::flt_bin:
								curnum=0;
								for(unsigned int i=0; i<taglength; curnum++)
									*(((char*)(&curnum))+3-i)=buf[pos+i];
								break;
							case fldformat::flt_ascii:
								lengthbuf.assign(buf+pos, buf+pos+taglength);
								curnum=atoi(lengthbuf.c_str());
								break;
							default:
								if(debug)
									printf("Error: Unknown tag format\n");
								return 0;
						}

						if(!frm->sfexist(curnum))
						{
							if(debug)
								printf("Error: No format for TLV tag %d.\n", curnum);
							return 0;
						}

						curfrm=&frm->sf(curnum);

						pos+=taglength;
					}

					fieldstack.push_back(curnum);

					sflen=0;

					if(bitmap_start!=-1 && sf(bitmap_start).frm->dataFormat==fldformat::fld_isobitmap && bitmap_end < curnum)
						break;

					if(bitmap_start==-1 || (bitmap_end > curnum-1 && sf(bitmap_start).data[curnum-bitmap_start-1]=='1'))
					{
						if(sfexist(curnum))
						{
							sflen=sf(curnum).blength;
							sf(curnum).clear();
							sf(curnum).blength=sflen;
						}

						if(curfrm->lengthFormat==fldformat::fll_unknown && curfrm->dataFormat!=fldformat::fld_isobitmap) //for unknown length, search for the smallest
						{
							sflen=-1;
							for(unsigned int i=sf(curnum).blength+1; i<length+lenlen-pos+1; i=-sflen)
							{
								if(debug)
									printf("trying pos %d length %d/%d for %s\n", pos, i, length+lenlen-pos, curfrm->get_description().c_str());
								sf(curnum).blength=0;
								sflen=sf(curnum).parse_field(buf+pos, buf+pos+i);

								if(sflen>=0)
									break;

								sf(curnum).reset_altformat(); //restore frm that might be replaced with altformat by parse_field

								if((unsigned int)-sflen<=i)
									sflen=-(i+1);
							}
						}
						else
							sflen=sf(curnum).parse_field(buf+pos, buf+length+lenlen);

						if(sflen==0 && sf(curnum).frm->maxLength==0)
						{
							if(debug)
								printf("Optional subfield %d (%s) skipped\n", curnum, sf(curnum).frm->get_description().c_str());
							subfields.erase(curnum);
							continue;
						}

						if(sflen<=0)
						{
							if(debug)
								printf("Error: unable to parse subfield %d (%s/%s, %d)\n", curnum, get_description().c_str(), sf(curnum).frm->get_description().c_str(), sflen);
							subfields.erase(curnum);
							parse_failed=1;
							break;
						}

						if(frm->dataFormat==fldformat::fld_subfields && (sf(curnum).frm->dataFormat==fldformat::fld_bitmap || sf(curnum).frm->dataFormat==fldformat::fld_isobitmap))	//TODO: change sf(curnum).frm to curfrm or remove curfrm
						{
							if(debug && bitmap_start!=-1)
								printf("Warning: Only one bitmap per subfield allowed\n");
							bitmap_start=curnum;
							bitmap_end=bitmap_start+sf(bitmap_start).data.length();

							for(int i=0; i<bitmap_end-bitmap_start; i++)
								if(sf(bitmap_start).data[i]=='1' && !frm->sfexist(bitmap_start+1+i))
								{
									if(debug)
										printf("Error: No format for subfield %d which is present in bitmap\n", bitmap_start+1+i);
									subfields.erase(curnum);
									bitmap_start=-1;
									parse_failed=1;
									break;
								}
						}

						sf(curnum).start=pos-taglength;
						sf(curnum).blength+=taglength;

						pos+=sflen;
					}
				}

				if(!parse_failed && pos!=length+lenlen)
				{
					if(debug)
						printf("Error: Not enough subfield formats (%d, %d) for %s\n", pos, length, frm->get_description().c_str());
					parse_failed=1;
				}

				if(parse_failed)
				{
					if(sflen<0 && (minlength==0 || sflen-(int)pos>minlength))
						minlength=sflen-pos;

					while(!fieldstack.empty())
					{
						if(frm->dataFormat==fldformat::fld_subfields)
							--cursf;
						curnum=fieldstack.back();
						fieldstack.pop_back();

						if(curnum==bitmap_start)
							bitmap_start=-1;

						if(sfexist(curnum) && ((sf(curnum).frm->lengthFormat==fldformat::fll_unknown && sf(curnum).frm->dataFormat!=fldformat::fld_isobitmap && sf(curnum).blength < length+lenlen-sf(curnum).start) || sf(curnum).frm->altformat))
						{
							if(debug)
								printf("Come back to sf %d of %s (%s)\n", curnum, frm->get_description().c_str(), sf(curnum).frm->get_description().c_str());
							pos=sf(curnum).start;
							parse_failed=0;
							break;
						}
						else if(sfexist(curnum))
							subfields.erase(curnum);
					}

					if(parse_failed)
					{
						if(debug)
							printf("Not comming back (%s)\n", frm->get_description().c_str());
						break;
					}

					parse_failed=1;
				}
			}

			if(parse_failed)
				return minlength;
			
			break;

		case fldformat::fld_bcdsf:
			if(!parse_bcdl(buf+lenlen, data, length))
			{
				if(debug)
					printf("Error: Not BCD field\n");
				return 0;
			}

			tmpfrm.copyFrom(*frm);
			tmpfrm.lengthFormat=fldformat::fll_fixed;
			tmpfrm.lengthLength=0;
			tmpfrm.maxLength=length;
			tmpfrm.addLength=0;
			tmpfrm.dataFormat=fldformat::fld_subfields;
			frmold=frm;
			firstfrmold=firstfrm;
			altformatold=altformat;
			set_frm(&tmpfrm);

			sflen=parse_field(data.begin(), data.end());
			
			data.clear();
			set_frm(firstfrmold);
			change_format(frmold);
			altformat=altformatold;

			if(sflen<=0)
			{
				if(debug)
					printf("Error: Unable to parse BCD subfields\n");
				return sflen;
			}

			break;

		case fldformat::fld_isobitmap:
			for(unsigned int i=0; i==0 || buf[i*8-8]>>7; i++)
			{
				length=i*64+63;
				newblength=i*8+8;
				if((i+1)*8>maxlength)
				{
					if(debug)
						printf("Error: Field is too long\n");
					return -(i+1)*8;
				}

				if(i!=0)
					data.push_back('0');

				for(j=1; j<64; j++)
					data.push_back(buf[i*8+j/8] & (1<<(7-j%8)) ? '1':'0');
			}
			break;

		case fldformat::fld_bitmap:
		case fldformat::fld_bitstr:
			data.clear();
			for(unsigned int i=0; i<length;i++)
				data.push_back(buf[lenlen + i/8] & (1<<(7-i%8)) ? '1':'0');
			break;

		case fldformat::fld_ascii:
			data.assign(buf+lenlen, buf+lenlen+length);
			break;

		case fldformat::fld_hex:
			parse_hex(buf+lenlen, data, length);
			break;

		case fldformat::fld_bcd:
			if(!parse_bcdr(buf+lenlen, data, length))
				return 0;
			break;

		case fldformat::fld_ebcdic:
			parse_ebcdic(buf+lenlen, data, newblength);
			break;

		default:
			if(debug)
				printf("Error: Unknown data format\n");
			return 0;
	}

	if(!frm->data.empty() && !data.empty() && data!=frm->data)
	{
		if(debug)
			printf("Error: Format mandatory data (%s) does not match field data (%s) for %s\n", frm->data.c_str(), data.c_str(), frm->get_description().c_str());
		data.clear();
		return 0;
	}

	blength=lenlen+newblength;

	if(debug && !data.empty() && frm->dataFormat!=fldformat::fld_subfields)
		printf("%s \t[%d(%d)] [%s]\n", frm->get_description().c_str(), length, blength, data.c_str());

	return lenlen+newblength;
}

unsigned int parse_ebcdic(const string::const_iterator &from, string &to, unsigned int len)
{
//	const string ebcdic2ascii("\0\x001\x002\x003 \t \x07F   \x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013 \n\x008 \x018\x019  \x01C\x01D\x01E\x01F\x080 \x01C  \x00A\x017\x01B     \x005\x006\x007  \x016    \x004    \x014\x015 \x01A  âäàáãåçñ¢.<(+|&éêëèíîïìß!$*);¬-/ÂÄÀÁÃÅÇÑ\x0A6,%_>?øÉÊËÈÍÎÏÌ`:#@'=\"Øabcdefghi   ý  \x010jklmnopqr  Æ æ  ~stuvwxyz   Ý  ^£¥       []    {ABCDEFGHI ôöòóõ}JKLMNOPQR ûüùúÿ\\ STUVWXYZ ÔÖÒÓÕ0123456789 ÛÜÙÚŸ", 256);
	const string ebcdic2ascii(" \x001\x002\x003 \t \x07F   \x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013 \n\x008 \x018\x019  \x01C\x01D\x01E\x01F\x080 \x01C  \x00A\x017\x01B     \x005\x006\x007  \x016    \x004    \x014\x015 \x01A           .<(+|&         !$*); -/        \x0A6,%_>?         `:#@'=\" abcdefghi      \x010jklmnopqr       ~stuvwxyz      ^         []    {ABCDEFGHI      }JKLMNOPQR      \\ STUVWXYZ      0123456789      ", 256);

	for(unsigned int i=0; i<len; i++)
		to.push_back(ebcdic2ascii[(unsigned char)from[i]]);

	return len;
}

unsigned int parse_bcdr(const string::const_iterator &from, string &to, unsigned int len)
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
				to.push_back('^');
			}
			else if (t > 9)
			{
				if(debug)
					printf("Error: parse_bcdr: The string is not BCD (%02x, %02x, %d, %d)\n", from[i], t, separator_found, len);
				return 0;
			}
			else
				to.push_back('0'+t);
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
			to.push_back('^');
		}
		else if (t > 9)
		{
			if(debug)
				printf("Error: parse_bcdr: The string is not BCD\n");
			return 0;
		}
		else
			to.push_back('0'+t);
	}

	return len;
}

unsigned int parse_bcdl(const string::const_iterator &from, string &to, unsigned int len)
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
			to.push_back('^');
		}
		else if (t > 9)
		{
			if(debug)
				printf("Error: parse_bcdl: The string is not BCD (%02x, %02x, %d, %d)\n", from[i], t, separator_found, len);
			return 0;
		}
		else
			to.push_back('0'+t);

		if(u==0 || i!=(len+1)/2-1)
		{
			t=((unsigned char)from[i]) & 0x0F;
			if(17<len && len<38 && !separator_found && t==0xD)     //making one exception for track2
			{
				separator_found=1;
				to.push_back('^');
			}
			else if (t > 9)
			{
				if(debug)
					printf("Error: parse_bcdl: The string is not BCD\n");
				return 0;
			}
			else
				to.push_back('0'+t);
		}
		else if((((unsigned char)from[i]) & 0x0F)!=0)
		{
			if(debug)
				printf("Error: parse_bcdl: Last 4 bits not zero\n");
			return 0;
		}
	}

	return len;
}

unsigned int parse_hex(const string::const_iterator &from, string &to, unsigned int len)
{
	unsigned char t;
	unsigned int u=len/2*2==len?0:1;

	for(unsigned int i=0; i<(len+1)/2; i++)
	{
		if(i!=0 || u==0)
		{
			t=((unsigned char)from[i]) >> 4;
			if (t > 9)
				to.push_back('A'+t-10);
			else
				to.push_back('0'+t);
		}
		else if(((unsigned char)from[i])>>4!=0)
		{
			if(debug)
				printf("Warning: parse_hex: First 4 bits not zero\n");
			return 0;
		}

		t=((unsigned char)from[i]) & 0x0F;
		if (t > 9)
			to.push_back('A'+t-10);
		else
			to.push_back('0'+t);
	}

	return len;
}

