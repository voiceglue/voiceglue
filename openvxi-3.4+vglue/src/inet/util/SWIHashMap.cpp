
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

#if _MSC_VER >= 1100    // Visual C++ 5.x
#pragma warning( disable : 4786 4503 )
#endif

#include "SWIHashMap.hpp"
#include "SWItrdMonitor.hpp"

const int SWIHashMap::DEFAULT_CAPACITY = 11;
const double SWIHashMap::DEFAULT_LOAD_FACTOR = 0.75;

SWIHashMap::Entry::Entry(const SWIHashable* key, const void *value):
  _key(key->clone()), _value(value), _next(NULL), _prev(NULL)
{}

SWIHashMap::Entry::~Entry()
{
  delete _key;
}

const SWIHashable& SWIHashMap::Entry::getKey() const
{
  return *_key;
}

const void *SWIHashMap::Entry::getValue() const
{
  return _value;
}

const void *SWIHashMap::Entry::setValue(const void *value)
{
  const void *result = _value;
  _value = value;
  return result;
}

SWIHashMap::SWIHashMap()
{
  init(DEFAULT_CAPACITY, DEFAULT_LOAD_FACTOR);
}

SWIHashMap::SWIHashMap(int capacity)
{
  init(capacity, DEFAULT_LOAD_FACTOR);
}

SWIHashMap::SWIHashMap(int capacity,
                       double loadFactor)
{
  init(capacity, loadFactor);
}

void SWIHashMap::init(int capacity,
                      double loadFactor)
{
  _capacity = capacity <= 0 ? 1 : capacity;
  _loadFactor = loadFactor <= 0.0 ? DEFAULT_LOAD_FACTOR : loadFactor;
  _size = 0;
  _entries = new Entry*[capacity];
  for (int i = 0; i < capacity; i++)
  {
    _entries[i] = NULL;
  }
  _threshold = (int) (_capacity * _loadFactor);
}

SWIHashMap::SWIHashMap(const SWIHashMap& original)
{
  init(original._capacity, original._loadFactor);
  copy(original);
}

void SWIHashMap::copy(const SWIHashMap& original)
{
  _size = original.size();

  for (int i = 0; i < _capacity; i++)
  {
    Entry *origEntry = original._entries[i];
    Entry *lastEntry = NULL;
    while (origEntry != NULL)
    {
      // Create a copy of the entry and insert it at the end of the
      // list.
      Entry *tmp = new Entry(origEntry->_key, origEntry->_value);

      tmp->_hashCode = origEntry->_hashCode;
      tmp->_next = NULL;
      tmp->_prev = lastEntry;
      if (lastEntry != NULL)
        lastEntry->_next = tmp;
      else
        _entries[i] = tmp;

      lastEntry = tmp;
      origEntry = origEntry->_next;
    }
  }
}

SWIHashMap::~SWIHashMap()
{
  clear();
  delete [] _entries;
}

void SWIHashMap::clear()
{
  Entry *tmp;
  for (int i = 0; i < _capacity && _size > 0; i++)
  {
    while (_entries[i] != NULL)
    {
      tmp = _entries[i];
      _entries[i] = tmp->_next;
      delete tmp;
      _size--;
    }
  }
}

const void *SWIHashMap::getValue(const SWIHashable& key) const
{
  unsigned int hashCode = key.hashCode();
  const Entry *entry = _entries[hashCode % _capacity];
  while (entry != NULL)
  {
    if (entry->_hashCode == hashCode &&
        key.equals(entry->_key))
    {
      return entry->_value;
    }
    entry = entry->_next;
  }
  return NULL;
}

void SWIHashMap::rehash()
{
  int oldCapacity = _capacity;
  Entry** oldEntries = _entries;

  _capacity = _capacity * 2 + 1;
  _entries = new Entry*[_capacity];
  _threshold = (int)(_capacity * _loadFactor);

  int i;

  for (i = 0; i < _capacity; i++)
  {
    _entries[i] = NULL;
  }

  for (i = 0; i < oldCapacity; i++)
  {
    for (Entry * entry = oldEntries[i]; entry != NULL; )
    {
      Entry *e = entry;
      entry = entry->_next;
      int idx = e->_hashCode % _capacity;

      if (_entries[idx] != NULL)
      {
        _entries[idx]->_prev = e;
      }
      e->_next = _entries[idx];
      e->_prev = NULL;
      _entries[idx] = e;
    }
  }

  delete [] oldEntries;
}

const void * SWIHashMap::putValue(const SWIHashable& key, const void *value)
{
  unsigned int hashCode = key.hashCode();

  int idx = hashCode % _capacity;
  Entry *entry = _entries[idx];
  while (entry != NULL)
  {
    if (entry->_hashCode == hashCode &&
        key.equals(entry->_key))
    {
      // Replace current value by new one.
      const void *result = entry->_value;
      entry->_value = value;
      return result;
    }
    entry = entry->_next;
  }

  //If we get here, we need to add the key into the hash table.
  // First verify if we need to rehash.
  if (_size >= _threshold)
  {
    rehash();
    idx = hashCode % _capacity;
  }

  entry = new Entry(&key, value);
  entry->_hashCode = hashCode;
  entry->_next = _entries[idx];
  if (entry->_next != NULL)
    entry->_next->_prev = entry;
  entry->_prev = NULL;
  _entries[idx] = entry;
  _size++;

  return NULL;
}

void SWIHashMap::remove(Entry *entry, int idx)
{
  // Remove entry from the list.
  if (entry->_next != NULL)
    entry->_next->_prev = entry->_prev;
  if (entry->_prev != NULL)
    entry->_prev->_next = entry->_next;
  else
    _entries[idx] = entry->_next;

  delete entry;
  _size--;
}

const void *SWIHashMap::remove(const SWIHashable& key)
{
  unsigned int hashCode = key.hashCode();

  int idx = hashCode % _capacity;
  Entry *entry = _entries[idx];
  while (entry != NULL)
  {
    if (entry->_hashCode == hashCode &&
        key.equals(entry->_key))
    {
      const void *result = entry->_value;
      remove(entry, idx);
      return result;
    }
    entry = entry->_next;
  }
  return NULL;
}

void SWIHashMap::advance(Entry *&cursor, int& idx)
{
  if (cursor != NULL)
  {
    cursor = cursor->_next;
    if (cursor == NULL)
    {
      while (++idx < _capacity)
      {
        if (_entries[idx] != NULL)
        {
          cursor = _entries[idx];
          break;
        }
      }
    }
  }
  else
  {
    for (idx = 0; idx < _capacity; idx++)
    {
      if (_entries[idx] != NULL)
      {
        cursor = _entries[idx];
        break;
      }
    }
  }
}

SWIHashMap& SWIHashMap::operator=(const SWIHashMap& rhs)
{
  if (this != &rhs)
  {
    clear();

    if (_capacity < rhs._capacity)
    {
      delete [] _entries;
      init(rhs._capacity, rhs._loadFactor);
    }
    else
    {
      _capacity = rhs._capacity;
      _loadFactor = rhs._loadFactor;
      _threshold = rhs._threshold;
      _entries = new Entry *[_capacity];
      for (int i = 0; i < _capacity; i++)
      {
        _entries[i] = NULL;
      }
    }

    copy(rhs);

  }
  return *this;
}

int SWIHashMap::size() const
{
  return _size;
}

bool SWIHashMap::isEmpty() const
{
  return _size == 0;
}

SWIHashMap::Iterator::Iterator(SWIHashMap& hashMap):
  _cursor(NULL), _idx(0), _prevCursor(NULL), _prevIdx(0)
{
  _hashMap = &hashMap;
  _hashMap->advance(_cursor, _idx);
}

SWIHashMap::Iterator::Iterator(const Iterator& original)
{
  _hashMap = original._hashMap;
  _cursor = original._cursor;
  _idx = original._idx;
  _prevCursor = original._prevCursor;
  _prevIdx = original._prevIdx;
}

SWIHashMap::Iterator& SWIHashMap::Iterator::operator=(const Iterator& rhs)
{
  if (this != &rhs)
  {
    _hashMap = rhs._hashMap;
    _cursor = rhs._cursor;
    _idx = rhs._idx;
    _prevCursor = rhs._prevCursor;
    _prevIdx = rhs._prevIdx;
  }
  return *this;
}

SWIHashMap::Iterator::~Iterator()
{}

void SWIHashMap::Iterator::reset() const
{
  Iterator *pThis = (Iterator *) this;
  pThis->_prevCursor = pThis->_cursor = NULL;
  pThis->_prevIdx = pThis->_idx = 0;
  pThis->_hashMap->advance(pThis->_cursor, pThis->_idx);
}


bool SWIHashMap::Iterator::hasNext() const
{
  return _cursor != NULL;
}

SWIHashMap::Entry *SWIHashMap::Iterator::next()
{
  if (_cursor == NULL)
    return NULL;

  _prevCursor = _cursor;
  _prevIdx = _idx;

  _hashMap->advance(_cursor, _idx);

  return _prevCursor;
}

const SWIHashMap::Entry *SWIHashMap::Iterator::next() const
{
  Iterator *pThis = (Iterator *) this;
  return pThis->next();
}

bool SWIHashMap::Iterator::remove()
{
  if (_prevCursor == NULL)
  {
    return false;
  }

  _hashMap->remove(_prevCursor, _prevIdx);
  _prevCursor = NULL;

  return true;
}

const void *SWIHashMap::Iterator::setValue(const void *value)
{
  if (_prevCursor == NULL)
    return NULL;

  const void *oldValue = _prevCursor->getValue();
  _prevCursor->setValue(value);
  return oldValue;
}
