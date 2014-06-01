/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2014, Cauldron Development LLC
                 Copyright (c) 2003-2014, Stanford University
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

#ifndef THREAD_H
#define THREAD_H

#include <cbang/StdTypes.h>
#include <cbang/util/UniqueID.h>

namespace cb {
  template <typename T> class ThreadLocalStorage;

  /// A wrapper class for threads
  class Thread : protected UniqueID<Thread, 1> {
  public:
    enum state_t {
      THREAD_STOPPED,
      THREAD_STARTING,
      THREAD_RUNNING,
      THREAD_DONE,
    };

  protected:
    struct private_t;
    private_t *p;

    volatile state_t state;
    volatile bool shutdown;
    unsigned id;
    int exitStatus;

    static ThreadLocalStorage<Thread *> threads;

  public:
    Thread();
    virtual ~Thread();

    /**
     * Start the thread.  This call will change the state to THREAD_STARTING.
     * Upon return there is no guarantee the thread will actually be started
     * after this call.  Once it is started the state will change to
     * THREAD_RUNNING.  However, there is also the possiblity that the thread
     * exits and the state changes to THREAD_STOPPED.
     */
    virtual void start();

    /**
     * Signal the thread to stop.  There is no guarantee the thread will
     * actually stop.  The thread routine must periodically call
     * shouldShutdown() to check if it should exit.
     */
    virtual void stop();

    /// Tell the thread to stop then call wait().
    virtual void join();

    /**
     * Block until this thread exits.  If the thread does not exit this
     * function will never return.
     */
    virtual void wait();

    /// @return The current thread state.
    state_t getState() const {return state;}

    /**
     * This is not the system thread id but one maintained by this class.
     * @return The thread id.
     */
    unsigned getID() const {return id;}

    int getExitStatus() const {return exitStatus;}

    /**
     * When true the thread routine should exit as soon as possible.
     * @return True if a call to stop() has signaled this thread should end.
     */
    bool shouldShutdown() const {return shutdown;}

    void cancel();

    /// Deliver a signal to the thread
    void kill(int signal);

    /// Yield the current threads remaining time slice to the operating system.
    static void yield();

    /// Return the system ID of the current thread
    static uint64_t self();

    static Thread &current();

    /// This function is used internally to start the thread.
    virtual void starter();

  protected:
    /**
     * This function should be overriden by sub classes and will be called
     * by the running thread.
     */
    virtual void run() = 0;
  };


  template<class T, typename METHOD_T = void (T::*)()>
  class ThreadFunc : public Thread {
    T *obj;          // pointer to object
    METHOD_T method; // pointer to method function
    bool destroy;

  public:
    /**
     * Construct a thread which will execute a method function of the class T.
     *
     * @param obj The class instance.
     * @param method A pointer to the method function.
     */
    ThreadFunc(T *obj, METHOD_T method, bool destroy = false) :
      obj(obj), method(method), destroy(destroy) {}

    virtual void starter() {
      Thread::starter();

      if (destroy) {
        state = THREAD_STOPPED;
        delete this;
      }
    }

  private:
    /// Passes the polymorphic call to run on to the target class.
    virtual void run() {(*obj.*method)();}
  };
}

#endif // THREAD_H
