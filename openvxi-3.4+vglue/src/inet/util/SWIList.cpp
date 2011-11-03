
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * Vocalocity, the Vocalocity logo, and VocalOS are trademarks or 
 * registered trademarks of Vocalocity, Inc. 
 * OpenVXI is a trademark of Scansoft, Inc. and used under license 
 * by Vocalocity.
 ***********************************************************************/

#include "SWIutilInternal.h"

#include <stdlib.h>
#include "SWIList.hpp"

#ifdef REUSE_SWI_CELLS
#include "SWItrdMonitor.hpp"
static SWItrdMonitor SWIListMonitor;
#endif

class SWIListCell
{
 public:

  // Creates a cell with given pointers.
  SWIListCell():
    _item(NULL), _prev(NULL), _next(NULL)
  {}

  SWIListCell(void *item, SWIListCell *prev, SWIListCell *next):
    _item(item), _prev(prev), _next(next)
  {
    prev->_next = this;
    next->_prev = this;
  }

#ifdef REUSE_SWI_CELLS
  // Overloads the new and delete to do memory memanagement of SWIListCell.
  // There is a free list of cell from which cells are taken by
  // new.  delete just pust the cell at the top of the free list.
  void* operator new(size_t);
  void operator delete(void* cell);
#endif

  // Pointer to the list element.
  void* _item;

  // Pointer to the previous cell, null if first cell.
  SWIListCell *_prev;

  // Pointer to the next cell, null if last cell.
  SWIListCell *_next;

 private:
#ifdef REUSE_SWI_CELLS
  // Points to the first free cell.  new and delete take and put
  // SWIListCell objects in this list.
  static SWIListCell* _freeList;
#endif
};


#ifdef REUSE_SWI_CELLS
SWIListCell* SWIListCell::_freeList = NULL;

#define NBCELLS 16
void* SWIListCell::operator new(size_t)
{
  ::SWIListMonitor.lock();

  SWIListCell* pCell = _freeList;

  if (pCell == NULL)
  {
    // Allocates NBCELLS cells.
    pCell  = (SWIListCell*) new char[ NBCELLS * sizeof(SWIListCell) ];

    if (pCell != NULL)
    {
      // Put NBCELLS-1 of them in the free list, and return the last one.
      for (int cell = 1; cell < NBCELLS; cell++)
      {
        pCell->_next = _freeList;
        _freeList = pCell;
        pCell++;
      }
    }
  }
  else
  {
    _freeList = pCell->_next;
  }

  ::SWIListMonitor.unlock();
  return pCell;
}

void SWIListCell::operator delete(void* cell)
{
  ::SWIListMonitor.lock();
  ((SWIListCell*) cell)->_next = _freeList;
  _freeList = (SWIListCell*) cell;
  ::SWIListMonitor.unlock();
}
#endif

SWIList::SWIList()
{
  _head = new SWIListCell();
  _tail = new SWIListCell();
  _head->_prev = _head;
  _head->_next = _tail;
  _tail->_prev = _head;
  _tail->_next = _tail;
  _size = 0;
}

SWIList::SWIList(SWIList const & aList)
{
  _head = new SWIListCell();
  _tail = new SWIListCell();
  _head->_prev = _head;
  _head->_next = _tail;
  _tail->_prev = _head;
  _tail->_next = _tail;
  _size = 0;

  SWIListCell *cell = aList._head->_next;
  while (cell != aList._tail &&
         addLast(cell->_item))
  {
    cell = cell->_next;
  }
}

SWIList::~SWIList()
{
  clear();
  delete _head;
  delete _tail;
}

SWIList& SWIList::operator=(const SWIList& aList)
{
  if (this != &aList)
  {
    clear();
    SWIListCell *cell = aList._head->_next;
    while (cell != aList._tail &&
           addLast(cell->_item))
    {
      cell = cell->_next;
    }
  }

  return *this;
}

void *SWIList::first() const
{
  return _head->_next->_item;
}

void *SWIList::last() const
{
  return _tail->_prev->_item;
}

bool SWIList::addFirst(void* pItem)
{
  if (new SWIListCell(pItem, _head, _head->_next) != NULL)
  {
    _size++;
    return true;
  }
  return false;
}

bool SWIList::addLast(void* pItem)
{
  if (new SWIListCell(pItem, _tail->_prev, _tail) != NULL)
  {
    _size++;
    return true;
  }
  return false;
}

void* SWIList::removeFirst()
{
  if (_size == 0)
  {
    return NULL;
  }

  SWIListCell *first = _head->_next;

  void *item = first->_item;
  remove(first);

  return item;
}

void* SWIList::removeLast()
{
  if (_size == 0)
  {
    return NULL;
   }

  SWIListCell *last = _tail->_prev;
  void *item = last->_item;
  remove(last);

  return item;
}

bool SWIList::remove(void* pItem)
{
  SWIListCell* pRemove = getCell(pItem);

  if (pRemove == NULL)
  {
    return false;
  }

  remove(pRemove);

  return true;
}

bool SWIList::removeAll(void* pItem)
{
  SWIListCell* pNext = NULL;
  bool found = false;

  for (SWIListCell* pCurrent = _head->_next;
       pCurrent != _tail;
       pCurrent = pNext)
  {
    pNext = pCurrent->_next;
    if (pCurrent->_item == pItem)
    {
      remove(pCurrent);
      found = true;
    }
  }
  return found;
}

void SWIList::clear()
{
  SWIListCell *cell = _head->_next;

  while (cell != _tail)
  {
    SWIListCell *tmp = cell;
    cell = cell->_next;
    delete tmp;
  }

  _size = 0;
  _head->_next = _tail;
  _tail->_prev = _head;
}

SWIListCell* SWIList::getCell(void* pItem)
{
  for (SWIListCell* cell = _head->_next;
       cell != _tail;
       cell = cell->_next)
   {
     if (cell->_item == pItem)
     {
       return cell;
     }
   }

   return NULL;
}

void SWIList::remove(SWIListCell* cell)
{
  cell->_prev->_next = cell->_next;
  cell->_next->_prev = cell->_prev;

  delete cell;
  _size--;
}

SWIList::Iterator SWIList::iterator()
{
  Iterator iter(*this, Iterator::BEG);
  return iter;
}

const SWIList::Iterator SWIList::iterator() const
{
  Iterator iter((SWIList &) *this, Iterator::BEG);
  return iter;
}

SWIList::Iterator SWIList::reverseIterator()
{
  Iterator iter(*this, Iterator::END);
  return iter;
}

const SWIList::Iterator SWIList::reverseIterator() const
{
  Iterator iter((SWIList &) *this, Iterator::END);
  return iter;
}

SWIList::Iterator::Iterator(SWIList& listRep,
                            Position pos)
{
  _list = &listRep;
  _cursor = pos == BEG ? listRep._head : listRep._tail;
}

SWIList::Iterator::Iterator(Iterator const& iter)
{
  _list = iter._list;
  _cursor = iter._cursor;
}

SWIList::Iterator::~Iterator()
{}


SWIList::Iterator& SWIList::Iterator::operator=(Iterator const& iter)
{
  if (this != &iter)
  {
    _cursor = iter._cursor;
    _list = iter._list;
  }
  return *this;
}

void SWIList::Iterator::setPos(Position pos)
{
  _cursor = pos == BEG ? _list->_head : _list->_tail;
}

bool SWIList::Iterator::hasNext() const
{
  return _cursor->_next != _list->_tail;
}

bool SWIList::Iterator::hasPrevious() const
{
  return _cursor->_prev != _list->_head;
}

void* SWIList::Iterator::next() const
{
  _cursor = _cursor->_next;
  return _cursor->_item;
}

void* SWIList::Iterator::previous() const
{
  _cursor = _cursor->_prev;
  return _cursor->_item;
}

void * SWIList::Iterator::removeBack()
{
  if (_cursor == _list->_head ||
      _cursor == _list->_tail)
  {
    // either at tail or head of the list.
    return NULL;
  }

  void *item = _cursor->_item;
  _cursor = _cursor->_prev;
  _list->remove(_cursor->_next);
  return item;
}

void * SWIList::Iterator::removeFront()
{
  if (_cursor == _list->_head ||
      _cursor == _list->_tail)
  {
    // either at tail or head of the list.
    return NULL;
  }

  void *item = _cursor->_item;
  _cursor = _cursor->_next;
  _list->remove(_cursor->_prev);
  return item;
}

void * SWIList::Iterator::set(void *item)
{
  if (_cursor == _list->_tail ||
      _cursor == _list->_head)
  {
    // either at tail or head of the list.
    return NULL;
  }

  void *olditem = _cursor->_item;
  _cursor->_item = item;
  return olditem;
}

bool SWIList::Iterator::insertAfter(void *item)
{
  SWIListCell *cell;

  if (_list->_size == 0 || _cursor == _list->_tail)
  {
    cell = new SWIListCell(item, _list->_tail->_prev, _list->_tail);
  }
  else
  {
    cell = new SWIListCell(item, _cursor, _cursor->_next);
  }
  if (cell != NULL)
  {
    _list->_size++;
    return true;
  }
  return false;
}


bool SWIList::Iterator::insertBefore(void *item)
{
  SWIListCell *cell;

  if (_list->_size == 0 || _cursor == _list->_head)
  {
    cell = new SWIListCell(item, _list->_head, _list->_head->_next);
  }
  else
  {
    cell = new SWIListCell(item, _cursor->_prev, _cursor);
  }
  if (cell != NULL)
  {
    _list->_size++;
    return true;
  }
  return false;
}
