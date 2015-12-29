#ifndef _PARSER_H_
#define _PARSER_H_

#include <string>
#include <map>

extern int debug;

class fldformat;
class field;

class frmiterator;

class fldformat
{
	private:
	std::string description;
	enum fieldformat
	{
		fld_isobitmap,	// primary and secondary bitmaps	ISOBITMAP
		fld_bitmap,	// fixed length bitmap			BITMAP
		fld_subfields,	// the contents is split to subfields	SF
		fld_tlv,	// TLV subfields			TLV
		fld_ebcdic,	// EBCDIC				EBCDIC
		fld_bcd,	// Binary Coded Decimal			BCD
		fld_ascii,	// ASCII				ASC
		fld_hex,	// 0x0123FD -> "0123FD"			HEX
		fld_bcdsf,	// The field is first converted from BCD to ASCII, and then is split into subfields	BCDSF
		fld_bitstr	// Same as BITMAP but does not define subfields						BITMAP
	} dataFormat;
	enum lengthformat
	{
		fll_unknown,	// unknown format		U
		fll_ebcdic,	// 0xF1F2F3F4			E
		fll_bcd,	// 0x012345			C
		fll_bin,	// 12345			B
		fll_ascii,	// "12345"			L
		fll_fixed,	// Fixed length			F
		fll_ber		// Length format for EMV tags	R
	} lengthFormat;
	unsigned int lengthLength;
	unsigned short lengthInclusive;
	unsigned int maxLength;
	int addLength;
	enum tagformat
	{
		flt_ebcdic,	// 0xF1F2F3F4		EBCDIC
		flt_bcd,	// 0x012345		BCD
		flt_bin,	// 12345		BIN
		flt_ascii,	// 0x31323334		ASCII
		flt_ber		// EMV tag format	TLVBER
	} tagFormat;
	unsigned int tagLength;
	std::string data;
	std::map<int,fldformat> subfields;
	fldformat *altformat;
	fldformat *parent;

	void fill_default(void);
	inline fldformat *get_lastaltformat(void) {fldformat *last; for(last=this; last->altformat!=NULL; ) last=last->altformat; return last;};
	bool parseFormat(char*, std::map<std::string,fldformat> &orphans);
	fldformat* get_by_number(const char *number, std::map<std::string,fldformat> &orphans);

	friend field;

	public:
	typedef frmiterator iterator;
	iterator begin(void);
	iterator end(void);
	iterator find(int);

	fldformat(void);
	fldformat(const std::string &filename);
	fldformat(const fldformat&);
	~fldformat(void);
	void print_format(std::string prefix="");
	void clear(void);
	bool is_empty(void) const;
	bool load_format(const std::string &filename);
	void copyFrom(const fldformat &from);
	void moveFrom(fldformat &from);
	inline fldformat *get_altformat(void) const {return altformat;};
	inline const std::string& get_description(void) const {return description;};
	inline const unsigned int get_lengthLength(void) const {return lengthLength;};
	inline fldformat& sf(int n) {if(!subfields.count(n) && subfields.count(-1)) return subfields[-1]; else return subfields[n];};
	inline bool sfexist(int n) const {return subfields.count(n) || subfields.count(-1);};
	void erase(void);
};

class field
{
	private:
	std::string data;  //parsed data
	unsigned int start;  //start position inside the message binary data relative to the parent field
	unsigned int blength;  //length of the field inside the message binary data (including length length)
	unsigned int length;  //parsed data length
	fldformat *frm;  //field format
	fldformat *firstfrm;
	bool deletefrm;
	std::map<int,field> subfields;
	unsigned int altformat;  //altformat number

	void fill_default(void);

	int parse_field(const std::string::const_iterator&, const std::string::const_iterator&);
	int parse_field_alt(const std::string::const_iterator&, const std::string::const_iterator&);
	int parse_field_length(const std::string::const_iterator&, const std::string::const_iterator&);
	unsigned int build_field(std::string&);
	unsigned int build_field_length(std::string&);
	unsigned int build_field_alt(std::string&);
	unsigned int build_isobitmap(std::string&, unsigned int);
	unsigned int build_bitmap(std::string&, unsigned int);
	bool change_format(fldformat*);

	public:
	typedef std::map<int,field>::iterator iterator;
	typedef std::map<int,field>::const_iterator const_iterator;
	iterator begin(void) { return subfields.begin();};
	const_iterator begin(void) const { return subfields.begin();};
	iterator end(void) { return subfields.end();};
	const_iterator end(void) const { return subfields.end();};
	iterator find(int n) { return subfields.find(n);};
	const_iterator find(int n) const { return subfields.find(n);};
	typedef std::map<int,field>::reverse_iterator reverse_iterator;
	typedef std::map<int,field>::const_reverse_iterator const_reverse_iterator;
	reverse_iterator rbegin(void) { return subfields.rbegin();};
	const_reverse_iterator rbegin(void) const { return subfields.rbegin();};
	reverse_iterator rend(void) { return subfields.rend();};
	const_reverse_iterator rend(void) const { return subfields.rend();};

	field(void);
	field(fldformat *frm);
	field(const std::string &filename);
	field(const field&);
	~field(void);
	void print_message(std::string prefix="") const;
	void clear(void);
	bool is_empty(void) const;
	bool set_frm(fldformat*);
	bool switch_altformat(void);
	bool reset_altformat(void);
	void copyFrom(const field &from);
	void moveFrom(field &from);

	int parse_message(const std::string&);
	inline unsigned int build_message(std::string& buf) {return build_field(buf);};
	unsigned int get_blength(void);
	unsigned int get_flength(void);
	unsigned int get_mlength(void);

	inline const std::string& get_description(void) const {return frm->get_description();};
	inline const int get_parsed_blength(void) const {return blength;};
	inline const int get_lengthLength(void) const {return frm?frm->get_lengthLength():0;};
	field& sf(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	bool sfexist(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1) const;

	const std::string& get_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	std::string& add_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	int field_format(unsigned int altformat, int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	void remove_field(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	bool has_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
};

class frmiterator: public std::iterator<std::bidirectional_iterator_tag, std::pair<int,fldformat> >
{
	friend class fldformat;
	private:
	std::map<int,fldformat> tmpmap;
	fldformat *wildcard;
	std::map<int,fldformat>::iterator it;
	std::map<int,fldformat>::iterator next;
	std::map<int,fldformat>::iterator begin;
	std::map<int,fldformat>::iterator end;
	int curnum;

	frmiterator(fldformat*, std::map<int,fldformat>::iterator, std::map<int,fldformat>::iterator, std::map<int,fldformat>::iterator, int);

	public:
	frmiterator(void);
	frmiterator(const frmiterator &it);
	~frmiterator(void);

	frmiterator& operator=(const frmiterator &other);
	bool operator!=(frmiterator const& other) const;
	bool operator==(frmiterator const& other) const;
	std::pair<const int, fldformat>& operator*(void);
	std::pair<const int, fldformat>* operator->(void);
	frmiterator& operator++(void);
	frmiterator& operator--(void);
};

#endif

