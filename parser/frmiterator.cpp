#include <iterator>
#include "parser.h"

#define ITMPL typename iterator_type, typename reference_type, typename iterator_type_const, typename reference_type_const, typename iterator_type_nonconst, typename reference_type_nonconst
#define ITPAR iterator_type, reference_type, iterator_type_const, reference_type_const, iterator_type_nonconst, reference_type_nonconst
#define ITPAR_CONST iterator_type_const, reference_type_const, iterator_type_const, reference_type_const, iterator_type_nonconst, reference_type_nonconst

template<ITMPL>
frmiterator<ITPAR>::frmiterator(void)
{
	return;
}

template<ITMPL>
frmiterator<ITPAR>::frmiterator(const fldformat *iwildcard, iterator_type iit, iterator_type ibegin, iterator_type iend, int icurnum) :
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

template<ITMPL>
frmiterator<ITPAR>::frmiterator(const frmiterator &it) :
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

template<ITMPL>
frmiterator<ITPAR>::frmiterator(const std::map<int,fldformat> &i_tmpmap, const fldformat *i_wildcard, const iterator_type &i_it, const iterator_type &i_next, const iterator_type &i_begin, const iterator_type &i_end, int i_curnum) :
	tmpmap(i_tmpmap),
	wildcard(i_wildcard),
	it(i_it),
	next(i_next),
	begin(i_begin),
	end(i_end),
	curnum(i_curnum)
{
	return;
}

template<ITMPL>
frmiterator<ITPAR>::~frmiterator(void)
{
	return;
}

template<ITMPL>
frmiterator<ITPAR>& frmiterator<ITPAR>::operator=(const frmiterator<ITPAR> &other)
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

template<ITMPL>
bool frmiterator<ITPAR>::operator!=(frmiterator<ITPAR> const &other) const
{
	return it!=other.it || curnum!=other.curnum;
}

template<ITMPL>
bool frmiterator<ITPAR>::operator==(frmiterator<ITPAR> const &other) const
{
	return it==other.it && curnum==other.curnum;
}

template<ITMPL>
reference_type& frmiterator<ITPAR>::operator*(void)
{
	return *this->operator->();
}

template<ITMPL>
reference_type* frmiterator<ITPAR>::operator->(void)
{
	if(!wildcard || (it!=end && (it->first==curnum || curnum<0)))
		return it.operator->();
	else
	{
		tmpmap[curnum]=*wildcard; //construct a temporary map of the missing references
		return tmpmap.find(curnum).operator->();
	}
}

template<ITMPL>
frmiterator<ITPAR>& frmiterator<ITPAR>::operator++(void)
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

template<ITMPL>
frmiterator<ITPAR>& frmiterator<ITPAR>::operator--(void)
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

template<ITMPL>
frmiterator<ITPAR>::operator frmiterator<ITPAR_CONST>(void) const
{
	return frmiterator<ITPAR_CONST>(tmpmap, wildcard, it, next, begin, end, curnum);
}


template class frmiterator<std::map<int,fldformat>::iterator,	std::pair<const int, fldformat>,	std::map<int,fldformat>::const_iterator, const std::pair<const int, fldformat>, std::map<int,fldformat>::iterator, std::pair<const int, fldformat> >;
template class frmiterator<std::map<int,fldformat>::const_iterator,	const std::pair<const int, fldformat>,	std::map<int,fldformat>::const_iterator, const std::pair<const int, fldformat>, std::map<int,fldformat>::iterator, std::pair<const int, fldformat> >;
