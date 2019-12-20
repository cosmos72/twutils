/*
 * TList.h, a simple double-linked list template class
 * 
 * Copyright (C) 2001 Massimiliano Ghilardi
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

class s_TT {
  public:
    s_TT *Prev, *Next;
    const void *V;
};
typedef s_TT *TT;

class TListV {
  public:
    TListV()				{ Count = 0; Head = Tail = Current = (TT)0; }
    ~TListV()				{ deleteTT(); }
    inline int   count( )   const	{ return Count; }
    inline void *current( ) const	{ return getV(getCurrent()); }
    inline void *first( )		{ return getV(setCurrent(getHead())); }
    inline void *last( )		{ return getV(setCurrent(getTail())); }
    inline void *next( )		{ return getV(setCurrent(getNext())); }
    inline void *prev( )		{ return getV(setCurrent(getPrev())); }
    inline void *getFirst()		{ return first(); }
    inline void *getLast()		{ return last(); }
    inline bool  clear( )		{ return deleteTT(); }
    inline bool  remove( )		{ return deleteTT(getCurrent()); }
    inline bool  removeFirst( )		{ first(); return remove(); }
    inline bool  removeLast( )		{  last(); return remove(); }
    void append( const void *d )
    {
	TT node = new s_TT();

	node->Prev = Tail;
	node->Next = (TT)0;
	node->V = d;
	if (Tail)
	    Tail->Next = node;
	else
	    Head = node;
	Tail = node;
	Count++;
    }
    void inSort( const void *d, int (*compar)(const void *, const void *) )
    {
	/* start searching from current position */
	const void *C;
	if (!current())
	    first();
	if (!(C = current())) {
	    append(d);
	    return;
	}
	int i = (*compar)(d, C);
	if (i < 0) {
	    while ((C = prev()) && (*compar)(d, C) < 0)
		;
	    insert(d, Current, Current ? Current->Next : Head);
	} else if (i > 0) {
	    while ((C = next()) && (*compar)(d, C) > 0)
		;
	    insert(d, Current ? Current->Prev : Tail, Current);
	} else
	    insert(d, Current, Current ? Current->Next : Head);
    }
    const void *at( int n )
    {
	if (n < count() / 2) {
	    first();
	    while (n-- > 0 && next())
		;
	} else {
	    last();
	    n = count() - n - 1;
	    while (n-- > 0 && prev())
		;
	}
	return current();
    }
  protected:
    int Count;
    TT Head, Tail, Current;
    inline TT getHead( )    const	{ return Head; }
    inline TT getTail( )    const	{ return Tail; }
    inline TT getCurrent( ) const	{ return Current; }
    inline TT getPrev( )    const	{ return Current ? Current->Prev : Current; }
    inline TT getNext( )    const	{ return Current ? Current->Next : Current; }
    inline TT setCurrent( TT curr )	{ return Current = curr; }
    inline void *getV( TT curr ) const	{ return curr ? (void *)curr->V : (void *)0; }
    inline bool  deleteTT( )		{ while (deleteTT(getHead())) ; return ttrue; }
    void insert(const void *d, TT _Prev, TT _Next)
    {
	TT node = new s_TT();

	node->V = d;
	
	if (_Prev)
	    _Next = _Prev->Next;
	else if (_Next)
	    _Prev = _Next->Prev;
    
	if ((node->Prev = _Prev))
	    _Prev->Next = node;
	else
	    Head = node;
    
	if ((node->Next = _Next))
	    _Next->Prev = node;
	else
	    Tail = node;
	
	Count++;
    }
    bool deleteTT( TT curr )
    {
	if (curr) {
	    if (curr->Prev)
		curr->Prev->Next = curr->Next;
	    else
		Head = curr->Next;
	    if (curr->Next)
		curr->Next->Prev = curr->Prev;
	    else
		Tail = curr->Prev;
	    curr->~s_TT();
	    Count--;
	}
	return !!curr;
    }
};


template<class type> class TList : public TListV {
  public:
    TList( )				{ }
    TList( const TList<type> &l )	{ }
    inline type *current( ) const	{ return getT(getCurrent()); }
    inline type *first( )		{ return getT(setCurrent(getHead())); }
    inline type *last( )		{ return getT(setCurrent(getTail())); }
    inline type *next( )		{ return getT(setCurrent(getNext())); }
    inline type *prev( )		{ return getT(setCurrent(getPrev())); }
    inline type *getFirst() 		{ return first(); }
    inline type *getLast()  		{ return last(); }
    inline void  append( const type *d ){ TListV::append((const void *)d); }
    inline void  inSort( const type *d )
    {
	TListV::inSort((const void *)d, (int (*)(const void *, const void *))compar);
    }
    inline const type *at(int n)	{ return (const type *)TListV::at(n); }
  protected:
    inline type *getT( TT curr ) const	{ return (type *)getV(curr); }
    static int compar(const type *a, const type *b)	{ return *a<*b ? -1 : *a>*b ? 1 : 0; }
};


