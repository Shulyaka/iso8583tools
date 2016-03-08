#include <cstdio>
#include <sstream>
#include <cstring>

#include "parser.h"

using namespace std;

size_t field::serialize(char *s, size_t n) //TODO: invent something more zero-copy
{
	string buf;
	size_t res=serialize(buf);
	if(res>n)
		throw overflow_error("Not enough buffer for the message");

	memcpy(s, buf.data(), res);

	return res;
}

//Returns field data length. If field has subfields, their total size is returned
size_t field::get_flength(void)
{
	size_t lenlen=0;
	size_t flength=data.length();
	size_t pos, sflen, taglength=0;
	int bitmap_found=-1;
	fldformat tmpfrm;

	flength=data.length();

	if(frm->lengthFormat==fldformat::fll_ber)
		if(frm->dataFormat==fldformat::fld_bcdl || frm->dataFormat==fldformat::fld_bcdr || frm->dataFormat==fldformat::fld_hex)
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

			for(iterator i=begin(); i!=end(); ++i)
			{
				if(pos==frm->maxLength+lenlen)
					break;

				if(bitmap_found!=-1 && frm->sf(bitmap_found).dataFormat!=fldformat::fld_isobitmap && frm->sf(bitmap_found).maxLength < (unsigned int)(i->first-bitmap_found)) //TODO: check this condition
					break;

				if(!frm->sfexist(i->first))
					throw("Subfield format does not exist");

				if(bitmap_found==-1 || frm->sf(bitmap_found).dataFormat==fldformat::fld_isobitmap || frm->sf(bitmap_found).maxLength > (unsigned int)(i->first-bitmap_found-1)) //TODO: change frm->sf(*) into i->second.frm or something
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
					
					if(frm->dataFormat==fldformat::fld_tlv && frm->tagFormat==fldformat::flt_ber)
						taglength=i->first>0xFF?2:1;

					pos+=taglength+sflen;
				}
			}

			flength=pos-lenlen;
			break;

		case fldformat::fld_isobitmap:
		case fldformat::fld_bitmap:
		case fldformat::fld_bitstr:
		case fldformat::fld_ascii:
		case fldformat::fld_ebcdic:
		case fldformat::fld_hex:
		case fldformat::fld_bcdl:
		case fldformat::fld_bcdr:
			break;

		default:
			throw invalid_argument("Unknown data format");
	}

	return flength;
}

//Returns field binary size including length field if present
size_t field::get_blength(void)
{
	size_t lenlen=0;
	size_t newblength=0;

	if(frm->lengthFormat==fldformat::fll_ber)
		if(frm->dataFormat==fldformat::fld_bcdl || frm->dataFormat==fldformat::fld_bcdr || frm->dataFormat==fldformat::fld_hex)
			lenlen=(data.length()+1)/2>127?2:1;
		else
			lenlen=data.length()>127?2:1;
	else
		lenlen=frm->lengthLength;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_tlv:
			newblength=get_flength();
			break;

		case fldformat::fld_bcdsf:
		case fldformat::fld_hex:
		case fldformat::fld_bcdl:
		case fldformat::fld_bcdr:
			newblength=(get_flength()+1)/2;
			break;

		case fldformat::fld_isobitmap:
			newblength=(get_flength()/64)*8;
			break;

		case fldformat::fld_bitmap:
		case fldformat::fld_bitstr:
			newblength=(get_flength()+7)/8;
			break;

		case fldformat::fld_ascii:
		case fldformat::fld_ebcdic:
			newblength=get_flength();
			break;

		default:
			throw invalid_argument("Unknown data format");
	}

	return lenlen+newblength;
}

//Returns field length as represented in the message
size_t field::get_mlength(void)
{
	size_t lenlen=0;
	size_t mlength=0;

	if(frm->lengthFormat==fldformat::fll_ber)
		if(frm->dataFormat==fldformat::fld_bcdl || frm->dataFormat==fldformat::fld_bcdr || frm->dataFormat==fldformat::fld_hex)
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
		case fldformat::fld_bcdl:
		case fldformat::fld_bcdr:
			mlength=get_flength();
			break;

		case fldformat::fld_bcdsf:
		case fldformat::fld_hex:
			mlength=get_blength()-lenlen;
			break;

		default:
			throw invalid_argument("Unknown data format");
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

	lenlen=frm->lengthLength;

	switch(frm->lengthFormat)
	{
		case fldformat::fll_fixed:
			if(frm->maxLength != get_flength())
				throw invalid_argument("Bad length for fixed-length field");
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
				throw invalid_argument("Length of length is too small");

			buf.append(lenlen - (lengthbuf.length()+1)/2, '\0');

			build_bcdr(lengthbuf.begin(), buf, lengthbuf.length(), '0');

			break;

		case fldformat::fll_ascii:
			lengthbuf=to_string(mlength);
			if(lengthbuf.length()>lenlen)
				throw invalid_argument("Length of length is too small");

			buf.append(lenlen - lengthbuf.length(), '\0');

			buf.append(lengthbuf);

			break;
	
		case fldformat::fll_ebcdic:
			lengthbuf=to_string(mlength);
			if(lengthbuf.length()>lenlen)
				throw invalid_argument("Length of length is too small");

			buf.append(lenlen - lengthbuf.length(), 0xF0U);

			build_ebcdic(lengthbuf.begin(), buf, lengthbuf.length());

			break;

		case fldformat::fll_unknown:
			break;

		default:
			if(frm->dataFormat!=fldformat::fld_isobitmap)
				throw invalid_argument("Unknown length format");
	}

	if(debug && lenlen)
		printf("build_field_length: %s, lenlen %lu, mlength %lu, total length %lu\n", frm->get_description().c_str(), lenlen, mlength, buf.length());

	return lenlen;
}

size_t field::build_field(string &buf)  //TODO: remove build_field_alt() and make sure altformat is set during filling of the field
{
	size_t newlength;
	const fldformat *frmold=frm;

	while(true)
	{
		try
		{
			newlength=build_field_alt(buf);
			return newlength;
		}
		catch(const exception& e)
		{
			if(debug)
				printf("Info: Retrying with an alternate format\n");

			try
			{
				switch_altformat();
			}
			catch(const exception &e)
			{
				break;
			}
		}
	}

	if(debug)
		printf("Info: No alternate format\nWarning: Unable to build the field. Trying to reset to the first altformat and start over\n");

	reset_altformat();

	while(true)
	{
		if(frmold==frm) //full loop
			break;

		try
		{
			newlength=build_field_alt(buf);
			return newlength;
		}
		catch(const exception& e)
		{
			if(debug)
				printf("Info: Retrying with an alternate format\n");

			try
			{
				switch_altformat();
			}
			catch(const exception &e)
			{
				break;
			}
		}
	}

	throw invalid_argument("No alternate format to try");
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
		throw invalid_argument("Format mandatory data does not match field data");

	lenlen=build_field_length(buf);

	pos=lenlen;

	switch(frm->dataFormat)
	{
		case fldformat::fld_subfields:
		case fldformat::fld_tlv:
		case fldformat::fld_bcdsf:
			if(frm->dataFormat==fldformat::fld_tlv)
				taglength=frm->tagLength;

			for(iterator i=begin(); i!=end(); ++i)	//TODO: implement a stack and go back if build failed
			{
				if(bitmap_found!=-1 && frm->sf(bitmap_found).dataFormat!=fldformat::fld_isobitmap && frm->sf(bitmap_found).maxLength < i->first-(unsigned int)bitmap_found)	//TODO: suspicious condition
					break;

				if(!frm->sfexist(i->first)) //TODO: Should we remove this?
					throw invalid_argument("No format for subfield");

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
										throw invalid_argument("TLV tag number is too big");

									if(frm->dataFormat!=fldformat::fld_bcdsf)
									{
										buf.append(taglength - lengthbuf.length(), 0xF0);
										build_ebcdic(lengthbuf.begin(), buf, lengthbuf.length());
									}
									else
									{
										bcd.append(taglength - lengthbuf.length(), 0xF0);
										build_ebcdic(lengthbuf.begin(), bcd, lengthbuf.length());
									}

									break;

								case fldformat::flt_bcd:
									lengthbuf=to_string(i->first);
									if(lengthbuf.length()>taglength*2)
										throw invalid_argument("TLV tag number is too big");

									if(frm->dataFormat!=fldformat::fld_bcdsf)
									{
										buf.append(taglength - (lengthbuf.length()+1)/2, '\0');
										build_bcdr(lengthbuf.begin(), buf, lengthbuf.length(), frm->fillChar);
									}
									else
									{
										bcd.append(taglength - (lengthbuf.length()+1)/2, '\0');
										build_bcdr(lengthbuf.begin(), bcd, lengthbuf.length(), frm->fillChar);
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
										throw invalid_argument("TLV tag number is too big");

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
									throw invalid_argument("Error: Unknown tag format");
							}

							pos+=taglength;
						}

						if(frm->dataFormat!=fldformat::fld_bcdsf)
							sflen=i->second.build_field(buf);
						else
							sflen=i->second.build_field(bcd);
					}
					
					pos+=sflen;
				}
			}

			flength=pos-lenlen;
			newblength=pos-lenlen;

			if(frm->dataFormat==fldformat::fld_bcdsf)
			{
				build_bcdl(bcd.begin(), buf, flength, frm->fillChar);
				newblength=(newblength+1)/2;
			}

			break;

		case fldformat::fld_isobitmap:
			flength=data.length();

			for(size_t i=0; i<flength/64+1; newblength=(++i)*8)
			{
				if(i!=flength/64)
					tmpc=0x80;
				else
					tmpc=0;

				if(i>0 && data[i*64-1]!='0') //ISOBITMAP can't encode fields 1, 65, 129, etc. Counting from 2, their offsets are i*64-1. So, if these bits are present, it's not an ISOBITMAP
					throw invalid_argument("Not a bitmap");

				for(size_t j=1; j<64 && i*64+j-1<flength; j++)
				{
					if(data[i*64+j-1]=='1')
						tmpc|=1<<(7-j%8);
					else if(data[i*64+j-1]!='0')
						throw invalid_argument("Not a bitmap");
					if((j+1)/8*8==j+1 || i*64+j==flength)
					{	buf.push_back(tmpc);
						tmpc=0;
					}
					if(i*64+j==flength)
						buf.append(7-j/8, '\0');
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
			build_hex(data.begin(), buf, flength, '0');

			break;

		case fldformat::fld_bcdr:
			flength=data.length();
			newblength=(flength+1)/2;
			build_bcdr(data.begin(), buf, flength, frm->fillChar);

			break;

		case fldformat::fld_bcdl:
			flength=data.length();
			newblength=(flength+1)/2;
			build_bcdl(data.begin(), buf, flength, frm->fillChar);

			break;

		case fldformat::fld_ebcdic:
			newblength=data.length();;
			build_ebcdic(data.begin(), buf, newblength);
			break;

		default:
			throw invalid_argument("Unknown data format");
	}

	if(debug)
		printf("build_field: %s, length %lu, total length %lu\n", frm->get_description().c_str(), newblength, buf.length());

	return lenlen+newblength;
}

size_t field::build_ebcdic(const string::const_iterator &from, string &to, size_t len)
{
	const string ascii2ebcdic("\0\x001\x002\x003\x037\x02D\x02E\x02F\x016\x005\x025\x00B\x00C\x00D\x00E\x00F\x010\x011\x012\x013\x03C\x03D\x032\x026\x018\x019\x03F\x027\x022\x01D\x01E\x01F\x040\x05A\x07F\x07B\x05B\x06C\x050\x07D\x04D\x05D\x05C\x04E\x06B\x060\x04B\x061\x0F0\x0F1\x0F2\x0F3\x0F4\x0F5\x0F6\x0F7\x0F8\x0F9\x07A\x05E\x04C\x07E\x06E\x06F\x07C\x0C1\x0C2\x0C3\x0C4\x0C5\x0C6\x0C7\x0C8\x0C9\x0D1\x0D2\x0D3\x0D4\x0D5\x0D6\x0D7\x0D8\x0D9\x0E2\x0E3\x0E4\x0E5\x0E6\x0E7\x0E8\x0E9\x0BA\x0E0\x0BB\x0B0\x06D\x079\x081\x082\x083\x084\x085\x086\x087\x088\x089\x091\x092\x093\x094\x095\x096\x097\x098\x099\x0A2\x0A3\x0A4\x0A5\x0A6\x0A7\x0A8\x0A9\x0C0\x04F\x0D0\x0A1\x007\x020\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x0FF\x040\x040\x04A\x0B1\x040\x0B2\x06A\x040\x040\x0C3\x040\x040\x05F\x040\x0D9\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x040\x07F\x040\x040\x040\x040\x064\x065\x062\x066\x063\x067\x09C\x068\x074\x071\x072\x073\x078\x075\x076\x077\x040\x069\x0ED\x0EE\x0EB\x0EF\x0EC\x040\x080\x0FD\x0FE\x0FB\x0FC\x0AD\x040\x059\x044\x045\x042\x046\x043\x047\x09E\x048\x054\x051\x052\x053\x058\x055\x056\x057\x040\x049\x0CD\x0CE\x0CF\x0CB\x0CC\x040\x070\x0DD\x0DE\x0DB\x0DC\x08D\x040\x0DF", 256);

	for(size_t i=0; i<len; i++)
		to.push_back(ascii2ebcdic[(unsigned char)from[i]]);
	
	return len;
}

size_t field::build_bcdr(const string::const_iterator &from, string &to, size_t len, char fillChar)
{
	unsigned char t, tmpc=0;
	size_t u=len/2*2==len?0:1;
	size_t separator_found=0;

	for(size_t i=0; i<(len+1)/2; i++)
	{
		if(i!=0 || u==0)
		{
			t=(unsigned char)from[i*2-u];
			if(17<len && len<38 && !separator_found && (t=='^' || t=='=' || t=='D'))     //making one exception for track2
			{
				separator_found=1;
				tmpc=0xD0;
			}
			else if(t>='0' && t<='9')
				tmpc=(t-'0')<<4;
			else
				throw invalid_argument("The string is not BCD");
		}
		else if(fillChar=='F')
			tmpc=0xF0;

		t=(unsigned char)from[i*2+1-u];
		if(17<len && len<38 && !separator_found && (t=='^' || t=='=' || t=='D'))     //making one exception for track2
		{
			separator_found=1;
			tmpc|=0xD;
		}
		else if(t>='0' && t<='9')
			tmpc|=t-'0';
		else
		{
			to.push_back(tmpc);
			throw invalid_argument("The string is not BCD");
		}
		to.push_back(tmpc);
	}
	return (len+1)/2;
}

size_t field::build_bcdl(const string::const_iterator &from, string &to, size_t len, char fillChar)
{
	unsigned char t, tmpc=0;
	size_t u=len/2*2==len?0:1;
	size_t separator_found=0;

	for(size_t i=0; i<(len+1)/2; i++)
	{
		t=(unsigned char)from[i*2];
		if(17<len && len<38 && !separator_found && (t=='^' || t=='=' || t=='D'))     //making one exception for track2
		{
			separator_found=1;
			tmpc=0xD0;
		}
		else if(t>='0' && t<='9')
			tmpc=(t-'0')<<4;
		else
			throw invalid_argument("The string is not BCD");

		if(u==0 || i!=(len+1)/2-1)
		{
			t=(unsigned char)from[i*2+1];
			if(17<len && len<38 && !separator_found && (t=='^' || t=='=' || t=='D'))     //making one exception for track2
			{
				separator_found=1;
				tmpc|=0xD;
			}
			else if(t>='0' && t<='9')
				tmpc|=t-'0';
			else
			{
				to.push_back(tmpc);
				throw invalid_argument("The string is not BCD");
			}
		}
		else if(fillChar=='F')
			tmpc|=0xF;

		to.push_back(tmpc);
	}
	return (len+1)/2;
}

size_t field::build_hex(const string::const_iterator &from, string &to, size_t len, char fillChar)
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
				throw invalid_argument("The string is not HEX");
		}
		else if(fillChar=='F')
			tmpc=0xF0;

		t=(unsigned char)from[i*2+1-u];
		if(t>='0' && t<='9')
			tmpc|=t-'0';
		else if(t>='A' && t<='F')
			tmpc|=t-'A'+0xA;
		else if(t>='a' && t<='f')
			tmpc|=t-'a'+0xA;
		else
		{
			to.push_back(tmpc);
			throw invalid_argument("The string is not HEX");
		}
		to.push_back(tmpc);
	}
	return (len+1)/2;
}

size_t field::build_isobitmap(string &buf, unsigned int index) //TODO: const
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

