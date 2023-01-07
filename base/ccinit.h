
/*
 * Copyright 2022 wtcat
 */

#ifndef BASE_INITIALIZER_H_
#define BASE_INITIALIZER_H_

#include <vector>
#include <memory>
#include <algorithm>

namespace base {

class cc_constructor {
public:
    cc_constructor(int level) : order_(level) {}
    virtual ~cc_constructor() {}
    virtual int run() = 0;
    int order() const { return order_; }
        
protected:
    int order_;
};

class cc_initializer {
public:
    enum {
        kFirst = 10,
        kMiddle = 50,
        kLast = 99
    };
    ~cc_initializer() {
        clear();
    }
    cc_initializer(const cc_initializer&) = delete;
    cc_initializer(cc_initializer &&) = delete;
    cc_initializer& operator=(const cc_initializer&) = delete;

    static cc_initializer* get_instance() {
        static cc_initializer* initializer_ptr;
        if (initializer_ptr == nullptr)
            initializer_ptr = new cc_initializer();
        return initializer_ptr;
    }
    cc_constructor* add_constructor(cc_constructor* c) {
        container_.push_back(c);
        return c;
    }
    int run() {
        for (auto iter : container_) {
            int ret = iter->run();
            if (ret)
                return ret;
        }
        return 0;
    }
    cc_initializer* sort() {
        std::sort(container_.begin(), container_.end(),
            [](cc_constructor* a, cc_constructor* b) {
                return a->order() < b->order();
            }
        );
        return this;
    }

private:
    cc_initializer() = default;
    void clear() {
        for (auto iter : container_)
            delete iter;
    }
private:
    std::vector<cc_constructor*> container_;
};

inline int cc_initializer_run() {
    std::unique_ptr<cc_initializer> _initializer(
        cc_initializer::get_instance());
    return _initializer->sort()->run();
}

} //namespace base

#ifndef _CONTACT
#define _CONTACT(_a, _b) _a ## _b
#endif

#define CC_INIT(_name, _order) \
    class _CONTACT(_cc_contructor_, _name) : public base::cc_constructor { \
    public:                                                       \
        _CONTACT(_cc_contructor_, _name)(int order) : base::cc_constructor(order) {}           \
        ~_CONTACT(_cc_contructor_, _name)() = default;   \
        int run() override; \
    private:                    \
        static base::cc_constructor* const construct_;  \
    };  \
    base::cc_constructor* const _CONTACT(_cc_contructor_, _name)::construct_ = \
        base::cc_initializer::get_instance()->add_constructor( \
        new _CONTACT(_cc_contructor_, _name)(_order)); \
    int _CONTACT(_cc_contructor_, _name)::run()

#endif /* BASE_INITIALIZER_H_ */
