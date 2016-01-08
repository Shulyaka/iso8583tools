#include <cstdio>
#include <sstream>
#include <cstring>

#include "parser.h"

using namespace std;

size_t build_ebcdic(const string::const_iterator&, string&, size_t);
size_t build_hex(const string::const_iterator&, string&, size_t);
size_t build_bcdl(const string::const_iterator&, string&, size_t);
size_t build_bcdr(const string::const_iterator&, string&, size_t);
string to_string(unsigned int);

size_t field::serialize(char *s, size_t n) //TODO: invent something more zero-copy
{
	string buf;
	size_t res=serialize(buf);
	if(res>n)
		return 0;

	memcpy(s, buf.data(), res);

	return res;
}

//On success, returns field data length. If field has subfields, their total size is returned
//On failure, returns 0
size_t field::get_flength(void)
{
	size_t lenlen=0;
	size_t flength=data.length();
	fldformat tmpfrm;

	flength=data.length();

	if(frm->lengthFormat==fldformat::fll_ber)
		if(frm->dataFormat==fldformat::fld_bcd || frm->dataFormat==fldformat::fld_hex)
			lenlen=(flength+1)/2>127?2:1;
		else
			lenlen=flength>127?2:1;
	else
		lenlen=frm->lengthLength;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_tlv:
		case fldformat::fld_bcdsf:
			flength=get_blength();
			if(flength<=lenlen)
				return 0;
			flength-=lenlen;
			break;

		case fldformat::fld_isobitmap:
		case fldformat::fld_bitmap:
		case fldformat::fld_bitstr:
		case fldformat::fld_ascii:
		case fldformat::fld_ebcdic:
		case fldformat::fld_hex:
		case fldformat::fld_bcd:
			break;

		default:
			if(debug)
				printf("Error: Unknown data format\n");
			return 0;
	}

	return flength;
}

//On success, returns field binary size including length field if present
//On failure, returns 0
size_t field::get_blength(void)
{
	size_t lenlen=0;
	size_t newblength=0;
	size_t flength=data.length();
	size_t pos, sflen, taglength=0;
	int bitmap_found=-1;
	fldformat tmpfrm;

	if(frm->lengthFormat==fldformat::fll_ber)
		if(frm->dataFormat==fldformat::fld_bcd || frm->dataFormat==fldformat::fld_hex)
			lenlen=(flength+1)/2>127?2:1;
		else
			lenlen=flength>127?2:1;
	else
		lenlen=frm->lengthLength;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_tlv:
		case fldformat::fld_bcdsf:
			pos=lenlen;

			if(frm->dataFormat==fldformat::fld_tlv)
				taglength=frm->tagLength;
			else
			{
				for(fldformat::iterator i=frm->begin(); i!=frm->end(); ++i)
					if(i->second.dataFormat==fldformat::fld_isobitmap || i->second.dataFormat==fldformat::fld_bitmap)
						for(field::iterator j=begin(); j!=end(); ++j)
							if(j->first > i->first && !j->second.empty())
							{
								sf(i->first); //ensure bit map fields are present
								break;
							}
			}

			for(field::iterator i=begin(); i!=end(); ++i)
			{
				if(pos==frm->maxLength+lenlen)
					break;

				if(bitmap_found!=-1 && frm->sf(bitmap_found).dataFormat!=fldformat::fld_isobitmap && frm->sf(bitmap_found).maxLength < (unsigned int)(i->first-bitmap_found))
					break;

				if(!frm->sfexist(i->first))
					return 0;

				if(bitmap_found==-1 || frm->sf(bitmap_found).dataFormat==fldformat::fld_isobitmap || frm->sf(bitmap_found).maxLength > (unsigned int)(i->first-bitmap_found-1))
				{
					if(frm->dataFormat!=fldformat::fld_tlv && frm->sf(i->first).dataFormat==fldformat::fld_isobitmap)
					{
						bitmap_found=i->first;
						sflen=((subfields.rbegin()->first - i->first)/64+1)*8;
					}
					else if(frm->dataFormat!=fldformat::fld_tlv && frm->sf(i->first).dataFormat==fldformat::fld_bitmap)
					{
						bitmap_found=i->first;
						sflen=(frm->sf(i->first).maxLength+7)/8;
					}
					else if(i->second.empty())
						continue;
					else
						sflen=i->second.get_blength();
					
					if(!sflen)
						return 0;

					if(frm->dataFormat==fldformat::fld_tlv && frm->tagFormat==fldformat::flt_ber)
						taglength=i->first>0xFF?2:1;

					pos+=taglength+sflen;
				}
			}

			newblength=pos-lenlen;

			if(frm->dataFormat==fldformat::fld_bcdsf)
				newblength=(newblength+1)/2;
			break;

		case fldformat::fld_hex:
		case fldformat::fld_bcd:
			flength=get_flength();

			if(!flength)
				return 0;

			newblength=(flength+1)/2;

			break;

		case fldformat::fld_isobitmap:
			flength=get_flength();
			if(!flength)
				return 0;
			newblength=(flength/64)*8;
			break;

		case fldformat::fld_bitmap:
		case fldformat::fld_bitstr:
			flength=get_flength();
			if(!flength)
				return 0;
			newblength=(flength+7)/8;
			break;

		case fldformat::fld_ascii:
		case fldformat::fld_ebcdic:
			flength=get_flength();
			if(!flength)
				return 0;
			newblength=flength;
			break;

		default:
			if(debug)
				printf("Error: Unknown data format\n");
			return 0;
	}

	return lenlen+newblength;
}

//On success, returns field length as represented in the message
//On failure, returns 0
size_t field::get_mlength(void)
{
	size_t lenlen=0;
	size_t mlength=0;

	if(frm->lengthFormat==fldformat::fll_ber)
		if(frm->dataFormat==fldformat::fld_bcd || frm->dataFormat==fldformat::fld_hex)
			lenlen=(data.length()+1)/2>127?2:1;
		else
			lenlen=data.length()>127?2:1;
	else
		lenlen=frm->lengthLength;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_tlv:
		case fldformat::fld_isobitmap:
		case fldformat::fld_bitmap:
		case fldformat::fld_bitstr:
		case fldformat::fld_ascii:
		case fldformat::fld_ebcdic:
		case fldformat::fld_bcd:
			mlength=get_flength();
			if(!mlength)
				return 0;
			break;

		case fldformat::fld_bcdsf:
		case fldformat::fld_hex:
			mlength=get_blength();
			if(mlength<=lenlen)
				return 0;
			mlength-=lenlen;
			break;

		default:
			if(debug)
				printf("Error: Unknown data format\n");
			return 0;
	}

	if(frm->lengthInclusive)
		mlength+=lenlen;

	return mlength+frm->addLength;
}

size_t field::build_field_length(string &buf)
{
	size_t lenlen=0;
	size_t mlength=0;
	string lengthbuf;
	fldformat tmpfrm;

	mlength=get_mlength();
	if(!mlength)
	{
		if(debug)
			printf("Error: build_field_length: Wrong length\n");
		return 0;
	}

	lenlen=frm->lengthLength;

	switch(frm->lengthFormat)
	{
		case fldformat::fll_fixed:
			if(frm->maxLength != mlength)
			{
				if(debug)
					printf("Error: Bad length for fixed-length field! %lu (field) != %lu (format) for %s\n", mlength, frm->maxLength, frm->get_description().c_str());
				return 0;
			}
			break;

		case fldformat::fll_bin:
			if(lenlen>4)
				buf.append(lenlen-4, '\0');

			for(size_t i=0; i<(lenlen>4?4:lenlen); i++)
				buf.push_back(((unsigned char *)(&mlength))[(lenlen>4?4:lenlen)-i-1]);  //TODO: htonl()

			break;

		case fldformat::fll_ber:
			if(mlength>127)
			{
				lenlen=2;
				buf.push_back(127+lenlen);
			}

			buf.push_back(mlength);
			break;

		case fldformat::fll_bcd:
			lengthbuf=to_string(mlength);
			if(lengthbuf.length()>lenlen*2)
			{
				if(debug)
					printf("Error: Length of length is too small (%lu)\n", lenlen);
				return 0;
			}

			buf.append(lenlen - (lengthbuf.length()+1)/2, '\0');

			if(!build_bcdr(lengthbuf.begin(), buf, lengthbuf.length()))
				return 0;

			break;

		case fldformat::fll_ascii:
			lengthbuf=to_string(mlength);
			if(lengthbuf.length()>lenlen)
			{
				if(debug)
					printf("Error: Length of length is too small (%lu)\n", lenlen);
				return 0;
			}

			buf.append(lenlen - lengthbuf.length(), '\0');

			buf.append(lengthbuf);

			break;
	
		case fldformat::fll_ebcdic:
			lengthbuf=to_string(mlength);
			if(lengthbuf.length()>lenlen)
			{
				if(debug)
					printf("Error: Length of length is too small (%lu)\n", lenlen);
				return 0;
			}

			buf.append(lenlen - lengthbuf.length(), 0xF0U);

			if(!build_ebcdic(lengthbuf.begin(), buf, lengthbuf.length()))
				return 0;

			break;

		case fldformat::fll_unknown:
			break;

		default:
			if(frm->dataFormat!=fldformat::fld_isobitmap)
			{
				if(debug)
					printf("Error: Unknown length format\n");
				return 0;
			}
	}

	if(debug && lenlen)
		printf("build_field_length: %s, lenlen %lu, mlength %lu, total length %lu\n", frm->get_description().c_str(), lenlen, mlength, buf.length());

	return lenlen;
}

size_t field::build_field(string &buf)  //TODO: remove build_field_alt() and make sure altformat is set during filling of the field
{
	size_t newlength;
	fldformat *frmold=frm;

	do
	{
		newlength=build_field_alt(buf);

		if(newlength)
			return newlength;

		if(debug)
			printf("Info: Retrying with an alternate format\n");
	}
	while(switch_altformat());

	if(debug)
		printf("Info: No alternate format\nWarning: Unable to build the field. Trying to reset to the first altformat and start over\n");

	reset_altformat();

	do
	{
		if(frmold==frm) //full loop
			break;

		newlength=build_field_alt(buf);

		if(newlength)
			return newlength;

		if(debug)
			printf("Info: Retrying with an alternate format\n");
	}
	while(switch_altformat());

	if(debug)
		printf("Info: No alternate format\n");

	return 0;
}

size_t field::build_field_alt(string &buf)
{
	size_t lenlen=0;
	size_t newblength=0;
	size_t flength=0;
	string lengthbuf, bcd;
	size_t pos, sflen, taglength=0;
	int bitmap_found=-1;
	fldformat tmpfrm;
	unsigned char tmpc=0;

//	printf("Building %s\n", frm->get_description().c_str());

	if(!frm->data.empty() && !data.empty() && data!=frm->data)
	{
		if(debug)
			printf("Error: Format mandatory data (%s) does not match field data (%s) for %s\n", frm->data.c_str(), data.c_str(), frm->get_description().c_str());
		return 0;
	}

	lenlen=build_field_length(buf);
	if(!lenlen && frm->lengthLength)
	{
		if(debug)
			printf("Error: build_field_alt: Wrong length for %s\n", get_description().c_str());
		return 0;
	}

	pos=lenlen;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_tlv:
		case fldformat::fld_bcdsf:
			if(frm->dataFormat==fldformat::fld_tlv)
				taglength=frm->tagLength;
			else
			{
				for(fldformat::iterator i=frm->begin(); i!=frm->end(); ++i)
					if(i->second.dataFormat==fldformat::fld_isobitmap || i->second.dataFormat==fldformat::fld_bitmap)
						for(field::iterator j=begin(); j!=end(); ++j)
							if(j->first > i->first && !j->second.empty())
							{
								sf(i->first); //ensure bit map fields are present
								break;
							}
			}

			for(field::iterator i=begin(); i!=end(); ++i)	//TODO: implement a stack and go back if build failed
			{
				if(bitmap_found!=-1 && frm->sf(bitmap_found).dataFormat!=fldformat::fld_isobitmap && frm->sf(bitmap_found).maxLength < i->first-(unsigned int)bitmap_found)	//TODO: suspicious condition
					break;

				if(!frm->sfexist(i->first))
				{
					if(debug)
						printf("Error: No format for subfield %d\n", i->first);
					return 0;
				}

				if(bitmap_found==-1 || frm->sf(bitmap_found).dataFormat==fldformat::fld_isobitmap || frm->sf(bitmap_found).maxLength > i->first-(unsigned int)bitmap_found-1)
				{
					if(frm->dataFormat!=fldformat::fld_tlv && frm->sf(i->first).dataFormat==fldformat::fld_isobitmap)
					{
						bitmap_found=i->first;
						if(frm->dataFormat!=fldformat::fld_bcdsf)
							sflen=build_isobitmap(buf, i->first);
						else
							sflen=build_isobitmap(bcd, i->first);
					}
					else if(frm->dataFormat!=fldformat::fld_tlv && frm->sf(i->first).dataFormat==fldformat::fld_bitmap)
					{
						bitmap_found=i->first;
						if(frm->dataFormat!=fldformat::fld_bcdsf)
							sflen=build_bitmap(buf, i->first);
						else
							sflen=build_bitmap(bcd, i->first);
					}
					else if(i->second.empty())
					{
						if(debug)
							printf("Warning: Empty subfield %d\n", i->first);
						continue;
					}
					else
					{
						if(frm->dataFormat==fldformat::fld_tlv)
						{
							switch(frm->tagFormat)
							{
								case fldformat::flt_ebcdic:
									lengthbuf=to_string(i->first);
									if(lengthbuf.length()>taglength)
									{
										if(debug)
											printf("Error: TLV tag number is too big (%d)\n", i->first);
										return 0;
									}

									if(frm->dataFormat!=fldformat::fld_bcdsf)
									{
										buf.append(taglength - lengthbuf.length(), 0xF0);

										if(!build_ebcdic(lengthbuf.begin(), buf, lengthbuf.length()))
											return 0;
									}
									else
									{
										bcd.append(taglength - lengthbuf.length(), 0xF0);

										if(!build_ebcdic(lengthbuf.begin(), bcd, lengthbuf.length()))
											return 0;
									}

									break;

								case fldformat::flt_bcd:
									lengthbuf=to_string(i->first);
									if(lengthbuf.length()>taglength*2)
									{
										if(debug)
											printf("Error: TLV tag number is too big (%d)\n", i->first);
										return 0;
									}

									if(frm->dataFormat!=fldformat::fld_bcdsf)
									{
										buf.append(taglength - (lengthbuf.length()+1)/2, '\0');

										if(!build_bcdr(lengthbuf.begin(), buf, lengthbuf.length()))
											return 0;
									}
									else
									{
										bcd.append(taglength - (lengthbuf.length()+1)/2, '\0');

										if(!build_bcdr(lengthbuf.begin(), bcd, lengthbuf.length()))
											return 0;
									}
									break;

								case fldformat::flt_ber:
									taglength=i->first>0xFF?2:1;
									//no break intentionally
								case fldformat::flt_bin:
									if(frm->dataFormat!=fldformat::fld_bcdsf)
									{
										if(taglength>4)
											buf.append(taglength-4, '\0');

										for(size_t j=0; j<(taglength>4?4:taglength); j++)
											buf.push_back(((unsigned char *)(&(i->first)))[(taglength>4?4:taglength)-j-1]);  //TODO: htonl()
									}
									else
									{
										if(taglength>4)
											bcd.append(taglength-4, '\0');

										for(size_t j=0; j<(taglength>4?4:taglength); j++)
											bcd.push_back(((unsigned char *)(&(i->first)))[(taglength>4?4:taglength)-j-1]);  //TODO: htonl()
									}
									break;

								case fldformat::flt_ascii:
									lengthbuf=to_string(i->first);
									if(lengthbuf.length()>taglength)
									{
										if(debug)
											printf("Error: TLV tag number is too big (%d)\n", i->first);
										return 0;
									}

									if(frm->dataFormat!=fldformat::fld_bcdsf)
									{
										buf.append(taglength - lengthbuf.length(), '0');
										buf.append(lengthbuf);
									}
									else
									{
										bcd.append(taglength - lengthbuf.length(), '0');
										bcd.append(lengthbuf);
									}

									break;

								default:
									if(debug)
										printf("Error: Unknown tag format\n");
									return 0;
							}

							pos+=taglength;
						}

						if(frm->dataFormat!=fldformat::fld_bcdsf)
							sflen=i->second.build_field(buf);
						else
							sflen=i->second.build_field(bcd);
					}
					
					if(!sflen)
					{
						if(debug)
							printf("Error: unable to build subfield %d: %s\n", i->first, frm->sf(i->first).get_description().c_str());
						return 0;
					}
					pos+=sflen;
				}
			}

			flength=pos-lenlen;
			newblength=pos-lenlen;

			if(frm->dataFormat==fldformat::fld_bcdsf)
			{
				if(!build_bcdl(bcd.begin(), buf, flength))
				{
					if(debug)
						printf("Error: Not BCD subfield\n");
					return 0;
				}

				newblength=(newblength+1)/2;
			}

			break;

		case fldformat::fld_isobitmap:
			flength=data.length();

			if(flength==0)
				return 0;

			for(size_t i=0; i<flength/64+1; newblength=(++i)*8)
			{
				if(i!=flength/64)
					tmpc=0x80;
				else
					tmpc=0;

				if(i>0 && data[i*64-1]!='0') //ISOBITMAP can't encode fields 1, 65, 129, etc. Counting from 2, their offsets are i*64-1. So, if these bits are present, it's not an ISOBITMAP
				{
					if(debug)
						printf("Error: Not a bitmap\n");
					return 0;
				}

				for(size_t j=1; j<64 && i*64+j-1<flength; j++)
				{
					if(data[i*64+j-1]=='1')
						tmpc|=1<<(7-j%8);
					else if(data[i*64+j-1]!='0')
					{
						if(debug)
							printf("Error: Not a bitmap\n");
						return 0;
					}
					if((j+1)/8*8==j+1 || i*64+j==flength)
					{	buf.push_back(tmpc);
						tmpc=0;
					}
					if(i*64+j==flength)
					{
						buf.append(7-j/8, '\0');
					}
				}
			}

			break;

		case fldformat::fld_bitmap:
		case fldformat::fld_bitstr:
			flength=data.length();

			newblength=(flength+7)/8;

			tmpc=0;
			for(size_t i=0; i<flength;i++)
			{
				if(data[i]=='1')
					tmpc|=1<<(7-i%8);
				if((i+1)/8*8==i+1 || i==flength-1)
				{
					buf.push_back(tmpc);
					tmpc=0;
				}
			}

			break;

		case fldformat::fld_ascii:
			newblength=data.length();
			buf.append(data);
			break;

		case fldformat::fld_hex:
			flength=data.length();
			newblength=(flength+1)/2;
			if(!build_hex(data.begin(), buf, flength))
				return 0;

			break;

		case fldformat::fld_bcd:
			flength=data.length();
			newblength=(flength+1)/2;
			if(!build_bcdr(data.begin(), buf, flength))
				return 0;

			break;

		case fldformat::fld_ebcdic:
			newblength=data.length();;
			build_ebcdic(data.begin(), buf, newblength);
			break;

		default:
			if(debug)
				printf("Error: Unknown data format\n");
			return 0;
	}

	if(debug)
		printf("build_field: %s, length %lu, total length %lu\n", frm->get_description().c_str(), newblength, buf.length());

	return lenlen+newblength;
}

size_t build_ebcdic(const string::const_iterator &from, string &to, size_t len)
{
	const string ascii2ebcdic("\0\x001\x002\x003\x037\x02D\x02E\x02F\x016\x005\x025\x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013\x03C\x03D\x032\x026\x018\x019\x03F\x027\x022\x01D\x01E\x01F\x040\x05A\x07F\x07B\x05B\x06C\x050\x07D\x04D\x05D\x05C\x04E\x06B\x060\x04B\x061\x0F0\x0F1\x0F2\x0F3\x0F4\x0F5\x0F6\x0F7\x0F8\x0F9\x07A\x05E\x04C\x07E\x06E\x06F\x07C\x0C1\x0C2\x0C3\x0C4\x0C5\x0C6\x0C7\x0C8\x0C9\x0D1\x0D2\x0D3\x0D4\x0D5\x0D6\x0D7\x0D8\x0D9\x0E2\x0E3\x0E4\x0E5\x0E6\x0E7\x0E8\x0E9\x0BA\x0E0\x0BB\x0B0\x06D\x079\x081\x082\x083\x084\x085\x086\x087\x088\x089\x091\x092\x093\x094\x095\x096\x097\x098\x099\x0A2\x0A3\x0A4\x0A5\x0A6\x0A7\x0A8\x0A9\x0C0\x04F\x0D0\x0A1\x007\x020\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x0FF\x040\x040\x04A\x0B1\x040\x0B2\x06A\x040\x040\x0C3\x040\x040\x05F\x040\x0D9\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x07F\x040\x040\x040\x040\x064\x065\x062\x066\x063\x067\x09C\x068\x074\x071\x072\x073\x078\x075\x076\x077\x040\x069\x0ED\x0EE\x0EB\x0EF\x0EC\x040\x080\x0FD\x0FE\x0FB\x0FC\x0AD\x040\x059\x044\x045\x042\x046\x043\x047\x09E\x048\x054\x051\x052\x053\x058\x055\x056\x057\x040\x049\x0CD\x0CE\x0CF\x0CB\x0CC\x040\x070\x0DD\x0DE\x0DB\x0DC\x08D\x040\x0DF", 256);

	for(size_t i=0; i<len; i++)
		to.push_back(ascii2ebcdic[(unsigned char)from[i]]);
	
	return len;
}

size_t build_bcdr(const string::const_iterator &from, string &to, size_t len)
{
	unsigned char t, tmpc=0;
	size_t u=len/2*2==len?0:1;
	size_t separator_found=0;

	for(size_t i=0; i<(len+1)/2; i++)
	{
		if(i!=0 || u==0)
		{
			t=(unsigned char)from[i*2-u];
			if(17<len && len<38 && !separator_found && t=='^')     //making one exception for track2
			{
				separator_found=1;
				tmpc=0xD0;
			}
			else if(t>='0' && t<='9')
				tmpc=(t-'0')<<4;
			else
			{
				if(debug)
					printf("Error: build_bcdr: The string is not BCD\n");
				return 0;
			}
		}

		t=(unsigned char)from[i*2+1-u];
		if(17<len && len<38 && !separator_found && t=='^')     //making one exception for track2
		{
			separator_found=1;
			tmpc|=0xD;
		}
		else if(t>='0' && t<='9')
			tmpc|=t-'0';
		else
		{
			if(debug)
				printf("Error: build_bcdr: The string is not BCD\n");
			to.push_back(tmpc);
			return 0;
		}
		to.push_back(tmpc);
	}
	return (len+1)/2;
}

size_t build_bcdl(const string::const_iterator &from, string &to, size_t len)
{
	unsigned char t, tmpc=0;
	size_t u=len/2*2==len?0:1;
	size_t separator_found=0;

	for(size_t i=0; i<(len+1)/2; i++)
	{
		t=(unsigned char)from[i*2];
		if(17<len && len<38 && !separator_found && t=='^')     //making one exception for track2
		{
			separator_found=1;
			tmpc=0xD0;
		}
		else if(t>='0' && t<='9')
			tmpc=(t-'0')<<4;
		else
		{
			if(debug)
				printf("Error: build_bcdl: The string is not BCD\n");
			return 0;
		}

		if(u==0 || i!=(len+1)/2-1)
		{
			t=(unsigned char)from[i*2+1];
			if(17<len && len<38 && !separator_found && t=='^')     //making one exception for track2
			{
				separator_found=1;
				tmpc|=0xD;
			}
			else if(t>='0' && t<='9')
				tmpc|=t-'0';
			else
			{
				if(debug)
					printf("Error: build_bcdl: The string is not BCD\n");
				to.push_back(tmpc);
				return 0;
			}
		}
		to.push_back(tmpc);
	}
	return (len+1)/2;
}

size_t build_hex(const string::const_iterator &from, string &to, size_t len)
{
	unsigned char t, tmpc=0;
	size_t u=len/2*2==len?0:1;

	for(size_t i=0; i<(len+1)/2; i++)
	{
		if(i!=0 || u==0)
		{
			t=(unsigned char)from[i*2-u];
			if(t>='0' && t<='9')
				tmpc=(t-'0')<<4;
			else if(t>='A' && t<='F')
				tmpc=(t-'A'+0xA)<<4;
			else if(t>='a' && t<='f')
				tmpc=(t-'a'+0xA)<<4;
			else
			{
				if(debug)
					printf("Error: build_hex: The string is not HEX\n");
				return 0;
			}
		}

		t=(unsigned char)from[i*2+1-u];
		if(t>='0' && t<='9')
			tmpc|=t-'0';
		else if(t>='A' && t<='F')
			tmpc|=t-'A'+0xA;
		else if(t>='a' && t<='f')
			tmpc|=t-'a'+0xA;
		else
		{
			if(debug)
				printf("Error: build_hex: The string is not HEX\n");
			to.push_back(tmpc);
			return 0;
		}
		to.push_back(tmpc);
	}
	return (len+1)/2;
}

size_t field::build_isobitmap(string &buf, unsigned int index)
{
	size_t newblength=0;
	unsigned char tmpc=0;

	int fields=subfields.empty()?0:subfields.rbegin()->first+1;
	//printf("ISOBITMAP: %d %u %d\n", fields, index, (fields-index-1)/64+1);

	for(size_t i=0; i<(fields-index-1)/64+1; newblength=(++i)*8)
	{
		if(i!=(fields-index-1)/64)
			tmpc=0x80;
		else
			tmpc=0;

		for(unsigned char j=1; j<64; j++)
		{
			if(sfexist(i*64+j+index) && !sf(i*64+j+index).empty())
				tmpc|=1<<(7-j%8);
			if((j+1)/8*8==j+1)
			{
				buf.push_back(tmpc);
				tmpc=0;
			}
		}
	}
	return newblength;
}

size_t field::build_bitmap(string &buf, unsigned int index)
{
	size_t newblength, flength;
	unsigned char tmpc=0;

	int fields=subfields.empty()?0:subfields.rbegin()->first+1;
	flength=frm->sf(index).maxLength;

	newblength=(flength+7)/8;

	for(size_t i=0; i<flength && i<fields-index-1; i++)
	{
		if(sfexist(index+1+i) && !sf(index+1+i).empty())
			tmpc|=1<<(7-i%8);
		if((i+1)/8*8==i+1 || i==flength-1 || i==fields-index-2)
		{
			buf.push_back(tmpc);
			tmpc=0;
		}
		if(i==fields-index-2)
			buf.append(newblength-i/8-1, '\0');
	}
	return newblength;
}

