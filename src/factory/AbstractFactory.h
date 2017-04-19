/*
 * Copyright 2017 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef FACTORY_ABSTRACTFACTORY_H_
#define FACTORY_ABSTRACTFACTORY_H_

#include <string.h>
#include <string>

template<class BaseClass, class ... Args>
class AbstractFactory {
 public:
  AbstractFactory(const AbstractFactory&) = delete;
  AbstractFactory &operator=(const AbstractFactory&) = delete;

  typedef BaseClass* (*FactFunType)(Args...);

  static void Register(const char *key, FactFunType fn) {
    (*FnList)[key] = fn;
  }

  static BaseClass* Create(const char *key, Args ... args) {
    for (auto iter = FnList->begin(); iter != FnList->end(); ++iter) {
      if (!strcmp(iter->first, key)) {
        return (iter->second)(args...);
      }
    }
    return nullptr;
  }

  static AbstractFactory &Instance() {
    if (FnList == nullptr) {
      FnList = new FnMap();
    }
    static AbstractFactory<BaseClass, Args...> gf;
    return gf;
  }

 private:
  AbstractFactory() = default;

  typedef std::unordered_map<const char *, FactFunType> FnMap;
  static FnMap *FnList;
};

template<class BaseClass, class ... Args> std::unordered_map<const char *,
    BaseClass* (*)(Args...)>* AbstractFactory<BaseClass, Args...>::FnList =
    nullptr;

template<class Base, class Derived, class ... Args>
class RegisterClass {
 public:
  explicit RegisterClass(const char *key) {
    AbstractFactory<Base, Args...>::Instance().Register(key, FactFn);
  }
  static Base *FactFn(Args ... args) {
    return new Derived(args...);
  }
};

#define FACTORY_REGISTER(TYPE, ...) \
  static RegisterClass<__VA_ARGS__> registerme(TYPE)

#endif  // FACTORY_ABSTRACTFACTORY_H_
