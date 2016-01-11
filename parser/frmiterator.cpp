#include <iterator>
#include "parser.h"

template<class T, typename iterator_type, typename reference_type>
frmiterator<T, iterator_type, reference_type>::frmiterator(void)
{
	return;
}

template<class T, typename iterator_type, typename reference_type>
frmiterator<T, iterator_type, reference_type>::frmiterator(const T *iwildcard, iterator_type iit, iterator_type ibegin, iterator_type iend, int icurnum) :
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

template<class T, typename iterator_type, typename reference_type>
frmiterator<T, iterator_type, reference_type>::frmiterator(const frmiterator &it) :
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

template<class T, typename iterator_type, typename reference_type>
frmiterator<T, iterator_type, reference_type>::~frmiterator(void)
{
	return;
}

template<class T, typename iterator_type, typename reference_type>
frmiterator<T, iterator_type, reference_type>& frmiterator<T, iterator_type, reference_type>::operator=(const frmiterator<T, iterator_type, reference_type> &other)
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

template<class T, typename iterator_type, typename reference_type>
bool frmiterator<T, iterator_type, reference_type>::operator!=(frmiterator<T, iterator_type, reference_type> const &other) const
{
	return it!=other.it || curnum!=other.curnum;
}

template<class T, typename iterator_type, typename reference_type>
bool frmiterator<T, iterator_type, reference_type>::operator==(frmiterator<T, iterator_type, reference_type> const &other) const
{
	return it==other.it && curnum==other.curnum;
}

template<class T, typename iterator_type, typename reference_type>
reference_type& frmiterator<T, iterator_type, reference_type>::operator*(void)
{
	return *this->operator->();
}

template<class T, typename iterator_type, typename reference_type>
reference_type* frmiterator<T, iterator_type, reference_type>::operator->(void)
{
	if(!wildcard || (it!=end && (it->first==curnum || curnum<0)))
		return it.operator->();
	else
	{
		tmpmap[curnum]=*wildcard; //construct a temporary map of the missing references
		return tmpmap.find(curnum).operator->();
	}
}

template<class T, typename iterator_type, typename reference_type>
frmiterator<T, iterator_type, reference_type>& frmiterator<T, iterator_type, reference_type>::operator++(void)
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

template<class T, typename iterator_type, typename reference_type>
frmiterator<T, iterator_type, reference_type>& frmiterator<T, iterator_type, reference_type>::operator--(void)
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

template class frmiterator<fldformat, std::map<int,fldformat>::iterator, std::pair<const int, fldformat> >;
template class frmiterator<fldformat, std::map<int,fldformat>::const_iterator, const std::pair<const int, fldformat> >;
