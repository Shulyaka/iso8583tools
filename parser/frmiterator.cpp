#include <iterator>
#include "parser.h"

frmiterator::frmiterator(void)
{
	return;
}

frmiterator::frmiterator(fldformat *iwildcard, std::map<int,fldformat>::iterator iit, std::map<int,fldformat>::iterator ibegin, std::map<int,fldformat>::iterator iend, int icurnum) :
	wildcard(iwildcard),
	it(iit),
	next(iit==iend?iend:++iit),
	begin(ibegin),
	end(iend),
	curnum(icurnum)
{
	while(it!=end && it->first<0)
		++it;
	return;
}

frmiterator::frmiterator(const frmiterator &it) :
	tmpmap(it.tmpmap),
	wildcard(it.wildcard),
	it(it.it),
	next(it.next),
	begin(it.begin),
	end(it.end),
	curnum(it.curnum)
{
	return;
}

frmiterator::~frmiterator(void)
{
	return;
}

frmiterator& frmiterator::operator=(const frmiterator &other)
{
	wildcard=other.wildcard;
	it=other.it;
	next=other.next;
	begin=other.begin;
	end=other.end;
	tmpmap=other.tmpmap;
	curnum=other.curnum;
	return *this;
}

bool frmiterator::operator!=(frmiterator const &other) const
{
	return it!=other.it || curnum!=other.curnum;
}

bool frmiterator::operator==(frmiterator const &other) const
{
	return it==other.it && curnum==other.curnum;
}

std::pair<const int, fldformat>& frmiterator::operator*(void)
{
	return *this->operator->();
}

std::pair<const int, fldformat>* frmiterator::operator->(void)
{
	if(!wildcard || (it!=end && (it->first==curnum || curnum<0)))
		return it.operator->();
	else
	{
		tmpmap[curnum]=*wildcard; //construct a temporary map of the missing references
		return tmpmap.find(curnum).operator->();
	}
}

frmiterator& frmiterator::operator++(void)
{
	if(!wildcard)
	{
		++it;
		return *this;
	}

	++curnum;

	if(next!=end && curnum==next->first)
	{
		it=next;
		++next;
	}
	return *this;
}

frmiterator& frmiterator::operator--(void)
{
	if(!wildcard)
	{
		--it;
		return *this;
	}

	if(it!=end && curnum==it->first)
	{
		next=it;
		--it;
	}

	--curnum;

	return *this;
}
