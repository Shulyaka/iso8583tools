#ifndef _PARSER_H_
#define _PARSER_H_

//TODO: switch to enum
#define FRM_UNKNOWN 0    //unknown format    U
#define FRM_ISOBITMAP 1  //primary and secondary bitmaps
#define FRM_BITMAP 2     //fixed length bitmap
#define FRM_SUBFIELDS 3  //the contents is split to subfields     SF
#define FRM_TLV1 4       //TLV subfields, 1 byte tag format      TLV1
#define FRM_TLV2 5       //TLV subfields, 2 bytes tag format     TLV2
#define FRM_TLV3 6       //TLV subfields, 3 bytes tag format     TLV3
#define FRM_TLV4 7       //TLV subfields, 4 bytes tag format     TLV4
#define FRM_TLVEMV 8     //TLV subfields, EMV tag format (BER)   TLVE
#define FRM_EBCDIC 9    // EBCDIC EEEEE
#define FRM_BCD 10   //  0x012345  CCCCC  BCD
#define FRM_BIN 11   //  12345     BBBBB  BIN
#define FRM_ASCII 12  // "12345"   LLLLL  ASC
#define FRM_FIXED 13     // Fixed length   F12345
#define FRM_HEX	14	// 0x0123FD -> "0123FD"
#define FRM_BCDSF 15	// The field is first converted from BCD to ASCII, and then is split into subfields
#define FRM_BITSTR 16	//Same as BITMAP but does not define subfields
#define FRM_EMVL 17     // Length format for EMV tags

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
	unsigned int lengthFormat;
	unsigned int lengthLength;
	unsigned short lengthInclusive;
	unsigned int maxLength;
	int addLength;
	unsigned int dataFormat;
	unsigned int tagFormat;
	std::string data;
	std::map<int,fldformat> subfields;
	fldformat *altformat;
	fldformat *parent;

	void fill_default(void);

	int parseFormat(char*, std::map<std::string,fldformat> &orphans);
	fldformat* get_by_number(const char *number, std::map<std::string,fldformat> &orphans);

	friend field;

	public:
	typedef frmiterator iterator;
	iterator begin();
	iterator end();
	iterator find(int);

	fldformat(void);
	fldformat(const fldformat&);
	~fldformat(void);
	void clear(void);
	int is_empty(void);
	int load_format(const std::string &filename);
	void copyFrom(const fldformat &from);
	void moveFrom(fldformat &from);
	inline fldformat *get_altformat(void);
	inline fldformat *get_lastaltformat(void);
	const std::string& get_description(void);
	inline const unsigned int get_lengthLength() {return lengthLength;};
	fldformat& sf(int n);
	bool sfexist(int n) const;
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

	public:
	typedef std::map<int,field>::iterator iterator;
	typedef std::map<int,field>::const_iterator const_iterator;
	iterator begin() { return subfields.begin();};
	const_iterator begin() const { return subfields.begin();};
	iterator end() { return subfields.end();};
	const_iterator end() const { return subfields.end();};
	iterator find(int n) { return subfields.find(n);};
	const_iterator find(int n) const { return subfields.find(n);};
	typedef std::map<int,field>::reverse_iterator reverse_iterator;
	typedef std::map<int,field>::const_reverse_iterator const_reverse_iterator;
	reverse_iterator rbegin() { return subfields.rbegin();};
	const_reverse_iterator rbegin() const { return subfields.rbegin();};
	reverse_iterator rend() { return subfields.rend();};
	const_reverse_iterator rend() const { return subfields.rend();};

	field(void);
	field(const field&);
	~field(void);
	void print_message(std::string prefix="") const;
	void clear(void);
	int is_empty(void) const;
	int change_format(fldformat*);
	void copyFrom(const field &from);
	void moveFrom(field &from);

	int parse_message(const std::string&);
	unsigned int build_message(std::string&);
	unsigned int get_blength(void);
	unsigned int get_flength(void);
	unsigned int get_mlength(void);

	const std::string& get_description(void);
	inline const int get_parsed_blength() {return blength;};
	inline const int get_lengthLength() {return frm?frm->get_lengthLength():0;};
	field& sf(int n);
	bool sfexist(int n) const;

	const std::string& get_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	std::string& add_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	int field_format(int altformat, int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	void remove_field(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	bool has_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
};

class frmiterator: public std::iterator<std::bidirectional_iterator_tag, std::pair<int,fldformat> >
{
	friend class fldformat;
	private:
	fldformat *wildcard;
	std::map<int,fldformat>::iterator it;
	std::map<int,fldformat>::iterator begin;
	std::map<int,fldformat>::iterator end;
	std::map<int,fldformat>::iterator next;
	int curnum;
	std::map<int,fldformat> tmpmap;

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

