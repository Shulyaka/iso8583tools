#ifndef _PARSER_H_
#define _PARSER_H_

#include <string>
#include <map>
#include <cstdarg>

extern int debug;

class fldformat;
class field;

template<class T=fldformat, typename iterator_type=typename std::map<int,T>::iterator, typename reference_type=typename std::pair<const int,T>, typename iterator_type_const=typename std::map<int,T>::const_iterator, typename reference_type_const=const reference_type, typename iterator_type_nonconst=iterator_type, typename reference_type_nonconst=reference_type>
class frmiterator;

template<typename iterator_type, typename reference_type>
class flditerator;

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
		fll_ber		// Length format for EMV tags	M
	} lengthFormat;
	size_t lengthLength;
	bool lengthInclusive;
	size_t maxLength;
	long int addLength;
	enum tagformat
	{
		flt_ebcdic,	// 0xF1F2F3F4		EBCDIC
		flt_bcd,	// 0x012345		BCD
		flt_bin,	// 12345		BIN
		flt_ascii,	// 0x31323334		ASCII
		flt_ber		// EMV tag format	TLVBER
	} tagFormat;
	size_t tagLength;
	std::string data;
	std::map<int,fldformat> subfields;
	fldformat *altformat;
	fldformat *parent;
	int hasBitmap;

	void fill_default(void);
	inline fldformat *get_lastaltformat(void) {fldformat *last; for(last=this; last->altformat!=NULL; ) last=last->altformat; return last;};
	bool parseFormat(const char*, std::map<std::string,fldformat> &orphans);
	fldformat* get_by_number(const char *number, std::map<std::string,fldformat> &orphans, bool isBitmap=false);

	friend field;

	public:
	typedef frmiterator<> iterator;
	typedef frmiterator<fldformat, std::map<int,fldformat>::const_iterator, const std::pair<const int, fldformat>, std::map<int,fldformat>::const_iterator, const std::pair<const int, fldformat>, std::map<int,fldformat>::iterator, std::pair<const int, fldformat> > const_iterator;
	iterator begin(void);
	const_iterator begin(void) const;
	iterator end(void);
	const_iterator end(void) const;
	iterator find(int);
	const_iterator find(int) const;
	//there are no reverse iterators because we may have a wildcard format which implies infinite number if subformats

	fldformat(void);
	fldformat(const std::string &filename);
	fldformat(const fldformat&);
	~fldformat(void);
	void print_format(std::string prefix="");
	void clear(void);
	bool empty(void) const;
	bool load_format(const std::string &filename);
	fldformat& operator= (const fldformat &from);
	void moveFrom(fldformat &from);
	inline fldformat *get_altformat(void) const {return altformat;};
	inline const std::string& get_description(void) const {return description;};
	inline const size_t get_lengthLength(void) const {return lengthLength;};
	inline fldformat& sf(int n) {if(!subfields.count(n) && subfields.count(-1)) return subfields[-1]; else return subfields[n];};
	const fldformat& sf(int n) const;
	inline bool sfexist(int n) const {return subfields.count(n) || subfields.count(-1);};
	void erase(void);
};

class field
{
	private:
	std::string data;  //parsed data
	size_t start;  //start position inside the message binary data relative to the parent field
	size_t blength;  //length of the field inside the message binary data (including length length)
	size_t flength;  //parsed data length
	const fldformat *frm;  //field format
	const fldformat *firstfrm;
	bool deletefrm;
	std::map<int,field> subfields;
	unsigned int altformat;  //altformat number
	std::map<int,int> tagmap;  //TODO: multimap for multiple tags support
	int tag;

	void fill_default(void);

	long int parse_field(const char*, size_t);
	long int parse_field_alt(const char*, size_t);
	long int parse_field_length(const char*, size_t);
	size_t build_field(std::string&);
	size_t build_field_length(std::string&);
	size_t build_field_alt(std::string&);
	size_t build_isobitmap(std::string&, unsigned int);
	size_t build_bitmap(std::string&, unsigned int);
	bool change_format(const fldformat*);

	public:
	typedef flditerator<std::map<int,field>::iterator, std::pair<const int,field> > iterator;
	typedef flditerator<std::map<int,field>::const_iterator, const std::pair<const int,field> > const_iterator;
	iterator begin(void);
	const_iterator begin(void) const;
	iterator end(void);
	const_iterator end(void) const;
	iterator find(int n);
	const_iterator find(int n) const;
	typedef std::reverse_iterator<field::iterator> reverse_iterator;
	typedef std::reverse_iterator<field::const_iterator> const_reverse_iterator;
	reverse_iterator rbegin(void);
	const_reverse_iterator rbegin(void) const;
	reverse_iterator rend(void);
	const_reverse_iterator rend(void) const;

	field(const std::string &str=""); //TODO: add more constructors
	field(const fldformat *frm, const std::string &str="");
	field(const std::string &filename, const std::string &str="");
	field(const field&);
	~field(void);
	void print_message(std::string prefix="") const;
	void clear(void);
	bool empty(void) const;
	bool set_frm(const fldformat* firstfrm, const fldformat* altfrm=NULL);
	bool switch_altformat(void);
	bool reset_altformat(void);
	void moveFrom(field &from);
	void swap(field &from);
	field& operator= (const field &from);

	long int parse(const std::string&);
	long int parse(const char*, size_t);
	inline size_t serialize(std::string& buf) {return build_field(buf);}; //TODO: Consider changing the return type to std::string
	size_t serialize(char*, size_t);
	size_t get_blength(void);
	size_t get_flength(void);
	size_t get_mlength(void);
	inline size_t get_maxLength(void) {return frm->maxLength;}; //TODO: Loop through all altformats

	inline const std::string& get_description(void) const {return frm->get_description();};
	inline const size_t get_cached_blength(void) const {return blength;};
	inline const size_t get_lengthLength(void) const {return frm?frm->get_lengthLength():0;};
	field& sf(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1);
	inline field& operator() (int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1)
		{return sf(n0, n1, n2, n3, n4, n5, n6, n7, n8, n9);};
	bool sfexist(int n0, int n1=-1, int n2=-1, int n3=-1, int n4=-1, int n5=-1, int n6=-1, int n7=-1, int n8=-1, int n9=-1) const;

	//cstdio API:
	//operator const char*() const { return c_str(); } //removed to avoid ambigous overloads. Use c_str() directly instead.
	inline int sprintf(const char *format, ...) {va_list args; va_start(args, format); int count=vsprintf(format, args); va_end(args); return count;};
	inline int sprintf(size_t pos, const char *format, ...) {va_list args; va_start(args, format); int count=vsprintf(pos, format, args); va_end(args); return count;};
	inline int snprintf(size_t size, const char *format, ...) {va_list args; va_start(args, format); int count=vsnprintf(size, format, args); va_end(args); return count;};
	inline int snprintf(size_t pos, size_t size, const char *format, ...) {va_list args; va_start(args, format); int count=vsnprintf(pos, size, format, args); va_end(args); return count;};
	inline int vsprintf(const char *format, va_list ap) {return vsprintf(0, format, ap);};
	inline int vsprintf(size_t pos, const char *format, va_list ap) {return vsnprintf(pos, get_maxLength(), format, ap);};
	inline int vsnprintf(size_t size, const char *format, va_list ap) {return vsnprintf(0, size, format, ap);};
	int vsnprintf(size_t pos, size_t size, const char *format, va_list ap);
	inline size_t strftime(const char *format, const struct tm *tm) {return strftime(get_maxLength(), format, tm);};
	size_t strftime(size_t max, const char *format, const struct tm *tm);

	//std::string API:
	operator std::string() const { return data; }
	inline field& operator= (const std::string& str) {data=str; return *this;};
	inline field& operator= (const char* s) {data=s; return *this;};
	inline field& operator= (char c) {data=c; return *this;};

	inline std::string::iterator dbegin(void) {return data.begin();};
	inline std::string::const_iterator dbegin(void) const {return data.begin();};
	inline std::string::iterator dend(void) {return data.end();};
	inline std::string::const_iterator dend(void) const {return data.end();};
	inline std::string::reverse_iterator drbegin(void) {return data.rbegin();};
	inline std::string::const_reverse_iterator drbegin(void) const {return data.rbegin();};
	inline std::string::reverse_iterator drend(void) {return data.rend();};
	inline std::string::const_reverse_iterator drend(void) const {return data.rend();};

	inline size_t length(void) {return get_flength();};
	inline size_t size(void) {return get_flength();};
	inline size_t max_size(void) const {return data.max_size()<=frm->maxLength ? data.max_size() : frm->maxLength;};
	inline void resize(size_t n) {data.resize(n);};
	inline void resize(size_t n, char c) {data.resize(n, c);};
	inline size_t capacity(void) const {return data.capacity();};
	inline void reserve(size_t n=0) {data.reserve(n);};

	inline char& operator[] (size_t pos) {return data[pos];};
	inline const char& operator[] (size_t pos) const {return data[pos];};
	inline char& at (size_t pos) {return data.at(pos);};
	inline const char& at (size_t pos) const {return data.at(pos);};

	inline field& operator+= (const field& f) {data+=f.data; return *this;};
	inline field& operator+= (const std::string& str) {data+=str; return *this;};
	inline field& operator+= (const char* s) {data+=s; return *this;};
	inline field& operator+= (char c) {data+=c; return *this;};

	inline field& append (const field& f) {data.append(f.data); return *this;};
	inline field& append (const std::string& str) {data.append(str); return *this;};
	inline field& append (const field& f, size_t subpos, size_t sublen) {data.append(f.data, subpos, sublen); return *this;};
	inline field& append (const std::string& str, size_t subpos, size_t sublen) {data.append(str, subpos, sublen); return *this;};
	inline field& append (const char* s) {data.append(s); return *this;};
	inline field& append (const char* s, size_t n) {data.append(s, n); return *this;};
	inline field& append (size_t n, char c) {data.append(n, c); return *this;};
	template <class InputIterator>
		inline field& append (InputIterator first, InputIterator last) {data.append(first, last); return *this;};
	inline void push_back (char c) {data.push_back(c);};
	inline field& assign (const field& f) {data.assign(f.data); return *this;};
	inline field& assign (const std::string& str) {data.assign(str); return *this;};
	inline field& assign (const field& f, size_t subpos, size_t sublen) {data.assign(f.data, subpos, sublen); return *this;};
	inline field& assign (const std::string& str, size_t subpos, size_t sublen) {data.assign(str, subpos, sublen); return *this;};
	inline field& assign (const char* s) {data.assign(s); return *this;};
	inline field& assign (const char* s, size_t n) {data.assign(s, n); return *this;};
	inline field& assign (size_t n, char c) {data.assign(n, c); return *this;};
	template <class InputIterator>
		inline field& assign (InputIterator first, InputIterator last) {data.assign(first, last); return *this;};
	inline field& insert (size_t pos, const field& f) {data.insert(pos, f.data); return *this;};
	inline field& insert (size_t pos, const std::string& str) {data.insert(pos, str); return *this;};
	inline field& insert (size_t pos, const field& f, size_t subpos, size_t sublen) {data.insert(pos, f.data, subpos, sublen); return *this;};
	inline field& insert (size_t pos, const std::string& str, size_t subpos, size_t sublen) {data.insert(pos, str, subpos, sublen); return *this;};
	inline field& insert (size_t pos, const char* s) {data.insert(pos, s); return *this;};
	inline field& insert (size_t pos, const char* s, size_t n) {data.insert(pos, s, n); return *this;};
	inline field& insert (size_t pos, size_t n, char c) {data.insert(pos, n, c); return *this;};
	inline void insert (std::string::iterator p, size_t n, char c) {data.insert(p, n, c);};
	inline std::string::iterator insert (std::string::iterator p, char c) {return data.insert(p, c);};
	template <class InputIterator>
		inline void insert (std::string::iterator p, InputIterator first, InputIterator last) {data.insert(p, first, last);};
	inline field& erase (size_t pos = 0, size_t len = npos) {data.erase(pos, len); return *this;};
	inline std::string::iterator erase (std::string::iterator p) {return data.erase(p);};
	inline std::string::iterator erase (std::string::iterator first, std::string::iterator last) {return data.erase(first, last);};
	inline field& replace (size_t pos, size_t len, const field& f) {data.replace(pos, len, f.data); return *this;};
	inline field& replace (size_t pos, size_t len, const std::string& str) {data.replace(pos, len, str); return *this;};
	inline field& replace (std::string::iterator i1, std::string::iterator i2, const field& f) {data.replace(i1, i2, f.data); return *this;};
	inline field& replace (std::string::iterator i1, std::string::iterator i2, const std::string& str) {data.replace(i1, i2, str); return *this;};
	inline field& replace (size_t pos, size_t len, const field& f, size_t subpos, size_t sublen) {data.replace(pos, len, f.data, subpos, sublen); return *this;};
	inline field& replace (size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen) {data.replace(pos, len, str, subpos, sublen); return *this;};
	inline field& replace (size_t pos, size_t len, const char* s) {data.replace(pos, len, s); return *this;};
	inline field& replace (std::string::iterator i1, std::string::iterator i2, const char* s) {data.replace(i1, i2, s); return *this;};
	inline field& replace (size_t pos, size_t len, const char* s, size_t n) {data.replace(pos, len, s, n); return *this;};
	inline field& replace (std::string::iterator i1, std::string::iterator i2, const char* s, size_t n) {data.replace(i1, i2, s, n); return *this;};
	inline field& replace (size_t pos, size_t len, size_t n, char c) {data.replace(pos, len, n, c); return *this;};
	inline field& replace (std::string::iterator i1, std::string::iterator i2, size_t n, char c) {data.replace(i1, i2, n, c); return *this;};
	template <class InputIterator>
		inline field& replace (std::string::iterator i1, std::string::iterator i2, InputIterator first, InputIterator last) {data.replace(i1, i2, first, last); return *this;};

	inline const char* c_str() const {return data.c_str();};
	inline size_t copy (char* s, size_t len, size_t pos = 0) const {return data.copy(s, len, pos);};
	inline size_t find (const field& f, size_t pos = 0) const {return data.find(f.data, pos);};
	inline size_t find (const std::string& str, size_t pos = 0) const {return data.find(str, pos);};
	inline size_t find (const char* s, size_t pos = 0) const {return data.find(s, pos);};
	inline size_t find (const char* s, size_t pos, size_t n) const {return data.find(s, pos, n);};
	inline size_t find (char c, size_t pos = 0) const {return data.find(c, pos);};
	inline size_t rfind (const field& f, size_t pos = npos) const {return data.rfind(f.data, pos);};
	inline size_t rfind (const std::string& str, size_t pos = npos) const {return data.rfind(str, pos);};
	inline size_t rfind (const char* s, size_t pos = npos) const {return data.rfind(s, pos);};
	inline size_t rfind (const char* s, size_t pos, size_t n) const {return data.rfind(s, pos, n);};
	inline size_t rfind (char c, size_t pos = npos) const {return data.rfind(c, pos);};
	inline size_t find_first_of (const field& f, size_t pos = 0) const {return data.find_first_of(f.data, pos);};
	inline size_t find_first_of (const std::string& str, size_t pos = 0) const {return data.find_first_of(str, pos);};
	inline size_t find_first_of (const char* s, size_t pos = 0) const {return data.find_first_of(s, pos);};
	inline size_t find_first_of (const char* s, size_t pos, size_t n) const {return data.find_first_of(s, pos, n);};
	inline size_t find_first_of (char c, size_t pos = 0) const {return data.find_first_of(c, pos);};
	inline size_t find_last_of (const field& f, size_t pos = npos) const {return data.find_last_of(f.data, pos);};
	inline size_t find_last_of (const std::string& str, size_t pos = npos) const {return data.find_last_of(str, pos);};
	inline size_t find_last_of (const char* s, size_t pos = npos) const {return data.find_last_of(s, pos);};
	inline size_t find_last_of (const char* s, size_t pos, size_t n) const {return data.find_last_of(s, pos, n);};
	inline size_t find_last_of (char c, size_t pos = npos) const {return data.find_last_of(c, pos);};
	inline size_t find_first_not_of (const field& f, size_t pos = 0) const {return data.find_first_not_of(f.data, pos);};
	inline size_t find_first_not_of (const std::string& str, size_t pos = 0) const {return data.find_first_not_of(str, pos);};
	inline size_t find_first_not_of (const char* s, size_t pos = 0) const {return data.find_first_not_of(s, pos);};
	inline size_t find_first_not_of (const char* s, size_t pos, size_t n) const {return data.find_first_not_of(s, pos, n);};
	inline size_t find_first_not_of (char c, size_t pos = 0) const {return data.find_first_not_of(c, pos);};
	inline size_t find_last_not_of (const field& f, size_t pos = npos) const {return data.find_last_not_of(f.data, pos);};
	inline size_t find_last_not_of (const std::string& str, size_t pos = npos) const {return data.find_last_not_of(str, pos);};
	inline size_t find_last_not_of (const char* s, size_t pos = npos) const {return data.find_last_not_of(s, pos);};
	inline size_t find_last_not_of (const char* s, size_t pos, size_t n) const {return data.find_last_not_of(s, pos, n);};
	inline size_t find_last_not_of (char c, size_t pos = npos) const {return data.find_last_not_of(c, pos);};
	inline std::string substr (size_t pos = 0, size_t len = npos) const {return data.substr(pos, len);};
	int compare (const field& f) const;
	inline int compare (const std::string& str) const {return data.compare(str);};
	inline int compare (size_t pos, size_t len, const field& f) const {return data.compare(pos, len, f.data);};
	inline int compare (size_t pos, size_t len, const std::string& str) const {return data.compare(pos, len, str);};
	inline int compare (size_t pos, size_t len, const field& f, size_t subpos, size_t sublen) const {return data.compare(pos, len, f.data, subpos, sublen);};
	inline int compare (size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen) const {return data.compare(pos, len, str, subpos, sublen);};
	inline int compare (const char* s) const {return data.compare(s);};
	inline int compare (size_t pos, size_t len, const char* s) const {return data.compare(pos, len, s);};
	inline int compare (size_t pos, size_t len, const char* s, size_t n) const {return data.compare(pos, len, s, n);};

	static const size_t npos = std::string::npos;

	friend inline field operator+ (const field& lhs, const field& rhs) {field res(lhs); return res+=rhs.data;};
	friend inline field operator+ (const field& lhs, const std::string rhs) {field res(lhs); return res+=rhs;};
	friend inline field operator+ (const std::string lhs, const field& rhs) {field res(rhs); res.data=lhs+rhs.data; return res;};
	friend inline field operator+ (const field& lhs, const char* rhs) {field res(lhs); return res+=rhs;};
	friend inline field operator+ (const char* lhs, const field& rhs) {field res(rhs); res.data=lhs+rhs.data; return res;};
	friend inline field operator+ (const field& lhs, char rhs) {field res(lhs); return res+=rhs;};
	friend inline field operator+ (char lhs, const field& rhs) {field res(rhs); res.data=lhs+rhs.data; return res;};
	friend inline bool operator== (const field&  lhs, const field&  rhs) {return !lhs.compare(rhs);};
	friend inline bool operator== (const field&  lhs, const std::string& rhs) {return lhs.data==rhs;};
	friend inline bool operator== (const std::string& lhs, const field&  rhs) {return lhs==rhs.data;};
	friend inline bool operator== (const char*   lhs, const field&  rhs) {return lhs==rhs.data;};
	friend inline bool operator== (const field&  lhs, const char*   rhs) {return lhs.data==rhs;};
	friend inline bool operator!= (const field&  lhs, const field&  rhs) {return lhs.compare(rhs);};
	friend inline bool operator!= (const field&  lhs, const std::string& rhs) {return lhs.data!=rhs;};
	friend inline bool operator!= (const std::string& lhs, const field&  rhs) {return lhs!=rhs.data;};
	friend inline bool operator!= (const char*   lhs, const field&  rhs) {return lhs!=rhs.data;};
	friend inline bool operator!= (const field&  lhs, const char*   rhs) {return lhs.data!=rhs;};
	friend inline bool operator<  (const field&  lhs, const field&  rhs) {return lhs.compare(rhs)>0;};
	friend inline bool operator<  (const field&  lhs, const std::string& rhs) {return lhs.data<rhs;};
	friend inline bool operator<  (const std::string& lhs, const field&  rhs) {return lhs<rhs.data;};
	friend inline bool operator<  (const char*   lhs, const field&  rhs) {return lhs<rhs.data;};
	friend inline bool operator<  (const field&  lhs, const char*   rhs) {return lhs.data<rhs;};
	friend inline bool operator<= (const field&  lhs, const field&  rhs) {return lhs.compare(rhs)>=0;};
	friend inline bool operator<= (const field&  lhs, const std::string& rhs) {return lhs.data<=rhs;};
	friend inline bool operator<= (const std::string& lhs, const field&  rhs) {return lhs<=rhs.data;};
	friend inline bool operator<= (const char*   lhs, const field&  rhs) {return lhs<=rhs.data;};
	friend inline bool operator<= (const field&  lhs, const char*   rhs) {return lhs.data<=rhs;};
	friend inline bool operator>  (const field&  lhs, const field&  rhs) {return lhs.compare(rhs)<0;};
	friend inline bool operator>  (const field&  lhs, const std::string& rhs) {return lhs.data>rhs;};
	friend inline bool operator>  (const std::string& lhs, const field&  rhs) {return lhs>rhs.data;};
	friend inline bool operator>  (const char*   lhs, const field&  rhs) {return lhs>rhs.data;};
	friend inline bool operator>  (const field&  lhs, const char*   rhs) {return lhs.data>rhs;};
	friend inline bool operator>= (const field&  lhs, const field&  rhs) {return lhs.compare(rhs)<=0;};
	friend inline bool operator>= (const field&  lhs, const std::string& rhs) {return lhs.data>=rhs;};
	friend inline bool operator>= (const std::string& lhs, const field&  rhs) {return lhs>=rhs.data;};
	friend inline bool operator>= (const char*   lhs, const field&  rhs) {return lhs>=rhs.data;};
	friend inline bool operator>= (const field&  lhs, const char*   rhs) {return lhs.data>=rhs;};

	friend inline std::istream& operator>> (std::istream& is, field& f) {return is>>f.data;}; //TODO: parse from istream
	friend inline std::ostream& operator<< (std::ostream& os, const field& f) {return os<<f.data;}; //TODO: build to ostream
	friend inline std::istream& getline (std::istream& is, field& f, char delim) {return getline(is, f.data, delim);};
	//friend inline std::istream& getline (std::istream& is, field& f) {return getline(is, f.data);};
};

template<class T, typename iterator_type, typename reference_type, typename iterator_type_const, typename reference_type_const, typename iterator_type_nonconst, typename reference_type_nonconst>
class frmiterator: public std::iterator<std::bidirectional_iterator_tag, std::pair<int,T> >
{
	friend T;
	private:
	std::map<int,T> tmpmap;
	const T *wildcard;
	iterator_type it;
	iterator_type next;
	iterator_type begin;
	iterator_type end;
	int curnum;

	frmiterator(const T*, iterator_type, iterator_type, iterator_type, int);
	frmiterator(const std::map<int,T>&, const T*, const iterator_type&, const iterator_type&, const iterator_type&, const iterator_type&, int);

	public:
	frmiterator(void);
	frmiterator(const frmiterator &it);
	~frmiterator(void);

	frmiterator& operator=(const frmiterator &other);
	bool operator!=(frmiterator const& other) const;
	bool operator==(frmiterator const& other) const;
	reference_type& operator*(void);
	reference_type* operator->(void);
	frmiterator& operator++(void);
	frmiterator& operator--(void);

	friend frmiterator<T, iterator_type_nonconst, reference_type_nonconst, iterator_type_const, reference_type_const, iterator_type_nonconst, reference_type_nonconst>;
	operator frmiterator<T, iterator_type_const, reference_type_const, iterator_type_const, reference_type_const, iterator_type_nonconst, reference_type_nonconst>(void) const;
};

template<typename iterator_type, typename reference_type>
class flditerator: public std::iterator<std::bidirectional_iterator_tag, std::pair<int,field> >
{
	friend field;
	private:
	iterator_type it;
	flditerator(iterator_type mapit) : it(mapit) {};

	public:
	flditerator(void) {};
	flditerator(const flditerator &other) : it(other.it) {};
	~flditerator(void) {};

	flditerator& operator=(const flditerator &other) {it=other.it; return *this;};
	bool operator!=(flditerator const& other) const {return it!=other.it;};
	bool operator==(flditerator const& other) const {return it==other.it;};
	reference_type& operator*(void) {return *it;};
	reference_type* operator->(void) {return it.operator->();};
	flditerator& operator++(void) {++it; return *this;};
	flditerator& operator--(void) {--it; return *this;};

	friend flditerator<std::map<int,field>::iterator, std::pair<const int,field> >;
	operator flditerator<std::map<int,field>::const_iterator, const reference_type>(void) const {return flditerator<std::map<int,field>::const_iterator, const reference_type>(it);};
};

#endif

