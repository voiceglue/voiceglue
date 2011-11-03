#ifndef SWIHASHMAP_HPP
#define SWIHASHMAP_HPP

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

#include "SWIutilHeaderPrefix.h"

#include "SWIHashable.hpp"

/**
 * class to abstract hash table operations.
 * @doc <p>
 **/

class SWIUTIL_API_CLASS SWIHashMap
{
 public:
  static const int DEFAULT_CAPACITY;
  static const double DEFAULT_LOAD_FACTOR;

  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
 public:
  SWIHashMap();

  SWIHashMap(int capacity);

  SWIHashMap(int capacity ,
             double loadFactor);

  /**
   * Destructor.
   **/
 public:
  virtual ~SWIHashMap();

  /**
   * Copy constructor.
   **/
 public:
  SWIHashMap(const SWIHashMap&);

  /**
   * Assignment operator.
   **/
 public:
  SWIHashMap& operator=(const SWIHashMap&);

 public:
  int size() const;

 public:
  bool isEmpty() const;

 public:
  const void *getValue(const SWIHashable& key) const;

 public:
  const void *putValue(const SWIHashable& key, const void *value);

 public:
  const void *remove(const SWIHashable& key);

 public:
  void clear();

 public:
  class SWIUTIL_API_CLASS Entry
  {
   private:
    Entry(const SWIHashable *key, const void *value);
    ~Entry();

   public:
    const SWIHashable& getKey() const;
    const void *getValue() const;
    const void *setValue(const void *value);

   private:
    friend class SWIHashMap;
    SWIHashable *_key;
    const void *_value;
    unsigned int _hashCode;
    Entry* _next;
    Entry* _prev;
  };

 public:
  class SWIUTIL_API_CLASS Iterator
  {
   public:
    Iterator(SWIHashMap& hashMap);

   public:
    Iterator(const Iterator& iter);

   public:
    Iterator& operator=(const Iterator& iter);

   public:
    ~Iterator();

   public:
    void reset() const;

   public:
    bool hasNext() const;

   public:
    const Entry *next() const;

   public:
    Entry *next();

    // Modification Operations

   public:
    const void *setValue(const void *value);

   public:
    bool remove();

   private:
    friend class SWIHashMap;

   private:
    Entry* _cursor;
    SWIHashMap* _hashMap;
    int _idx;
    Entry* _prevCursor;
    int _prevIdx;
  };

 private:
  int _capacity;
  double _loadFactor;
  Entry **_entries;
  int _size;
  int _threshold;

 private:
  void copy(const SWIHashMap& original);
  void init(int capacity , double loadFactor);
  void rehash();
  void remove(Entry* entry, int idx);
  void advance(Entry*& entry, int& idx);
  friend class Iterator;
};
#endif
