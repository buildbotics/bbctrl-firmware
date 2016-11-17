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

#include "Value.h"
#include "JSImpl.h"
#include "Sink.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>

#include <Jsrt/ChakraCore.h>

#include <string.h>

using namespace cb::chakra;
using namespace cb;
using namespace std;


namespace {
  JsPropertyIdRef getPropID(const string &key) {
    JsPropertyIdRef id;
    CHAKRA_CHECK(JsCreatePropertyIdUtf8(CPP_TO_C_STR(key), key.length(), &id));
    return id;
  }

  extern "C" JsValueRef _callback(JsValueRef callee, bool constructor,
                                  JsValueRef *argv, unsigned short argc,
                                  void *data) {
    js::Callback &cb = *(js::Callback *)data;
    string msg;

    try {
      // Process args
      Value args;
      int index = 0;
      const js::Signature &sig = cb.getSignature();

      if (argc == 2 && Value(argv[1]).isObject()) args = Value(argv[1]);
      else {
        args = Value::createObject();

        for (unsigned i = 1; i < argc; i++)
          if (i <= sig.size()) args.set(sig.keyAt(i - 1), argv[i]);
          else {
            while (args.has(String(index))) index++;
            args.set(String(index), argv[i]);
          }
      }

      // Fill in defaults
      for (unsigned i = 0; i < sig.size(); i++) {
        string key = sig.keyAt(i);

        if (!args.has(key))
          switch (sig.get(i)->getType()) {
          case JSON::Value::JSON_NULL: args.setNull(key); break;
          case JSON::Value::JSON_BOOLEAN:
            args.set(key, sig.getBoolean(i)); break;
          case JSON::Value::JSON_NUMBER: args.set(key, sig.getNumber(i)); break;
          case JSON::Value::JSON_STRING: args.set(key, sig.getString(i)); break;
          default: break; // Ignore
          }
      }

      // Call function
      Sink sink;
      cb(args, sink);

      // Make sure sink calls concluded correctly
      if (!sink.getRoot().isUndefined()) sink.close();

      return sink.getRoot();

    } catch (const Exception &e) {msg = SSTR(e);
    } catch (const exception &e) {msg = e.what();
    } catch (...) {msg = "Unknown exception";}

    // Set exception
    JsSetException(Value::createError(msg));
    return Value::getUndefined();
  }
}


Value::Value(const SmartPointer<js::Value> &value) :
  ref(value.cast<Value>()->ref) {}


Value::Value(const string &value) {
  CHAKRA_CHECK(JsCreateStringUtf8
               ((uint8_t *)CPP_TO_C_STR(value), value.length(), &ref));
}


Value::Value(int value) {CHAKRA_CHECK(JsIntToNumber(value, &ref));}
Value::Value(double value) {CHAKRA_CHECK(JsDoubleToNumber(value, &ref));}
Value::Value(bool value) {CHAKRA_CHECK(JsBoolToBoolean(value, &ref));}


Value::Value(const js::Function &func) {
  const SmartPointer<js::Callback> &cb = func.getCallback();
  JSImpl::current().add(cb);
  CHAKRA_CHECK
    (JsCreateNamedFunction(Value(func.getName()), _callback, cb.get(), &ref));
}


Value::Value(const string &name, callback_t cb, void *data) {
  CHAKRA_CHECK(JsCreateNamedFunction(Value(name), cb, data, &ref));
}


int Value::getType() const {
  JsValueType type;
  CHAKRA_CHECK(JsGetValueType(ref, &type));
  return type;
}


bool Value::isArray() const {return getType() == JsArray;}
bool Value::isBoolean() const {return getType() == JsBoolean;}
bool Value::isFunction() const {return getType() == JsFunction;}
bool Value::isNull() const {return getType() == JsNull;}
bool Value::isNumber() const {return getType() == JsNumber;}


bool Value::isObject() const {
  switch (getType()) {
  case JsObject:
  case JsFunction:
  case JsError:
  case JsArrayBuffer:
  case JsTypedArray:
  case JsDataView:
    return true;
  default: return false;
  }
}


bool Value::isString() const {return getType() == JsString;}
bool Value::isUndefined() const {return getType() == JsUndefined;}


bool Value::toBoolean() const {
  JsValueRef ref;
  CHAKRA_CHECK(JsConvertValueToBoolean(this->ref, &ref));
  bool x;
  CHAKRA_CHECK(JsBooleanToBool(ref, &x));
  return x;
}


int Value::toInteger() const {
  JsValueRef ref;
  CHAKRA_CHECK(JsConvertValueToNumber(this->ref, &ref));
  int x;
  CHAKRA_CHECK(JsNumberToInt(ref, &x));
  return x;
}


double Value::toNumber() const {
  JsValueRef ref;
  CHAKRA_CHECK(JsConvertValueToNumber(this->ref, &ref));
  double x;
  CHAKRA_CHECK(JsNumberToDouble(ref, &x));
  return x;
}


string Value::toString() const {
  JsValueRef ref;
  CHAKRA_CHECK(JsConvertValueToString(this->ref, &ref));

  size_t size;
  CHAKRA_CHECK(JsCopyStringUtf8(ref, 0, 0, &size));

  SmartPointer<char>::Array buffer = new char[size];
  CHAKRA_CHECK(JsCopyStringUtf8(ref, (uint8_t *)buffer.get(), size, 0));

  return string(buffer.get(), size);
}


unsigned Value::length() const {
  if (isObject()) return getOwnPropertyNames().length();
  return getInteger("length");
}


SmartPointer<js::Value> Value::get(int i) const {
  JsValueRef ref;
  CHAKRA_CHECK(JsGetIndexedProperty(this->ref, Value(i), &ref));
  return new Value(ref);
}


bool Value::has(const string &key) const {
  bool x;
  CHAKRA_CHECK(JsHasProperty(ref, getPropID(key), &x));
  return x;
}


SmartPointer<js::Value> Value::get(const string &key) const {
  JsValueRef ref;
  CHAKRA_CHECK(JsGetProperty(this->ref, getPropID(key), &ref));
  return new Value(ref);
}


void Value::set(int i, const Value &value) {
  CHAKRA_CHECK(JsSetIndexedProperty(ref, Value(i), value));
}


void Value::append(const Value &value) {set(length(), value);}


void Value::set(const string &key, const Value &value, bool strict) {
  CHAKRA_CHECK(JsSetProperty(ref, getPropID(key), value, strict));
}


Value Value::call(vector<Value> _args) const {
  vector<JsValueRef> args(_args.begin(), _args.end());
  JsValueRef ref;
  CHAKRA_CHECK(JsCallFunction(this->ref, &args[0], args.size(), &ref));
  return ref;
}


Value Value::getOwnPropertyNames() const {
  JsValueRef ref;
  CHAKRA_CHECK(JsGetOwnPropertyNames(this->ref, &ref));
  return ref;
}


void Value::copyProperties(const Value &value) {
  Value props = value.getOwnPropertyNames();
  unsigned length = props.length();

  for (unsigned i = 0; i < length; i++) {
    string key = props.getString(i);
    set(key, value.get(key));
  }
}


void Value::setNull(const string &key) {set(key, getNull());}


Value Value::setObject(const string &key) {
  Value o = createObject();
  set(key, o);
  return o;
}


Value Value::setArray(const string &key, unsigned length) {
  Value a = createArray(length);
  set(key, a);
  return a;
}


Value Value::getGlobal() {
  JsValueRef ref;
  CHAKRA_CHECK(JsGetGlobalObject(&ref));
  return ref;
}


Value Value::getNull() {
  JsValueRef ref;
  CHAKRA_CHECK(JsGetNullValue(&ref));
  return ref;
}


Value Value::getUndefined() {
  JsValueRef ref;
  CHAKRA_CHECK(JsGetUndefinedValue(&ref));
  return ref;
}


bool Value::hasException() {
  bool x = false;
  CHAKRA_CHECK(JsHasException(&x));
  return x;
}


Value Value::getException() {
  JsValueRef ref;
  CHAKRA_CHECK(JsGetAndClearException(&ref));
  return ref;
}


Value Value::createArray(unsigned length) {
  JsValueRef ref;
  CHAKRA_CHECK(JsCreateArray(length, &ref));
  return ref;
}


Value Value::createArrayBuffer(unsigned length, const char *data) {
  JsValueRef ref;
  CHAKRA_CHECK(JsCreateArrayBuffer(length, &ref));

  if (data) {
    uint8_t *buffer;
    CHAKRA_CHECK(JsGetArrayBufferStorage(ref, &buffer, &length));
    memcpy(buffer, data, length);
  }

  return ref;
}


Value Value::createArrayBuffer(const string &s) {
  return createArrayBuffer(s.length(), CPP_TO_C_STR(s));
}


Value Value::createObject() {
  JsValueRef ref;
  CHAKRA_CHECK(JsCreateObject(&ref));
  return ref;
}


Value Value::createError(const string &msg) {
  JsValueRef ref;
  CHAKRA_CHECK(JsCreateError(Value(msg), &ref));
  return ref;
}


Value Value::createSyntaxError(const string &msg) {
  JsValueRef ref;
  CHAKRA_CHECK(JsCreateSyntaxError(Value(msg), &ref));
  return ref;
}


const char *Value::errorToString(int error) {
  switch (error) {
  case JsNoError: return "No error";
  case JsErrorInvalidArgument: return "Invalid argument";
  case JsErrorNullArgument: return "Null argument";
  case JsErrorNoCurrentContext: return "No current context";
  case JsErrorInExceptionState: return "In exception state";
  case JsErrorNotImplemented: return "Not implemented";
  case JsErrorWrongThread: return "Wrong thread";
  case JsErrorRuntimeInUse: return "Runtime in use";
  case JsErrorBadSerializedScript: return "Bad serialized script";
  case JsErrorInDisabledState: return "In disabled state";
  case JsErrorCannotDisableExecution: return "Cannot disable execution";
  case JsErrorHeapEnumInProgress: return "Heap enum in progress";
  case JsErrorArgumentNotObject: return "Argument not object";
  case JsErrorInProfileCallback: return "In profile callback";
  case JsErrorInThreadServiceCallback: return "In thread service callback";
  case JsErrorCannotSerializeDebugScript:
    return "Cannot serialize debug script";
  case JsErrorAlreadyDebuggingContext: return "Already debugging context";
  case JsErrorAlreadyProfilingContext: return "Already profiling context";
  case JsErrorIdleNotEnabled: return "Idle not enabled";
  case JsCannotSetProjectionEnqueueCallback:
    return "Cannot set projection enqueue callback";
  case JsErrorCannotStartProjection: return "Cannot start projection";
  case JsErrorInObjectBeforeCollectCallback:
    return "In object before collect callback";
  case JsErrorObjectNotInspectable: return "Object not inspectable";
  case JsErrorPropertyNotSymbol: return "Property not symbol";
  case JsErrorPropertyNotString: return "Property not string";
  case JsErrorInvalidContext: return "Invalid context";
  case JsInvalidModuleHostInfoKind: return "Invalid module host info kind";
  case JsErrorModuleParsed: return "Module parsed";
  case JsErrorModuleEvaluated: return "Module evaluated";
  case JsErrorOutOfMemory: return "Out of memory";
  case JsErrorBadFPUState: return "Bad FPU state";
  case JsErrorScriptException: return "Script exception";
  case JsErrorScriptCompile: return "Script compile";
  case JsErrorScriptTerminated: return "Script terminated";
  case JsErrorScriptEvalDisabled: return "Script eval disabled";
  case JsErrorFatal: return "Fatal";
  case JsErrorWrongRuntime: return "Wrong runtime";
  case JsErrorDiagAlreadyInDebugMode: return "Diag already in debug mode";
  case JsErrorDiagNotInDebugMode: return "Diag not in debug mode";
  case JsErrorDiagNotAtBreak: return "Diag not at break";
  case JsErrorDiagInvalidHandle: return "Diag invalid handle";
  case JsErrorDiagObjectNotFound: return "Diag object not found";
  case JsErrorDiagUnableToPerformAction: return "Diag unable to perform action";
  default: return "Unknown error";
  }
}
