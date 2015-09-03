/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Exception.h"

#ifndef CBANG_SMART_POINTER_H
#define CBANG_SMART_POINTER_H

#include "RefCounter.h"


namespace cb {
  /**
   * This is an implementation of a smart pointer.  It is ONLY THREAD
   * SAFE IF you create a protected smart pointer.
   *
   * You can use a SmartPointer on a dynamically allocated instance
   * of a class A like this:
   *
   *   SmartPointer<A> aPtr = new A;
   *
   * Since aPtr is static when it goes out of scope it will be
   * destructed.  When this happens the reference counter will be
   * decremented and if it is zero the dynamic instance of A will be
   * destructed.
   *
   * You can make a copy of a smart pointer like this:
   *
   *   SmartPointer<A> anotherPtr = aPtr;
   *
   * Now the dynamic instance of A will not be destructed until both
   * aPtr and anotherPtr go out of scope.
   *
   * Correct deallocation can vary depending on the type of resource
   * being protected by the smart pointer.  The CounterT template
   * parameter selects the reference counter whos DeallocT template
   * parameter selects the deallocator.  The default deallocator is
   * used for any data allocated with the 'new' operator.
   * DeallocArray is necessary for arrays allocated with the 'new []'
   * operator.  DeallocMalloc will correctly deallocate data allocated
   * with calls to 'malloc', 'calloc', 'realloc', et.al.  Other
   * deallocators can be created to handle other resources.
   *
   * You can also create your own deallocators for other resources.
   * The managed resources need not even be memory.
   *
   * Note that you cannot use non-protected SmartPointers to the
   * same data in different threads.  Even if you make a new copy of
   * the SmartPointer.  This is because the copies will still share the
   * same Counter.  However, non-protected SmartPointers are faster.
   *
   * Protected SmartPointers are only protected in the sense that
   * multiple threads can safely access different copies of the same
   * SmartPointer concurrently.  Accessing the same copy of a
   * SmartPointer from multiple threads safely requires futher
   * synchronization.
   *
   * See http://ootips.org/yonat/4dev/smart-pointers.html for more
   * information about smart pointers and why to use them.
   */
  class SmartPointerBase {
  public:
    void raise(const std::string &msg) const;
  };


  template <typename T, typename DeallocT = DeallocNew<T>,
            typename CounterT = RefCounterImpl<T, DeallocT> >
  class SmartPointer : public SmartPointerBase {
  protected:
    /// A pointer to the reference counter
    RefCounter *refCounter;

    /// The actual pointer
    T *ptr;

    typedef SmartPointer<T, DeallocT, CounterT> SmartPointerT;

  public:
    typedef SmartPointer<T, DeallocPhony, RefCounterPhonyImpl> Phony;
    typedef SmartPointer<T, DeallocMalloc> Malloc;
    typedef SmartPointer<T, DeallocArray<T> > Array;
    typedef SmartPointer<T, DeallocNew<T>,
                         ProtectedRefCounterImpl<T, DeallocNew<T> > > Protected;
    typedef SmartPointer<T, DeallocMalloc,
                         ProtectedRefCounterImpl<T, DeallocMalloc> >
    ProtecteMalloc;
    typedef SmartPointer<T, DeallocArray<T>,
                         ProtectedRefCounterImpl<T, DeallocArray<T> > >
    ProtectedArray;

    /**
     * The copy constructor.  If the smart pointer being copied
     * is not NULL the object reference count will be incremented.
     *
     * @param smartPtr The pointer to copy.
     */
    SmartPointer(const SmartPointerT &smartPtr) : refCounter(0), ptr(0) {
      *this = smartPtr;
    }

    /**
     * Create a smart pointer from a pointer value.  If ptr is
     * non-NULL the reference count will be set to one.
     * NOTE: It is an error to create more than one smart pointer
     * through this constructor for a single pointer value because
     * this will cause the object to which it points to be deallocated
     * more than once.  To create a copy of a smart pointer either use
     * the copy constructor or the smart pointer assignment operator.
     *
     * @param ptr The pointer to point to.
     * @param refCounter The reference counter.
     */
    SmartPointer(T *ptr = 0, RefCounter *refCounter = 0) :
      refCounter(refCounter), ptr(ptr) {
      if (ptr) {
        if (refCounter) refCounter->incCount();
        else this->refCounter = CounterT::create();
      }
    }

    /**
     * Destroy this smart pointer.  If this smart pointer is set to
     * a non-NULL value and there are no other references to the
     * object to which it points the object will be deleted.
     */
    ~SmartPointer() {release();}

    /**
     * Compare two smart pointers for equality.  Two smart pointers
     * are equal if and only if their internal pointers are equal.
     *
     * @param smartPtr The pointer to compare with.
     *
     * @return True if the smart pointers are equal. False otherwise.
     */
    bool operator==(const SmartPointerT &smartPtr) const {
      return ptr == smartPtr.ptr;
    }

    /**
     * Compare two smart pointers for inequality.  Two smart pointers
     * are not equal if and only if their internal pointers are not equal.
     *
     * @param smartPtr The pointer to compare with.
     *
     * @return True if the smart pointers are not equal. False otherwise.
     */
    bool operator!=(const SmartPointerT &smartPtr) const {
      return ptr != smartPtr.ptr;
    }

    /**
     * Compare two smart pointers.  Useful for ordering smart pointers.
     *
     * @param smartPtr The pointer to compare with.
     *
     * @return True if less than @param smartPtr. False otherwise.
     */
    bool operator<(const SmartPointerT &smartPtr) const {
      return ptr < smartPtr.ptr;
    }

    /**
     * Compare this smart pointer to an actual pointer value.
     *
     * @param ptr The pointer to compare with.
     *
     * @return True if this smart pointers internal pointer equals ptr.
     *         False otherwise.
     */
    bool operator==(const T *ptr) const {
      return this->ptr == ptr;
    }

    /**
     * Compare this smart pointer to an actual pointer value.
     *
     * @param ptr The pointer to compare with.
     *
     * @return False if this smart pointers internal pointer equals ptr.
     *         True otherwise.
     */
    bool operator!=(const T *ptr) const {
      return this->ptr != ptr;
    }

    /**
     * Compare this smart pointer to an actual pointer value.
     *
     * @param ptr The pointer to compare with.
     *
     * @return True if less than @param ptr. False otherwise.
     */
    bool operator<(const T *ptr) const {
      return this->ptr < ptr;
    }

    /**
     * Assign this smart pointer to another.  If the passed smart pointer
     * is non-NULL a reference will be added.
     *
     * @param smartPtr The pointer to copy.
     *
     * @return A reference to this smart pointer.
     */
    SmartPointerT &operator=(const SmartPointerT &smartPtr) {
      if (*this == smartPtr) return *this;

      release();

      refCounter = smartPtr.refCounter;
      if (refCounter) refCounter->incCount();
      ptr = smartPtr.ptr;

      return *this;
    }

    /**
     * Dereference this smart pointer.
     * A Exception will be thrown if the pointer is NULL.
     *
     * @return A reference to the object pointed to by this smart pointer.
     */
    T *operator->() const {checkPtr(); return get();}

    /**
     * Dereference this smart pointer.
     * A Exception will be thrown if the pointer is NULL.
     *
     * @return A reference to the object pointed to by this smart pointer.
     */
    T &operator*() const {checkPtr(); return *get();}

    /**
     * Dereference this smart pointer with an array index.
     * A Exception will be thrown if the pointer is NULL.
     *
     * @return A reference to an object in the array pointed to by
     * this smart pointer.
     */
    T &operator[](const long x) const {checkPtr(); return get()[x];}

    /**
     * Access this smart pointers internal object pointer
     *
     * @return The value of the internal object pointer
     */
    T *get() const {return ptr;}

    /**
     * Not operator
     * @return true if the pointer is zero.
     */
    bool operator!() const {return ptr == 0;}


    /// Convert to a base type.
    template <typename _BaseT, typename _DeallocT, typename _CounterT>
    operator SmartPointer<_BaseT, _DeallocT, _CounterT> () const {
      return SmartPointer<_BaseT, _DeallocT, _CounterT>(ptr, refCounter);
    }

    /// Dynamic cast
    template <typename CastT>
    CastT *castPtr() const {
      CastT *ptr = dynamic_cast<CastT *>(this->ptr);
      if (!ptr && this->ptr) raise("Invalid SmartPointer cast");

      return ptr;
    }

    template <typename CastT>
    SmartPointer<CastT, DeallocT, CounterT> cast() const {
      return SmartPointer<CastT, DeallocT, CounterT>
        (castPtr<CastT>(), refCounter);
    }

    // Type check
    template <typename InstanceT>
    bool isInstance() const {return dynamic_cast<InstanceT *>(ptr);}

    /**
     * Release this smart pointer's reference.
     * If the reference count is one the object to which this
     * smart pointer points will be deleted.
     */
    void release() {
      if (refCounter) refCounter->decCount(ptr);
      refCounter = 0;
      ptr = 0;
    }

    /**
     * Assume responsibility for this pointer.  If the reference
     * counter is more than one a Exception will be thrown.
     * When successful this smart pointer will be NULL.
     *
     * This function can be useful for moving the pointer to
     * a base class smart pointer.
     *
     * @return The value of the internal pointer.
     */
    T *adopt() {
      if (refCounter && refCounter->getCount() > 1)
        raise("SmartPointer: Cannot adopt a pointer with multiple references!");

      if (refCounter) {
        delete refCounter;
        refCounter = 0;
      }

      T *tmp = ptr;
      ptr = 0;

      return tmp;
    }

    /**
     * Get the number of references to the object pointed to by
     * this smart pointer.  The reference count will be zero
     * if this is a NULL smart pointer other wise it will be
     * one or more.
     *
     * @return The reference count.
     */
    long getRefCount() const {return refCounter ? refCounter->getCount() : 0;}

    /**
     * Check for NULL pointer value;
     *
     * @return True if the internal pointer is NULL. False otherwise.
     */
    bool isNull() const {return get() == 0;}

  protected:
    void checkPtr() const {
      if (!ptr) raise("SmartPointer: Can't dereference a NULL pointer!");
    }
  };
}

#define CBANG_SP(T) cb::SmartPointer<T>
#define CBANG_SP_PHONY(T) cb::SmartPointer<T>::Phony
#define CBANG_SP_MALLOC(T) cb::SmartPointer<T>::Malloc
#define CBANG_SP_ARRAY(T) cb::SmartPointer<T>::Array
#define CBANG_SP_DEALLOC(T, DEALLOC) \
  cb::SmartPointer<T, cb::DeallocFunc<T, DEALLOC> >

#ifdef USING_CBANG
#define SP(T) CBANG_SP(T)
#define SP_PHONY(T) CBANG_SP_PHONY(T)
#define SP_MALLOC(T) CBANG_SP_MALLOC(T)
#define SP_ARRAY(T) CBANG_SP_ARRAY(T)
#define SP_DEALLOC(T, DEALLOC) CBANG_SP_DEALLOC(T, DEALLOC)
#endif

#endif // CBANG_SMART_POINTER_H
