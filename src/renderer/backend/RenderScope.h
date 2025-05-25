#ifndef RENDER_SCOPE_H_
#define RENDER_SCOPE_H_

#include <unordered_map>
#include <typeindex>

#include "Platform.h"

namespace zorya
{
    class Render_Scope
    {

    public:
        template <typename T>
        void set(T&& val)
        {
            using DecayedT = typename std::decay<T>::type;
            resources[std::type_index(typeid(T))] = new DecayedT(std::forward<T>(val));
        }

        template <typename T>
        T& get()
        {
            auto val = (T*)resources[std::type_index(typeid(T))];
            zassert(val != nullptr);
            return *val;
        }

    private:
        std::unordered_map<std::type_index, void*> resources;

    };
}

#endif // !RENDER_SCOPE_H_
