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
#define FRM_TLVEMV 8     //TLV subfields, EMV tag format         TLVE
#define FRM_EBCDIC 9    // EBCDIC EEEEE
#define FRM_BCD 10   //  0x012345  CCCCC  BCD
#define FRM_BIN 11   //  12345     BBBBB  BIN
#define FRM_ASCII 12  // "12345"   LLLLL  ASC
#define FRM_FIXED 13     // Fixed length   F12345
#define FRM_HEX	14	// 0x0123FD -> "0123FD"
#define FRM_BCDSF 15	// The field is first converted from BCD to ASCII, and then is split into subfields
#define FRM_BITSTR 16	//Same as BITMAP but does not define subfields
#define FRM_EMVL 17     // Length format for EMV tags
#define FRM_TLVDS 18

#include <stdlib.h>

#include <string>
#include <map>
using namespace std;

extern int debug;

class fldformat;
class field;

class fldformat
{
	private:
	char *description;
	unsigned int lengthFormat;
	unsigned int lengthLength;
	unsigned short lengthInclusive;
	unsigned int maxLength;
	int addLength;
	unsigned int dataFormat;
	unsigned int tagFormat;
	char *data;
	map<int,fldformat> subfields;
	fldformat *altformat;
	fldformat *parent;

	void fill_default(void);

	int parseFormat(char*, map<string,fldformat> &orphans);
	fldformat* get_by_number(const char *number, map<string,fldformat> &orphans);

	friend field;

	public:
	fldformat(void);
	fldformat(const fldformat&);
	~fldformat(void);
	void clear(void);
	int is_empty(void);
	int load_format(const char *filename);
	void copyFrom(const fldformat &from);
	void moveFrom(fldformat &from);
	inline fldformat *get_altformat(void);
	inline fldformat *get_lastaltformat(void);
	const char *get_description(void);
	inline const unsigned int get_lengthLength() {return lengthLength;};
	fldformat& sf(int n);
	bool sfexist(int n) const;
	void erase(void);
};

class field
{
	private:
	char* data;  //parsed data
	char* tag;   //parsed TLV tag name
	unsigned int start;  //start position inside the message binary data relative to the parent field
	unsigned int blength;  //length of the field inside the message binary data (including length length)
	unsigned int length;  //parsed data length
	fldformat *frm;  //field format
	map<int,field> subfields;
	unsigned int altformat;  //altformat number

	void fill_default(void);

	int parse_field(const char*, unsigned int);
	int parse_field_alt(const char*, unsigned int);
	int parse_field_length(const char*, unsigned int);
	unsigned int build_field(char*, unsigned int);
	unsigned int build_field_alt(char*, unsigned int);
	unsigned int build_isobitmap(char*, unsigned int, unsigned int);
	unsigned int build_bitmap(char*, unsigned int, unsigned int);

	public:
	field(void);
	field(const field&);
	~field(void);
	void print_message(void) const;
	void clear(void);
	int is_empty(void) const;
	int change_format(fldformat*);
	void copyFrom(const field &from);
	void moveFrom(field &from);

	int parse_message(const char*, unsigned int);
	inline unsigned int build_message(char *buf, unsigned int len) {return build_field(buf, len);};
	unsigned int estimate_length(void);

	const char *get_description(void);
	inline const int get_parsed_blength() {return blength;};
	inline const int get_lengthLength() {return frm?frm->get_lengthLength():0;};
	field& sf(int n);
	bool sfexist(int n) const;

	const char* get_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	char* add_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	int field_format(int altformat, int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	void remove_field(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	int has_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	char* add_tag(const char *tag, int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	const char* get_tag(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
};

#endif

