#ifndef SWILIST_HPP
#define SWILIST_HPP

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

/**
 * Class to encapsulate list operations.  In order to be able to easily
 * implement copy-on-write semantic, SWILists created using the copy which
 * takes in a list or using the operator= share the same data.  When SWILists
 * are shared, changes made on one SWIList object will have been made on the
 * other SWIList object as well.  Although NULL pointers can be put in a list,
 * one as to be careful with the use of NULL pointers as they are used as a
 * way to determine whether a list is empty, or whether iteration is
 * complete.
 *
 * @doc
 **/

#include "SWIutilHeaderPrefix.h"

class SWIListCell;

class SWIUTIL_API_CLASS SWIList
{
  /**
   * Default constructor.
   **/
 public:
  SWIList();

  /**
    * copy constructor It creates a new list whose representation is a copy
    * of aList.
   **/
 public:
  SWIList(SWIList const& aList);

  // ------------------------------------------------------------
  /**
   * Destructor.
   * Note that the elements of the list need to be reclaimed by the user.
   **/
 public:
  ~SWIList();

  /**
   * Assignment operator.  Assigning SWIList copies data into the new list.
   **/
 public:
  SWIList& operator=(const SWIList& aList);

  /**
   * Returns the number of elements in this list.
   *
   * @return the number of elements in this collection
   */
 public:
  int size() const
  {
    return _size;
  }

  /**
   * Returns <tt>true</tt> if this collection contains no elements.
   *
   * @return <tt>true</tt> if this collection contains no elements
   **/
 public:
  bool isEmpty() const
  {
    return _size == 0;
  };

  /**
   * Returns <tt>true</tt> if this list contains the specified
   * element.  More formally, returns <tt>true</tt> if and only if this
   * collection contains at least one element <tt>e</tt> such that
   * <tt>obj == e</tt>.
   *
   * @param obj element whose presence in this collection is to be tested.
   * @return <tt>true</tt> if this collection contains the specified
   *         element
   **/
 public:
  bool contains(void *obj) const;

 public:
  class SWIUTIL_API_CLASS Iterator
  {
   public:
    enum Position
    {
      BEG,
      END
    };

   public:
    Iterator(SWIList& theList, Position pos = BEG);

    /**
     * Copy constructor.  Creates an Iterator iterating on the same list and
     * whose cursor is at the same position than the iterator.  Changing the
     * cursor position on one Iterator does not affect the cursor position of
     * the other Iterator.  One has to be careful about concurrent
     * modifications to the list.
     * @param iter The iterator to be copied.
     **/
   public:
    Iterator(const Iterator& iter);

   public:
    void setPos(Position pos);

    /**
     * Assignment.  Make the Iterator iterating on the same list and having
     * the cursor at the same position than the specified iterator.  Changing
     * the cursor position on one Iterator does not affect the cursor position
     * of the other Iterator.  One has to be careful about concurrent
     * modifications to the list.
     * @param iter The iterator to be copied.
     **/
   public:
    Iterator& operator=(const Iterator& iter);

   public:
    ~Iterator();

    /**
     * Returns <tt>true</tt> if this iterator has more elements when
     * traversing the list in the forward direction. (In other words, returns
     * <tt>true</tt> if <tt>next</tt> would return an element rather than
     * NULL.
     *
     * @return <tt>true</tt> if the iterator has more elements when
     *		traversing the list in the forward direction.
     **/
   public:
    bool hasNext() const;

    /**
     * Returns <tt>true</tt> if this list iterator has more elements when
     * traversing the list in the reverse direction.  (In other words, returns
     * <tt>true</tt> if <tt>previous</tt> would return an element rather than
     * NULL.
     *
     * @return <tt>true</tt> if the iterator has more elements when
     *	       traversing the list in the reverse direction.
     */
   public:
    bool hasPrevious() const;

    /**
     * Advances the cursor and returns the element at the new cursor
     * position. This method may be called repeatedly to iterate through the
     * list, or intermixed with calls to <tt>previous</tt> to go back and
     * forth.
     *
     * @return the next element in the list, NULL if at end of list.
     **/
   public:
    void *next() const;

    /**
     * Moves the cursor backward and Return the element at the new cursor
     * position.  This method may be called repeatedly to iterate through the
     * list, or intermixed with calls to <tt>previous</tt> to go back and
     * forth.
     *
     * @return the previous element in the list, NULL if at beginning of list.
     **/
   public:
    void *previous() const;

    // Modification Operations

    /**
     * Replaces the element at the current cursor position with the specified
     * element.
     *
     * @param item the element with which to replace the current element
     * returned by <tt>next</tt> or <tt>previous</tt>.
     *
     * @return The replaced element or NULL, if the cursor is before the first
     * element or after the last.
     **/
   public:
    void *set(void *item);

    /**
     * Inserts the specified element into the list before the current cursor
     * position.  If the cursor is before the first element or the list is
     * empty, the element is added at the beginning of the list.  The cursor
     * position is not affected, so a call to <code>previous</code> returns
     * the newly inserted element (except for the case where the cursor was
     * before the first element), and a call to <code>next</code> is
     * unaffected.
     *
     * @param item the element to insert.
     * @return <code>true</code> if the element could be added or <code>false</code> otherwise.
     **/
   public:
    bool insertBefore(void *item);

    /**
     * Inserts the specified element into the list after the current cursor
     * position.  If the cursor is after the last element or the list is
     * empty, the element is added at the end of the list.  The cursor
     * position is not affected, so a call to <code>next</code> returns
     * the newly inserted element (except for the case where the cursor was
     * after the last element), and a call to <code>previous</code> is
     * unaffected.
     *
     * @param item the element to insert.
     * @return <code>true</code> if the element could be added or <code>false</code> otherwise.
     **/
   public:
    bool insertAfter(void *item);

    /**
     * Removes the current item from the list and moves the cursor backward.
     * The new current element is the one that would have been returned by
     * <code>previous</code> so a call to <code>next</code> is unaffected.
     *
     * @return the removed element or NULL if the cursor was before the
     * first element or after the last.
     **/
   public:
    void* removeBack();

    /**
     * Removes the current from the list and moves the cursor forward.
     * The new current element is the one that would have been returned by
     * <code>next</code> so a call to <code>previous</code> is unaffected.
     *
     * @return the removed element or NULL if the cursor was before the
     * first element or after the last.
     **/
   public:
    void* removeFront();

   private:
    friend class SWIList;

   private:
    mutable SWIListCell* _cursor;
    SWIList* _list;
  };

  /**
   * Returns an iterator over the elements in this list positioned before
   * the first element of the list.
   *
   * @return an <tt>Iterator</tt> over the elements in this list.
   **/
 public:
  Iterator iterator();

  /**
   * Returns an iterator over the elements in this list positioned before
   * the first element of the list.
   *
   * @return an <tt>Iterator</tt> over the elements in this list.
   **/
 public:
  const Iterator iterator() const;

  /**
   * Returns an iterator over the elements in this list positioned after the
   * last element of the list.
   *
   * @return an <tt>Iterator</tt> over the elements in this list.
   **/
 public:
  Iterator reverseIterator();

  /**
   * Returns an iterator over the elements in this list positioned after the
   * last element of the list.
   *
   * @return an <tt>Iterator</tt> over the elements in this list.
   **/
 public:
  const Iterator reverseIterator() const;

  /**
   * Add the specified element at the end of this list.
   *
   * @param item the element to insert.
   * @return <code>true</code> if the element could be added or <code>false</code> otherwise.
   **/
 public:
  bool addLast(void *obj);

  /**
   * Add the specified element at the beginning of this list.
   *
   * @param item the element to insert.
   * @return <code>true</code> if the element could be added or <code>false</code> otherwise.
   **/
 public:
  bool addFirst(void *obj);

  /**
   * Returns the first element of the list.
   *
   * @return the first element of the list or NULL if the list is empty.
   **/
 public:
  void *first() const;

  /**
   * Returns the last element of the list.
   *
   * @return the last element of the list or NULL if the list is empty.
   **/
 public:
  void *last() const;

  /**
   * Removes the first element from this list, if it the list is not
   * empty. Returns the element removed or NULL if this is empty.  This method
   * should not be called while iterating on a list as it may invalidate the
   * iterator.
   * @return The element that was removed or NULL if the list was empty.
   **/
 public:
  void *removeFirst();

  /**
   * Removes the last element from this list, if it the list is not
   * empty. Returns the element removed or NULL if this is empty.  This method
   * should not be called while iterating on a list as it may invalidate the
   * iterator.
   * @return The element that was removed or NULL if the list was empty.
   **/
 public:
  void *removeLast();

  /**
   * Removes the first occurence of the specified element from this list, if
   * it is present. Returns true if this list contained the specified
   * element (or equivalently, if this list changed as a result of the
   * call).
   *
   * @param obj element to be removed from this list, if present.
   * @return <tt>true</tt> if this collection changed as a result of the
   *         call
   **/
 public:
   bool remove(void *obj);

  /**
   * Removes all occurences of the specified element from this list, if it is
   * present. Returns true if this list contained the specified element (or
   * equivalently, if this list changed as a result of the call).  This method
   * should not be called while iterating on a list as it may invalidate the
   * iterator.
   *
   * @param obj element to be removed from this list.
   * @return <tt>true</tt> if this collection changed as a result of the call
   **/
 public:
  bool removeAll(void *obj);

  /**
   * Removes all of the elements from this list.  This list will be empty
   * after this method returns.  This method should not be called while
   * iterating on a list as it may invalidate the iterator.
   **/
 public:
  void clear();

  /**
   * The list representation.
   **/
 private:
  // Pointer to the head and tail elements of the list.  They are not really
  // part of the list.  They are present to simplify insertion and removal
  // algorithm.
  SWIListCell* _head;
  SWIListCell* _tail;

  int _size;
  /**
   * Returns a pointer to the first cell which points to pItem
   **/
  SWIListCell* getCell(void* pItem);

   /**
    * Removes the given cell by relinking it previous and next cell
    * and deleting it.  It also decrements the size member of the
    * list representation.
    **/
  void remove(SWIListCell* pCell);
  friend class Iterator;
};

#endif
