#ifndef _PARSE_H_
#define _PARSE_H_

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

extern int debug;

class fldformat;
class field;

//during format loading, the array of links would contain loaded formats without parents
typedef struct link
{
	char name[256];
	fldformat *frm;
} link;

fldformat *findFrmParent(link**, int*, char*, int*, fldformat *frm=NULL);
int parseFormat(fldformat*, char*, link**, int*);
int linkFrmChild(fldformat*, unsigned int, fldformat*, link*);
int findLinkNumber(link**, int*, const char*, int maxlen=-1, fldformat *frm=NULL);

field *parse_message(char*, unsigned int, fldformat*);
int parse_field_length(char*, unsigned int, fldformat*);
int parse_field(char*, unsigned int, field*);
int parse_field_alt(char*, unsigned int, field*);

unsigned int build_message(char*, unsigned int, field*);
unsigned int get_length(field*);
unsigned int build_field(char*, unsigned int, field*);
unsigned int build_field_alt(char*, unsigned int, field*);
unsigned int build_isobitmap(char*, unsigned int, field*, unsigned int);
unsigned int build_bitmap(char*, unsigned int, field*, unsigned int);

class fldformat
{
	private:
	unsigned int lengthFormat;
	unsigned int lengthLength;
	unsigned short lengthInclusive;
	unsigned int maxLength;
	int addLength;
	unsigned int dataFormat;
	unsigned int tagFormat;
	char *description;
	char *data;
	unsigned int maxFields;
	unsigned int fields;
	fldformat **fld;
	fldformat *altformat;

	void fill_default(void);

	friend field;
	friend fldformat *findFrmParent(link**, int*, char*, int*, fldformat*);
	friend int parseFormat(fldformat*, char*, link**, int*);
	friend int linkFrmChild(fldformat*, unsigned int, fldformat*, link*);
	friend int findLinkNumber(link**, int*, const char*, int, fldformat*);

	friend field *parse_message(char*, unsigned int, fldformat*);
	friend int parse_field(char*, unsigned int, field*);
	friend int parse_field_alt(char*, unsigned int, field*);
	friend int parse_field_length(char*, unsigned int, fldformat*);
	friend unsigned int build_message(char*, unsigned int, field*);
	friend unsigned int get_length(field*);
	friend unsigned int build_field(char*, unsigned int, field*);
	friend unsigned int build_field_alt(char*, unsigned int, field*);
	friend unsigned int build_isobitmap(char*, unsigned int, field*, unsigned int);
	friend unsigned int build_bitmap(char*, unsigned int, field*, unsigned int);

	public:
	fldformat(void);
	~fldformat(void);
	void clear(void);
	int is_empty(void);
	int load_format(char *filename);
	void copyFrom(fldformat *from);
	fldformat *get_altformat(void);
	const char *get_description(void);
};

class field
{
	private:
	char* data;  //parsed data
	char* tag;   //parsed TLV tag name
	unsigned int start;  //start position inside the message binary data relative to the parent field
	unsigned int blength;  //length of the field inside the message binary data (including length length)
	unsigned int length;  //parsed data length
	unsigned int fields;  //number of subfields
	fldformat *frm;  //field format
	struct field **fld;  //array of subfields
	unsigned int altformat;  //altformat number

	void fill_default(void);
	int change_format(fldformat *frmnew);

	friend field *parse_message(char*, unsigned int, fldformat*);
	friend int parse_field(char*, unsigned int, field*);
	friend int parse_field_alt(char*, unsigned int, field*);
	friend int parse_field_length(char*, unsigned int, fldformat*);
	friend unsigned int build_message(char*, unsigned int, field*);
	friend unsigned int get_length(field*);
	friend unsigned int build_field(char*, unsigned int, field*);
	friend unsigned int build_field_alt(char*, unsigned int, field*);
	friend unsigned int build_isobitmap(char*, unsigned int, field*, unsigned int);
	friend unsigned int build_bitmap(char*, unsigned int, field*, unsigned int);

	public:
	field(void);
	~field(void);
	void print_message(void);
	void clear(void);
	int is_empty(void);
	//int change_format(fldformat*);

	const char *get_description(void);
	inline int get_parsed_blength() {return this->blength;};

	const char* get_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	char* add_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	int field_format(int altformat, int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	void remove_field(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	int has_field(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	char* add_tag(const char *tag, int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	const char* get_tag(int n0=-1, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
};

#endif

